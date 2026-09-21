#include "PassTime.h"

#include "Util.h"

namespace CIGAR
{
	namespace
	{
		constexpr auto kIdleDelay = 5s;
		constexpr auto kWaitUserEvent = "Wait"sv;
	}

	PassTime::PassTime()
	{
		wait.SetPromptType(SkyPromptAPI::kHold);
	}

	PassTime* PassTime::GetSingleton()
	{
		static PassTime singleton;
		return &singleton;
	}

	void PassTime::OnGameLoaded()
	{
		wait.Reset();
		lastGate.clear();
		idleSince = Clock::now();
		Util::WarnIfSIModuleOn("IdleActions.enabled_passtime", "/MCP/modules/IdleActions/enabled_passtime");
		Log("ready; idle delay={}s", std::chrono::duration_cast<std::chrono::seconds>(kIdleDelay).count());
	}

	void PassTime::Tick()
	{
		Util::WarnIfSIModuleOn("IdleActions.enabled_passtime", "/MCP/modules/IdleActions/enabled_passtime");
	}

	void PassTime::FastTick()
	{
		auto* player = Util::Player();
		auto* ui = RE::UI::GetSingleton();
		const auto* controls = RE::ControlMap::GetSingleton();
		const bool moving = player && player->IsMoving();
		const bool combat = player && player->IsInCombat();
		const bool movable = controls && controls->IsMovementControlsEnabled();
		const bool menu = !ui || ui->IsApplicationMenuOpen() || ui->IsMenuOpen(RE::SleepWaitMenu::MENU_NAME);
		const bool canWait = player && player->CanSleepWait();
		const auto now = Clock::now();

		if (moving || combat || !movable || menu || !canWait) {
			idleSince = now;
		}
		const auto idleFor = now - idleSince;
		const bool available = player && !moving && !combat && movable && !menu && canWait && idleFor >= kIdleDelay;

		LogGate(std::format("moving={} combat={} movable={} menu={} canWait={} idle={:.1f}s",
			moving, combat, movable, menu, canWait,
			std::chrono::duration<float>(idleFor).count()));
		wait.Update(available, [] { return "시간 보내기 (길게)"s; });
	}

	void PassTime::OnAccepted(std::uint16_t a_eventID)
	{
		if (a_eventID != kWait) {
			return;
		}
		auto* player = Util::Player();
		auto* ui = RE::UI::GetSingleton();
		if (!player || !player->CanSleepWait() || player->IsInCombat() || !ui || ui->IsApplicationMenuOpen()) {
			Log("wait request ignored: player={} canWait={} combat={} menu={}", player != nullptr,
				player && player->CanSleepWait(), player && player->IsInCombat(), ui && ui->IsApplicationMenuOpen());
			return;
		}
		if (!Util::SendUserEvent(kWaitUserEvent)) {
			Log("wait request failed: input event source unavailable");
			Util::Notify("CIGAR: 시간 보내기 호출 실패. 로그 확인");
			return;
		}
		idleSince = Clock::now();
		wait.Withdraw();
		Log("requested Skyrim wait menu");
	}

	void PassTime::OnDisabled()
	{
		idleSince = Clock::now();
	}
}
