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

	std::vector<RE::Actor*> NearbyHostiles(RE::Actor* a_actor, float a_radius)
	{
		std::vector<std::pair<float, RE::Actor*>> found;
		auto* lists = RE::ProcessLists::GetSingleton();
		if (!lists || !a_actor) {
			return {};
		}
		const auto origin = a_actor->GetPosition();
		for (auto& handle : lists->highActorHandles) {
			const auto ptr = handle.get();
			auto* other = ptr.get();
			if (!other || other == a_actor || other->IsDead() || !other->Is3DLoaded()) {
				continue;
			}
			const float distance = origin.GetDistance(other->GetPosition());
			if (distance > a_radius || !other->IsHostileToActor(a_actor)) {
				continue;
			}
			found.emplace_back(distance, other);
		}
		std::ranges::sort(found, {}, &std::pair<float, RE::Actor*>::first);
		std::vector<RE::Actor*> result;
		for (auto& [distance, other] : found) {
			result.push_back(other);
		}
		return result;
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
		// A placed reference (an actor) has no name of its own in TESForm::GetName; the display name
		// is what the game shows (leveled and renamed actors included).
		if (const auto* ref = a_form->As<RE::TESObjectREFR>()) {
			if (const char* shown = const_cast<RE::TESObjectREFR*>(ref)->GetDisplayFullName(); shown && *shown) {
				return shown;
			}
			if (const auto* base = ref->GetBaseObject(); base && base->GetName() && *base->GetName()) {
				return base->GetName();
			}
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

	bool EqualsNoCase(std::string_view a_left, std::string_view a_right)
	{
		return a_left.size() == a_right.size() && std::ranges::equal(a_left, a_right, [](char a, char b) {
			return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
		});
	}

	bool ContainsNoCase(std::string_view a_haystack, std::string_view a_needle)
	{
		const auto it = std::ranges::search(a_haystack, a_needle, [](char a, char b) {
			return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
		});
		return !it.empty();
	}

	namespace
	{
		RE::VMHandle HandleOf(RE::VMTypeID a_type, const void* a_object)
		{
			auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			if (!vm) {
				return 0;
			}
			auto* policy = vm->GetObjectHandlePolicy();
			return a_object ? policy->GetHandleForObject(a_type, a_object) : policy->EmptyHandle();
		}

		RE::BSTSmartPointer<RE::BSScript::Object> BoundObject(RE::VMHandle a_handle, const char* a_class)
		{
			RE::BSTSmartPointer<RE::BSScript::Object> object;
			auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			if (vm && a_handle != vm->GetObjectHandlePolicy()->EmptyHandle()) {
				vm->FindBoundObject(a_handle, a_class, object);
			}
			return object;
		}
	}

	RE::VMHandle Handle(RE::TESForm* a_form)
	{
		return HandleOf(a_form ? static_cast<RE::VMTypeID>(a_form->GetFormType()) : 0, a_form);
	}

	RE::VMHandle Handle(RE::BGSRefAlias* a_alias)
	{
		return HandleOf(RE::BGSRefAlias::VMTYPEID, a_alias);
	}

	RE::BSTSmartPointer<RE::BSScript::Object> ScriptObject(RE::TESForm* a_form, const char* a_class)
	{
		return a_form ? BoundObject(Handle(a_form), a_class) : nullptr;
	}

	RE::BSTSmartPointer<RE::BSScript::Object> ScriptObject(RE::BGSRefAlias* a_alias, const char* a_class)
	{
		return a_alias ? BoundObject(Handle(a_alias), a_class) : nullptr;
	}

	std::int32_t ScriptInt(const RE::BSTSmartPointer<RE::BSScript::Object>& a_object, const char* a_name, std::int32_t a_default)
	{
		const auto* var = a_object ? a_object->GetProperty(a_name) : nullptr;
		return var && var->IsInt() ? var->GetSInt() : a_default;
	}

	bool ScriptBool(const RE::BSTSmartPointer<RE::BSScript::Object>& a_object, const char* a_name)
	{
		const auto* var = a_object ? a_object->GetProperty(a_name) : nullptr;
		return var && var->IsBool() && var->GetBool();
	}

	float ScriptFloat(const RE::BSTSmartPointer<RE::BSScript::Object>& a_object, const char* a_name, float a_default)
	{
		const auto* var = a_object ? a_object->GetProperty(a_name) : nullptr;
		return var && var->IsFloat() ? var->GetFloat() : a_default;
	}

	std::optional<std::int64_t> IniInt(const std::filesystem::path& a_path, std::string_view a_section, std::string_view a_key)
	{
		std::ifstream file{ a_path };
		if (!file) {
			return std::nullopt;
		}
		const auto trim = [](std::string_view s) {
			const auto first = s.find_first_not_of(" \t\r");
			if (first == std::string_view::npos) {
				return std::string_view{};
			}
			return s.substr(first, s.find_last_not_of(" \t\r") - first + 1);
		};
		std::string line;
		bool inSection = false;
		while (std::getline(file, line)) {
			const auto text = trim(line);
			if (text.empty() || text.front() == ';' || text.front() == '#') {
				continue;
			}
			if (text.front() == '[') {
				inSection = text.size() > 2 && EqualsNoCase(text.substr(1, text.size() - 2), a_section);
				continue;
			}
			const auto eq = text.find('=');
			if (!inSection || eq == std::string_view::npos || !EqualsNoCase(trim(text.substr(0, eq)), a_key)) {
				continue;
			}
			const auto value = trim(text.substr(eq + 1));
			std::int64_t result = 0;
			const auto [end, ec] = std::from_chars(value.data(), value.data() + value.size(), result);
			if (ec == std::errc{}) {
				return result;
			}
			return std::nullopt;
		}
		return std::nullopt;
	}

	bool IniSetInt(const std::filesystem::path& a_path, std::string_view a_section, std::string_view a_key, std::int64_t a_value)
	{
		std::string content;
		if (std::ifstream in{ a_path, std::ios::binary }) {
			content.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
		}
		constexpr std::string_view kBom = "\xEF\xBB\xBF";
		const bool bom = content.starts_with(kBom);
		if (bom) {
			content.erase(0, kBom.size());
		}
		const std::string eol = content.find("\r\n") != std::string::npos ? "\r\n" : "\n";
		const bool finalEol = content.empty() || content.ends_with('\n');
		std::vector<std::string> lines;
		for (std::size_t pos = 0; pos < content.size();) {
			const auto next = content.find('\n', pos);
			auto line = content.substr(pos, next == std::string::npos ? std::string::npos : next - pos);
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}
			lines.push_back(std::move(line));
			pos = next == std::string::npos ? content.size() : next + 1;
		}
		const auto trim = [](std::string_view s) {
			const auto first = s.find_first_not_of(" \t");
			return first == std::string_view::npos ? std::string_view{} : s.substr(first, s.find_last_not_of(" \t") - first + 1);
		};
		const auto entry = std::format("{} = {}", a_key, a_value);
		std::optional<std::size_t> sectionEnd;  // one past the section's last non-empty line
		bool inSection = false;
		bool done = false;
		for (std::size_t i = 0; i < lines.size() && !done; ++i) {
			const auto text = trim(lines[i]);
			if (text.starts_with('[')) {
				inSection = text.size() > 2 && EqualsNoCase(text.substr(1, text.size() - 2), a_section);
				if (inSection) {
					sectionEnd = i + 1;
				}
				continue;
			}
			if (!inSection || text.empty()) {
				continue;
			}
			sectionEnd = i + 1;
			const auto eq = text.find('=');
			if (text.front() != ';' && text.front() != '#' && eq != std::string_view::npos && EqualsNoCase(trim(text.substr(0, eq)), a_key)) {
				lines[i] = entry;
				done = true;
			}
		}
		if (!done && sectionEnd) {
			lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(*sectionEnd), entry);
		} else if (!done) {
			if (!lines.empty() && !lines.back().empty()) {
				lines.emplace_back();
			}
			lines.push_back(std::format("[{}]", a_section));
			lines.push_back(entry);
		}
		std::string out = bom ? std::string(kBom) : std::string{};
		for (std::size_t i = 0; i < lines.size(); ++i) {
			out += lines[i];
			if (i + 1 < lines.size() || finalEol) {
				out += eol;
			}
		}
		std::error_code ec;
		std::filesystem::create_directories(a_path.parent_path(), ec);
		std::ofstream file{ a_path, std::ios::binary | std::ios::trunc };
		file << out;
		file.close();
		return static_cast<bool>(file);
	}

	bool PressKey(std::int64_t a_code)
	{
		RE::INPUT_DEVICE device;
		std::uint32_t id;
		if (a_code >= 0 && a_code < 256) {
			device = RE::INPUT_DEVICE::kKeyboard;
			id = static_cast<std::uint32_t>(a_code);
		} else if (a_code >= 256 && a_code < 264) {
			device = RE::INPUT_DEVICE::kMouse;
			id = static_cast<std::uint32_t>(a_code - 256);
		} else {
			return false;
		}
		auto* manager = RE::BSInputDeviceManager::GetSingleton();
		if (!manager) {
			return false;
		}
		// No user event: the game's own controls ignore it, key-code listeners do not.
		const RE::BSFixedString none{ "" };
		auto* down = RE::ButtonEvent::Create(device, none, id, 1.0f, 0.0f);
		auto* up = RE::ButtonEvent::Create(device, none, id, 0.0f, 0.1f);
		if (!down || !up) {
			RE::free(down);
			RE::free(up);
			return false;
		}
		auto* source = static_cast<RE::BSTEventSource<RE::InputEvent*>*>(manager);
		RE::InputEvent* event = down;
		source->SendEvent(&event);
		event = up;
		source->SendEvent(&event);
		RE::free(down);
		RE::free(up);
		return true;
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
