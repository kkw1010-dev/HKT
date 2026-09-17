#include "Prompt.h"

#include "Module.h"

namespace CIGAR
{
	namespace
	{
		SkyPromptAPI::ClientID clientID = 0;

		constexpr RE::FormID kPlayerRef = 0x14;

		const char* EventName(SkyPromptAPI::PromptEventType a_type)
		{
			switch (a_type) {
			case SkyPromptAPI::kAccepted:
				return "accepted";
			case SkyPromptAPI::kDeclined:
				return "declined";
			case SkyPromptAPI::kRemovedByMod:
				return "removed";
			case SkyPromptAPI::kTimingOut:
				return "timing-out";
			case SkyPromptAPI::kTimeout:
				return "timeout";
			case SkyPromptAPI::kDown:
				return "down";
			case SkyPromptAPI::kUp:
				return "up";
			case SkyPromptAPI::kMove:
				return "move";
			default:
				return "unknown";
			}
		}
	}

	bool Prompts::Init()
	{
		if (clientID == 0) {
			clientID = SkyPromptAPI::RequestClientID();
		}
		logs::info("SkyPrompt client id {} (API {}.{})", clientID, SkyPromptAPI::MAJOR, SkyPromptAPI::MINOR);
		return clientID != 0;
	}

	bool Prompts::Available() { return clientID != 0; }

	SkyPromptAPI::ClientID Prompts::Client() { return clientID; }

	namespace
	{
		// Every slot, so a module switched off in the control panel can be cleared from outside.
		std::vector<std::pair<const Module*, PromptSlot*>>& Slots()
		{
			static std::vector<std::pair<const Module*, PromptSlot*>> slots;
			return slots;
		}
	}

	void Prompts::WithdrawAll(const Module* a_owner)
	{
		for (auto [owner, slot] : Slots()) {
			if (owner == a_owner) {
				slot->Reset();
				slot->Withdraw();
			}
		}
	}

	PromptSlot::PromptSlot(Module* a_owner, SkyPromptAPI::EventID a_id) :
		owner(a_owner),
		id(a_id)
	{
		Slots().emplace_back(a_owner, this);
	}

	void PromptSlot::Update(bool a_can, const std::function<std::string()>& a_text)
	{
		if (a_can && !offered) {
			offered = true;
			Offer(a_text());
		} else if (!a_can && offered) {
			offered = false;
			Withdraw();
		}
	}

	void PromptSlot::Offer(std::string a_text)
	{
		if (!Prompts::Available()) {
			return;
		}
		// SkyPrompt reads the prompt later through GetPrompts(), so the text must outlive this call.
		text = std::move(a_text);
		// No button list: SkyPrompt assigns the user's default keys for keyboard and gamepad.
		prompts[0] = SkyPromptAPI::Prompt(text, id, 0, SkyPromptAPI::kSinglePress, kPlayerRef);
		const bool sent = SkyPromptAPI::SendPrompt(this, clientID);
		owner->Log("offer event={} '{}' sent={}", id, text, sent);
	}

	void PromptSlot::Withdraw()
	{
		if (Prompts::Available()) {
			SkyPromptAPI::RemovePrompt(this, clientID);
		}
	}

	std::span<const SkyPromptAPI::Prompt> PromptSlot::GetPrompts() const
	{
		return prompts;
	}

	void PromptSlot::ProcessEvent(SkyPromptAPI::PromptEvent a_event) const
	{
		// Called from SkyPrompt's thread: log, then hand the action to the game thread.
		const auto type = a_event.type;
		const auto eventID = a_event.prompt.eventID;
		const auto module = owner;
		logs::info("[{}] prompt event {} ({}) event={}", module->Name(), EventName(type), static_cast<int>(type), eventID);
		if (type != SkyPromptAPI::kAccepted) {
			return;
		}
		auto* self = const_cast<PromptSlot*>(this);
		SKSE::GetTaskInterface()->AddTask([self, module, eventID]() {
			self->Withdraw();
			module->OnAccepted(eventID);
		});
	}
}
