#include "PromptAnchor.h"

#include "Prompt.h"
#include "Settings.h"
#include "Util.h"

namespace CIGAR::PromptAnchor
{
	namespace
	{
		using Clock = std::chrono::steady_clock;

		template <class... Args>
		void Log(std::format_string<Args...> a_fmt, Args&&... a_args)
		{
			logs::info("[PromptAnchor] {}", std::format(a_fmt, std::forward<Args>(a_args)...));
		}

		constexpr RE::FormID kXMarker = 0x3B;  // Skyrim.esm XMarker: IsMarker (never drawn), bounds -18,-20,0..18,18,16
		constexpr RE::FormID kPlayer = 0x14;
		constexpr auto kRetryEvery = 10s;       // a lost marker is placed again at most this often
		constexpr int kMaxPlaceFailures = 3;    // then the player keeps the prompts until the next load
		constexpr auto kHookSilence = 5s;       // no frame from the update hook this long while playing: another mod replaced it

		// Read by PromptSlot::Offer and Tick (game thread), written by the update hook (main thread).
		std::atomic<RE::FormID> attachID{ kPlayer };
		std::atomic<RE::FormID> markerID{ 0 };      // 0: no usable marker
		std::atomic_bool setPosition{ false };      // enabled-marker fallback when a disabled one has no bounds
		std::atomic_bool markerLost{ false };       // the hook saw the handle fail; Tick places a new marker
		std::atomic_bool hookDead{ false };         // the hook stopped being called; prompts stay on the player
		std::atomic<std::uint64_t> frames{ 0 };     // counted by the hook
		std::atomic<float> right{ Settings::kPromptRightDefault };  // Settings passes the player's choice

		// Main thread only.
		RE::ObjectRefHandle marker;
		RE::FormID savedID = 0;  // from the co-save
		float lift = 26.0f;      // SkyPrompt draws 10 units above the box top; XMarker's top is 16 up
		bool attachedToMarker = false;
		// Tick bookkeeping (game thread).
		Clock::time_point nextRetry{};
		int placeFailures = 0;
		bool gaveUpNotified = false;
		std::uint64_t lastFrames = 0;
		Clock::time_point lastFrameSeen{};

		std::mutex statusLock;
		std::string status = "not installed";

		void SetStatus(std::string a_text)
		{
			std::scoped_lock lock(statusLock);
			status = std::move(a_text);
		}

		using UpdateFn = void (*)(RE::PlayerCharacter*, float);
		UpdateFn originalUpdate = nullptr;

		std::string Point(const RE::NiPoint3& a_p)
		{
			return std::format("({:.0f}, {:.0f}, {:.0f})", a_p.x, a_p.y, a_p.z);
		}

		void Attach(bool a_marker, std::string_view a_why)
		{
			if (a_marker == attachedToMarker) {
				return;
			}
			attachedToMarker = a_marker;
			attachID = a_marker ? markerID.load() : kPlayer;
			Log("prompts attach to {} ({})", a_marker ? std::format("the marker {:08X}", markerID.load()) : "the player"s, a_why);
			// SkyPrompt keeps a queued prompt's reference; every prompt is offered again on its next tick.
			Prompts::WithdrawEverything();
		}

		void Move(RE::PlayerCharacter* a_player)
		{
			auto* camera = RE::PlayerCamera::GetSingleton();
			const bool firstPerson = camera && camera->IsInFirstPerson();
			if (firstPerson || hookDead || markerID.load() == 0) {
				Attach(false, firstPerson ? "first person" : hookDead ? "update hook not called" : "no marker");
				return;
			}
			const auto ref = marker.get();
			if (!ref || ref->IsDeleted()) {
				markerID = 0;
				markerLost = true;
				Log("WARN the marker is gone: prompts go to the player, and a new marker is placed");
				Attach(false, "marker lost");
				return;
			}
			const auto* middle = a_player->GetMiddleHighProcess();
			const auto* head = middle ? middle->headNode : nullptr;
			if (!head) {
				return;  // a transformation or a missing process; the next frame tries again
			}
			Attach(true, "third person");
			const auto& headPos = head->world.translate;
			// Ahead of the head along the direction the character faces (the user, N6), then to the
			// camera's right, the side SkyPrompt itself uses for actors.
			const float heading = a_player->GetAngleZ();
			const RE::NiPoint3 forward{ std::sin(heading), std::cos(heading), 0.0f };
			RE::NiPoint3 target = headPos + forward * kForward;
			if (const float r = right.load(); r != 0.0f && camera) {
				const auto toHead = headPos - camera->GetRuntimeData2().pos;
				target += toHead.UnitCross(RE::NiPoint3{ 0.0f, 0.0f, 1.0f }) * r;
			}
			target.z -= lift;
			if (setPosition) {
				ref->SetPosition(target);
			} else {
				ref->data.location = target;
			}
		}

		void UpdateHook(RE::PlayerCharacter* a_player, float a_delta)
		{
			// The previous function first, whoever it belongs to (Acheron and Grapple hook this slot too).
			originalUpdate(a_player, a_delta);
			++frames;
			if (a_player) {
				Move(a_player);
			}
		}

		// The bounds SkyPrompt reads (BoundingBox::GetOBB -> GetBoundMin/Max): zero bounds would put the
		// prompt at the world origin, because SkyPrompt ignores GetOBB's result.
		bool HasBounds(RE::TESObjectREFR* a_ref, RE::NiPoint3& a_min, RE::NiPoint3& a_max)
		{
			a_min = a_ref->GetBoundMin();
			a_max = a_ref->GetBoundMax();
			const auto size = a_max - a_min;
			return std::abs(size.x) > 1e-3f || std::abs(size.y) > 1e-3f || std::abs(size.z) > 1e-3f;
		}

		bool IsOurMarker(RE::TESObjectREFR* a_ref)
		{
			const auto* base = a_ref ? a_ref->GetBaseObject() : nullptr;
			return base && base->GetFormID() == kXMarker && !a_ref->IsDeleted();
		}

		// Uses the co-saved marker when it is still ours, else places a new one; game thread.
		bool Prepare(std::string_view a_why)
		{
			auto* player = Util::Player();
			RE::TESObjectREFR* ref = savedID ? RE::TESForm::LookupByID<RE::TESObjectREFR>(savedID) : nullptr;
			if (ref && !IsOurMarker(ref)) {
				Log("WARN co-saved marker {:08X} is no longer an XMarker reference; placing a new one", savedID);
				ref = nullptr;
			}
			const bool reused = ref != nullptr;
			RE::NiPointer<RE::TESObjectREFR> placed;
			if (!ref) {
				auto* base = RE::TESForm::LookupByID<RE::TESBoundObject>(kXMarker);
				placed = base && player ? player->PlaceObjectAtMe(base, true) : nullptr;
				ref = placed.get();
			}
			if (!ref) {
				Log("WARN could not place the marker ({}): prompts stay on the player", a_why);
				return false;
			}
			if (!ref->IsDisabled()) {
				ref->Disable();
			}
			RE::NiPoint3 min, max;
			bool bounds = HasBounds(ref, min, max);
			bool enabled = false;
			if (!bounds) {
				// A disabled marker without 3D may report no bounds; an enabled one has its 3D.
				Log("WARN the disabled marker reports no bounds; enabling it and moving it with SetPosition");
				ref->Enable(false);
				bounds = HasBounds(ref, min, max);
				enabled = true;
			}
			if (!bounds) {
				Log("WARN the marker has no bounds even when enabled: prompts stay on the player");
				return false;
			}
			setPosition = enabled;
			lift = max.z * ref->GetScale() + 10.0f;
			marker = ref->GetHandle();
			savedID = ref->GetFormID();
			markerID = savedID;
			Log("marker {:08X} {} ({}; {}): bounds {} .. {}, drawn {:.0f} units above it; forward {:.0f}, right {:.0f}", savedID,
				reused ? "reused" : "placed", a_why, enabled ? "enabled, SetPosition" : "disabled, position written directly", Point(min),
				Point(max), lift, kForward, right.load());
			SetStatus(std::format("marker {:08X} ({}), right {:.0f}", savedID, enabled ? "enabled" : "disabled", right.load()));
			return true;
		}
	}

	void Install()
	{
		REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_PlayerCharacter[0] };
		originalUpdate = reinterpret_cast<UpdateFn>(vtbl.write_vfunc(REL::Relocate(0xAD, 0xAD, 0xAF), &UpdateHook));
		Log("PlayerCharacter::Update hooked (chained: {})", originalUpdate != nullptr);
	}

	void OnGameLoaded()
	{
		markerID = 0;
		markerLost = false;
		attachID = kPlayer;
		attachedToMarker = false;
		marker = {};
		placeFailures = 0;
		gaveUpNotified = false;
		lastFrames = frames.load();
		lastFrameSeen = Clock::now();
		if (hookDead.exchange(false)) {
			Log("update hook check reset for this load");
		}
		if (!Prepare("load")) {
			++placeFailures;
			nextRetry = Clock::now() + kRetryEvery;
			SetStatus("no marker: prompts on the player");
		}
	}

	void Tick()
	{
		const auto now = Clock::now();
		// The update hook must keep being called while the game runs; Tick only runs then.
		if (const auto f = frames.load(); f != lastFrames) {
			lastFrames = f;
			lastFrameSeen = now;
		} else if (!hookDead && now - lastFrameSeen >= kHookSilence) {
			hookDead = true;
			Log("WARN PlayerCharacter::Update has not reached CIGAR for {} s (another mod replaced it?): prompts stay on the player",
				std::chrono::duration_cast<std::chrono::seconds>(kHookSilence).count());
			Util::NotifyDiagnostic(Text::L("CIGAR: 프롬프트 위치 갱신 중단. 플레이어 기준으로 표시. 로그 확인",
				"CIGAR: Prompt placement stopped updating; prompts stay on the player. See the log"));
			SetStatus("update hook not called: prompts on the player");
		}

		// Recovery: a lost marker, or one that could not be placed at load.
		if ((markerLost || markerID.load() == 0) && !hookDead && placeFailures < kMaxPlaceFailures && now >= nextRetry) {
			nextRetry = now + kRetryEvery;
			if (Prepare("recovery")) {
				markerLost = false;
			} else if (++placeFailures >= kMaxPlaceFailures && !gaveUpNotified) {
				gaveUpNotified = true;
				Log("WARN the marker could not be placed {} times: prompts stay on the player until the next load", kMaxPlaceFailures);
				Util::NotifyDiagnostic(Text::L("CIGAR: 프롬프트 마커를 만들 수 없음. 플레이어 기준으로 표시",
					"CIGAR: Could not place the prompt marker; prompts stay on the player"));
				SetStatus("marker failed: prompts on the player");
			}
		}
	}

	void Save(SKSE::SerializationInterface* a_intfc)
	{
		if (savedID == 0 || !a_intfc->OpenRecord('ANCH', 1)) {
			return;
		}
		a_intfc->WriteRecordData(savedID);
	}

	void Load(SKSE::SerializationInterface* a_intfc, std::uint32_t)
	{
		RE::FormID id = 0;
		if (a_intfc->ReadRecordData(id) && a_intfc->ResolveFormID(id, id)) {
			savedID = id;
		}
	}

	void Revert()
	{
		savedID = 0;
		markerID = 0;
		attachID = kPlayer;
	}

	RE::FormID RefID()
	{
		return attachID.load();
	}

	void SetRight(float a_units)
	{
		right = a_units;
		Log("right offset set to {:.0f}", a_units);
		if (const auto id = markerID.load()) {
			SetStatus(std::format("marker {:08X}, right {:.0f}", id, a_units));
		}
	}

	std::string Status()
	{
		std::scoped_lock lock(statusLock);
		return status;
	}
}
