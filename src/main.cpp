#include "BaboKey.h"
#include "Bathe.h"
#include "Deflate.h"
#include "Dress.h"
#include "Eat.h"
#include "Execute.h"
#include "Grapple.h"
#include "Jujutsu.h"
#include "ItemEquip.h"
#include "Light.h"
#include "LockOn.h"
#include "Module.h"
#include "Needs.h"
#include "Panel.h"
#include "Potion.h"
#include "Prompt.h"
#include "QuestTrack.h"
#include "Rest.h"
#include "Settings.h"
#include "Surrender.h"
#include "Util.h"
#include "WeaponSwap.h"

#include <set>

namespace CIGAR
{
	std::span<Module* const> Modules()
	{
		static const std::array<Module*, 17> modules{
			Bathe::GetSingleton(), Dress::GetSingleton(), BaboKey::GetSingleton(),
			LockOn::GetSingleton(), Grapple::GetSingleton(), Deflate::GetSingleton(), Surrender::GetSingleton(),
			Eat::GetSingleton(), WeaponSwap::GetSingleton(), Execute::GetSingleton(), Jujutsu::GetSingleton(),
			Needs::GetSingleton(), Potion::GetSingleton(), QuestTrack::GetSingleton(), ItemEquip::GetSingleton(),
			Rest::GetSingleton(), Light::GetSingleton()
		};
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
		if (!player || !player->Is3DLoaded() || (ui && ui->GameIsPaused()) || menuBlocked) {
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
			Util::ResetSIWarning();
			if (!Prompts::Available()) {
				logs::error("SkyPrompt is missing or incompatible; CIGAR shows no prompts");
				Util::Notify("CIGAR: SkyPrompt 없음 또는 버전 불일치. 프롬프트 비활성");
			}
			if (Prompts::HasDuplicateIDs()) {
				Util::Notify("CIGAR: 프롬프트 ID 중복. 한 키에 두 동작이 실행됨. 로그 확인");
			}
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
			// Valhalla Combat reads its settings file at kDataLoaded; set its execution key before that.
			Execute::GetSingleton()->PrepareKey();
			Panel::Register();
			break;
		case SKSE::MessagingInterface::kDataLoaded:
			Prompts::Init();
			if (auto* ui = RE::UI::GetSingleton()) {
				ui->AddEventSink<RE::MenuOpenCloseEvent>(MenuWatch::GetSingleton());
				logs::info("menu watch registered");
			} else {
				logs::error("UI unavailable: prompts will stay on screen over menus");
			}
			Dress::GetSingleton()->RegisterEvents();
			Needs::GetSingleton()->RegisterEvents();
			QuestTrack::GetSingleton()->RegisterEvents();
			ItemEquip::GetSingleton()->RegisterEvents();
			Grapple::GetSingleton()->ReadIni();
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
	}

	void OnLoad(SKSE::SerializationInterface* a_intfc)
	{
		std::uint32_t type = 0;
		std::uint32_t version = 0;
		std::uint32_t length = 0;
		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			if (type == 'DRES' && (version == 1 || version == 2)) {
				Dress::GetSingleton()->Load(a_intfc, version);
			} else {
				logs::warn("skipping unknown co-save record {:08X} v{}", type, version);
			}
		}
	}

	void OnRevert(SKSE::SerializationInterface*)
	{
		gameReady = false;
		Dress::GetSingleton()->Revert();
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();
	SKSE::Init(a_skse, false);

	const auto* plugin = SKSE::PluginDeclaration::GetSingleton();
	logs::info("{} {} loaded (runtime {})", plugin->GetName(), plugin->GetVersion().string(), a_skse->RuntimeVersion().string());

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
