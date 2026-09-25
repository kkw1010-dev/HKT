# 032 · PartyOutfit (파티 의상)

Status (2026-09-25): passed in game on a test save (test run 2).

## As built

- MQ201 Diplomatic Immunity (`035D5F`), MQ201PartyOutfit (`0E40DF`), MQ201PartyBoots (`0E40DE`).
- 파티 의상 입기 (hold) while objective 40 or 50 is displayed (the party), the clothes are carried and not
  worn, out of combat. It stores every strippable worn piece (co-save record `QOUT`), unequips them,
  and puts on the clothes and boots.
- 원래 장비로 (hold) once neither objective is displayed, while the clothes are worn and a stored piece is
  carried again.
