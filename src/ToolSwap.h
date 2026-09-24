#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// SI's WeaponSwap tool scenes: facing an ore vein, 곡괭이 들기 puts a pickaxe from the vein's own
	// tool list (MineOreScript.mineOreToolsList) in the right hand, so the vein can be mined by
	// striking it (vanilla MineOreScript.OnHit); facing a tree, 도끼 들기 does the same with a
	// woodcutter's axe (vanilla woodChoppingAxes), for roleplay. Walking away offers 무기 되돌리기,
	// which puts back what the right hand held before.
	//
	// SI's third scene, a fishing rod near fishing supplies, is not rebuilt: Streamlined Fishing
	// already equips the rod when the supplies are activated (docs/024-tool-swap.md).
	class ToolSwap final : public Module
	{
	public:
		static ToolSwap* GetSingleton();

		const char* Name() const override { return "ToolSwap"; }
		void OnGameLoaded() override;
		void Tick() override;
		void OnAccepted(std::uint16_t a_eventID) override;
		void OnDisabled() override;

	private:
		ToolSwap();

		enum : std::uint16_t
		{
			kTake = PromptID::kTakeTool,
			kReturn = PromptID::kReturnWeapon
		};

		using Clock = std::chrono::steady_clock;

		enum class Kind
		{
			kNone,
			kVein,
			kTree
		};

		struct Target
		{
			Kind kind{ Kind::kNone };
			RE::TESObjectREFR* ref{ nullptr };
			RE::BGSListForm* tools{ nullptr };
			float distance{ 0.0f };
			float angle{ 0.0f };
		};

		struct Scan
		{
			Target target;
			// Any vein or tree within the leave radius, faced or not: the return prompt waits for none.
			bool anyNear{ false };
			std::size_t veins{ 0 };
			std::size_t depleted{ 0 };
			std::size_t trees{ 0 };
		};

		Scan Look(RE::PlayerCharacter* a_player) const;
		// The best tool from a_tools the player carries: favourites first, then the highest damage.
		static RE::TESObjectWEAP* PickTool(RE::PlayerCharacter* a_player, RE::BGSListForm* a_tools);
		static const char* KindTag(Kind a_kind);
		void Restore(RE::PlayerCharacter* a_player);

		PromptSlot take{ this, kTake };
		PromptSlot giveBack{ this, kReturn };

		RE::BGSListForm* woodAxes{ nullptr };

		Kind offeredKind{ Kind::kNone };
		RE::TESObjectWEAP* offeredTool{ nullptr };

		// What the right hand held before CIGAR put a tool in it: a weapon, a spell, or nothing.
		bool swapped{ false };
		RE::TESForm* savedRight{ nullptr };
		RE::TESObjectWEAP* heldTool{ nullptr };
		Clock::time_point awaySince{};
		bool away{ false };
		Clock::time_point checkAt{};
		bool checkPending{ false };
		RE::TESForm* expectedRight{ nullptr };
	};
}
