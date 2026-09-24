#pragma once

#include "SkyPrompt/API.hpp"

namespace CIGAR
{
	class Module;

	namespace Prompts
	{
		// Requests the SkyPrompt client ID. Returns false when SkyPrompt is missing or its API
		// major version differs.
		bool Init();
		bool Available();
		SkyPromptAPI::ClientID Client();
		// True when two prompts share an event ID (found by Init, which logs the pair).
		bool HasDuplicateIDs();
		// Takes every prompt of a_owner off the screen (game thread).
		void WithdrawAll(const Module* a_owner);
		// Takes every CIGAR prompt off the screen; each is offered again on its next tick (game thread).
		void WithdrawEverything();
	}

	// Every prompt's SkyPrompt event ID, unique across modules. SkyPrompt treats prompts with the
	// same (event, action) as one interaction, so a shared ID fires every owner at once. SkyPrompt
	// shows at most four event IDs per client at once; CIGAR gives each one on screen its own key
	// slot (Settings::PromptKeys), so distinct IDs also get distinct keys when they are up together.
	namespace PromptID
	{
		inline constexpr std::uint16_t kBathe = 1;
		inline constexpr std::uint16_t kShower = 2;
		inline constexpr std::uint16_t kUndress = 3;
		inline constexpr std::uint16_t kDress = 4;
		inline constexpr std::uint16_t kBaboAct = 5;
		inline constexpr std::uint16_t kLock = 6;
		inline constexpr std::uint16_t kGrapple = 7;
		inline constexpr std::uint16_t kDeflate = 8;
		inline constexpr std::uint16_t kSurrender = 9;
		inline constexpr std::uint16_t kEat = 10;
		inline constexpr std::uint16_t kRanged = 11;
		inline constexpr std::uint16_t kMelee = 12;
		inline constexpr std::uint16_t kExecute = 13;
		inline constexpr std::uint16_t kJujutsu = 14;
		inline constexpr std::uint16_t kUrinate = 15;
		inline constexpr std::uint16_t kDefecate = 16;
		inline constexpr std::uint16_t kDrink = 17;
		inline constexpr std::uint16_t kTrackQuest = 18;
		inline constexpr std::uint16_t kEquipItem = 19;
		inline constexpr std::uint16_t kSit = 20;
		inline constexpr std::uint16_t kLieDown = 21;
		inline constexpr std::uint16_t kLean = 23;
		inline constexpr std::uint16_t kPassTime = 24;
		inline constexpr std::uint16_t kWarmHands = 25;
		inline constexpr std::uint16_t kMakeLight = 26;
		inline constexpr std::uint16_t kRecharge = 27;

		inline constexpr std::array kAll{ kBathe, kShower, kUndress, kDress, kBaboAct, kLock, kGrapple, kDeflate, kSurrender, kEat, kRanged, kMelee, kExecute, kJujutsu, kUrinate, kDefecate, kDrink, kTrackQuest, kEquipItem, kSit, kLieDown, kLean, kPassTime, kWarmHands, kMakeLight, kRecharge };
		constexpr bool Unique()
		{
			for (std::size_t i = 0; i < kAll.size(); ++i) {
				for (std::size_t j = i + 1; j < kAll.size(); ++j) {
					if (kAll[i] == kAll[j]) {
						return false;
					}
				}
			}
			return true;
		}
		static_assert(Unique(), "prompt event IDs must be unique");
	}

	// One on-screen prompt. Each prompt is its own sink, because the installed SkyPrompt
	// (2.3.15) only removes prompts sink by sink.
	class PromptSlot final : public SkyPromptAPI::PromptSink
	{
	public:
		PromptSlot(Module* a_owner, SkyPromptAPI::EventID a_id);

		// Offers the prompt when a_can starts holding and withdraws it when it stops. While it holds,
		// the prompt is kept on screen (SkyPrompt would otherwise fade it out after its lifetime).
		// a_text is only called when the prompt is offered.
		void Update(bool a_can, const std::function<std::string()>& a_text);
		void Withdraw();
		void Reset() { offered = false; }
		// Offer the prompt again after it was accepted, as long as a_can still holds. Without this an
		// accepted prompt stays gone until a_can has been false once.
		void SetRepeat(bool a_repeat) { repeat = a_repeat; }
		// A hold prompt reports key down/up to OnHold and stays on screen when accepted.
		void SetHoldMode(bool a_hold) { hold = a_hold; }
		bool Offered() const { return offered; }
		SkyPromptAPI::EventID ID() const { return id; }
		// Changes the text colour (ImGui ABGR); an offered prompt is updated in place.
		void SetColor(std::uint32_t a_color);
		// Updates an offered prompt's text and progress (0-1) in place, re-sending only on a change.
		void SetLive(std::string a_text, float a_progress);
		// The keyboard key listed for this prompt while offered (SKSE key code), or 0.
		std::uint32_t Key() const { return offered ? key : 0; }
		// kHold shows SkyPrompt's progress ring and accepts only after a full hold.
		void SetPromptType(SkyPromptAPI::PromptType a_type) { promptType = a_type; }

		std::span<const SkyPromptAPI::Prompt> GetPrompts() const override;
		void ProcessEvent(SkyPromptAPI::PromptEvent a_event) const override;

	private:
		void Offer(std::string a_text);
		void KeepAlive();

		Module* owner;
		SkyPromptAPI::EventID id;
		std::string text;
		std::array<SkyPromptAPI::Prompt, 1> prompts;
		std::array<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>, 1> buttons{};
		bool offered{ false };
		bool hold{ false };
		bool repeat{ false };
		std::chrono::steady_clock::time_point lastSent{};
		std::uint32_t color{ 0xFFFFFFFF };
		float progress{ 0.0f };
		std::uint32_t key{ 0 };
		SkyPromptAPI::PromptType promptType{ SkyPromptAPI::kSinglePress };
	};
}
