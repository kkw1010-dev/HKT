# CIGAR

CIGAR is an ESP-less SKSE plugin for Skyrim Special Edition, Anniversary Edition and VR. It shows actions through [SkyPrompt](https://www.nexusmods.com/skyrimspecialedition/mods/148703) when they make sense in play. One prompt key completes the action, including actions that would otherwise need another mod's hotkey. Each module has its own switch, so you can choose which prompts to use.

The same DLL targets SE, AE and VR. The player release has been tested on Skyrim 1.6.1170 with SkyPrompt 2.4.0 and 2.3.15. There is no `.esp` or `.esl` to add to the load order.

## Requirements

- [SKSE64](https://skse.silverlock.org/) for your game version
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- [SkyPrompt](https://www.nexusmods.com/skyrimspecialedition/mods/148703), which draws all CIGAR prompts
- SkyPrompt's requirements, including [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) and ImGui Icons. Menu Framework also provides CIGAR's settings panel.

## Install and use

Install the release archive with MO2 or Vortex, then launch Skyrim through SKSE. A new game is not needed. The in-game **CIGAR** panel lets you switch modules on or off, change the four prompt keys and adjust prompt behavior. Settings are saved in `Data/SKSE/Plugins/CIGAR.json`.

Most non-combat prompts fill a ring while you hold a key. A quick tap does nothing. Double-tap a prompt key to dismiss that prompt until its situation changes; combat prompts use a single press. Squeeze Past starts on the press and lasts while you hold its prompt. Prompts hide when a menu is open. The Keys page can use SkyPrompt's gamepad buttons or D-pad keys, and the mouse's middle and side buttons. Gamepad behavior was checked with one Xbox controller.

CIGAR includes prompts for resting, eating, gear and quest actions, potions, combat actions and outfit changes with mannequins. Squeeze Past lets you move past a standing or walking person blocking your way while your weapon is sheathed. It temporarily changes the player's collision group during the pass, leaving the NPC's collision in place. It excludes enemies and anyone talking to you. It uses an EVG gesture when the required animation mods are installed; otherwise the feature stays off.

## Optional integrations

Other mods are detected at runtime. If one is absent, its prompt stays inactive without an error. CIGAR does not include their files. Some are available off-site.

| Prompt or behavior | Optional mod | Source |
|---|---|---|
| Bathe and Shower | Bathing in Skyrim - Renewed | [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/135288) |
| Eat | Survival Mode and Survival Mode Improved - SKSE | Creation Club / [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/78244) |
| Food classification | Gourmet | [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/96876) |
| Relief prompts | Private Needs - Orgasm | [LoversLab](https://www.loverslab.com/files/file/39023-private-needs-orgasm/) |
| Deflate | Fill Her Up Baka Edition | LoversLab / SubscribeStar |
| Choose Action | BaboDialogue | [LoversLab](https://www.loverslab.com/files/file/17496-babodialogue/) |
| Lock On | True Directional Movement | [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/51614) |
| Grapple | Grapple by Smooth | Patreon |
| Execute | Valhalla Combat | [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/64741) |
| Surrender | Acheron; Yamete Kudasai for its cooldown rule | [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/108159) / [LoversLab](https://www.loverslab.com/files/file/23123-yamete-kudasai/) |
| Wizard Warrior Mode | The Wizard Warrior | [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/14890) |
| Squeeze Past | EVG Animated Traversal, Offset Movement Animation and Open Animation Replacer for the gesture | [EVG](https://www.nexusmods.com/skyrimspecialedition/mods/63232) / [Offset Movement Animation](https://www.nexusmods.com/skyrimspecialedition/mods/110408) |
| Scene-aware prompt suppression | SexLab or OStim | Off-site / Nexus |

The optional integrations are separate downloads. Squeeze Past prepares its gesture from the player's installed EVG clip; CIGAR does not ship that clip. Squeeze Past is disabled when EVG or Offset Movement Animation is absent.

## Known issues and logs

- In third person, prompts may still stick to the player position. Version 3.1.2 includes an attempted fix that has not yet been confirmed in game; CIGAR logs a stalled update hook.
- Throw may refuse until the first death after loading a game. Execute may miss its first press.
- A weapon enchantment can remain after End Wizard Warrior until it expires. This follows that mod's behavior.

`Documents/My Games/Skyrim Special Edition/SKSE/CIGAR.log` is recreated on each launch. It records which optional integrations were found and why a prompt or action was unavailable. Include it with a bug report.

## Build from source

On Windows, install Visual Studio Build Tools with the C++ toolchain, CMake, Ninja, Python and vcpkg. The CMake presets use the `x64-windows-static-md` triplet. Adjust the local Visual Studio and vcpkg paths in `tools/Build.ps1` and `CMakePresets.json` if they differ from this repository's configuration.

```powershell
git submodule update --init --recursive
powershell -ExecutionPolicy Bypass -File tools/Build.ps1
```

This produces the multi-runtime DLL in `build/release/`. Use `-Package` instead to build the player release in `build/dist/` and assemble the installable archive. `-Deploy` is for the author's local MO2 setup and is not needed to build from source.

## License

CIGAR is licensed under [GPL-3.0-or-later](LICENSE), consistent with CommonLibSSE-NG. See the release archive's third-party notices for bundled dependencies. Mods detected at runtime retain their own licenses and are not shipped with CIGAR.
