#include "Text.h"

#include "Settings.h"
#include "Util.h"

namespace CIGAR::Text
{
	namespace
	{
		std::atomic<Language> current{ Language::kEnglish };
		std::mutex lock;
		std::string described = "not resolved yet";

		// Base-game forms whose names every translation renames: gold, the iron sword, the lockpick.
		constexpr std::array kProbeForms{ RE::FormID{ 0x0000000F }, RE::FormID{ 0x00012EB7 }, RE::FormID{ 0x0000000A } };

		bool HasHangul(std::string_view a_text)
		{
			// UTF-8 Hangul syllables U+AC00-U+D7A3 start with the lead bytes 0xEA-0xED.
			for (std::size_t i = 0; i + 2 < a_text.size(); ++i) {
				const auto c = static_cast<unsigned char>(a_text[i]);
				if (c >= 0xEA && c <= 0xED) {
					const auto c1 = static_cast<unsigned char>(a_text[i + 1]);
					const auto c2 = static_cast<unsigned char>(a_text[i + 2]);
					const char32_t code = ((c & 0x0Fu) << 12) | ((c1 & 0x3Fu) << 6) | (c2 & 0x3Fu);
					if (code >= 0xAC00 && code <= 0xD7A3) {
						return true;
					}
				}
			}
			return false;
		}
	}

	void Resolve()
	{
		const auto choice = Settings::LanguageChoice();
		Language language = Language::kEnglish;
		std::string why;
		if (choice == "ko") {
			language = Language::kKorean;
			why = "set to Korean";
		} else if (choice == "en") {
			why = "set to English";
		} else {
			std::string names;
			for (const auto id : kProbeForms) {
				const auto* form = RE::TESForm::LookupByID(id);
				const std::string name = form ? Util::NameOf(form) : "";
				names += std::format("{}'{}'", names.empty() ? "" : " ", name);
				if (HasHangul(name)) {
					language = Language::kKorean;
				}
			}
			why = std::format("auto from the game's names {}", names);
		}
		current = language;
		std::scoped_lock guard(lock);
		described = std::format("{} ({})", language == Language::kKorean ? "Korean" : "English", why);
		logs::info("language: {}", described);
	}

	Language Current()
	{
		return current.load();
	}

	std::string Describe()
	{
		std::scoped_lock guard(lock);
		return described;
	}
}
