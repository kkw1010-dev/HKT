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
		// Takes every prompt of a_owner off the screen (game thread).
		void WithdrawAll(const Module* a_owner);
	}

	// One on-screen prompt. Each prompt is its own sink, because the installed SkyPrompt
	// (2.3.15) only removes prompts sink by sink.
	class PromptSlot final : public SkyPromptAPI::PromptSink
	{
	public:
		PromptSlot(Module* a_owner, SkyPromptAPI::EventID a_id);

		// Offers the prompt when a_can starts holding and withdraws it when it stops.
		// a_text is only called when the prompt is offered.
		void Update(bool a_can, const std::function<std::string()>& a_text);
		void Withdraw();
		void Reset() { offered = false; }
		// A hold prompt reports key down/up to OnHold and stays on screen when accepted.
		void SetHoldMode(bool a_hold) { hold = a_hold; }
		bool Offered() const { return offered; }
		// Changes the text colour (ImGui ABGR); an offered prompt is updated in place.
		void SetColor(std::uint32_t a_color);
		// kHold shows SkyPrompt's progress ring and accepts only after a full hold.
		void SetPromptType(SkyPromptAPI::PromptType a_type) { promptType = a_type; }

		std::span<const SkyPromptAPI::Prompt> GetPrompts() const override;
		void ProcessEvent(SkyPromptAPI::PromptEvent a_event) const override;

	private:
		void Offer(std::string a_text);

		Module* owner;
		SkyPromptAPI::EventID id;
		std::string text;
		std::array<SkyPromptAPI::Prompt, 1> prompts;
		bool offered{ false };
		bool hold{ false };
		std::uint32_t color{ 0xFFFFFFFF };
		SkyPromptAPI::PromptType promptType{ SkyPromptAPI::kSinglePress };
	};
}
