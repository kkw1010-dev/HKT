#include "PromptAnchor.h"

#include "Prompt.h"
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
		constexpr std::uint32_t kRecord = 'ANCH';
		constexpr std::uint32_t kVersion = 1;
		constexpr auto kSampleEvery = 5s;

		// The probe starts on the middle candidate (the user's examples: 25/40/60).
		std::atomic<float> distance{ 40.0f };
		// Read by PromptSlot::Offer (game thread), written by the update hook (main thread).
		std::atomic<RE::FormID> attachID{ kPlayer };
		std::atomic<RE::FormID> markerID{ 0 };  // 0: no usable marker
		std::atomic_bool setPosition{ false };  // enabled-marker fallback when a disabled one has no bounds

		// Main thread only.
		RE::ObjectRefHandle marker;
		RE::FormID savedID = 0;  // from the co-save
		float lift = 26.0f;      // SkyPrompt draws 10 units above the box top; XMarker's top is 16 up
		bool attachedToMarker = false;
		Clock::time_point nextSample{};

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

		void Move(RE::PlayerCharacter* a_player)
		{
			auto* camera = RE::PlayerCamera::GetSingleton();
			const bool firstPerson = camera && camera->IsInFirstPerson();
			const float d = distance.load();
			const bool want = !firstPerson && d > 0.0f && markerID.load() != 0;
			if (want != attachedToMarker) {
				attachedToMarker = want;
				attachID = want ? markerID.load() : kPlayer;
				Log("prompts attach to {} ({})", want ? std::format("the marker {:08X}", markerID.load()) : "the player"s,
					firstPerson ? "first person" : d <= 0.0f ? "distance 0: the player as before" : want ? "third person" : "no marker");
				// SkyPrompt keeps a queued prompt's reference; every prompt is offered again next tick.
				Prompts::WithdrawEverything();
			}
			if (!want) {
				return;
			}
			const auto ref = marker.get();
			if (!ref) {
				markerID = 0;
				Log("WARN the marker is gone: prompts fall back to the player");
				SetStatus("marker gone: prompts on the player");
				return;
			}
			const auto* middle = a_player->GetMiddleHighProcess();
			const auto* head = middle ? middle->headNode : nullptr;
			if (!head) {
				return;
			}
			// Ahead of the head, in the direction the character faces; no sideways part (the user).
			const float heading = a_player->GetAngleZ();
			const RE::NiPoint3 forward{ std::sin(heading), std::cos(heading), 0.0f };
			RE::NiPoint3 target = head->world.translate + forward * d;
			target.z -= lift;
			if (setPosition) {
				ref->SetPosition(target);
			} else {
				ref->data.location = target;
			}
			if (const auto now = Clock::now(); now >= nextSample) {
				nextSample = now + kSampleEvery;
				const auto cameraPos = camera ? camera->GetRuntimeData2().pos : RE::NiPoint3{};
				Log("sample: d={:.0f} head {} marker {} (drawn at z+{:.0f}), camera {} at {:.0f} units", d, Point(head->world.translate), Point(target),
					lift, Point(cameraPos), cameraPos.GetDistance(head->world.translate));
			}
		}

		void UpdateHook(RE::PlayerCharacter* a_player, float a_delta)
		{
			originalUpdate(a_player, a_delta);
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
		attachID = kPlayer;
		attachedToMarker = false;
		marker = {};
		auto* player = Util::Player();
		RE::TESObjectREFR* ref = savedID ? RE::TESForm::LookupByID<RE::TESObjectREFR>(savedID) : nullptr;
		const bool reused = ref != nullptr;
		RE::NiPointer<RE::TESObjectREFR> placed;
		if (!ref) {
			auto* base = RE::TESForm::LookupByID<RE::TESBoundObject>(kXMarker);
			placed = base && player ? player->PlaceObjectAtMe(base, true) : nullptr;
			ref = placed.get();
		}
		if (!ref) {
			Log("WARN could not place the marker: prompts stay on the player");
			SetStatus("no marker: prompts on the player");
			return;
		}
		if (!ref->IsDisabled()) {
			ref->Disable();
		}
		RE::NiPoint3 min, max;
		bool bounds = HasBounds(ref, min, max);
		setPosition = false;
		if (!bounds) {
			// A disabled marker without 3D may report no bounds; an enabled one has its 3D.
			Log("WARN the disabled marker reports no bounds; enabling it and moving it with SetPosition");
			ref->Enable(false);
			bounds = HasBounds(ref, min, max);
			setPosition = true;
		}
		if (!bounds) {
			Log("WARN the marker has no bounds even when enabled: prompts stay on the player");
			SetStatus("marker without bounds: prompts on the player");
			return;
		}
		lift = max.z * ref->GetScale() + 10.0f;
		marker = ref->GetHandle();
		savedID = ref->GetFormID();
		markerID = savedID;
		Log("marker {:08X} {} ({}): bounds {} .. {}, drawn {:.0f} units above it; distance {:.0f}", savedID, reused ? "reused" : "placed",
			setPosition ? "enabled, SetPosition" : "disabled, position written directly", Point(min), Point(max), lift, distance.load());
		SetStatus(std::format("marker {:08X} ({}), distance {:.0f}", savedID, setPosition ? "enabled" : "disabled", distance.load()));
	}

	void Save(SKSE::SerializationInterface* a_intfc)
	{
		if (savedID == 0 || !a_intfc->OpenRecord(kRecord, kVersion)) {
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

	float Distance()
	{
		return distance.load();
	}

	void SetDistance(float a_units)
	{
		distance = a_units;
		Log("distance set to {:.0f} ({})", a_units, a_units > 0.0f ? "ahead of the head" : "the player, as before");
		SetStatus(std::format("marker {:08X}, distance {:.0f}", markerID.load(), a_units));
	}

	std::string Status()
	{
		std::scoped_lock lock(statusLock);
		return status;
	}
}
