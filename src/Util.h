#pragma once

namespace CIGAR::Util
{
	RE::PlayerCharacter* Player();

	// Shows a HUD notification (UTF-8).
	void Notify(const std::string& a_text);

	bool IsBusy(RE::Actor* a_actor);

	// Hair, tail, ears and decapitation slots belong to the body. Non-playable items (such as
	// the Softbody SMP collision carrier HDTSMPObjectBase in slot 60), no-strip items and
	// locked devices stay on.
	bool IsStrippable(const RE::TESObjectARMO* a_armor, std::uint32_t a_slot);

	// Worn armour undress may remove, each item once.
	std::vector<RE::TESObjectARMO*> GetStrippable(RE::Actor* a_actor);

	// "slot:name" for every worn slot, "(kept)" marking what undress leaves on.
	std::string DescribeWorn(RE::Actor* a_actor);

	std::string NameOf(const RE::TESForm* a_form);

	std::int32_t ItemCount(RE::Actor* a_actor, RE::TESBoundObject* a_item);

	bool ContainsNoCase(std::string_view a_haystack, std::string_view a_needle);

	// The script object of class a_class bound to a_form, or null.
	RE::BSTSmartPointer<RE::BSScript::Object> ScriptObject(RE::TESForm* a_form, const char* a_class);

	// A form-typed Auto property of a bound script object, or null.
	template <class T>
	T* ScriptProperty(const RE::BSTSmartPointer<RE::BSScript::Object>& a_object, const char* a_name)
	{
		if (!a_object) {
			return nullptr;
		}
		const auto* var = a_object->GetProperty(a_name);
		if (!var || !var->IsObject()) {
			return nullptr;
		}
		return var->Unpack<T*>();
	}

	// Streamlined Interactions switches CIGAR replaces. SI rewrites its settings.json from its menu,
	// so the file is read as it is now. Returns 1 on, 0 off, -1 unreadable or SI absent.
	int SISetting(std::string_view a_jsonPointer);

	// Notifies once per session when a replaced SI switch is on again.
	void WarnIfSIModuleOn(std::string_view a_label, std::string_view a_jsonPointer);
	void ResetSIWarning();
}
