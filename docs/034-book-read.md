# 034 · BookRead (책 읽기)

Status (2026-09-25): passed in game (spell tome in test run 1, quest letter in test run 3). Books were
part of `ItemEquip` until the user split them out the same day, so each has its own switch and its
own prompt (PromptID 39).

## As built

- A newly acquired spell tome whose spell is not known, or a book that is a quest item, gets the same
  15 s hold prompt as gear, reading `읽기 (길게): <이름>`.
- Spell tome: `TESObjectBOOK::Read`, `AddSpell` if still unknown, the tome removed, "<주문> 습득"
  notification. Quest book: `Read`, then `BookMenu::OpenMenuFromBaseForm` shows the page.
- Other books (loot) are not offered.
