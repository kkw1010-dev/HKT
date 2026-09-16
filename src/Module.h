#pragma once

namespace CIGAR
{
	// A CIGAR interaction. Every method runs on the game thread.
	class Module
	{
	public:
		virtual ~Module() = default;

		virtual const char* Name() const = 0;
		// Resolves integrations and clears per-session state; runs after every new game or load.
		virtual void OnGameLoaded() = 0;
		// Runs about once a second while a game is loaded and not paused.
		virtual void Tick() = 0;
		virtual void OnAccepted(std::uint16_t a_eventID) = 0;

		// Logs a line tagged with the module name.
		template <class... Args>
		void Log(std::format_string<Args...> a_fmt, Args&&... a_args) const
		{
			logs::info("[{}] {}", Name(), std::format(a_fmt, std::forward<Args>(a_args)...));
		}

		// Logs the prompt-gate inputs only when they change, so a missing prompt is explained by the log.
		void LogGate(std::string a_gate)
		{
			if (a_gate != lastGate) {
				Log("gate {}", a_gate);
				lastGate = std::move(a_gate);
			}
		}

	protected:
		std::string lastGate;
	};

	std::span<Module* const> Modules();
}
