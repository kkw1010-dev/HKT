# 010 · WeaponSwap: ranged weapon when the enemy is far, melee when it is near

Status (2026-09-18): built and deployed, not yet tested in game.

The user asked for a prompt that equips a bow or other ranged weapon when the
enemy moves away or flees, and the reverse when it comes back. No mod is
needed; TDM is used when present.

## Decisions the user made

- A prompt, not an automatic swap.
- The switch distance defaults to **800** units and is adjustable in
  CIGAR / 3. 세부 설정 / 무기 전환 (300–3000). 800 is the user's choice.
- Staves do not count as ranged weapons. Only bows and crossbows do.
- The reverse case (enemy near, bow in hand → melee weapon) is required.
- **Going back returns to the loadout held before** (the user's correction,
  2026-09-19). Axe + shield → bow → axe + shield, not the strongest melee
  weapon. The fallback order below applies only when that loadout is gone.
- Weapon order when there is nothing to go back to: favourites first, then the strongest
  weapon, then anything else of that kind. With staves excluded, "anything
  else" is already covered by "strongest", because every bow or crossbow has a
  damage figure.

## Gate

- **Enemy.** TDM's locked target while TDM is locked on
  (`IVTDM1::GetCurrentTarget`). Otherwise the nearest hostile within 4096 units
  that is in combat (`Util::NearbyHostiles`, filtered by `IsInCombat`).
- **Zone.**
  - `flee`: the enemy's `CombatController::state->isFleeing`.
  - `far`: distance at or beyond the switch distance.
  - `near`: distance more than 100 units inside it.
  - Inside that 100-unit band the previous zone holds, so an enemy standing
    at the line does not flip the prompts.
- **Hands** (right hand): `ranged` (bow/crossbow), `melee`, `empty`
  (nothing or fists), `other` (a spell or a staff). A player holding a spell
  or a staff is never offered anything.
- 원거리 무기: <이름> shows in combat, zone `far` or `flee`, hands `melee` or
  `empty`, movement controls on, not in a SexLab scene.
- 근접 무기: <이름> shows in combat, zone `near`, hands `ranged`, same
  conditions.
- For 1.5 s after an accept, nothing is offered, and then the result is
  checked.

## Picking

- **Previous loadout first.** Taking the bow remembers the melee weapon in
  the right hand and the left-hand item. Taking the melee weapon remembers
  the bow and its ammo. Each side's prompt names the remembered weapon while it
  is still carried (a bow also needs ammo), and the gate marks it `(prev)`.
  Only when it is gone does the fallback order apply.

- The inventory is scanned only while a prompt could show, at most once a
  second, and again on accept.
- Bound, non-playable and fist weapons are skipped. A bow with no arrows or a
  crossbow with no bolts is skipped.
- `favorite` = `InventoryEntryData::IsFavorited`. Damage =
  `PlayerCharacter::GetDamage(entry)`, the figure the inventory menu shows
  (tempering and perks included). Ties go to the lower FormID.
- The equip takes the favourited instance (`ExtraHotkey`), otherwise the
  best-tempered one (`ExtraHealth`), otherwise the game's choice.
- **Ammo.** The equipped ammo is kept if it fits the weapon (arrow vs bolt);
  otherwise the strongest playable ammo that fits is equipped as a whole stack.

## Equipping

- Ranged: `ActorEquipManager::EquipObject(bow)` with the weapon's own slot,
  then ammo. The left hand (shield, one-handed weapon or spell) is remembered
  first.
- Melee: a one-handed weapon goes to the right-hand slot, and the remembered
  left-hand item comes back if it is still carried (or the spell still known).
  The same one-handed weapon in both hands needs two of it. A two-handed
  weapon uses its own slot and drops the remembered left hand.
- The remembered left hand lives only for the session; it is not co-saved.

## Self-reporting (`CIGAR.log`, `[WeaponSwap]`)

- At load: `tdm= sexlab= rightSlot= leftSlot= range=`.
- `gate combat= target= locked= zone= hands= movable= sexlab= quiet= ranged= melee=`;
  the picks read `-` while not wanted, `none` when nothing qualifies, and
  carry `(fav)` for a favourite.
- `zone <near|far|flee> at distance N (switch at R, target T, locked=)` on
  every zone change.
- `equip ranged|melee ...` with FormID, damage, favourite, ammo and the saved
  left hand; `left hand back: ...` or `left hand not restored: ...`.
- `after equip: right ... left ... ammo ...` 1.5 s later, or
  `WARN after equip: right hand is X, expected Y` with a one-time HUD notice
  `CIGAR: 무기 전환 실패. 로그 확인`.
