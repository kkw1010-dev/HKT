# 042 · Ring prompts and declines that stay declined

Status (2026-09-29): built, not yet tested in game. Two design rules from the user, relayed by the
orchestrator after the r8b run.

## The rules

1. **A declined prompt stays hidden while its situation lasts.** SkyPrompt's decline is a double
   tap of the prompt's key. Sitting and lying down came back right after a decline; that is wrong
   for every prompt, not only those two.
2. **Every non-combat action fills a ring (hold).** A single-press prompt acts on the first tap, so
   the second tap of a decline never reaches it: the player cannot decline it at all. Combat
   prompts keep the single press, because a ring costs time in a fight.

## How CIGAR does it

### Declines: in `PromptSlot`, for every prompt

- `PromptSlot` catches SkyPrompt's `kDeclined` itself (`PromptSlot::Decline`, on the game thread,
  before the module's own `OnDeclined`). It withdraws the prompt and keeps it hidden.
- **When it comes back.** By default, once the prompt's condition has been false for 1 s
  (`kDeclineRelease`; the margin keeps a one-tick flicker from releasing it; my choice). With
  `SetDeclineDistance(d)`, only once the player is `d` units from where they declined, or in another
  cell. Rest's four poses use 300 units (Observe's leave distance): their condition drops with every
  step, so "false once" would bring them back at the next stop.
- A game load forgets every decline (`Prompts::ClearDeclines`).
- Modules that already had their own dismissal (pass time, helmet, chair drink, observe, mannequin,
  Wizard Warrior) keep it; the slot's latch adds nothing they would notice, because their condition
  already includes their own flag.
- This also delivers the Bathe and Shower decline planned in `docs/041` (independent per prompt,
  back once the prompt's own eligibility has ended).
- Log: `event=N declined: hidden while its condition holds` (or `... until the player is 300 units
  away or in another cell`), then `event=N no longer declined: the situation changed`.

### Rings: every non-combat prompt is `kHold` or `kHoldAndKeep`

| Module | Prompt | Was | Now |
|---|---|---|---|
| `Bathe` | 목욕하기, 샤워하기 | single press | `kHold`, "(길게)" |
| `Dress` | 탈의하기, 착용하기 | single press | `kHold`, "(길게)" |
| `BaboKey` | 행동 선택 | single press | `kHold`, "(길게)" |
| `Eat` | 먹기 | single press | `kHold`, "먹기 (길게): food" |
| `Needs` | 소변 보기, 대변 보기 | single press | `kHold`, "소변 보기 (길게): N%" |
| `Deflate` | 배출 | hold mode, single-press type | `kHoldAndKeep` ring; FHU's key-down goes out only after the key has been down 0.5 s (`kRingFill`, my choice), so a tap or a decline sends nothing and starts no FHU cooldown |
| `WizardWarrior` | 마검사 해제 | single press | `kHold` (shown out of combat only) |

Already rings: BookRead, ChairDrink, Helmet (off), ItemEquip, MannequinSwap, Observe, PartyOutfit,
Poison, QuestAction, QuestTrack, Recharge, Rest (all five), Surrender.

Combat prompts, single press on purpose: WeaponSwap (원거리 무기, 근접 무기), Execute, Jujutsu,
LockOn, Grapple, Helmet (on, offered only in combat), Wizard Warrior (on, raised on weapon draw).

Potion (the user's D18, 2026-09-29): a single press in combat, a ring ("마시기 (길게)") out of combat,
so the double tap declines it there. Entering or leaving combat offers it again with the other type.
`check_prompt_rules.py` lists it as `COMBAT_ONLY_PRESS` and requires both types to be set.

## Self-reporting

`tools/check_prompt_rules.py` runs before every build (`Build.ps1`) and fails it when a PromptSlot
is neither `kHold` nor `kHoldAndKeep` and is not on its combat list. It also fails a typed
`LookupForm<T>` whose `T` has no `FORMTYPE` of its own (such a lookup always returns null, found in
the r8b run).

## In-game checks (r9)

`TEST-next-ingame.md` "r9": one ring prompt (a tap does nothing, a hold acts), one decline by
condition (Bathe), one decline by distance (Sit).
