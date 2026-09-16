#include "Bathe.h"
#include "Dress.h"
#include "Module.h"
#include "Prompt.h"
#include "Util.h"

namespace CIGAR
{
	std::span<Module* const> Modules()
	{
		static const std::array<Module*, 2> modules{ Bathe::GetSingleton(), Dress::GetSingleton() };
		return modules;
	}
}

namespace
{
	using namespace CIGAR;

	constexpr std::uint32_t kSerializationID = 'CIGR';
	constexpr auto kTickInterval = 1000ms;

	std::atomic_bool gameReady{ false };
	std::atomic_bool tickQueued{ false };

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
		if (!gameReady) {
			return;
		}
		auto* ui = RE::UI::GetSingleton();
		const auto* player = Util::Player();
		if (!player || !player->Is3DLoaded() || (ui && ui->GameIsPaused())) {
			return;
		}
		for (auto* module : Modules()) {
			module->Tick();
		}
	}

	// Paces the tick from its own thread and posts one task per interval; a task that re-queues
	// itself would run the whole loop inside a single frame.
	void StartTicker()
	{
		static std::once_flag started;
		std::call_once(started, [] { std::thread([] {
			for (;;) {
				std::this_thread::sleep_for(kTickInterval);
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
			for (auto* module : Modules()) {
				module->OnGameLoaded();
			}
			gameReady = true;
		});
	}

	void OnMessage(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kDataLoaded:
			Prompts::Init();
			Dress::GetSingleton()->RegisterEvents();
			StartTicker();
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
			if (type == 'DRES' && version == 1) {
				Dress::GetSingleton()->Load(a_intfc);
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
