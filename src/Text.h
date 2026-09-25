#pragma once

namespace CIGAR::Text
{
	// Player-facing text (prompts, notifications, the control panel) in Korean or English. Every
	// string is written as a pair at its call site: Text::L("탈의하기", "Undress").
	enum class Language
	{
		kKorean,
		kEnglish
	};

	// Settings::LanguageChoice() is "ko", "en" or "auto". Auto reads the game's own text once the data
	// is loaded: a Hangul name on a base-game form means a Korean game, anything else English. Called
	// at kDataLoaded and whenever the panel changes the choice. Safe from any thread.
	void Resolve();
	Language Current();
	// What Resolve() decided and why, for the log and the panel.
	std::string Describe();

	inline const char* L(const char* a_ko, const char* a_en)
	{
		return Current() == Language::kEnglish ? a_en : a_ko;
	}

	template <class... Args>
	std::string F(std::string_view a_ko, std::string_view a_en, const Args&... a_args)
	{
		return std::vformat(Current() == Language::kEnglish ? a_en : a_ko, std::make_format_args(a_args...));
	}
}
