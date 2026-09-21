#pragma once

namespace CIGAR::Util
{
	RE::PlayerCharacter* Player();

	// Shows a HUD notification (UTF-8).
	void Notify(const std::string& a_text);

	bool IsBusy(RE::Actor* a_actor);

	// Living, loaded actors hostile to a_actor within a_radius (high process only), nearest first.
	std::vector<RE::Actor*> NearbyHostiles(RE::Actor* a_actor, float a_radius);

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

	bool EqualsNoCase(std::string_view a_left, std::string_view a_right);
	bool ContainsNoCase(std::string_view a_haystack, std::string_view a_needle);

	// The script object of class a_class bound to a_form, or null.
	RE::BSTSmartPointer<RE::BSScript::Object> ScriptObject(RE::TESForm* a_form, const char* a_class);
	RE::BSTSmartPointer<RE::BSScript::Object> ScriptObject(RE::BGSRefAlias* a_alias, const char* a_class);
	// The VM handle of a form or alias, or the empty handle.
	RE::VMHandle Handle(RE::TESForm* a_form);
	RE::VMHandle Handle(RE::BGSRefAlias* a_alias);

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

	// An int property of a bound script object, or a_default.
	std::int32_t ScriptInt(const RE::BSTSmartPointer<RE::BSScript::Object>& a_object, const char* a_name, std::int32_t a_default = -1);
	bool ScriptBool(const RE::BSTSmartPointer<RE::BSScript::Object>& a_object, const char* a_name);
	// A float property of a bound script object, or a_default.
	float ScriptFloat(const RE::BSTSmartPointer<RE::BSScript::Object>& a_object, const char* a_name, float a_default = 0.0f);

	// An integer from a Data-relative INI file (read through MO2's VFS), or nullopt.
	std::optional<std::int64_t> IniInt(const std::filesystem::path& a_path, std::string_view a_section, std::string_view a_key);
	// Sets an integer in a Data-relative INI file (written through MO2's VFS), keeping every other line,
	// the byte-order mark and the line endings; adds the key or the section when missing.
	bool IniSetInt(const std::filesystem::path& a_path, std::string_view a_section, std::string_view a_key, std::int64_t a_value);

	// Presses and releases a key through the game's input event source, as SKSE-style key codes:
	// 0-255 keyboard scan codes, 256-263 mouse buttons. Input sinks such as True Directional
	// Movement see it like a real press. Returns false for other codes (gamepad is not supported).
	bool PressKey(std::int64_t a_code);
	// Dispatches a named vanilla input action (such as "Wait") through the same input event source.
	// Returns false only when the event source or event allocation is unavailable.
	bool SendUserEvent(std::string_view a_event);

	// Streamlined Interactions switches CIGAR replaces. SI rewrites its settings.json from its menu,
	// so the file is read as it is now. Returns 1 on, 0 off, -1 unreadable or SI absent.
	int SISetting(std::string_view a_jsonPointer);

	// Notifies once per session when a replaced SI switch is on again.
	void WarnIfSIModuleOn(std::string_view a_label, std::string_view a_jsonPointer);
	void ResetSIWarning();
}
