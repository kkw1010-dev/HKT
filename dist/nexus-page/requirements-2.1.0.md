# Nexus requirements for CIGAR 2.1.0 (mod 193080)

What to enter on the page's Requirements tab. Every Nexus ID and off-site URL below was checked on
2026-09-26: the Nexus IDs by name through the public v2 GraphQL API; BaboDialogue's and Fill Her Up's
URLs are the ones other Nexus files already use for the same mods (BaboDialogue PT-BR 79571; the Fill
Her Up Spanish translation 148200, uploaded by BakaFactory); Yamete Kudasai and Private Needs - Orgasm
from the installed mods' `meta.ini`.

As of 2026-09-26 the page lists **no** requirements, and its summary still says "Base-game edition: no
other mods needed".

Nexus has no "optional" flag, so optional entries say so in their notes. Notes are what players see
next to each entry.

## Nexus requirements

| Mod | ID | Notes |
|---|---|---|
| Skyrim Script Extender (SKSE64) | 30379 | Required |
| Address Library for SKSE Plugins | 32444 | Required |
| SkyPrompt | 148703 | Required. Draws the prompts |
| SKSE Menu Framework | 120352 | Required by SkyPrompt. Also hosts CIGAR's settings panel |
| Bathing in Skyrim - Renewed | 135288 | Optional. Bathe and shower prompts |
| True Directional Movement - Modernized Third Person Gameplay | 51614 | Optional. Lock On prompt |
| Valhalla Combat | 64741 | Optional. Execute prompt |
| Acheron - Death Alternative | 108159 | Optional. Surrender prompt |
| Survival Mode Improved - SKSE | 78244 | Optional. Eat prompt, with the Survival Mode Creation Club content |
| Gourmet - A Cooking Overhaul | 96876 | Optional. Eat skips Gourmet's non-meal foods; chair drinking knows its drinks |

## Off-site requirements

| Mod | URL | Notes |
|---|---|---|
| Yamete Kudasai | https://www.loverslab.com/files/file/23123-yamete-kudasai/ | Optional. Surrender respects its 3-minute rule |
| Fill Her Up Baka Edition (BakaFactory) | https://subscribestar.adult/posts/118628 | Optional. Deflate prompt |
| Private Needs - Orgasm | https://www.loverslab.com/files/file/39023-private-needs-orgasm/ | Optional. Urinate and defecate prompts |
| BaboDialogue (BakaFactory) | https://www.loverslab.com/files/file/17496-babodialogue/ | Optional. Kidnap-room action prompt |

## Not entered

- **Grapple** (Smooth's Patreon mod, "For Honor in Skyrim - Grapple",
  <https://www.patreon.com/SmoothAanimation/posts/for-honor-in-146661018>): the user left it off the
  Nexus page (2026-09-26). The module and the readmes keep it; the Nexus description does not name it.

- **Survival Mode** (Creation Club, `ccqdrsse001-survivalmode.esl`) comes with the current game
  (Anniversary Upgrade and the free Creation Club content), so it is not a Nexus mod. The Eat and Warm
  Hands notes mention it.
- **ImGui Icons** is SkyPrompt's own requirement; SkyPrompt's page lists it.
- **SexLab** is only read to hide prompts during scenes; CIGAR does nothing with it otherwise, so it
  is not listed as a requirement.

## Settings CIGAR changes in these mods

Also on the description page and in both readmes: Grapple's hotkey to F13 and its lock key to TDM's,
Acheron's surrender key to F14, Valhalla's execution key to F15, Fill Her Up's and Private Needs'
hotkeys cleared, all through the "prompt only" switches that are on by default.
