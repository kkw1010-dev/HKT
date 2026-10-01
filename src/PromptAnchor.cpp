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
		// The last Tick: a long gap (a pause, a menu) stopped both the ticks and the player's Update, so it
		// says nothing about the hook (review 2026-09-30).
		Clock::time_point lastTickAt{};

		std::mutex statusLock;
		std::string status = "not installed";

		void SetStatus(std::string a_text)
		{
			std::scoped_lock lock(statusLock);
			status = std::move(a_text);
		}

		using UpdateFn = void (*)(RE::PlayerCharacter*, float);
		// What CIGAR's hook calls on to: the slot's holder when CIGAR last hooked it. Written from Tick on a
		// re-hook, read by the hook on the main thread.
		std::atomic<UpdateFn> originalUpdate{ nullptr };
		// The function that held the slot at the first install: where a re-entered hook goes on, see UpdateHook.
		UpdateFn baseUpdate = nullptr;
		// Another DLL can write the slot again later with a chain that leaves CIGAR out (r13: valhallaCombat.dll,
		// minutes into play). CIGAR then hooks once more on top of it, at most this many times a session, so two
		// DLLs can never trade the slot back and forth for long.
		constexpr int kMaxRehooks = 3;
		int rehooks = 0;
		bool rehookUnconfirmed = false;

		std::string Point(const RE::NiPoint3& a_p)
		{
			return std::format("({:.0f}, {:.0f}, {:.0f})", a_p.x, a_p.y, a_p.z);
		}

		// 3.1.1 (promised on Nexus, the user's wording: a wider angle at which the prompt mark shows): SkyPrompt
		// draws a prompt where its reference projects on screen, so a marker outside the view takes the
		// prompts with it (a large right offset, a camera turned or zoomed in Observe). The marker is pulled
		// to the nearest place that is on screen: its own place, then without the right offset, then the head,
		// then a point straight ahead of the camera. 0 = its own place. Only at the default side offset; a
		// position the player chose in Options is never moved (5 = off screen, left as set).
		int pulled{ 0 };
		constexpr float kScreenEdge = 0.06f;
		constexpr float kAheadOfCamera = 200.0f;

		bool OnScreen(RE::NiCamera* a_camera, const RE::NiPoint3& a_point, float a_edge = kScreenEdge)
		{
			float x = 0.0f;
			float y = 0.0f;
			float z = 0.0f;
			return a_camera->WorldPtToScreenPt3(a_point, x, y, z, 1.0e-5f) && z > 0.0f && x > a_edge && x < 1.0f - a_edge && y > a_edge && y < 1.0f - a_edge;
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
			const auto* head = middle && a_player->Is3DLoaded() ? middle->headNode : nullptr;
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
			int now = 0;
			if (auto* view = RE::Main::WorldRootCamera()) {
				// Once pulled in, the marker goes back only when its own place is well inside the screen, so it
				// does not hop at the edge.
				if (!OnScreen(view, target, pulled ? kScreenEdge * 2.0f : kScreenEdge)) {
					// Only at the default side offset (the user, 2026-10-01): a position the player picked is
					// left where it is, on screen or not.
					if (std::abs(right.load() - Settings::kPromptRightDefault) > 0.01f) {
						now = 5;
					} else {
						RE::NiPoint3 plain = headPos + forward * kForward;
						plain.z -= lift;
						// A Gamebryo camera looks along its local x axis.
						const auto& m = view->world.rotate;
						const RE::NiPoint3 ahead = view->world.translate + RE::NiPoint3{ m.entry[0][0], m.entry[1][0], m.entry[2][0] } * kAheadOfCamera;
						if (OnScreen(view, plain)) {
							target = plain;
							now = 1;
						} else if (OnScreen(view, headPos)) {
							target = headPos;
							now = 2;
						} else if (OnScreen(view, ahead)) {
							target = ahead;
							now = 3;
						} else {
							now = 4;
						}
					}
				}
			}
			if (now != pulled) {
				pulled = now;
				Log("{}", now == 0 ? "marker back at its own place (on screen)" :
				          now == 1 ? "marker off screen: pulled in, without the right offset" :
				          now == 2 ? "marker off screen: pulled onto the head" :
				          now == 3 ? "marker and head off screen: put straight ahead of the camera" :
				          now == 5 ? "marker off screen at a custom position: left as set" :
				                     "prompts cannot show: no place for the marker is on screen (camera data unusable)");
			}
			if (setPosition) {
				ref->SetPosition(target);
			} else {
				ref->data.location = target;
			}
		}

		void UpdateHook(RE::PlayerCharacter* a_player, float a_delta)
		{
			// After a re-hook the function CIGAR chains to may itself chain back to this hook (it does when it
			// had saved CIGAR's hook as its own "original"). The second entry goes straight on to the function
			// from before CIGAR, so the loop closes after one round and the work below runs once a frame.
			thread_local bool inside = false;
			if (inside) {
				baseUpdate(a_player, a_delta);
				return;
			}
			inside = true;
			// The previous function first, whoever it belongs to (Acheron and Grapple hook this slot too).
			originalUpdate.load()(a_player, a_delta);
			inside = false;
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

	namespace
	{
		// Which DLL a code address belongs to, for the log.
		std::string ModuleOf(std::uintptr_t a_address)
		{
			HMODULE module = nullptr;
			if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
					reinterpret_cast<LPCWSTR>(a_address), &module) || !module) {
				return "unknown module";
			}
			wchar_t path[MAX_PATH]{};
			GetModuleFileNameW(module, path, MAX_PATH);
			return std::filesystem::path(path).filename().string();
		}

		// Read only: what sits in PlayerCharacter::Update's vtable slot now, and whether it is CIGAR's hook.
		std::string SlotOwner()
		{
			REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_PlayerCharacter[0] };
			const auto slot = *reinterpret_cast<const std::uintptr_t*>(vtbl.address() + sizeof(void*) * REL::Relocate(0xAD, 0xAD, 0xAF));
			return slot == reinterpret_cast<std::uintptr_t>(&UpdateHook) ? "still CIGAR's hook (so the chain after it stopped calling on)" :
			                                                                std::format("now {:X} in {}", slot, ModuleOf(slot));
		}
	}

	namespace
	{
		// The hook went silent and the slot holds someone else's function: put CIGAR's hook back on top and
		// chain to that function. Only this one vtable slot is written, as at Install. False when the slot is
		// still CIGAR's (the silence has another cause) or the limit is reached.
		bool Rehook()
		{
			REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_PlayerCharacter[0] };
			const auto index = REL::Relocate(0xAD, 0xAD, 0xAF);
			const auto mine = reinterpret_cast<std::uintptr_t>(&UpdateHook);
			const auto holder = *reinterpret_cast<const std::uintptr_t*>(vtbl.address() + sizeof(void*) * index);
			if (holder == mine || holder == 0) {
				return false;
			}
			if (rehooks >= kMaxRehooks) {
				Log("WARN update hook: the slot was taken again ({:X} in {}) after {} re-installs; giving up for this session", holder,
					ModuleOf(holder), rehooks);
				return false;
			}
			// The chain target first, so the hook never runs with a stale one.
			originalUpdate = reinterpret_cast<UpdateFn>(holder);
			const auto replaced = vtbl.write_vfunc(index, &UpdateHook);
			if (replaced != holder && replaced != mine && replaced != 0) {
				originalUpdate = reinterpret_cast<UpdateFn>(replaced);  // the slot changed between the read and the write
			}
			++rehooks;
			rehookUnconfirmed = true;
			Log("update hook re-installed (#{} of {}): the slot held {:X} in {}; CIGAR now chains to {:X} in {}", rehooks, kMaxRehooks, holder,
				ModuleOf(holder), reinterpret_cast<std::uintptr_t>(originalUpdate.load()),
				ModuleOf(reinterpret_cast<std::uintptr_t>(originalUpdate.load())));
			return true;
		}
	}

	void Install()
	{
		REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_PlayerCharacter[0] };
		baseUpdate = reinterpret_cast<UpdateFn>(vtbl.write_vfunc(REL::Relocate(0xAD, 0xAD, 0xAF), &UpdateHook));
		originalUpdate = baseUpdate;
		Log("PlayerCharacter::Update hooked (chained: {}, to {})", baseUpdate != nullptr, ModuleOf(reinterpret_cast<std::uintptr_t>(baseUpdate)));
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
		if (now - lastTickAt > 500ms) {
			lastFrameSeen = now;
		}
		lastTickAt = now;
		// The update hook must keep being called while the game runs; Tick only runs then.
		if (const auto f = frames.load(); f != lastFrames) {
			lastFrames = f;
			lastFrameSeen = now;
			if (rehookUnconfirmed) {
				rehookUnconfirmed = false;
				Log("update hook is being called again after re-install #{}", rehooks);
			}
		} else if (!hookDead && now - lastFrameSeen >= kHookSilence && Rehook()) {
			// One comparison of the slot, here where the silence is noticed anyway; the next silence is timed afresh.
			lastFrameSeen = now;
		} else if (!hookDead && now - lastFrameSeen >= kHookSilence) {
			hookDead = true;
			Log("WARN PlayerCharacter::Update has not reached CIGAR for {} s: prompts stay on the player; the vtable slot is {}",
				std::chrono::duration_cast<std::chrono::seconds>(kHookSilence).count(), SlotOwner());
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
