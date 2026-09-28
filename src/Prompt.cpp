#include "Prompt.h"

#include <map>

#include "Module.h"
#include "PromptAnchor.h"
#include "Settings.h"

namespace CIGAR
{
	namespace
	{
		SkyPromptAPI::ClientID clientID = 0;

		// SkyPrompt fades a prompt out after its lifetime setting; re-sending well within it keeps
		// the prompt up.
		constexpr auto kKeepAliveInterval = 2s;

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

	namespace
	{
		// Every slot, so a module switched off in the control panel can be cleared from outside.
		std::vector<std::pair<const Module*, PromptSlot*>>& Slots()
		{
			static std::vector<std::pair<const Module*, PromptSlot*>> slots;
			return slots;
		}

		bool duplicateIDs = false;

		// Which event ID holds each key slot (0 = free; event IDs start at 1). Game thread only.
		std::array<SkyPromptAPI::EventID, Settings::kPromptKeyCount> keySlots{};

		// The slot this event already holds, or the lowest free one; -1 when all are taken.
		int AcquireKeySlot(SkyPromptAPI::EventID a_id)
		{
			for (std::size_t i = 0; i < keySlots.size(); ++i) {
				if (keySlots[i] == a_id) {
					return static_cast<int>(i);
				}
			}
			for (std::size_t i = 0; i < keySlots.size(); ++i) {
				if (keySlots[i] == 0) {
					keySlots[i] = a_id;
					return static_cast<int>(i);
				}
			}
			return -1;
		}

		void ReleaseKeySlot(SkyPromptAPI::EventID a_id)
		{
			for (auto& holder : keySlots) {
				if (holder == a_id) {
					holder = 0;
				}
			}
		}
	}

	namespace
	{
		using Clock = std::chrono::steady_clock;

		struct Command
		{
			PromptSlot* slot;
			bool send;
			PromptData data;
			std::string note;
		};

		std::mutex queueLock;
		std::vector<Command> queue;
		// Held while a batch is delivered, so the fallback below never delivers alongside the hook.
		std::mutex deliverLock;

		// The hook's last call, and the render thread's ID (0 until the first call).
		std::atomic<Clock::rep> lastPresent{ 0 };
		std::atomic<std::uint32_t> renderThread{ 0 };
		std::atomic_bool inPresent{ false };
		bool hookInstalled = false;
		// Without a Present call for this long, the queue is delivered from the game thread instead,
		// as before the hook (logged): prompts keep working if another mod cuts the call chain.
		constexpr auto kPresentSilence = 5s;
		std::atomic_bool fallbackLogged{ false };

		void Deliver()
		{
			std::scoped_lock deliver(deliverLock);
			std::vector<Command> batch;
			{
				std::scoped_lock guard(queueLock);
				batch.swap(queue);
			}
			for (auto& command : batch) {
				command.slot->Deliver(command.send, std::move(command.data), command.note);
			}
		}

		bool PresentAlive()
		{
			const auto last = lastPresent.load();
			return hookInstalled && last != 0 && Clock::now() - Clock::time_point(Clock::duration(last)) < kPresentSilence;
		}

		void Enqueue(Command&& a_command)
		{
			{
				std::scoped_lock guard(queueLock);
				queue.push_back(std::move(a_command));
			}
			if (PresentAlive()) {
				fallbackLogged = false;
				return;
			}
			if (!fallbackLogged.exchange(true)) {
				logs::warn("prompt queue: no Present call for {} s (hook {}); SkyPrompt calls go out from the game thread until it returns",
					std::chrono::duration_cast<std::chrono::seconds>(kPresentSilence).count(), hookInstalled ? "installed" : "not installed");
			}
			Deliver();
		}

		// BSGraphics::Renderer::End's call to Present, the call SkyPrompt draws in (its DrawHook, same site).
		struct PresentHook
		{
			static void thunk(std::uint32_t a_timer)
			{
				lastPresent = Clock::now().time_since_epoch().count();
				if (renderThread.load() == 0) {
					renderThread = ::GetCurrentThreadId();
					logs::info("prompt queue: first Present call on thread {} (SkyPrompt draws on this thread)", renderThread.load());
				}
				Deliver();
				inPresent = true;
				func(a_timer);
				inPresent = false;
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};
	}

	void Prompts::InstallRenderHook()
	{
		const REL::Relocation<std::uintptr_t> target{ REL::RelocationID(75461, 77246) };
		const auto site = target.address() + REL::Relocate(0x9, 0x9, 0x15);
		// A 5-byte relative call (E8) is what SkyPrompt and the other overlays hook here; anything else
		// means a different runtime layout, and the queue then delivers from the game thread.
		if (*reinterpret_cast<const std::uint8_t*>(site) != 0xE8) {
			logs::error("prompt queue: no call at the Present site ({:02X}); SkyPrompt calls go out from the game thread",
				*reinterpret_cast<const std::uint8_t*>(site));
			return;
		}
		SKSE::AllocTrampoline(14);
		PresentHook::func = SKSE::GetTrampoline().write_call<5>(site, PresentHook::thunk);
		hookInstalled = true;
		logs::info("prompt queue: Present hooked (chained: {})", PresentHook::func.address() != 0);
	}

	void Prompts::NoteTick()
	{
		static bool noted = false;
		static bool threadLogged = false;
		if (!threadLogged && renderThread.load() != 0) {
			threadLogged = true;
			logs::info("prompt queue: ticks run on thread {}, Present on thread {}", ::GetCurrentThreadId(), renderThread.load());
		}
		if (!noted && inPresent.load()) {
			noted = true;
			logs::info("prompt queue: a tick ran while the render thread was inside Present (threads {} and {}): "
					   "SkyPrompt calls from ticks would race its drawing",
				::GetCurrentThreadId(), renderThread.load());
		}
	}

	bool Prompts::Init()
	{
		if (clientID == 0) {
			clientID = SkyPromptAPI::RequestClientID();
		}
		logs::info("SkyPrompt client id {} (API {}.{})", clientID, SkyPromptAPI::MAJOR, SkyPromptAPI::MINOR);
		// A shared event ID makes one key press fire every prompt that uses it.
		std::map<SkyPromptAPI::EventID, std::string> owners;
		for (const auto& [owner, slot] : Slots()) {
			const auto [it, fresh] = owners.emplace(slot->ID(), owner->Name());
			if (!fresh) {
				logs::error("prompt event id {} is used by both {} and {}", slot->ID(), it->second, owner->Name());
				duplicateIDs = true;
			}
		}
		return clientID != 0;
	}

	bool Prompts::Available() { return clientID != 0; }

	SkyPromptAPI::ClientID Prompts::Client() { return clientID; }

	bool Prompts::HasDuplicateIDs() { return duplicateIDs; }

	void Prompts::WithdrawAll(const Module* a_owner)
	{
		for (auto [owner, slot] : Slots()) {
			if (owner == a_owner) {
				slot->Reset();
				slot->Withdraw();
			}
		}
	}

	void Prompts::WithdrawEverything()
	{
		for (auto [owner, slot] : Slots()) {
			slot->Reset();
			slot->Withdraw();
		}
		keySlots.fill(0);
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
		} else if (a_can) {
			KeepAlive();
		} else if (offered) {
			offered = false;
			Withdraw();
		}
	}

	void PromptSlot::Offer(std::string a_text)
	{
		if (!Prompts::Available()) {
			return;
		}
		desired.text = std::move(a_text);
		// The keyboard key comes from CIGAR's settings; a device without a listed key (the gamepad)
		// gets SkyPrompt's default for the slot SkyPrompt picks. SkyPrompt keeps a queued prompt's key,
		// so the slot is held until the prompt is withdrawn.
		const int slot = AcquireKeySlot(id);
		key = 0;
		desired.progress = 0.0f;
		desired.buttonCount = 0;
		SkyPromptAPI::ButtonID pad = 0;
		if (slot >= 0) {
			key = Settings::PromptKeys()[slot];
			// SkyPrompt's mouse codes start at 256; a device with no listed button uses SkyPrompt's own.
			desired.buttons[0] = { key >= 256 ? RE::INPUT_DEVICE::kMouse : RE::INPUT_DEVICE::kKeyboard, key };
			desired.buttonCount = 1;
			if (Settings::PadButtons() == Settings::PadPreset::kDpad) {
				pad = Settings::kDpadButtons[slot];
				desired.buttons[1] = { RE::INPUT_DEVICE::kGamepad, pad };
				desired.buttonCount = 2;
			}
		}
		desired.type = promptType;
		// The player, or the marker PromptAnchor moves ahead of the head in third person.
		desired.refID = PromptAnchor::RefID();
		Send(std::format("offer event={} '{}' slot={} key={}{}", id, desired.text, slot + 1, key, pad ? std::format(" pad={}", pad) : ""));
	}

	void PromptSlot::Send(std::string a_note)
	{
		lastSent = std::chrono::steady_clock::now();
		Enqueue({ this, true, desired, std::move(a_note) });
	}

	void PromptSlot::KeepAlive()
	{
		// Re-sending a queued prompt makes SkyPrompt reset its lifetime (IsInQueue -> WakeUpQueue).
		const auto now = std::chrono::steady_clock::now();
		if (!Prompts::Available() || now - lastSent < kKeepAliveInterval) {
			return;
		}
		Send();
	}

	void PromptSlot::SetColor(std::uint32_t a_color)
	{
		if (desired.color == a_color) {
			return;
		}
		desired.color = a_color;
		if (offered && Prompts::Available()) {
			// SkyPrompt refreshes text, colour and progress of a prompt that is already queued.
			Send();
		}
	}

	void PromptSlot::SetLive(std::string a_text, float a_progress)
	{
		if (!offered || !Prompts::Available() || (a_text == desired.text && a_progress == desired.progress)) {
			return;
		}
		desired.text = std::move(a_text);
		desired.progress = a_progress;
		Send();
	}

	void PromptSlot::Withdraw()
	{
		if (Prompts::Available()) {
			Enqueue({ this, false, {}, {} });
		}
		ReleaseKeySlot(id);
	}

	void PromptSlot::Deliver(bool a_send, PromptData&& a_data, const std::string& a_note)
	{
		if (!a_send) {
			SkyPromptAPI::RemovePrompt(this, clientID);
			return;
		}
		// SkyPrompt keeps string_views into the text: the old text is kept one more round.
		previousText = std::move(published.text);
		published = std::move(a_data);
		const std::span<const std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>> keys(published.buttons.data(), published.buttonCount);
		prompts[0] = SkyPromptAPI::Prompt(published.text, id, 0, published.type, published.refID, keys, published.color, published.progress);
		const bool sent = SkyPromptAPI::SendPrompt(this, clientID);
		if (!a_note.empty()) {
			owner->Log("{} sent={}", a_note, sent);
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
		// Timing-out and move arrive once per frame; logging them buried everything else.
		if (type != SkyPromptAPI::kTimingOut && type != SkyPromptAPI::kMove) {
			logs::info("[{}] prompt event {} ({}) event={}", module->Name(), EventName(type), static_cast<int>(type), eventID);
		}
		auto* self = const_cast<PromptSlot*>(this);
		if (type == SkyPromptAPI::kTimeout) {
			// Faded out despite the keep-alive (e.g. while paused): offer again on the next tick.
			SKSE::GetTaskInterface()->AddTask([self]() {
				self->Reset();
				ReleaseKeySlot(self->ID());
			});
		}
		if (type == SkyPromptAPI::kDeclined) {
			SKSE::GetTaskInterface()->AddTask([module, eventID]() { module->OnDeclined(eventID); });
		}
		if (hold) {
			const bool down = type == SkyPromptAPI::kDown;
			const bool ends = type == SkyPromptAPI::kUp || type == SkyPromptAPI::kRemovedByMod ||
			                  type == SkyPromptAPI::kTimeout || type == SkyPromptAPI::kDeclined;
			if (down || ends) {
				SKSE::GetTaskInterface()->AddTask([module, eventID, down]() { module->OnHold(eventID, down); });
			}
			return;
		}
		if (type != SkyPromptAPI::kAccepted) {
			return;
		}
		SKSE::GetTaskInterface()->AddTask([self, module, eventID]() {
			self->Withdraw();
			if (self->repeat) {
				self->Reset();
			}
			module->OnAccepted(eventID);
		});
	}
}
