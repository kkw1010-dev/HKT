#ifndef CIGAR_NEXUS
#	include "BaboKey.h"
#	include "Bathe.h"
#	include "Deflate.h"
#	include "Eat.h"
#	include "Execute.h"
#	include "Grapple.h"
#	include "LockOn.h"
#	include "Needs.h"
#	include "Surrender.h"
#endif
#include "Dress.h"
#include "Jujutsu.h"
#include "ItemEquip.h"
#include "BookRead.h"
#include "Recharge.h"
#include "ChairDrink.h"
#include "QuestAction.h"
#include "Helmet.h"
#include "Poison.h"
#include "Observe.h"
#include "PartyOutfit.h"
#include "Module.h"
#include "Panel.h"
#include "Potion.h"
#include "Prompt.h"
#include "QuestTrack.h"
#include "Rest.h"
#include "Settings.h"
#include "Util.h"
#include "WeaponSwap.h"

#include <set>

namespace CIGAR
{
	std::span<Module* const> Modules()
	{
#ifdef CIGAR_NEXUS
		// The Nexus edition: base-game modules only (docs/035-nexus-edition.md).
		static const std::array<Module*, 15> modules{
			Dress::GetSingleton(), WeaponSwap::GetSingleton(), Jujutsu::GetSingleton(), Potion::GetSingleton(),
			QuestTrack::GetSingleton(), ItemEquip::GetSingleton(), BookRead::GetSingleton(), Rest::GetSingleton(),
			Recharge::GetSingleton(), ChairDrink::GetSingleton(), QuestAction::GetSingleton(), Helmet::GetSingleton(),
			Poison::GetSingleton(), Observe::GetSingleton(), PartyOutfit::GetSingleton()
		};
#else
		static const std::array<Module*, 24> modules{
			Bathe::GetSingleton(), Dress::GetSingleton(), BaboKey::GetSingleton(),
			LockOn::GetSingleton(), Grapple::GetSingleton(), Deflate::GetSingleton(), Surrender::GetSingleton(),
			Eat::GetSingleton(), WeaponSwap::GetSingleton(), Execute::GetSingleton(), Jujutsu::GetSingleton(),
			Needs::GetSingleton(), Potion::GetSingleton(), QuestTrack::GetSingleton(), ItemEquip::GetSingleton(),
			BookRead::GetSingleton(),
			Rest::GetSingleton(), Recharge::GetSingleton(), ChairDrink::GetSingleton(),
			QuestAction::GetSingleton(), Helmet::GetSingleton(), Poison::GetSingleton(), Observe::GetSingleton(),
			PartyOutfit::GetSingleton()
		};
#endif
		return modules;
	}
}

namespace
{
	using namespace CIGAR;

	constexpr std::uint32_t kSerializationID = 'CIGR';
	constexpr auto kFastInterval = 100ms;
	constexpr int kFastPerTick = 10;

	std::atomic_bool gameReady{ false };
	std::atomic_bool tickQueued{ false };
	std::atomic_bool fullTickDue{ false };

	// Menus that take the screen or the input: while any is open CIGAR's prompts are off and the
	// modules do not run. Another player saw 탈의하기 over the Journal: ticks stopped when the game
	// paused, but the prompts already queued stayed drawn. Decided by the menu's own flags, so
	// menus added by other mods count too, and HUD-type overlays (HUD, cursor, fader, QuickLoot's
	// LootMenu) do not.
	std::set<std::string, std::less<>> blockingMenus;  // game thread
	std::atomic_bool menuBlocked{ false };

	bool Blocks(const RE::IMenu& a_menu)
	{
		using Flag = RE::UI_MENU_FLAGS;
		return a_menu.menuFlags.any(Flag::kPausesGame, Flag::kUsesCursor, Flag::kUpdateUsesCursor, Flag::kModal);
	}

	void MenuChanged(const std::string& a_name, bool a_opening)
	{
		if (a_opening) {
			auto* ui = RE::UI::GetSingleton();
			const auto menu = ui ? ui->GetMenu(a_name) : nullptr;
			if (!menu || !Blocks(*menu)) {
				return;
			}
			const bool first = blockingMenus.empty();
			blockingMenus.emplace(a_name);
			if (first) {
				menuBlocked = true;
				Prompts::WithdrawEverything();
				logs::info("menu '{}' opened: prompts off", a_name);
			}
		} else if (blockingMenus.erase(a_name) != 0 && blockingMenus.empty()) {
			menuBlocked = false;
			logs::info("menu '{}' closed: prompts back", a_name);
		}
	}

	// SKSE Menu Framework draws its windows with ImGui, not as game menus, so the menu events never
	// see it (a player saw prompts over its window, 2026-09-24). Its DLL exports
	// IsAnyBlockingWindowOpened, which the bundled header predates; looked up by name.
	bool FrameworkWindowOpen()
	{
		using func_t = bool (*)();
		static const auto func = [] {
			const auto module = GetModuleHandleW(L"SKSEMenuFramework");
			return module ? reinterpret_cast<func_t>(GetProcAddress(module, "IsAnyBlockingWindowOpened")) : nullptr;
		}();
		return func && func();
	}

	std::atomic_bool frameworkBlocked{ false };

	// Logs when the HUD menu's movie is shown or hidden, so a report of a vanishing HUD can be told
	// apart from CIGAR's own prompts going away.
	void LogHudVisibility(RE::UI* a_ui)
	{
		static int last = -1;
		const auto hud = a_ui ? a_ui->GetMenu(RE::HUDMenu::MENU_NAME) : nullptr;
		const int now = hud && hud->uiMovie ? (hud->uiMovie->GetVisible() ? 1 : 0) : -1;
		if (now != last) {
			logs::info("HUD menu movie {}", now == 1 ? "visible" : now == 0 ? "hidden" : "absent");
			last = now;
		}
		// The vanilla HUD's mode stack (the top entry is the mode in force, such as "All" or
		// "MovementDisabled") and its root alpha. Skyrim Party Sheet hides its overlay while the
		// player sits (reported 2026-09-24/25) and its DLL knows a "HUD Mode"; this tells whether
		// sitting changes the mode it may be reading.
		if (!hud || !hud->uiMovie) {
			return;
		}
		static std::string lastMode;
		RE::GFxValue modes;
		std::string mode = "?";
		if (hud->uiMovie->GetVariable(&modes, "_root.HUDMovieBaseInstance.HUDModes") && modes.IsArray() && modes.GetArraySize() > 0) {
			RE::GFxValue top;
			if (modes.GetElement(modes.GetArraySize() - 1, &top) && top.IsString()) {
				mode = std::format("{} (depth {})", top.GetString(), modes.GetArraySize());
			}
		}
		RE::GFxValue alpha;
		if (hud->uiMovie->GetVariable(&alpha, "_root.HUDMovieBaseInstance._alpha") && alpha.IsNumber()) {
			mode += std::format(" alpha {:.0f}", alpha.GetNumber());
		}
		// Skyrim Party Sheet 3.5's DLL gates its overlay on the same four facts ("[HUDGate] fighting=
		// movement= looking= menusShowing=", a debug-level line it never writes at its default level);
		// logging them here names the one a chair sit turns off.
		if (const auto* controls = RE::ControlMap::GetSingleton()) {
			mode += std::format(" controls fighting={} movement={} looking={} menusShowing={}",
				controls->IsFightingControlsEnabled(), controls->IsMovementControlsEnabled(),
				controls->IsLookingControlsEnabled(), a_ui->IsShowingMenus());
		}
		if (mode != lastMode) {
			logs::info("HUD mode {}", mode);
			lastMode = std::move(mode);
		}
	}

	class MenuWatch final : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
	public:
		static MenuWatch* GetSingleton()
		{
			static MenuWatch singleton;
			return &singleton;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
		{
			if (a_event) {
				std::string name{ a_event->menuName.c_str() };
				const bool opening = a_event->opening;
				SKSE::GetTaskInterface()->AddTask([name, opening] { MenuChanged(name, opening); });
			}
			return RE::BSEventNotifyControl::kContinue;
		}
	};

	void InitializeLog()
	{
		auto path = logs::log_directory();
		if (!path) {
			SKSE::stl::report_and_fail("Failed to find the SKSE log directory"sv);
		}
		*path /= "CIGAR.log"sv;
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
		auto log = std::make_shared<spdlog::logger>("CIGAR"s, std::move(sink));
		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);
		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%H:%M:%S] [%l] %v"s);
	}

	void RunTick()
	{
		tickQueued = false;
		const bool full = fullTickDue.exchange(false);
		if (!gameReady) {
			return;
		}
		auto* ui = RE::UI::GetSingleton();
		const auto* player = Util::Player();
		if (const bool open = FrameworkWindowOpen(); open != frameworkBlocked) {
			frameworkBlocked = open;
			if (open) {
				Prompts::WithdrawEverything();
			}
			logs::info("SKSE Menu Framework window {}: prompts {}", open ? "opened" : "closed", open ? "off" : "back");
		}
		LogHudVisibility(ui);
		if (menuBlocked && ui) {
			// Backstop for a close event that never came: drop menus the UI no longer has open.
			for (auto it = blockingMenus.begin(); it != blockingMenus.end();) {
				if (ui->IsMenuOpen(*it)) {
					++it;
				} else {
					logs::info("menu '{}' is no longer open (missed close): dropped", *it);
					it = blockingMenus.erase(it);
				}
			}
			menuBlocked = !blockingMenus.empty();
		}
		if (!player || !player->Is3DLoaded() || (ui && ui->GameIsPaused()) || menuBlocked || frameworkBlocked) {
			return;
		}
		for (auto* module : Modules()) {
			if (!Settings::Enabled(module->Name())) {
				continue;
			}
			module->FastTick();
			if (full) {
				module->Tick();
			}
		}
	}

	// Paces the ticks from its own thread and posts one task per 100 ms, running Tick() on every
	// tenth; a task that re-queues itself would run the whole loop inside a single frame.
	void StartTicker()
	{
		static std::once_flag started;
		std::call_once(started, [] { std::thread([] {
			for (int n = 1;; ++n) {
				std::this_thread::sleep_for(kFastInterval);
				if (n % kFastPerTick == 0) {
					fullTickDue = true;
				}
				if (gameReady && !tickQueued.exchange(true)) {
					SKSE::GetTaskInterface()->AddTask(RunTick);
				}
			}
		}).detach(); });
	}

	void OnGameLoaded()
	{
		SKSE::GetTaskInterface()->AddTask([] {
			if (!Prompts::Available()) {
				logs::error("SkyPrompt is missing or incompatible; CIGAR shows no prompts");
				Util::Notify(Text::L("CIGAR: SkyPrompt 없음 또는 버전 불일치. 프롬프트 비활성", "CIGAR: SkyPrompt is missing or incompatible. Prompts off"));
			}
			if (Prompts::HasDuplicateIDs()) {
				Util::Notify(Text::L("CIGAR: 프롬프트 ID 중복. 한 키에 두 동작이 실행됨. 로그 확인", "CIGAR: Duplicate prompt IDs. One key runs two actions. See the log"));
			}
			Util::ResolveScenes();
			for (auto* module : Modules()) {
				module->OnGameLoaded();
			}
			gameReady = true;
		});
	}

	void OnMessage(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kPostLoad:
			Settings::Load();
#ifndef CIGAR_NEXUS
			// Valhalla Combat reads its settings file at kDataLoaded; set its execution key before that.
			Execute::GetSingleton()->PrepareKey();
#endif
			break;
		case SKSE::MessagingInterface::kDataLoaded:
			// The language reads the game's own names, so it waits for the data; the panel's page
			// titles are registered in it.
			Text::Resolve();
			Panel::Register();
			Prompts::Init();
			if (auto* ui = RE::UI::GetSingleton()) {
				ui->AddEventSink<RE::MenuOpenCloseEvent>(MenuWatch::GetSingleton());
				logs::info("menu watch registered");
			} else {
				logs::error("UI unavailable: prompts will stay on screen over menus");
			}
			Dress::GetSingleton()->RegisterEvents();
#ifndef CIGAR_NEXUS
			Needs::GetSingleton()->RegisterEvents();
#endif
			QuestTrack::GetSingleton()->RegisterEvents();
			ItemEquip::GetSingleton()->RegisterEvents();
			BookRead::GetSingleton()->RegisterEvents();
#ifndef CIGAR_NEXUS
			Grapple::GetSingleton()->ReadIni();
#endif
			Jujutsu::InstallHook();
			StartTicker();
			break;
		case SKSE::MessagingInterface::kSaveGame:
			// Sent before the save is written, on the thread that saves.
			Rest::GetSingleton()->BeforeSave();
			break;
		case SKSE::MessagingInterface::kPreLoadGame:
			gameReady = false;
			break;
		case SKSE::MessagingInterface::kNewGame:
			logs::info("new game");
			OnGameLoaded();
			break;
		case SKSE::MessagingInterface::kPostLoadGame:
			logs::info("save loaded (success={})", a_msg->data != nullptr);
			OnGameLoaded();
			break;
		default:
			break;
		}
	}

	void OnSave(SKSE::SerializationInterface* a_intfc)
	{
		Dress::GetSingleton()->Save(a_intfc);
		Helmet::GetSingleton()->Save(a_intfc);
		PartyOutfit::GetSingleton()->Save(a_intfc);
	}

	void OnLoad(SKSE::SerializationInterface* a_intfc)
	{
		std::uint32_t type = 0;
		std::uint32_t version = 0;
		std::uint32_t length = 0;
		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			if (type == 'DRES' && (version == 1 || version == 2)) {
				Dress::GetSingleton()->Load(a_intfc, version);
			} else if (type == 'HELM' && version == 1) {
				Helmet::GetSingleton()->Load(a_intfc, version);
			} else if (type == 'QOUT' && version == 1) {
				PartyOutfit::GetSingleton()->Load(a_intfc, version);
			} else {
				logs::warn("skipping unknown co-save record {:08X} v{}", type, version);
			}
		}
	}

	void OnRevert(SKSE::SerializationInterface*)
	{
		gameReady = false;
		Dress::GetSingleton()->Revert();
		Helmet::GetSingleton()->Revert();
		PartyOutfit::GetSingleton()->Revert();
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();
	SKSE::Init(a_skse, false);

	const auto* plugin = SKSE::PluginDeclaration::GetSingleton();
#if defined(CIGAR_NEXUS)
	constexpr auto kEdition = "Nexus edition, base game only";
#elif defined(CIGAR_RELEASE)
	constexpr auto kEdition = "release";
#else
	constexpr auto kEdition = "author";
#endif
	logs::info("{} {} loaded ({}; runtime {})", plugin->GetName(), plugin->GetVersion().string(), kEdition, a_skse->RuntimeVersion().string());

	if (!SKSE::GetMessagingInterface()->RegisterListener(OnMessage)) {
		logs::critical("could not register the SKSE message listener");
		return false;
	}

	auto* serialization = SKSE::GetSerializationInterface();
	serialization->SetUniqueID(kSerializationID);
	serialization->SetSaveCallback(OnSave);
	serialization->SetLoadCallback(OnLoad);
	serialization->SetRevertCallback(OnRevert);
	return true;
}
