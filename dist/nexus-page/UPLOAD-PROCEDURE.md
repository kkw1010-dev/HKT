# Changing the Nexus description (mod 193080)

The author edits the page by hand between releases (feature GIFs, wording, links). On 2026-10-02 the 3.1.2
upload replaced the description with the repo's text without reading the page first, and three GIFs the
author had placed were lost. So, every time:

1. **Fetch the page's current BBCode first** (the edit form, or the public description) and save it as
   `description-live.bbcode`. Never trust the repo copy to be what is posted.
2. **Compare it with the repo's previous copy** (`git diff`). Whatever is only on the page was put there by
   the author: images, sentences, links.
3. **Carry those into `description-next.bbcode`**, at the same places.
4. `python tools/check_nexus_page.py` (also run by `Build.ps1 -Package`): the next text must have at least
   as many `[img]` and `[url]` tags as the live one, and every live image.
5. Post, then read the page back and compare the image and link counts with the ones before.

Images of the gallery (author uploads, no captions; identified by looking at them, 2026-10-02):

| Feature | URL | Uploaded (UTC) |
|---|---|---|
| Mannequin outfit swap | https://staticdelivery.nexusmods.com/mods/1704/images/193080/193080-1790773807-785490409.gif | 2026-09-30 13:10 |
| Throw | https://staticdelivery.nexusmods.com/mods/1704/images/193080/193080-1790786116-1777505335.gif | 2026-09-30 16:35 |
| Squeeze Past (likely: a crowded interior; the smallest of the three, 320x135) | https://staticdelivery.nexusmods.com/mods/1704/images/193080/193080-1790773620-1101352375.gif | 2026-09-30 13:07 |
| Header picture | https://staticdelivery.nexusmods.com/mods/1704/images/193080/193080-1790762175-2036009474.jpg | 2026-09-30 09:56 |
| Eight screenshots (png) | `193080-1790378706-...` to `193080-1790378924-...` | 2026-09-25 |

Where the author had placed the GIFs in the old description is not known (no copy was kept); in
`description-next.bbcode` each sits directly under the line that describes its feature.
