# 034 · BookRead (책 읽기)

Status (2026-09-25): passed in game (spell tome in test run 1, quest letter in test run 3). Books were
part of `ItemEquip` until the user split them out the same day, so each has its own switch and its
own prompt (PromptID 39).

## As built

- A newly acquired spell tome whose spell is not known, or a book that is a quest item, gets the same
  15 s hold prompt as gear, reading `읽기 (길게): <이름>`.
- Spell tome: `TESObjectBOOK::Read`, `AddSpell` if still unknown, the tome removed, "<주문> 습득"
  notification. Quest book: `Read`, then `BookMenu::OpenMenuFromBaseForm` shows the page.
- **Scripted, unread notes (the user's D23, 2026-10-01, after a Nexus report).** A note found on a corpse
  or in a chest is not a quest item, so it was never offered, while a courier's letter (a quest alias
  item) was. A book that is unread and carries a script (`TESForm::HasVMAD`; such notes start their quest
  when read) is now offered too, and read the same way as a quest book. Not verified in game yet: that
  `HasVMAD` is true for a script on the book's base form, and that reading through the prompt runs the
  note's `OnRead`; the log line below and r13 decide.
- Other books (loot) are not offered; the log says why: `acquired <name>: not offered (...)`.
