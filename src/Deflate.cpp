#include "Deflate.h"

#include "Util.h"

namespace CIGAR
{
	namespace
	{
		constexpr auto kFHUPlugin = "sr_FillHerUp.esp"sv;
		constexpr RE::FormID kInflateQuestID = 0xD63;  // sr_inflateQuest
		constexpr std::uint32_t kPlayerAliasID = 1;     // "Player", script sr_infDeflateAbility
		constexpr auto kQuestScript = "sr_inflateQuest";
		constexpr auto kConfigScript = "sr_inflateConfig";
		constexpr auto kAbilityScript = "sr_infDeflateAbility";

		class TypeResult final : public RE::BSScript::IStackCallbackFunctor
		{
		public:
			void operator()(RE::BSScript::Variable a_result) override
			{
				Deflate::GetSingleton()->SetInflationType(a_result.IsInt() ? a_result.GetSInt() : -1);
			}

			void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}
		};

		class KeyResult final : public RE::BSScript::IStackCallbackFunctor
		{
		public:
			explicit KeyResult(const char* a_event) :
				event(a_event) {}

			void operator()(RE::BSScript::Variable) override
			{
				Deflate::GetSingleton()->Log("{} returned", event);
			}

			void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}

		private:
			const char* event;
		};
	}

	Deflate::Deflate()
	{
		deflate.SetHoldMode(true);
	}

	Deflate* Deflate::GetSingleton()
	{
		static Deflate singleton;
		return &singleton;
	}

	void Deflate::SetInflationType(std::int32_t a_type)
	{
		inflationType = a_type;
		queryPending = false;
	}

	bool Deflate::Resolve()
	{
		inflater = nullptr;
		config = nullptr;
		playerAlias = nullptr;
		inflateFaction = nullptr;
		oralFaction = nullptr;
		animatingFaction = nullptr;
		sexlabAnimating = nullptr;

		auto* handler = RE::TESDataHandler::GetSingleton();
		if (!handler || !handler->LookupModByName(kFHUPlugin)) {
			Log("Fill Her Up not found; the deflate prompt is off");
			return false;
		}
		auto* quest = handler->LookupForm<RE::TESQuest>(kInflateQuestID, kFHUPlugin);
		if (quest) {
			for (auto* alias : quest->aliases) {
				if (alias && alias->aliasID == kPlayerAliasID && alias->GetVMTypeID() == RE::BGSRefAlias::VMTYPEID) {
					playerAlias = static_cast<RE::BGSRefAlias*>(alias);
				}
			}
		}
		const auto ability = Util::ScriptObject(playerAlias, kAbilityScript);
		inflater = Util::ScriptProperty<RE::TESQuest>(ability, "inflater");
		config = Util::ScriptProperty<RE::TESQuest>(ability, "config");
		const auto questScript = Util::ScriptObject(inflater, kQuestScript);
		inflateFaction = Util::ScriptProperty<RE::TESFaction>(questScript, "inflateFaction");
		oralFaction = Util::ScriptProperty<RE::TESFaction>(questScript, "SR_InflateOralFaction");
		animatingFaction = Util::ScriptProperty<RE::TESFaction>(questScript, "inflaterAnimatingFaction");
		sexlabAnimating = Util::ScriptProperty<RE::TESFaction>(questScript, "slAnimatingFaction");

		Log("FHU quest={} alias={} ability={} inflater={} config={} inflateFaction={} oralFaction={} animating={} sexlab={} key={}",
			quest ? std::format("{:08X}", quest->GetFormID()) : "-", playerAlias != nullptr, static_cast<bool>(ability),
			inflater != nullptr, config != nullptr, inflateFaction != nullptr, oralFaction != nullptr,
			animatingFaction != nullptr, sexlabAnimating != nullptr, DeflateKey());
		return ability && questScript && inflateFaction && oralFaction && animatingFaction;
	}

	std::int32_t Deflate::DeflateKey() const
	{
		return Util::ScriptInt(Util::ScriptObject(config, kConfigScript), "defKey");
	}

	void Deflate::OnGameLoaded()
	{
		deflate.Reset();
		lastGate.clear();
		holding = false;
		inflationType = -1;
		queryPending = false;

		if (!Resolve()) {
			if (playerAlias || inflater) {
				Log("WARN Fill Her Up found but its scripts did not resolve; the deflate prompt is off");
				Util::Notify("CIGAR: FHU 연동 실패. 배출 프롬프트 비활성");
			}
			inflater = nullptr;
			return;
		}
		Log("ready");
	}

	void Deflate::QueryInflationType()
	{
		if (queryPending.exchange(true)) {
			return;
		}
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* args = RE::MakeFunctionArguments(static_cast<RE::Actor*>(Util::Player()));
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback{ new TypeResult() };
		if (!vm->DispatchMethodCall2(Util::Handle(inflater), kQuestScript, "GetMostRecentInflationType", args, callback)) {
			queryPending = false;
		}
	}

	void Deflate::Tick()
	{
		if (!inflater) {
			return;
		}
		auto* player = Util::Player();
		// FHU keeps the player in these factions while it tracks a pool; the oral one can stay at
		// rank 0 after emptying, so FHU's own GetMostRecentInflationType (the test its key uses)
		// decides.
		const bool tracked = player->IsInFaction(inflateFaction) || player->IsInFaction(oralFaction);
		if (tracked) {
			QueryInflationType();
		} else {
			inflationType = 0;
		}
		const auto type = inflationType.load();
		const bool animating = player->IsInFaction(animatingFaction);
		const bool sexlab = sexlabAnimating && player->IsInFaction(sexlabAnimating);

		LogGate(std::format("tracked={} type={} animating={} sexlab={} holding={}", tracked, type, animating, sexlab, holding));
		// While held, FHU itself is animating; keep the prompt so the release still arrives.
		const bool can = holding || (tracked && type > 0 && !animating && !sexlab);
		deflate.Update(can, [] { return "배출 (길게 누르기)"s; });
	}

	void Deflate::SendKey(const char* a_event, bool a_down)
	{
		const auto key = DeflateKey();
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		RE::BSScript::IFunctionArguments* args = nullptr;
		if (a_down) {
			args = RE::MakeFunctionArguments(static_cast<std::int32_t>(key));
		} else {
			const std::chrono::duration<float> held = std::chrono::steady_clock::now() - holdStart;
			args = RE::MakeFunctionArguments(static_cast<std::int32_t>(key), held.count());
		}
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback{ new KeyResult(a_event) };
		// The ability compares the key code with config.defKey, so pass that value.
		const bool queued = vm->DispatchMethodCall2(Util::Handle(playerAlias), kAbilityScript, a_event, args, callback);
		Log("{}({}) requested queued={}", a_event, key, queued);
	}

	void Deflate::OnDisabled()
	{
		// A switch-off mid-hold must still release FHU's key, or its push loop keeps running.
		OnHold(kDeflate, false);
	}

	void Deflate::OnHold(std::uint16_t a_eventID, bool a_down)
	{
		if (!inflater || a_eventID != kDeflate) {
			return;
		}
		if (a_down) {
			if (holding) {
				return;
			}
			holding = true;
			holdStart = std::chrono::steady_clock::now();
			SendKey("OnKeyDown", true);
		} else {
			if (!holding) {
				return;
			}
			holding = false;
			SendKey("OnKeyUp", false);
		}
	}
}
