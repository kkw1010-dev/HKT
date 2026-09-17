#include "Surrender.h"

#include "Util.h"

namespace CIGAR
{
	namespace
	{
		constexpr auto kAcheronPlugin = "Acheron.esm"sv;
		constexpr RE::FormID kDefeatedKeywordID = 0x801;  // Acheron_Defeated
		constexpr auto kAcheronSettings = "Data/SKSE/Acheron/Settings.yaml"sv;
		constexpr auto kYKPlugin = "YameteKudasai.esp"sv;
		// Kudasai_SurrenderTimeoutEFF: YK marks the enemies of a surrender for 3 minutes, and its
		// surrender quest (Kudasai_Surrender, Enemy01) will not fill with a marked actor.
		constexpr RE::FormID kYKTimeoutEffectID = 0x808;
		constexpr float kEnemyRadius = 3000.0f;

		constexpr auto kSexLabPlugin = "SexLab.esm"sv;
		constexpr RE::FormID kSexLabAnimatingID = 0xE50F;

		constexpr auto kQuietAfterPress = 5s;

		// Offered below this health fraction. Acheron's knockdown threshold (fKdHealthThresh) is also
		// 0.2 on this modlist, so the prompt marks the window before the next hit can defeat the player.
		constexpr float kLowHealth = 0.20f;
		// Slow motion on the moment the prompt appears; Streamlined Interactions' low-health potion
		// prompt uses 3 s on this modlist.
		constexpr float kSlowMultiplier = 0.3f;
		constexpr auto kSlowDuration = 3s;
		// Text pulse between white and gold (ImGui colours are ABGR).
		constexpr std::uint32_t kWhite = 0xFFFFFFFF;
		constexpr std::uint32_t kGold = 0xFF28BEFF;
		constexpr float kPulseHz = 1.5f;

		std::uint32_t Mix(std::uint32_t a_from, std::uint32_t a_to, float a_t)
		{
			std::uint32_t result = 0;
			for (int shift = 0; shift < 32; shift += 8) {
				const float from = static_cast<float>((a_from >> shift) & 0xFF);
				const float to = static_cast<float>((a_to >> shift) & 0xFF);
				result |= static_cast<std::uint32_t>(from + (to - from) * a_t + 0.5f) << shift;
			}
			return result;
		}

		// Top-level "key: value" scalars of Acheron's flat Settings.yaml.
		std::optional<std::string> YamlScalar(std::string_view a_key)
		{
			std::ifstream file{ std::filesystem::path{ kAcheronSettings } };
			std::string line;
			while (file && std::getline(file, line)) {
				const auto colon = line.find(':');
				if (colon == std::string::npos || line.starts_with(' ') || line.starts_with('#')) {
					continue;
				}
				if (std::string_view{ line }.substr(0, colon) != a_key) {
					continue;
				}
				auto value = line.substr(colon + 1);
				const auto first = value.find_first_not_of(" \t\"");
				const auto last = value.find_last_not_of(" \t\r\"");
				return first == std::string::npos ? std::string{} : value.substr(first, last - first + 1);
			}
			return std::nullopt;
		}

		std::optional<std::int64_t> YamlInt(std::string_view a_key)
		{
			const auto text = YamlScalar(a_key);
			std::int64_t value = 0;
			if (!text || std::from_chars(text->data(), text->data() + text->size(), value).ec != std::errc{}) {
				return std::nullopt;
			}
			return value;
		}
	}

	Surrender::Surrender()
	{
		// Surrendering cannot be undone, so it takes a full hold.
		surrender.SetPromptType(SkyPromptAPI::kHold);
	}

	Surrender* Surrender::GetSingleton()
	{
		static Surrender singleton;
		return &singleton;
	}

	void Surrender::OnGameLoaded()
	{
		EndSlow("game loaded");
		surrender.Reset();
		surrender.SetColor(kWhite);
		lastGate.clear();
		quietUntil = {};
		active = false;
		wasOffered = false;
		slowedThisEpisode = false;

		auto* handler = RE::TESDataHandler::GetSingleton();
		const bool esm = handler && handler->LookupModByName(kAcheronPlugin);
		const bool dll = GetModuleHandleW(L"Acheron.dll") != nullptr;
		if (!esm || !dll) {
			Log("Acheron not found (esm={} dll={}); the surrender prompt is off", esm, dll);
			return;
		}
		defeated = handler->LookupForm<RE::BGSKeyword>(kDefeatedKeywordID, kAcheronPlugin);
		ykTimeout = handler->LookupModByName(kYKPlugin) ? handler->LookupForm<RE::EffectSetting>(kYKTimeoutEffectID, kYKPlugin) : nullptr;
		sexlabAnimating = handler->LookupModByName(kSexLabPlugin) ? handler->LookupForm<RE::TESFaction>(kSexLabAnimatingID, kSexLabPlugin) : nullptr;

		// Acheron reads these once at startup; the MCM writes them back to the same file.
		const auto processing = YamlScalar("ProcessingEnabled");
		const auto key = YamlInt("iSurrenderKey");
		const auto modifier = YamlInt("iHunterPrideKeyMod");
		surrenderKey = key.value_or(-1);
		const bool yk = handler->LookupModByName(kYKPlugin) != nullptr;
		Log("Acheron settings: processing={} surrenderKey={} modifier={} defeatedKeyword={} yk={} ykTimeout={}",
			processing.value_or("?"), surrenderKey, modifier.value_or(-1), defeated != nullptr, yk, ykTimeout != nullptr);

		std::string problem;
		if (!key) {
			problem = "iSurrenderKey not readable from " + std::string{ kAcheronSettings };
		} else if (surrenderKey < 0 || surrenderKey >= 264) {
			problem = std::format("surrender key {} is unset or not a keyboard/mouse key", surrenderKey);
		} else if (modifier.value_or(-1) > -1) {
			// Acheron requires its Hunter's Pride modifier for the surrender key as well.
			problem = "Acheron's modifier key is set, which a single press cannot hold";
		} else if (processing && *processing != "true") {
			problem = "Acheron processing is disabled";
		}
		if (!problem.empty()) {
			Log("WARN {}; the surrender prompt is off", problem);
			if (!warned) {
				warned = true;
				Util::Notify("CIGAR: Acheron 항복 키 사용 불가. 항복 프롬프트 비활성");
			}
			return;
		}
		active = true;
		Log("ready");
	}

	bool Surrender::Blocked(RE::PlayerCharacter* a_player, std::string& a_why) const
	{
		const auto* controls = RE::ControlMap::GetSingleton();
		const float maxHealth = a_player->GetActorValueMax(RE::ActorValue::kHealth);
		const float health = a_player->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
		const float fraction = maxHealth > 0.0f ? health / maxHealth : 1.0f;
		if (!a_player->IsInCombat()) {
			a_why = "no-combat";
		} else if (fraction >= kLowHealth) {
			a_why = "health";
		} else if (a_player->IsDead() || a_player->AsActorState()->IsBleedingOut()) {
			a_why = "down";
		} else if (defeated && a_player->HasKeyword(defeated)) {
			a_why = "defeated";
		} else if (sexlabAnimating && a_player->IsInFaction(sexlabAnimating)) {
			a_why = "sexlab";
		} else if (!controls || !controls->IsMovementControlsEnabled()) {
			a_why = "no-movement";
		} else if (Clock::now() < quietUntil) {
			a_why = "quiet";
		} else if (AllEnemiesTimedOut(a_player)) {
			// Acheron would only report that no surrender event was found.
			a_why = "yk-timeout";
		} else {
			a_why = "ok";
			return false;
		}
		return true;
	}

	bool Surrender::AllEnemiesTimedOut(RE::PlayerCharacter* a_player) const
	{
		if (!ykTimeout) {
			return false;
		}
		const auto enemies = Util::NearbyHostiles(a_player, kEnemyRadius);
		if (enemies.empty()) {
			return false;
		}
		return std::ranges::all_of(enemies, [this](RE::Actor* a_enemy) {
			auto* target = a_enemy->AsMagicTarget();
			return target && target->HasMagicEffect(ykTimeout);
		});
	}

	void Surrender::StartSlow()
	{
		auto* timer = RE::BSTimer::GetSingleton();
		if (!timer) {
			return;
		}
		const float before = RE::BSTimer::QGlobalTimeMultiplier();
		timer->SetGlobalTimeMultiplier(kSlowMultiplier, true);
		slowOwned = true;
		slowUntil = Clock::now() + kSlowDuration;
		Log("slow motion x{} (was x{})", kSlowMultiplier, before);
	}

	void Surrender::EndSlow(const char* a_reason)
	{
		if (!slowOwned) {
			return;
		}
		slowOwned = false;
		const float now = RE::BSTimer::QGlobalTimeMultiplier();
		// Leave the multiplier alone if something else changed it in the meantime.
		if (std::abs(now - kSlowMultiplier) > 0.01f) {
			Log("slow motion end ({}): multiplier is x{}, changed elsewhere; left as is", a_reason, now);
			return;
		}
		if (auto* timer = RE::BSTimer::GetSingleton()) {
			timer->SetGlobalTimeMultiplier(1.0f, true);
		}
		Log("slow motion end ({})", a_reason);
	}

	void Surrender::Pulse()
	{
		const std::chrono::duration<float> elapsed = Clock::now() - pulseStart;
		const float phase = 0.5f + 0.5f * std::sin(elapsed.count() * kPulseHz * 2.0f * std::numbers::pi_v<float>);
		surrender.SetColor(Mix(kWhite, kGold, phase));
	}

	void Surrender::FastTick()
	{
		if (!active) {
			return;
		}
		std::string why;
		const bool offer = !Blocked(Util::Player(), why);
		LogGate(why);

		if (why == "no-combat" || why == "health") {
			slowedThisEpisode = false;  // the next drop below 20% is a new moment
		}
		if (offer && !wasOffered) {
			pulseStart = Clock::now();
			surrender.SetColor(kGold);
			if (!slowedThisEpisode) {
				slowedThisEpisode = true;
				StartSlow();
			}
		}
		surrender.Update(offer, [] { return "항복 (길게 누르기)"s; });
		wasOffered = offer;

		if (offer) {
			Pulse();
		}
		if (slowOwned && (!offer || Clock::now() >= slowUntil)) {
			EndSlow(offer ? "timer" : why.c_str());
		}
	}

	void Surrender::OnDisabled()
	{
		EndSlow("module switched off");
		wasOffered = false;
		slowedThisEpisode = false;
	}

	void Surrender::OnAccepted(std::uint16_t a_eventID)
	{
		if (!active || a_eventID != kSurrender) {
			return;
		}
		std::string why;
		if (Blocked(Util::Player(), why)) {
			EndSlow("accept ignored");
			Log("accept ignored: {}", why);
			return;
		}
		// Acheron finds the aggressor and consequence itself and shows its own message on failure.
		EndSlow("accepted");
		const bool pressed = Util::PressKey(surrenderKey);
		Log("surrender key {} pressed={}", surrenderKey, pressed);
		quietUntil = Clock::now() + kQuietAfterPress;
		// Offer again after the quiet period if the fight goes on.
		surrender.Reset();
		wasOffered = false;
	}
}
