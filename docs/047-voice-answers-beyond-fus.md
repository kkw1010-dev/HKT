# 047 · Voice answers beyond Unrelenting Force (approved D57, built 2026-10-04, not yet seen in game)

Asked by the user (D54, 2026-10-04): "푸스로다 말고 다른 용언은 또 뭐 없나?? 다대일에 써야함 + 푸스로다가 적합하지
않을 때". Source: Codex CX-58 (`C:\TAKEALOOK\_codex\results\CX-58.md`, UESP figures), checked against this load
order. **Note:** UESP gives vanilla numbers; in this game Stormcrown.esp rewrites the player shouts (recovery for
one / three words: Unrelenting Force 10 / 30 s, Disarm, Ice Form, Become Ethereal, Kyne's Peace 60 / 120 s,
Slow Time 180 / 300 s, Cyclone 120 / 180 s). The prompt only appears when the voice is ready, so long
recoveries only make an answer rarer, not wrong.

## Verdict

Add **four answers** now, all read from things VoiceAnswer already looks at (the hostile list, the player)
plus one cheap test each; leave the rest out. One prompt at a time, first match wins, and only shouts the
player knows and has not equipped (as today).

| Order | Situation (what is read) | Offer | Why not Fus here |
|---|---|---|---|
| 1 | Two or more enemies, and a **follower or neutral stands in front** (non-hostile actor within 1,000 and inside ±45° ahead) | **Slow Time** 시간 왜곡 (00048AC9) | Fus would throw the friend too; Slow Time touches no one |
| 2 | Two or more enemies, **all animals** (keyword ActorTypeAnimal), every one level 20 or lower | **Kyne's Peace** 카인의 평화 (0007097E) | ends the fight without killing; Fus only scatters them |
| 3 | Two or more enemies and **a dragon or a Dwarven centurion among them** (keyword ActorTypeDragon / ActorTypeDwarven) | **Slow Time** | they do not fall to Fus (UESP); time helps against everyone |
| 4 | Two or more enemies, **all at or below the level the known Dismay words reach** (7 / 15 / 24), none undead, daedra, dwarven or dragon | **Dismay** 공포 (0002395A) | breaks the whole group at once, Fus only the ones in front |
| 5 | Two or more enemies (today's rule) | **Unrelenting Force** (00013E07) | — |

## Left out, and why

- **Become Ethereal at low health**: a good escape, but it needs a health condition, which the user ruled out
  for the crowd prompts (D30, "체력 조건 없이 적 배치로만") and which would meet the Potion prompt at the same
  moment. Yours to decide; it would slot in above order 1.
- **Disarm, Ice Form, Fire / Frost Breath, Cyclone, Drain Vitality**: single-target or a narrow cone, and how
  many of a crowd they catch is unverified even on UESP; not a crowd answer without a test.
- **Storm Call**: hits friendly NPCs (UESP); **Marked for Death**: a UESP-noted permanent armour bug on
  followers; **Battle Fury, Dragon Aspect, Call of Valor, Animal Allegiance, Bend Will, Soul Tear,
  Summon Durnehviir**: buffs, summons or single-target, not "answers" to being outnumbered.
- Anything that needs navmesh, cliffs or a raycast per enemy per second.

## Unverified until a run

The front cone and the dragon / centurion keywords are standard reads but untested here; Dismay's level caps
and immunities are UESP's; whether these modlist's own creature overhauls change keywords is not checked.
Each answer logs its situation line, like Fus does now, so one run shows which rule fired.
