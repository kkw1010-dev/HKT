# 019 · Pass time

Status: built and deployed on 2026-09-21; awaiting runtime confirmation.

## Contract

After the player has stood still for five seconds outside combat, CIGAR offers a hold prompt for
Skyrim's normal wait menu. CIGAR replaces only Streamlined Interactions'
`IdleActions.enabled_passtime`; its furniture-based sitting, lying, leaning and other idle actions
remain enabled in SI.

This is a non-combat state change, so accepting requires a hold. Moving, entering combat, opening
an application menu, or reaching a location where Skyrim forbids waiting resets the idle timer.

## Implementation

- The module polls at CIGAR's 100 ms fast-tick rate. It uses `Actor::IsMoving`, combat state,
  movement controls, `Actor::CanSleepWait`, and UI state as the gate.
- Acceptance sends the native `Wait` input action through Skyrim's input event source. Skyrim then
  decides whether and how its vanilla Sleep/Wait menu opens; CIGAR does not change time scale or
  advance time itself.
- There is no persistent state. Accepting or invalidating the gate starts a new five-second idle
  interval.

## Test

1. Deploy the author build and confirm only `IdleActions.enabled_passtime` becomes false. The
   parent `IdleActions.enabled` must remain true.
2. Stand still outside combat at a location where the normal wait key works. After five seconds,
   expect `시간 보내기 (길게)`.
3. Hold the prompt key. Expect Skyrim's normal wait menu, not a CIGAR time multiplier.
4. Move, enter combat, or open an inventory menu before five seconds. No prompt should appear.
5. At a location where Skyrim blocks waiting, no prompt should appear.
