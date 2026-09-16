#include "Util.h"

#include <nlohmann/json.hpp>

namespace CIGAR::Util
{
	namespace
	{
		constexpr std::array kKeptSlots{ 31u, 40u, 41u, 43u, 50u, 51u };
		constexpr std::array kNoStripKeywords{ "SexLabNoStrip"sv, "OStimNoStrip"sv, "zad_Lockable"sv, "zad_QuestItem"sv };
		constexpr auto kSISettings = "Data/SKSE/Plugins/StreamlinedInteractions/settings.json"sv;

		bool warnedSIModule = false;

		RE::BGSBipedObjectForm::BipedObjectSlot SlotMask(std::uint32_t a_slot)
		{
			return static_cast<RE::BGSBipedObjectForm::BipedObjectSlot>(1u << (a_slot - 30));
		}
	}

	RE::PlayerCharacter* Player() { return RE::PlayerCharacter::GetSingleton(); }

	void Notify(const std::string& a_text)
	{
		RE::SendHUDMessage::ShowHUDMessage(a_text.c_str());
	}

	bool IsBusy(RE::Actor* a_actor)
	{
		return a_actor->IsInCombat() || a_actor->IsOnMount();
	}

	bool IsStrippable(const RE::TESObjectARMO* a_armor, std::uint32_t a_slot)
	{
		if (std::ranges::find(kKeptSlots, a_slot) != kKeptSlots.end()) {
			return false;
		}
		if (!a_armor->GetPlayable()) {
			return false;
		}
		return std::ranges::none_of(kNoStripKeywords, [&](std::string_view kw) { return a_armor->HasKeywordString(kw); });
	}

	std::vector<RE::TESObjectARMO*> GetStrippable(RE::Actor* a_actor)
	{
		std::vector<RE::TESObjectARMO*> found;
		for (std::uint32_t slot = 30; slot < 62; ++slot) {
			auto* armor = a_actor->GetWornArmor(SlotMask(slot));
			if (armor && IsStrippable(armor, slot) && std::ranges::find(found, armor) == found.end()) {
				found.push_back(armor);
			}
		}
		return found;
	}

	std::string DescribeWorn(RE::Actor* a_actor)
	{
		std::string worn;
		for (std::uint32_t slot = 30; slot < 62; ++slot) {
			if (auto* armor = a_actor->GetWornArmor(SlotMask(slot))) {
				worn += std::format(" {}:{}{}", slot, NameOf(armor), IsStrippable(armor, slot) ? "" : "(kept)");
			}
		}
		return worn;
	}

	std::string NameOf(const RE::TESForm* a_form)
	{
		if (!a_form) {
			return "-";
		}
		const char* name = a_form->GetName();
		if (name && *name) {
			return name;
		}
		return std::format("{:08X}", a_form->GetFormID());
	}

	std::int32_t ItemCount(RE::Actor* a_actor, RE::TESBoundObject* a_item)
	{
		const auto inventory = a_actor->GetInventory([a_item](RE::TESBoundObject& a_object) { return &a_object == a_item; });
		const auto it = inventory.find(a_item);
		return it != inventory.end() ? it->second.first : 0;
	}

	bool ContainsNoCase(std::string_view a_haystack, std::string_view a_needle)
	{
		const auto it = std::ranges::search(a_haystack, a_needle, [](char a, char b) {
			return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
		});
		return !it.empty();
	}

	RE::BSTSmartPointer<RE::BSScript::Object> ScriptObject(RE::TESForm* a_form, const char* a_class)
	{
		RE::BSTSmartPointer<RE::BSScript::Object> object;
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		if (!vm || !a_form) {
			return object;
		}
		auto* policy = vm->GetObjectHandlePolicy();
		const auto handle = policy->GetHandleForObject(a_form->GetFormType(), a_form);
		if (handle == policy->EmptyHandle()) {
			return object;
		}
		vm->FindBoundObject(handle, a_class, object);
		return object;
	}

	int SISetting(std::string_view a_jsonPointer)
	{
		std::ifstream file{ std::filesystem::path{ kSISettings } };
		if (!file) {
			return -1;
		}
		try {
			const auto json = nlohmann::json::parse(file);
			const nlohmann::json::json_pointer pointer{ std::string{ a_jsonPointer } };
			if (!json.contains(pointer)) {
				return -1;
			}
			const auto& value = json.at(pointer);
			if (value.is_boolean()) {
				return value.get<bool>() ? 1 : 0;
			}
			return -1;
		} catch (const std::exception& e) {
			logs::warn("could not read {}: {}", kSISettings, e.what());
			return -1;
		}
	}

	void WarnIfSIModuleOn(std::string_view a_label, std::string_view a_jsonPointer)
	{
		if (SISetting(a_jsonPointer) == 1 && !warnedSIModule) {
			warnedSIModule = true;
			logs::warn("Streamlined Interactions {} is on; its prompt will duplicate CIGAR's", a_label);
			Notify(std::format("CIGAR: SI 중복 모듈 켜짐 - {}", a_label));
		}
	}

	void ResetSIWarning() { warnedSIModule = false; }
}
