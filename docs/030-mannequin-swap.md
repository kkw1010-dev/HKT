# 030 · MannequinSwap (의상 교환 with a mannequin), plan

Status (2026-09-25): **planned, not started; work is on branch `v3`.** The user files it under
**CIGAR 3.0**: CIGAR 2.0.0 is released and was tested by the user with no problems, and MannequinSwap is the first 3.0 feature. The author build is back in the load order.

Sources: a plan GPT wrote (`claude.md` in the repo root, removed on the user's permission once this
file replaced it) and Claude's review of it against this load order on 2026-09-25. Everything under
"Facts from the load order" was read with houseCARL, not guessed.

## What it does

Looking at a mannequin, one hold prompt swaps what the player wears with what the mannequin wears,
moving the actual item instances both ways. Not an outfit manager: no preset list, no slot UI, no
co-save outfit store. The mannequin already is the preset, the storage and the display; CIGAR only
removes the inventory shuffling a player does by hand today.

| Player | Mannequin | Prompt |
|---|---|---|
| dressed | dressed | 의상 교환 (길게) |
| dressed | bare | 의상 보관 (길게) |
| bare | dressed | 의상 착용 (길게) |
| bare | bare | none |

All four run the same `Swap()`. Hold, as the project's prompt policy asks for non-combat actions.

## Facts from the load order

- **Which actors are mannequins.** 18 NPC records use `ManikinRace` (`10760A:Skyrim.esm`); 14 of them
  carry `MannequinActivatorSCRIPT` (vanilla, Hearthfire, Dragonborn, JK's Palace, ULP, the Almsivi CC
  pack). The swap relies on that script's contract, so **the test is "the actor has
  `MannequinActivatorSCRIPT` bound"**, with the race as a cheap pre-filter. No name or EditorID
  heuristics.
- **What the crosshair hits.** A mannequin ACHR has `ActivateParents` with `ParentActivateOnly`,
  pointing at a `MannequinActivateTrig` activator (`0D750D:Skyrim.esm`; 250 placed across 32 plugins:
  HearthFires 24, SkyrimHouseRemodel 23, CC homes...). The trigger's only script is
  `defaultBlockFollowerActivation`. So the crosshair most likely lands on the trigger, and the resolver
  must handle both cases: the actor itself, or a ref whose activate-ref children include such an actor.
  Which one happens is logged by the first build; no separate survey build is needed.
- **Which script version runs.** `Another Mannequin Script Fix (AE.SE)` overrides
  `MannequinActivatorSCRIPT.pex` (loose). Its behaviour, decompiled:
  - **20** `ArmorSlotNN` properties (GPT's plan assumed 10). They hold **base forms**.
  - `OnItemAdded`: an armor goes into the first empty slot and `EquipItem(base)`; anything else is
    bounced back to the player with a message.
  - `OnObjectUnequipped`: the matching base form's slot is emptied.
  - `OnLoad` / `OnCellLoad` / `OnInit`: `EnableAI(false)`, move to the linked XMarkerHeading,
    `EquipCurrentArmor()` from the slots.
- **Consequences.**
  - The mannequin wears by base form. The instance (enchantment, tempering, name) survives the move,
    but if the mannequin holds two items of one base, the engine picks which one it wears. Instance
    precision is guaranteed on the player's side only.
  - **Take the mannequin's items out before putting the player's in.** The other order lets an incoming
    cuirass unequip the outgoing one, whose slot is then emptied while it is still in the inventory.
  - A slot left behind by an item that is gone reduces capacity. Count the free slots by reading the
    `ArmorSlotNN` properties (read only); never write them.
- **Almsivi CC mannequins** (`ccASVSSE001_TempleMannequin01-04`) also carry `ccASVSSE001_EquipScript`,
  read 2026-09-25 (decompiled from `ccasvsse001-almsivi.bsa`): on the first `OnCellAttach` it
  `AddItem`s its four `ArmorNN` properties and goes to state `done`, which is empty. It only seeds the
  starting outfit once (through `MannequinActivatorSCRIPT.OnItemAdded`) and never reacts again, so
  **these mannequins are included**. The same archive's `ccASVSSE001_MannequinRefAliasClear` is a
  ReferenceAlias script that clears three quest aliases when the player *activates* the mannequin;
  CIGAR does not activate it, so a swap leaves those aliases (quest markers) as they were.

## Keep from GPT's plan

- Instance transfer: `RemoveItem(armor, 1, kStoreInContainer, extraList, destination)`, never a
  remove-and-add by FormID.
- Only what the mannequin actually wears moves; leftovers in its inventory are never pulled in.
- The player side reuses `Util::IsStrippable` (no-strip, locked, non-playable and SMP carriers stay on);
  shields, weapons, ammo and spells are out.
- Never drive the mannequin script (no slot writes, no forced `EquipCurrentArmor`, no script
  replacement); CIGAR only causes normal inventory events and the script reacts.
- Hard failure (an item did not move) stops the transaction and moves back what already moved. Soft
  failure (moved but not worn) keeps the items where they are, logs, and notifies once. Items before
  looks.
- No co-save of its own. A load mid-swap resets the transaction.
- Its own module (`src/MannequinSwap.*`), not part of `Dress`.

## Changed from GPT's plan

- **PromptID 40** (18 is `kTrackQuest`; 39 went to `BookRead`). **This doc is 030** (016 is the gamepad doc).
- **Capacity** is the free-slot count read from the script (20 on this modlist), not a constant 10.
- **No fixed waits.** Papyrus latency on a 4000-plugin order is not predictable. After each phase,
  poll the engine state every 100 ms (up to about 3 s): "the mannequin no longer wears B", then "the
  mannequin wears all of A". Then read the `ArmorSlotNN` properties and log whether A's forms are in
  them. **If they are, the mannequin will re-dress A after a cell reload** (that is exactly what
  `EquipCurrentArmor` reads), so the cell-reload test can be read from the log.
- **Preflight the player's side too.** If a kept item (a device, the SMP carrier) occupies a slot that
  a piece of B needs, refuse before moving anything.
- **More gates:** not in a SexLab / OStim scene, `PartyOutfit` not active (its stored gear would be
  split), no `Helmet` clip running.
- **One build, one test session.** Phase A (survey) is done above. The read-only prototype, one-way
  transfer and full swap are one build with self-verifying logs: per-item before/after lines with the
  enchantment, charge and tempering, an item-count conservation check for player + mannequin, and the
  slot readback.

## The user's decisions (2026-09-25)

- **Helmet:** the helmet `Helmet` stowed counts as part of the player's outfit and goes to the
  mannequin (its `HELM` list is cleared). The mannequin's helmet arrives **stowed**, not on the head:
  it is unequipped on arrival and added to `Helmet`'s list, so 투구 쓰기 offers it in combat and the
  face stays visible, as `Helmet` already does everywhere else.
- **Dress:** after a swap, `Dress` remembers what the player now wears (the same `Remember()` path
  it uses after 착용하기). After 의상 보관 the player wears nothing, so the memory ends up empty.
- **Almsivi CC mannequins:** included (see "Facts from the load order").

## Couplings with existing modules (decided above)

- **`Dress` (co-save `DRES`)** remembers the outfit by FormID. After a swap those items are on the
  mannequin, and 착용하기 would dress the player half-way. Options: refresh `Dress`'s memory from what
  is worn after the swap, or clear it.
- **`Helmet` (co-save `HELM`)** keeps the helmet off by default; the stowed helmet sits unworn in the
  inventory (the hip display was dropped). A plain swap leaves it there while the mannequin's helmet comes onto the
  head. Options: count the stowed helmet as part of the player's outfit, or leave helmets out of the
  swap and to `Helmet`.
- **`PartyOutfit` (co-save `QOUT`)**: blocked while active (see gates).

## Tests (one session, on a new game)

1. Dressed player, bare Breezehome mannequin → 의상 보관 → mannequin wears it, player bare.
2. Same mannequin → 의상 착용 → back on the player.
3. Two different outfits → 의상 교환 twice → back where it started.
4. Same base armor on both sides, one enchanted or tempered → the right instance ends on each side
   (the log lines show enchantment and tempering before and after).
5. A no-strip or locked item and a shield worn → they stay on the player.
6. Leave the house and come back → the mannequin still wears its outfit (the slot readback in the log
   predicts this; the walk confirms it once).

The first build also logs what the crosshair hit (trigger or actor) and which resolver path found the
mannequin.

## Second review (2026-09-26, before implementation)

The plan above was checked against one script only: `Another Mannequin Script Fix`, which overrides
`MannequinActivatorSCRIPT.pex` on this load order. CIGAR is now published on Nexus, where most
players run **USSEP's** version instead (`scripts\mannequinactivatorscript.pex` in
`unofficial skyrim special edition patch.bsa`, decompiled 2026-09-26; copy in the session
scratchpad, not in the repo). It differs where the plan depends on it:

| | Another Mannequin Script Fix | USSEP |
|---|---|---|
| Slot storage | 20 properties `ArmorSlot01`-`20` | a script **variable** `Form[] ArmorSlot` (10); `ConvertArmorSlots()` copies the 10 old properties into it on the first load and sets every property to `EmptySlot` |
| Capacity | 20 | 10 |
| Same base form twice | accepted | `IsDuplicated` refuses the second one and bounces it to the player |
| Full | bounced | bounced (`RemoveItem(base, n, true, player)`, by base form) |
| Cell load | re-equips from the properties | `UnequipAll()`, then re-equips from the array |

What this changes in the plan:

1. **Slot readback and capacity** read the `ArmorSlot` array variable when the script has it (USSEP),
   else the `ArmorSlotNN` properties that exist (vanilla 10, the fix 20). Reading only the properties
   on USSEP would see ten empty slots on a full mannequin.
2. **Phase 1 must wait for the script, not only the engine.** "The mannequin no longer wears B" is
   true before the script's `OnObjectUnequipped` has cleared B's slot. On USSEP a piece of A with the
   same base form as a piece of B would then be refused as a duplicate. Poll until B's forms are gone
   from the slots too, then start phase 2.
3. **The vanilla script** (no USSEP, no fix) is the third variant and has not been read yet; read it
   from `Skyrim - Misc.bsa` before writing the resolver.
4. **The OStim gate** has nothing behind it: CIGAR detects SexLab scenes only (`Util::InScene`);
   OStim appears only as a no-strip keyword. Either add OStim scene detection to `Util` (it would
   serve every module, and a Nexus comment asked for it) or drop OStim from this gate.

Still sound: instance transfer with the extra list, never driving the script, the mannequin's items
out before the player's go in, polling instead of fixed waits, hard/soft failure, its own module,
PromptID 40 (still free; 39 is the last used), and the user's Helmet/Dress/Almsivi decisions.
