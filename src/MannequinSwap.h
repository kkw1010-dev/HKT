#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// Looking at a mannequin, one hold prompt swaps what the player wears with what the mannequin
	// wears, moving the actual item instances both ways (docs/030-mannequin-swap.md). CIGAR only
	// moves items; the mannequin's own MannequinActivatorSCRIPT (vanilla, USSEP or Another Mannequin
	// Script Fix) records and equips them through its normal inventory events.
	class MannequinSwap final : public Module
	{
	public:
		static MannequinSwap* GetSingleton();

		const char* Name() const override { return "MannequinSwap"; }
		void OnGameLoaded() override;
		void Tick() override;
		void FastTick() override;
		void OnAccepted(std::uint16_t a_eventID) override;
		void OnDeclined(std::uint16_t a_eventID) override;
		void OnDisabled() override;

	private:
		MannequinSwap();

		enum : std::uint16_t
		{
			kSwap = PromptID::kMannequinSwap
		};

		using Clock = std::chrono::steady_clock;

		// One armor instance on one side of the swap.
		struct Piece
		{
			RE::TESObjectARMO* armor{ nullptr };
			RE::ExtraDataList* extra{ nullptr };
			bool head{ false };
			std::string text;  // name, FormID, enchantment, charge and tempering, for the log
		};

		// What the mannequin's script records, read without writing anything.
		struct Slots
		{
			const char* variant{ "none" };  // "array" (USSEP), "properties" (vanilla 10, the fix 20)
			std::vector<RE::TESForm*> forms;  // nullptr or EmptySlot = free
			RE::TESForm* empty{ nullptr };
			bool readable{ false };

			bool Free(const RE::TESForm* a_form) const { return !a_form || a_form == empty; }
			std::size_t Occupied() const;
			std::size_t CountOf(const RE::TESForm* a_form) const;
		};

		enum class Phase
		{
			kIdle,
			kTakeWait,  // the mannequin's pieces were moved to the player; wait for its script to let go
			kGiveWait,  // the player's pieces were moved to the mannequin; wait for it to wear them
			kSettle     // the mannequin's pieces were equipped on the player; check and finish
		};

		bool IsMannequin(RE::Actor* a_actor) const;
		RE::Actor* FindMannequin(RE::PlayerCharacter* a_player, std::string& a_how) const;
		Slots ReadSlots(RE::Actor* a_mannequin) const;
		bool IsHead(const RE::TESObjectARMO* a_armor) const;
		Piece MakePiece(RE::TESObjectARMO* a_armor, RE::ExtraDataList* a_extra) const;
		// The worn instance's extra list, or null.
		static RE::ExtraDataList* WornExtra(RE::Actor* a_actor, RE::TESBoundObject* a_item);
		static RE::ExtraDataList* AnyExtra(RE::Actor* a_actor, RE::TESBoundObject* a_item);
		static bool HasExtra(RE::Actor* a_actor, RE::TESBoundObject* a_item, const RE::ExtraDataList* a_extra);
		static bool IsWearing(RE::Actor* a_actor, RE::TESObjectARMO* a_armor);
		// Worn armor that is clothing for the swap, each piece once.
		static std::vector<RE::TESObjectARMO*> WornOutfit(RE::Actor* a_actor);
		// The player's side: worn strippable armor (no shields) plus the helmet Helmet keeps stowed.
		std::vector<Piece> PlayerPieces(RE::PlayerCharacter* a_player) const;
		// The mannequin's side: armor it wears (no shields). Leftovers in its inventory stay.
		std::vector<Piece> MannequinPieces(RE::Actor* a_mannequin) const;
		// Why a swap cannot run, or empty.
		std::string Preflight(RE::PlayerCharacter* a_player, const std::vector<Piece>& a_give, const std::vector<Piece>& a_take,
			const Slots& a_slots) const;

		void Begin(RE::PlayerCharacter* a_player, RE::Actor* a_mannequin);
		void Give(RE::PlayerCharacter* a_player, RE::Actor* a_mannequin);
		void EquipTaken(RE::PlayerCharacter* a_player);
		void Finish(RE::PlayerCharacter* a_player, RE::Actor* a_mannequin);
		void Abort(RE::PlayerCharacter* a_player, RE::Actor* a_mannequin, std::string_view a_why);
		void Reset();

		PromptSlot swap{ this, kSwap };

		RE::TESRace* manikinRace{ nullptr };
		RE::BGSKeyword* armorHelmet{ nullptr };
		RE::BGSKeyword* clothingHead{ nullptr };
		RE::BGSKeyword* clothingCirclet{ nullptr };

		// The transaction in progress.
		Phase phase{ Phase::kIdle };
		Clock::time_point phaseStart{};
		RE::ObjectRefHandle target;
		std::map<RE::FormID, std::size_t> slotCountsBefore;  // per base form, in the mannequin's slots
		std::vector<Piece> give;    // player -> mannequin
		std::vector<Piece> take;    // mannequin -> player
		std::vector<Piece> given;   // moved so far, for a rollback
		std::vector<Piece> taken;
		std::vector<RE::FormID> stowedHelmets;
		std::map<RE::FormID, std::int32_t> countsBefore;  // player + mannequin, per base form

		// A declined prompt stays away until the player is this far from where it was declined, or
		// looks at another mannequin.
		RE::ObjectRefHandle dismissedFor;
		// The mannequin the last tick found, for a decline.
		RE::ObjectRefHandle current;
		RE::NiPoint3 dismissedAt{};
		// A blocker already notified, so it is said once per mannequin and reason.
		std::string notifiedBlock;
	};
}
