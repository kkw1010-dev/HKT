# 044 · MannequinSwap for Mannequins Improved (design, not built)

Status (2026-09-30): design only. The user adopted Mannequins Improved (Nexus 55474, female, white
marble) for the home mannequins; it is installed after r11. Codex CX-15
(`C:\TAKEALOOK\_codex\results\CX-15.md`) surveyed the mod; the three `.psc` sources in
`C:\TAKEALOOK\_codex\work\cx15\extracted\` were read for this design. The vanilla branch of
`docs/030` stays as it is: homes the patches do not cover keep vanilla mannequins.

## How the mod keeps a mannequin (from the source, 1.22)

- One mannequin is four references, linked from an **activator** (`ftaACTI_ManikinActivator`, script
  `ftaSCR_Manikin_Activate`): `pLinkCustom01` the female display actor, `02` the male one, `03` the
  **chest** (`ftaCONT_ManikinChest`, script `ftaSCR_Manikin_Chest`), `04` the marker. The chest links
  back to the activator with its default linked ref. The display actors are race `ftaRACE_Manikin`
  (`fta_TES5Manikins.esp` 0x808), not `ManikinRace`, and carry no `MannequinActivatorSCRIPT`: today's
  `FindMannequin` finds none of them.
- **The chest is the storage.** Its `OnItemAdded` accepts Armor, Weapon and Ammo (anything else goes
  back to the player with an error message) and calls the activator's `addToCurrentItems(base form)`;
  `OnItemRemoved` calls `removeFromCurrentItems(base form)`.
- **The display is a copy.** `addToCurrentItems` puts the base form in the private `Form[20] ArmorSlot`
  (a base form already there is refused, 20 at most) and has the display actor `EquipItem` the base
  form, which creates a plain copy on the actor. The real items, with their enchantments and tempering,
  stay in the chest. `removeFromCurrentItems` clears the slot and removes the copy from both actors.
  On cell load and on menu exit `resetManikin` re-equips from `ArmorSlot`; with no armor left it puts on
  the default outfit list (`pftaFLST_Manikin_DefaultOutfit`, empty unless the rags option is installed).
- The mod's own menu ("Manage Equipment") simply opens the chest, so moving items in and out of the
  chest is exactly the path the mod expects.

## (1) Recognising one

- At load: `ftaRACE_Manikin` = `LookupForm(0x808, "fta_TES5Manikins.esp")` as `TESRace`; absent plugin
  → the branch is off (soft integration, one log line).
- At the crosshair (the existing `FindMannequin` search, same radius):
  - an actor of race `ftaRACE_Manikin`, or
  - a reference that has a linked `ftaSCR_Manikin_Activate` script (`Util::ScriptObject`), i.e. the
    activator itself.
- From an actor, find its activator: the references within `kNearCrosshair` of it whose script object
  is `ftaSCR_Manikin_Activate` and whose linked ref by `pLinkCustom01` or `pLinkCustom02` is this actor.
  The keywords are read from that script's `pLinkCustom01..03` properties (`Util::ScriptProperty`),
  so no keyword FormID is hard-coded. Then chest = activator's linked ref by `pLinkCustom03`, and it must
  itself carry `ftaSCR_Manikin_Chest`. The display actor that counts is the enabled one of the two.
- The resolved triple (activator, display actor, chest) is cached per activator handle for the gate;
  the gate line names the branch (`kind=mi`).

## (2) The swap

The existing phase machine stays; only the "mannequin's storage" changes from the vanilla actor's
inventory to the MI chest.

1. **Pieces.** The player's side is unchanged (worn, strippable armor; stowed helmets as today). The
   mannequin's side is the chest's Armor items (each instance with its own item data). Weapons and ammo
   in the chest stay where they are (the vanilla branch swaps armor only).
2. **Preflight (before the prompt shows).** The chest holds at most 20 distinct base forms after the
   swap (the mod's `ArmorSlot` limit), counting the weapons and ammo that stay; two pieces with the same
   base form are refused like the vanilla branch's duplicate rule, because the mod shows only one and
   would drop the slot when either copy leaves. A block is logged and shown only as a gate reason
   (`NotifyDiagnostic` rules of `docs/041`).
3. **Take** (phase `kTakeWait`): `chest->RemoveItem(armor, 1, kStoreInContainer, extra, player)` for
   each of the chest's pieces, instance by instance. The chest's `OnItemRemoved` clears the display.
4. **Wait** until the display actor no longer wears those base forms, or 3 s (the existing
   `kScriptWait`), so every removal event has run before anything is added: with a full 20 slots an
   early `addToCurrentItems` would otherwise be refused.
5. **Give** (phase `kGiveWait`): `player->RemoveItem(armor, 1, kStoreInContainer, extra, chest)` for each
   worn piece, with the extra list re-checked just before (the review fix of 2026-09-30).
6. **Wait** until the display actor wears the given base forms, or 3 s.
7. **Equip** what was taken on the player, with its item data (`EquipTaken`, unchanged).
8. **Self-report:** a `result` line compares the chest's contents and the display actor's worn list
   with what was intended; any piece that did not arrive, or a display that did not follow within 3 s,
   gives a WARN and one "did not fully complete" notice (the existing action-result notice).
- Abort and undo reuse the existing paths with the chest in place of the mannequin actor.

## (3) Where the design rests on unverified behaviour

| Assumption | Why it is believed | If wrong |
|---|---|---|
| A native `RemoveItem` into or out of the chest raises the chest's `OnItemAdded` / `OnItemRemoved` | CIGAR's vanilla branch already depends on the same for the vanilla script, and N3 (2026-09-27) passed | The display would not follow: the step 6 check reports it; the items are still correct in the chest |
| The chest's events for one swap run in the order they were raised | Papyrus queues events per object; step 4 waits for the removals anyway, so only the order among the adds matters, and it does not | Nothing beyond the step 8 WARN |
| `addToCurrentItems` equipping a base form on the display actor never touches the real item in the chest | The source equips `akBaseItem`, a Form, on the actor; the chest keeps the instance | An enchanted piece would show plain on the mannequin, which is what the mod does anyway |
| `ArmorSlot` is never read | It is a private script variable; the design uses the chest (the truth) and the display actor (what shows) | None |

No engine write beyond `RemoveItem` / `EquipObject`, which the vanilla branch already uses.

## (4) Work

- Recognition, the triple cache and the gate line: about 150 lines in `MannequinSwap`.
- The chest branch of pieces, preflight, the take/give targets and the display waits: about 150 lines,
  mostly switching the storage reference.
- Self-report lines, `verify_deploy.py` (the plugin and the two script names present when the mod is
  installed), `docs/030` cross-reference, README row note, CHANGELOG line.
- One build, one in-game round. It needs the mod installed (after r11) to be tested.

## (5) In-game checks (after the install; no save-then-load)

- **M1.** In a home with an MI mannequin (e.g. `coc RiftenHoneyside`; the exact homes depend on the
  patches the installer picks), look at the mannequin: the swap prompt shows (`kind=mi` in the gate).
  Swap: the player's worn armor goes on the marble mannequin, its pieces come to the player and are worn.
- **M2.** Give an enchanted or tempered piece and take it back with a second swap: it returns with its
  enchantment and tempering (the item's name and value in the inventory).
- **M3.** A mannequin wearing nothing (or the rags): the swap puts the player's gear on it and gives the
  player nothing back; no error, and no rags left on the player.
- **M4.** Open the mod's own menu → Manage Equipment after a swap: the chest holds exactly the pieces the
  mannequin shows.
- **M5.** Leave the house and come back: the mannequin still wears the swapped gear (the mod's
  `resetManikin` on cell load re-equips from `ArmorSlot`, so this proves the events updated it).
- **M6.** If any vanilla mannequin remains in the order, one swap there still works (the old branch).
- Log lines: `mannequin ... kind=mi activator=... chest=...`, the take/give lines per piece, and the
  `result` line with no WARN.
