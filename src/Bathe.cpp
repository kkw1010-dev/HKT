#include "Bathe.h"

#include "Util.h"

namespace CIGAR
{
	namespace
	{
		constexpr auto kBiSPlugin = "Bathing in Skyrim.esp"sv;
		constexpr auto kBiSQuestScript = "mzinBatheQuest";
		constexpr RE::FormID kBiSQuestID = 0x6B;  // fallback when the quest's EditorID is unavailable
		constexpr float kWaterfallSearch = 3000.0f;

		class WashResult final : public RE::BSScript::IStackCallbackFunctor
		{
		public:
			explicit WashResult(bool a_shower) :
				shower(a_shower) {}

			void operator()(RE::BSScript::Variable a_result) override
			{
				const bool washed = a_result.IsBool() && a_result.GetBool();
				Bathe::GetSingleton()->Log("wash shower={} result={}", shower, washed);
			}

			void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}

		private:
			bool shower;
		};
	}

	Bathe* Bathe::GetSingleton()
	{
		static Bathe singleton;
		return &singleton;
	}

	bool Bathe::ResolveBiS()
	{
		bisQuest = RE::TESForm::LookupByEditorID<RE::TESQuest>("mzinBatheQuest");
		if (!bisQuest) {
			if (auto* handler = RE::TESDataHandler::GetSingleton(); handler && handler->LookupModByName(kBiSPlugin)) {
				bisQuest = handler->LookupForm<RE::TESQuest>(kBiSQuestID, kBiSPlugin);
			}
		}
		if (!bisQuest) {
			return false;
		}

		const auto script = Util::ScriptObject(bisQuest, kBiSQuestScript);
		dirtiness = Util::ScriptProperty<RE::TESGlobal>(script, "DirtinessPercentage");
		waterRestriction = Util::ScriptProperty<RE::TESGlobal>(script, "WaterRestrictionEnabled");
		animationKeyword = Util::ScriptProperty<RE::BGSKeyword>(script, "AnimationKeyword");
		waterfalls = Util::ScriptProperty<RE::BGSListForm>(script, "WaterfallList");
		const auto menu = Util::ScriptObject(Util::ScriptProperty<RE::TESQuest>(script, "Menu"), "mzinBatheMCMMenu");
		bisEnabled = Util::ScriptProperty<RE::TESGlobal>(menu, "BathingInSkyrimEnabled");

		Log("BiS quest {:08X} script={} enabled={} dirt={} restriction={} animKeyword={} waterfalls={}",
			bisQuest->GetFormID(), static_cast<bool>(script), bisEnabled != nullptr, dirtiness != nullptr,
			waterRestriction != nullptr, animationKeyword != nullptr, waterfalls != nullptr);
		return script && bisEnabled && dirtiness;
	}

	void Bathe::OnGameLoaded()
	{
		bathe.Reset();
		shower.Reset();
		lastGate.clear();
		wasInWater = false;
		warnedBisOff = false;

		if (!ResolveBiS()) {
			bisQuest = nullptr;
			Log("Bathing in Skyrim - Renewed not found or not bound; bathing prompts are off");
			return;
		}
		Log("ready");
	}

	bool Bathe::UnderWaterfall(RE::PlayerCharacter* a_player) const
	{
		if (!waterfalls) {
			return false;
		}
		// BiS: the closest waterfall within 3000 units, with the player below its top (+1280)
		// and within 256 units horizontally.
		RE::TESObjectREFR* closest = nullptr;
		float best = kWaterfallSearch;
		const auto origin = a_player->GetPosition();
		RE::TES::GetSingleton()->ForEachReferenceInRange(a_player, kWaterfallSearch, [&](RE::TESObjectREFR* a_ref) {
			const auto* base = a_ref ? a_ref->GetBaseObject() : nullptr;
			if (base && waterfalls->HasForm(base)) {
				const float distance = origin.GetDistance(a_ref->GetPosition());
				if (distance < best) {
					best = distance;
					closest = a_ref;
				}
			}
			return RE::BSContainer::ForEachResult::kContinue;
		});
		if (!closest) {
			return false;
		}
		const auto fall = closest->GetPosition();
		return origin.z <= fall.z + 1280.0f && std::abs(origin.x - fall.x) <= 256.0f && std::abs(origin.y - fall.y) <= 256.0f;
	}

	std::string Bathe::DirtText() const
	{
		return std::format(" ({}%)", static_cast<int>(dirtiness->value * 100.0f));
	}

	void Bathe::Tick()
	{
		if (!bisQuest) {
			return;
		}
		auto* player = Util::Player();
		// While BiS animates the player, leave the offer state alone so the prompt does not
		// come back the moment the wash finishes.
		if (animationKeyword && player->HasMagicEffectWithKeyword(animationKeyword)) {
			return;
		}

		const bool inWater = player->IsInWater();
		const bool bisOn = bisEnabled->value > 0.0f;
		const bool busy = Util::IsBusy(player);
		const std::size_t strippable = inWater && !busy ? Util::GetStrippable(player).size() : 0;
		// With BiS's water restriction off, BiS treats everywhere as a waterfall, so only a real
		// waterfall is worth a separate prompt.
		const bool restricted = waterRestriction && waterRestriction->value != 0.0f;
		const bool underFall = inWater && strippable == 0 && !busy && restricted && UnderWaterfall(player);

		LogGate(std::format("water={} waterfall={} bis={} strippable={} busy={}", inWater, underFall, bisOn, strippable, busy));
		if (inWater && !wasInWater) {
			if (!bisOn && !warnedBisOff) {
				warnedBisOff = true;
				Log("WARN Bathing in Skyrim is disabled in its MCM; no bathe prompt is offered");
				Util::Notify("CIGAR: BiS 비활성 상태. MCM에서 켜야 목욕 프롬프트 표시");
			}
		}
		wasInWater = inWater;

		bathe.Update(inWater && !busy && strippable == 0 && bisOn, [this] { return "목욕하기" + DirtText(); });
		shower.Update(underFall && bisOn, [this] { return "샤워하기" + DirtText(); });
	}

	void Bathe::OnAccepted(std::uint16_t a_eventID)
	{
		if (!bisQuest || (a_eventID != kBathe && a_eventID != kShower)) {
			return;
		}
		bathe.Withdraw();
		shower.Withdraw();

		const bool isShower = a_eventID == kShower;
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* policy = vm->GetObjectHandlePolicy();
		const auto handle = policy->GetHandleForObject(bisQuest->GetFormType(), bisQuest);
		auto* args = RE::MakeFunctionArguments(static_cast<RE::Actor*>(Util::Player()), static_cast<RE::TESObjectMISC*>(nullptr), static_cast<bool>(isShower), true);
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback{ new WashResult(isShower) };
		const bool queued = vm->DispatchMethodCall2(handle, kBiSQuestScript, "TryWashActor", args, callback);
		Log("wash requested shower={} queued={}", isShower, queued);
	}
}
