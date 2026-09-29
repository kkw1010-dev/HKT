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
		// Forgets every decline and every accepted-and-spent prompt; on game load, where no situation carries over.
		void ClearDeclines();

		// Every SkyPrompt call CIGAR makes (send, remove) waits in a queue that a hook on the frame's
		// Present call drains, on the render thread, where SkyPrompt draws too. CIGAR's modules run on
		// the game's task thread, which runs alongside the render thread; SkyPrompt 2.4.0 draws from row
		// indices it collected before drawing, and a prompt removed from another thread in between made
		// it lock a null row (CTD 2026-09-28 23:30:09; docs/016). Installed at plugin load.
		void InstallRenderHook();
		// Called by main's tick: notes the first tick that runs while the render thread is inside
		// Present (the evidence that the two run at once), once per session.
		void NoteTick();
	}

	// What SkyPrompt reads through GetPrompts().
	struct PromptData
	{
		std::string text;
		SkyPromptAPI::PromptType type{ SkyPromptAPI::kSinglePress };
		RE::FormID refID{ 0 };
		// The keyboard or mouse key, and the gamepad button when the D-pad preset is on.
		std::array<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>, 2> buttons{};
		std::size_t buttonCount{ 0 };
		std::uint32_t color{ 0xFFFFFFFF };
		float progress{ 0.0f };
	};

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
		inline constexpr std::uint16_t kRecharge = 27;
		inline constexpr std::uint16_t kEquipShout = 30;
		inline constexpr std::uint16_t kChairDrink = 31;
		inline constexpr std::uint16_t kHelmetOff = 32;
		inline constexpr std::uint16_t kHelmetOn = 33;
		inline constexpr std::uint16_t kPoison = 34;
		inline constexpr std::uint16_t kObserve = 35;
		// 36 was GearSwap's (removed 2026-09-25); not reused.
		inline constexpr std::uint16_t kPartyOutfit = 37;
		inline constexpr std::uint16_t kPartyRevert = 38;
		inline constexpr std::uint16_t kReadBook = 39;
		inline constexpr std::uint16_t kMannequinSwap = 40;
		inline constexpr std::uint16_t kWizardWarrior = 41;
		inline constexpr std::uint16_t kWizardWarriorOff = 42;
		inline constexpr std::uint16_t kSqueezePast = 43;

		inline constexpr std::array kAll{ kBathe, kShower, kUndress, kDress, kBaboAct, kLock, kGrapple, kDeflate, kSurrender, kEat, kRanged, kMelee, kExecute, kJujutsu, kUrinate, kDefecate, kDrink, kTrackQuest, kEquipItem, kSit, kLieDown, kLean, kPassTime, kWarmHands, kRecharge, kEquipShout, kChairDrink, kHelmetOff, kHelmetOn, kPoison, kObserve, kPartyOutfit, kPartyRevert, kReadBook, kMannequinSwap, kWizardWarrior, kWizardWarriorOff, kSqueezePast };
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
		void Reset()
		{
			offered = false;
			spent = false;
		}
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
		// A prompt the player declines (SkyPrompt's double tap) stays hidden while the situation that
		// raised it lasts (the user's rule, 2026-09-29, for every prompt). By default the situation has
		// ended once a_can has been false for kDeclineRelease. With a distance, it ends once the player
		// is that far from where they declined or in another cell: for prompts whose a_can drops with
		// every step (Rest's poses), where "false once" would bring them back at the next stop.
		void SetDeclineDistance(float a_distance) { declineDistance = a_distance; }
		bool Declined() const { return declined; }
		// What the prompt is about (a target, a need): a decline ends as soon as this changes, even while
		// a_can stays true (review 2026-09-30: a new target or combat starting is a new situation).
		void SetSituation(std::uint64_t a_situation) { situation = a_situation; }
		void ClearDecline() { declined = false; }
		// Game thread: the player declined this prompt.
		void Decline();

		std::span<const SkyPromptAPI::Prompt> GetPrompts() const override;
		void ProcessEvent(SkyPromptAPI::PromptEvent a_event) const override;

		// Render thread only (the queue in Prompt.cpp): publishes a_data where GetPrompts() reads it and
		// sends the prompt, or removes it. a_note, when set, is the offer line logged with the result.
		void Deliver(bool a_send, PromptData&& a_data, const std::string& a_note);

	private:
		void Offer(std::string a_text);
		void KeepAlive();
		// True once the declined situation has ended (see SetDeclineDistance).
		bool DeclineOver(bool a_can);
		void Send(std::string a_note = {});

		Module* owner;
		SkyPromptAPI::EventID id;
		// Game thread: what the prompt should show.
		PromptData desired;
		bool offered{ false };
		bool hold{ false };
		bool repeat{ false };
		std::chrono::steady_clock::time_point lastSent{};
		std::uint32_t key{ 0 };
		SkyPromptAPI::PromptType promptType{ SkyPromptAPI::kSinglePress };
		// Accepted without repeat: withdrawn, and not re-sent (keep-alive, live text, colour) until a_can
		// has been false once (review 2026-09-30: the keep-alive re-sent it within 2 s on a key slot it had
		// already given back).
		bool spent{ false };
		bool declined{ false };
		float declineDistance{ 0.0f };
		RE::NiPoint3 declinedAt{};
		RE::FormID declinedCell{ 0 };
		std::uint64_t situation{ 0 };
		std::uint64_t declinedSituation{ 0 };
		std::chrono::steady_clock::time_point declineFalseSince{};
		// Render thread: what SkyPrompt reads. The text before the last change stays alive one more
		// round, for an event SkyPrompt queued with the old text.
		PromptData published;
		std::string previousText;
		std::array<SkyPromptAPI::Prompt, 1> prompts;
	};
}
