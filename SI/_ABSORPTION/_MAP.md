# Streamlined Interactions (SI) Analysis & Implementation Contracts

본 문서는 CIGAR가 아직 구현하지 않았으며 차후 흡수 가능성이 있는 Streamlined Interactions(SI) 기능만을 대상으로 정밀 분석한 결과입니다. 이미 CIGAR에 구현되었거나 제외된 기능(Bathe, Potion, KillMove 등)은 포함하지 않습니다.

## Evidence Hierarchy

본 문서의 모든 정보에는 다음 태그 중 하나가 붙습니다. 하위 태그의 내용이 이후 문맥에서 상위 태그로 승격되지 않습니다.

- **[EVIDENCE: SETTINGS]** — 실제 설정 파일(`settings.json`)에서 직접 확인된 key/value. key 이름의 존재만 증명하며, 해당 key의 정확한 runtime semantics는 증명하지 않음.
- **[EVIDENCE: TRANSLATIONS]** — 번역 파일에서 실제 문자열 존재만 증명. 해당 문자열의 trigger, action, module ownership은 증명하지 않음.
- **[EVIDENCE: OFFICIAL VIDEO/DOC]** — 공식 자료에서 직접 관찰/서술된 player-visible behavior만.
- **[EVIDENCE: CIGAR]** — 현재 CIGAR 코드/문서에서 확인된 사실만.
- **[INFERENCE]** — evidence에서 논리적으로 추론한 내용. 이후 섹션에서 "confirmed", "확정", "known"으로 승격하지 않음.
- **[IMPLEMENTATION POLICY]** — SI 원본 동작이 불명확하지만 CIGAR가 독립적으로 정할 수 있는 설계 선택.
- **[UNKNOWN]** — 현재 자료로 확정 불가능.

## CIGAR absorption status

- **Quest Tracking:** implemented as `QuestTrack` on 2026-09-21; hold, tracking, and objective-text label fallback confirmed in game.
- **Acquired gear equip:** implemented as `ItemEquip` on 2026-09-21; confirmed in game on 2026-09-25.
- **Potions:** `Potion` (`docs/015-potion.md`); all six needs confirmed in game by 2026-09-25.
- **SI's own help texts for the leftovers** (DLL strings, read 2026-09-25; these settle the player-visible behavior the sections below mark UNKNOWN):
  - Outfit Swap (Piecewise): "prompt to swap armor/clothing at different body parts with available ones in the vicinity. Note that the items can be out in the world or in a container." Scene names `SwapOutfitHead`, `SwapOutfitChest`, `SwapOutfitArms`, `SwapOutfitLegs`, `SwapOutfit`.
  - HelmetToggle: "prompt to toggle the helmet off/on when the player enters a safe/unsafe location." Classes `HelmetToggle`, `HelmetUndresser`; scene `ToggleHelmet`. Which locations count as safe is not in the strings.
  - Observer: scene `Observe`; settings Idle Timer, FOV Increment, FOV Offset, Observe Time, `moving_exits_observe`; the help texts for these three FOV keys are not present as strings.
  - Spellbook equip: `Equipper::ReadBook` ("Failed to read book", "Book not found", "Book Menu"), and an Immersive Spell Learning integration (`ModSupport::ImmersiveSpellLearning::Install`, "Spell notes formid is null", needs po3).
- **HelmetToggle:** `Helmet` (2026-09-25, `docs/028-helmet.md`), built on Helmet Toggle 2's own hotkey function and location lists.
- **Remaining, all off in SI (2026-09-25):** Observer (3), piecewise outfit swap (1), spellbook equip (6), quest note/book equip (4b), quest outfit swap (4c), TidyUp (4d, mod not installed), KillMove (covered by `Execute`). `IdleActions.enabled_passtime` reads true but its module is off.
- SI overlap disabled by CIGAR for ItemUse: equip weapon/armor, the six potion actions, make-light (`Light`, 2026-09-24) and recharge weapon (`Recharge`, 2026-09-24, `docs/023-recharge.md`). Spellbook equip stays off by the user's choice.
- WeaponSwap: built as `ToolSwap` and removed the same day at the user's call (`docs/024-tool-swap.md`); `WeaponSwap.enabled` stays off.
- IdleActions chair eat/drink: `ChairDrink` (inns and houses, alcohol only; `docs/026-chair-drink.md`).
- QuestActions: the Greybeards shout prompt is `QuestAction` (2026-09-24, `docs/025-quest-action.md`); `QuestActions.enabled` is off. 4b/4c below stay unconfirmed for SI 1.0.6.
- SI overlap disabled by CIGAR: `QuestActions.enabled_track` only. Other `QuestActions` behavior remains owned by SI.
- Runtime implementation uses `ObjectiveState::Event` for the new-objective gate and the verified native
  Papyrus call `Quest.SetActive(true)` for the action. This is CIGAR implementation policy, not evidence
  that SI uses the same engine path.

---

## 1. DressActions (Piecewise Outfit Swap)

- **SI module / setting name:** DressActions / `enabled_piecewiseoutfitswap` [EVIDENCE: SETTINGS]
- **Player-visible behavior:** 특정 부위(머리, 상의 등)의 장비를 개별적으로 탈의하거나 교체하도록 제안하는 것으로 추정 [INFERENCE — 번역 문자열 "탈의 (머리)", "의상 교체"의 존재 [EVIDENCE: TRANSLATIONS]에서 추론. 문자열 존재가 이 기능의 동작 방식을 증명하지는 않음]
- **Trigger condition:** [UNKNOWN]
- **Suppression condition:** [UNKNOWN]
- **Prompt text/type:** "탈의 (머리)", "상체 장비 교체" 등의 문자열이 존재함 [EVIDENCE: TRANSLATIONS] / Type: [UNKNOWN]
- **Input semantics:** [UNKNOWN]
- **Action performed:** 플레이어의 특정 BipedObjectSlot(머리, 상의 등) 장착 해제 또는 교체 [INFERENCE]
- **State remembered before action:** [UNKNOWN]
- **State restored after action:** [UNKNOWN]
- **Dependencies:** [UNKNOWN]
- **Skyrim engine information required:** `BGSBipedObjectForm` (Slot Mask), `ActorEquipManager` [IMPLEMENTATION POLICY — CIGAR 구현 시 사용할 후보 API]
- **Event-driven 가능 여부:** [UNKNOWN]
- **필요한 최소 polling 주기:** [UNKNOWN]
- **CIGAR에서 재사용 가능한 machinery:** `002-dress` 모듈의 Slot Check 및 Equip/Unequip 로직 [EVIDENCE: CIGAR]
- **SI에서 disable해야 하는 setting:** `DressActions.enabled_piecewiseoutfitswap` [EVIDENCE: SETTINGS]
- **Save persistence 필요 여부:** [UNKNOWN]
- **Known edge cases:** HDT-SMP 물리 가발, 퀘스트 고정 아이템 강제 해제 시 버그 [EVIDENCE: CIGAR `002-dress` — CIGAR 구현 경험에서 확인된 리스크]
- **구현에 필요한 미확인 정보:** 프롬프트 출현 조건(Gate), 교체 대상 선정 방식.

### IMPLEMENTATION CONTRACT

#### Evidence-backed behavior
없음. 번역 문자열 존재만 확인됨.

#### Parameter-supported hypothesis
`enabled_piecewiseoutfitswap` 설정이 존재하므로, 부위별 장비 교체 기능이 있을 것으로 추정 [INFERENCE].

#### Unknown
- Gate 조건 (무엇을 바라보거나 어떤 상태에서 프롬프트가 나타나는지) [UNKNOWN — 구현을 막음]
- Swap 대상 선정 기준 [UNKNOWN — 구현을 막음]

#### CIGAR implementation policy
해당 없음 — gate가 불명이므로 정책 결정 이전 단계.

#### Minimal experiment
SI에서 `enabled_piecewiseoutfitswap` 활성화 후, 옷장 근처/바닥 아이템/자유 상태에서 프롬프트가 뜨는 조건을 탐색. 인벤토리에 여러 투구가 있을 때 어떤 것이 교체 대상으로 뜨는지 확인.

---

## 2. IdleActions (Contextual & PassTime)

- **SI module / setting name:** IdleActions (`enabled`, `enabled_passtime`, `t_threshold: 1.0`, `passtime_delay: 5.0`, `max_timemult: 2.0`) [EVIDENCE: SETTINGS]
- **Player-visible behavior:** 주변 환경에 맞춰 앉기, 기대기, 눕기 등의 애니메이션을 수행하거나 시간 보내기를 수행 [EVIDENCE: OFFICIAL VIDEO/DOC — 공식 영상에서 가구 근처에서 idle action prompt 확인]. 번역 문자열: "앉기", "일어나기", "눕기", "기대기", "손 녹이기", "시간 보내기" [EVIDENCE: TRANSLATIONS]
- **Trigger condition:**
  - Contextual: 플레이어가 유휴 상태이며 특정 사물(의자, 침대, 벽, 모닥불) 근처에 있을 때 [EVIDENCE: OFFICIAL VIDEO/DOC — 공식 영상에서 가구 근처 프롬프트 확인]
  - PassTime: 설정값 `passtime_delay: 5.0` [EVIDENCE: SETTINGS]. 유휴 상태를 이 값 이상 유지했을 때 발동하는 것으로 추정 [INFERENCE — 설정 이름에서 추론]
  - 설정값 `t_threshold: 1.0` [EVIDENCE: SETTINGS]. 정확한 의미 불명 [UNKNOWN]
- **Suppression condition:** 플레이어 이동 시, 전투 중 [INFERENCE]
- **Prompt text/type:** 위 번역 문자열들 [EVIDENCE: TRANSLATIONS] / Type: [UNKNOWN]
- **Input semantics:** [UNKNOWN]
- **Action performed:** 가구 활성화(Activate), 유휴 애니메이션 재생, 또는 시간 배율 변경 [INFERENCE]. `max_timemult: 2.0`은 PassTime의 시간 배율 상한일 가능성이 높음 [INFERENCE — 설정 이름에서 추론]
- **State remembered before action:** PassTime의 경우 기존 시간 배율 [INFERENCE]
- **State restored after action:** 이동 시 시간 배율 원상 복구 [INFERENCE]
- **Dependencies:** [UNKNOWN]
- **Skyrim engine information required:** [IMPLEMENTATION POLICY — CIGAR 구현 시 후보: `PlayerCharacter` 이동 상태, 주변 Furniture 레퍼런스, `BSTimer::SetGlobalTimeMultiplier`]
- **Event-driven 가능 여부:** [INFERENCE — 가구는 CrosshairRefEvent 활용 가능할 수 있으나, PassTime은 Polling이 필요할 것으로 추정]
- **필요한 최소 polling 주기:** 1s 이하 [INFERENCE]
- **CIGAR에서 재사용 가능한 machinery:** `Tick()`, `PromptSlot` [EVIDENCE: CIGAR]
- **SI에서 disable해야 하는 setting:** `IdleActions.enabled`, `IdleActions.enabled_passtime` [EVIDENCE: SETTINGS]
- **Save persistence 필요 여부:** 없음 [INFERENCE]
- **Known edge cases:** 이미 다른 유휴 애니메이션이 재생 중이거나 무기를 뽑은 상태 [INFERENCE]
- **구현에 필요한 미확인 정보:** 벽 기대기나 손 녹이기의 대상 감지 방식(크로스헤어 타겟팅인지 위치 기반인지), `t_threshold`의 의미.

### IMPLEMENTATION CONTRACT

#### Evidence-backed behavior
가구(의자, 침대 등) 근처에서 idle action prompt가 나타남 [EVIDENCE: OFFICIAL VIDEO/DOC].

#### Parameter-supported hypothesis
`passtime_delay: 5.0` — 유휴 5초 후 PassTime 발동 [INFERENCE]. `max_timemult: 2.0` — 시간 배율 상한 [INFERENCE]. `t_threshold: 1.0` — 의미 불명 [UNKNOWN].

#### Unknown
- Lean(기대기), WarmHands(손 녹이기) 프롬프트의 대상 감지 반경 및 방식 [UNKNOWN — 구현을 막음. 오감지 위험]
- 프롬프트 유형 (Hold/Press) [UNKNOWN]
- `t_threshold`의 역할 [UNKNOWN]

#### CIGAR implementation policy
Sit(앉기), GetUp(일어나기), LayDown(눕기)는 크로스헤어 Furniture 감지 + Activate로 독립 설계 가능. PassTime은 idle timer + SGTM으로 독립 설계 가능. Lean/WarmHands는 gate 불명으로 보류.

#### Minimal experiment
모닥불 근처에서 시선을 모닥불에 두었을 때와 딴 곳에 두었을 때, 여관 벽 근처에서 벽을 볼 때 프롬프트가 뜨는지 확인.

---

## 3. Observer

- **SI module / setting name:** Observer (`enabled`, `moving_exits_observe: true`, `idle_timer: 5.0`, `fov_increment: 20.0`, `fov_offset: 40.0`, `t_observe: 5.0`, `max_distance: 5000.0`) [EVIDENCE: SETTINGS]
- **Player-visible behavior:** FOV를 조정하여 대상을 자세히 관찰하는 것으로 추정 [INFERENCE — 변수명 `fov_offset`, `fov_increment`에서 추론. 설정 이름이 runtime behavior를 증명하지 않음]
- **Trigger condition:**
  - 설정값: `idle_timer: 5.0`, `max_distance: 5000.0` [EVIDENCE: SETTINGS]
  - `idle_timer`는 유휴 대기 시간일 가능성이 높음 [INFERENCE]. `max_distance`는 대상까지의 거리 상한일 가능성이 높음 [INFERENCE]. 그러나 `max_distance`가 크로스헤어 대상 거리인지, 플레이어 주변 오브젝트 탐색 반경인지, 또는 카메라 관련 파라미터인지는 변수명만으로 확정 불가 [UNKNOWN]
- **Suppression condition:** `moving_exits_observe: true` — 이동 시 관찰이 종료됨을 의미할 가능성이 높음 [INFERENCE — 설정 이름에서 추론]
- **Prompt text/type:** "살펴보기" [EVIDENCE: TRANSLATIONS — 문자열 존재만 확인] / Type: [UNKNOWN]
- **Input semantics:** [UNKNOWN — Hold인지 Press인지 Toggle인지 확인 불가]
- **Action performed:**
  - 설정값: `fov_offset: 40.0`, `fov_increment: 20.0`, `t_observe: 5.0` [EVIDENCE: SETTINGS]
  - FOV를 변경하는 것으로 추정 [INFERENCE]. `fov_increment`가 단계적 변화량인지 초당 변화량인지 불명 [UNKNOWN]. `t_observe`가 FOV tween duration인지 관찰 유지 시간인지 불명 [UNKNOWN]. `fov_offset: 40.0`이 "현재 FOV에서 40을 빼는 것"인지 "FOV를 40으로 설정하는 것"인지 불명 [UNKNOWN]
- **State remembered before action:** 기존 FOV 값 [INFERENCE — FOV를 변경한다면 원래값 복구가 필요할 것]
- **State restored after action:** 관찰 종료 시 기존 FOV로 복구 [INFERENCE]
- **Dependencies:** 없음 [INFERENCE]
- **Skyrim engine information required:** [IMPLEMENTATION POLICY — CIGAR 구현 시 후보: `PlayerCamera` FOV 접근/변경, 플레이어 이동 상태]
- **Event-driven 가능 여부:** 불가 (Polling 필요) [INFERENCE]
- **필요한 최소 polling 주기:** FOV 트위닝을 수행한다면 100ms (`FastTick()`) [INFERENCE]. 단순 즉시 전환이면 1s (`Tick()`)으로 충분 [INFERENCE]
- **CIGAR에서 재사용 가능한 machinery:** `FastTick()`, `PromptSlot` [EVIDENCE: CIGAR]
- **SI에서 disable해야 하는 setting:** `Observer.enabled` [EVIDENCE: SETTINGS]
- **Save persistence 필요 여부:** 없음 [INFERENCE]
- **Known edge cases:** 관찰 도중 대화(Dialog)나 전투에 돌입하여 카메라 제어권이 강제 이전되는 경우 [INFERENCE]
- **구현에 필요한 미확인 정보:**
  1. 버튼 상호작용 방식 (Hold / Toggle / 기타)
  2. `max_distance`가 크로스헤어 대상 거리인지 다른 의미인지
  3. `t_observe`가 FOV 전환 소요 시간인지 관찰 유지 시간인지
  4. `fov_offset`이 현재 FOV에서의 상대 감소치인지 절대값인지
  5. `fov_increment`의 역할 (단계? 초당? `fov_offset`과의 관계?)
  6. 관찰 종료 조건 (버튼 릴리스? 이동? 타이머? 전부?)

### IMPLEMENTATION CONTRACT

#### Evidence-backed behavior
없음. 번역 문자열 "살펴보기"의 존재만 확인됨 [EVIDENCE: TRANSLATIONS].

#### Parameter-supported hypothesis
설정 key 이름에 기반한 추정:
- 유휴 `idle_timer`(5.0s) 후 gate 발동 [INFERENCE]
- 이동 시(`moving_exits_observe`) 종료 [INFERENCE]
- FOV를 `fov_offset`(40.0)만큼 변경, `fov_increment`(20.0)와 `t_observe`(5.0)가 관여 [INFERENCE]
- 대상이 `max_distance`(5000) 이내일 때 [INFERENCE]

이 중 어느 것도 설정 이름만으로는 확정되지 않음.

#### Unknown
- `fov_offset`, `fov_increment`, `t_observe` 세 값의 정확한 상호작용 [UNKNOWN]
- 프롬프트 유형과 관찰 종료 조건 [UNKNOWN]
- `max_distance`의 정확한 의미 [UNKNOWN]

SI와 동일한 Observer 재현은 최소 실험 전에는 확정 불가.

#### CIGAR implementation policy
CIGAR 자체 Observer를 독립 설계한다면 구현 가능 [IMPLEMENTATION POLICY]. 예: `PlayerCamera` FOV를 사용자 설정 offset만큼 tween → 이동 또는 전투 시 복구. SI 설정값은 기본값 참고로 사용 가능.

#### Minimal experiment
SI에서 Observer 활성화 후: (1) 정지 상태에서 5초 대기 후 프롬프트가 뜨는지, (2) 프롬프트 수락 후 FOV 변화가 즉시인지 점진적인지, (3) 버튼에서 손을 떼면 즉시 복구되는지 이동해야 복구되는지 관찰.

---

## 4. QuestActions

- **SI module / setting name:** QuestActions (`enabled`, `enabled_track`) [EVIDENCE: SETTINGS]
- **SI에서 disable해야 하는 setting:** CIGAR가 Quest Tracking만 흡수한 현재 상태에서는 `QuestActions.enabled_track`만 비활성화한다 [EVIDENCE: CIGAR]. `QuestActions.enabled` 자체를 끄면 아직 SI가 담당하는 다른 동작까지 함께 사라진다 [IMPLEMENTATION POLICY].

### 4a. Quest Tracking

- **Player-visible behavior:** 새 퀘스트 목표(objective)를 받으면 Track Quest prompt가 뜨고, 수락하면 해당 퀘스트가 활성 추적 상태로 전환됨 [EVIDENCE: OFFICIAL VIDEO — Quantumyilmaz Quest Tracking 공식 영상에서 new objective 수신 직후 Track Quest prompt 확인]
- **Trigger condition:** 플레이어가 새 quest objective를 수신한 시점 [EVIDENCE: OFFICIAL VIDEO]
- **Suppression condition:** [UNKNOWN]
- **Prompt text/type:** "퀘스트 추적 |" [EVIDENCE: TRANSLATIONS — 문자열 존재 확인] / Type: Hold [EVIDENCE: USER IN-GAME KNOWLEDGE, 2026-09-21]
- **Input semantics:** SI uses a hold interaction [EVIDENCE: USER IN-GAME KNOWLEDGE, 2026-09-21]. CIGAR uses `SkyPromptAPI::kHold` [EVIDENCE: CIGAR — runtime confirmed 2026-09-21].
- **Action performed:** 프롬프트 수락 시 해당 퀘스트가 활성 추적 상태로 전환됨 [EVIDENCE: OFFICIAL VIDEO/DOC — player-visible result]. CIGAR는 native Papyrus `Quest.SetActive(true)`를 호출한다 [EVIDENCE: CIGAR — runtime confirmed 2026-09-21].
- **Displayed label:** QUST `Name`이 있으면 퀘스트 이름을, 이름이 없는 miscellaneous quest는 새 objective의 `DisplayText`를 사용한다. FormID는 플레이어에게 표시하지 않는다 [EVIDENCE: QUST/QuestObjective schema + runtime report; fix build passed, runtime test pending, 2026-09-21].
- **State remembered before action:** 이전 추적 퀘스트 [INFERENCE]
- **State restored after action:** 없음 [INFERENCE]
- **Dependencies:** 없음
- **Skyrim engine information required:** CIGAR uses `ObjectiveState::Event` to observe a transition to `kDisplayed`, then dispatches the verified native Papyrus method `Quest.SetActive(true)` [EVIDENCE: CIGAR — runtime confirmed 2026-09-21]. This does not prove SI uses the same path.
- **Event-driven 가능 여부:** CIGAR implementation is event-driven [EVIDENCE: CIGAR — runtime confirmed 2026-09-21].
- **필요한 최소 polling 주기:** N/A for discovery. CIGAR's existing 100 ms `FastTick()` maintains and expires the prompt [EVIDENCE: CIGAR].
- **CIGAR에서 재사용 가능한 machinery:** `PromptSlot`, game-thread task marshalling, module gate logging [EVIDENCE: CIGAR]
- **Save persistence 필요 여부:** 없음 [INFERENCE]
- **Known edge cases:** 동시에 여러 objective가 갱신될 때 어떤 퀘스트를 제안하는지 [INFERENCE]
- **구현에 필요한 미확인 정보:** 없음. 실제 이벤트 firing과 동시 objective 처리는 런타임 검증 대상이다.

#### IMPLEMENTATION CONTRACT (Quest Tracking)

##### Evidence-backed behavior
new objective 수신 직후 Track Quest prompt가 출현하고, 수락 시 해당 퀘스트가 활성 추적 상태로 전환됨 [EVIDENCE: OFFICIAL VIDEO/DOC].

##### Parameter-supported hypothesis
`enabled_track` 설정이 이 기능의 on/off 스위치일 가능성이 높음 [INFERENCE — 설정 이름에서 추론].

##### Unknown
- SI의 정확한 suppression 조건과 동시에 여러 objective가 표시될 때의 선택 규칙 [UNKNOWN]

##### CIGAR implementation policy
`ObjectiveState::Event`의 `oldState -> kDisplayed` 전환을 gate로 사용한다 [IMPLEMENTATION POLICY]. 이벤트 싱크에서는 quest FormID와 objective index만 복사하고, 실제 폼 확인과 프롬프트 상태 변경은 게임 스레드에서 수행한다. 프롬프트는 hold이며 15초 동안 유지한다. 수락 시 `Quest.SetActive(true)`를 호출하고 `TESQuest::IsActive()`로 결과를 확인한다 [EVIDENCE: CIGAR — runtime confirmed 2026-09-21, including objective-text label fallback].

##### Minimal experiment
SI의 Quest Tracking 활성 상태에서 퀘스트 단계 진행 시 프롬프트가 즉시(이벤트) 뜨는지, 지연 후(polling) 뜨는지 관찰.

### 4b. Quest Item Equip

- **Player-visible behavior:** NPC의 시체에서 note를 습득한 직후, 인벤토리를 열지 않고 해당 note를 즉시 장착(읽기)할 수 있는 프롬프트가 발생 [EVIDENCE: OFFICIAL VIDEO — Quantumyilmaz Quest Tracking 공식 영상에서 Fred의 body에서 note 습득 직후 equip prompt 확인]
- **Trigger condition:** 아이템(note, book 등)을 습득한 직후 [EVIDENCE: OFFICIAL VIDEO]. quest 관련 아이템으로 한정되는지, 모든 note/book이 대상인지는 불명 [UNKNOWN]
- **Suppression condition:** [UNKNOWN]
- **Prompt text/type:** "장착하기" (Equip) [EVIDENCE: TRANSLATIONS — 문자열 존재 확인] / Type: [UNKNOWN]
- **Input semantics:** [UNKNOWN]
- **Action performed:** 습득한 아이템을 장착 (note/book의 경우 읽기 UI 호출) [INFERENCE]. [IMPLEMENTATION POLICY — CIGAR 구현 시 후보 API: `ActorEquipManager::EquipObject`]
- **State remembered before action:** 없음 [INFERENCE]
- **State restored after action:** 없음 [INFERENCE]
- **Dependencies:** 없음
- **Skyrim engine information required:** [IMPLEMENTATION POLICY — CIGAR 구현 시 후보: `TESContainerChangedEvent`로 습득 감지. SI가 이 이벤트를 사용한다는 증거는 없음]
- **Event-driven 가능 여부:** [INFERENCE — container change 이벤트 활용이 가능할 것으로 추정]
- **필요한 최소 polling 주기:** N/A (이벤트 기반 구현 시) [INFERENCE]
- **CIGAR에서 재사용 가능한 machinery:** `PromptSlot` [EVIDENCE: CIGAR]
- **Save persistence 필요 여부:** 없음 [INFERENCE]
- **Known edge cases:** 동시에 여러 아이템을 루팅할 때 [INFERENCE]
- **구현에 필요한 미확인 정보:** 프롬프트 대상의 범위(quest item only? 모든 book?), 프롬프트 유형(Hold/Press)

#### IMPLEMENTATION CONTRACT (Quest Item Equip)

##### Evidence-backed behavior
note 습득 직후 equip prompt 출현 [EVIDENCE: OFFICIAL VIDEO].

##### Parameter-supported hypothesis
해당 없음 — settings.json에 이 기능의 개별 설정이 없음. `QuestActions.enabled`로 제어되는 것으로 추정 [INFERENCE].

##### Unknown
- 프롬프트 대상 범위 (quest item only? 모든 book?) [UNKNOWN — 구현을 막지 않음. 넓은 범위로 시작 후 제한 가능]
- 프롬프트 유형(Hold/Press) [UNKNOWN]

##### CIGAR implementation policy
`TESContainerChangedEvent`로 습득 감지 후, `TESForm::formType == Book`인 경우로 시작하여 범위 조정 가능 [IMPLEMENTATION POLICY].

##### Minimal experiment
SI에서 quest note와 무관한 일반 book을 루팅했을 때도 equip 프롬프트가 뜨는지 확인.

### 4c. Quest Contextual Actions (Shout Equip, Quest Outfit Swap)

공식 Quest Interactions 영상에서 다음 behavior의 **존재**가 확인됨:
- NPC가 shout 사용을 요구하는 퀘스트 상황에서 해당 shout을 equip하는 prompt [EVIDENCE: OFFICIAL VIDEO — Confirmed in Quest Interactions video; current installed version verification required]
- Party Clothes 같은 quest outfit으로 교체하는 prompt [EVIDENCE: OFFICIAL VIDEO — Confirmed in Quest Interactions video; current installed version verification required]
- quest outfit에서 이전(previous) 장비로 복귀하는 prompt [EVIDENCE: OFFICIAL VIDEO — Confirmed in Quest Interactions video; current installed version verification required]

**Module ownership:** [UNKNOWN — 위 behavior가 `QuestActions` 모듈 내부 구현이라는 직접 증거 없음. settings.json의 `QuestActions` 키에 shout/outfit 관련 하위 설정이 존재하지 않음. SI 내 별도 모듈이거나 QuestActions의 일부일 수 있음]

- **Player-visible behavior:** 퀘스트 맥락에 맞는 shout equip 또는 outfit 교체 프롬프트 [EVIDENCE: OFFICIAL VIDEO]
- **Trigger condition:** [UNKNOWN — 어떤 quest의 어떤 stage/objective에서 발동하는지 불명확]
- **Suppression condition:** [UNKNOWN]
- **Prompt text/type:** outfit 관련은 "의상 교체" 문자열이 존재함 [EVIDENCE: TRANSLATIONS]. shout equip은 "장착하기" 또는 "시전하기" [INFERENCE — 문자열 존재에서 추론] / Type: [UNKNOWN]
- **Input semantics:** [UNKNOWN]
- **Action performed:** shout equip 및 outfit 교체 [INFERENCE]. [IMPLEMENTATION POLICY — CIGAR 구현 시 후보 API: `ActorEquipManager::EquipShout`, outfit 저장/복원]
- **State remembered before action:** outfit swap의 경우 교체 전 전체 장비 세트 (previous equipment 복귀 기능이 영상에서 확인됨) [EVIDENCE: OFFICIAL VIDEO]
- **State restored after action:** 이전 장비로 복귀 prompt 수락 시 [EVIDENCE: OFFICIAL VIDEO]
- **Dependencies:** [UNKNOWN]
- **Skyrim engine information required:** [IMPLEMENTATION POLICY — CIGAR 구현 시 후보: Quest stage 감지, Shout form lookup, 전체 장비 세트 저장/복원]
- **Event-driven 가능 여부:** [UNKNOWN]
- **필요한 최소 polling 주기:** [UNKNOWN]
- **CIGAR에서 재사용 가능한 machinery:** `002-dress` 모듈의 outfit 저장/복원, co-save [EVIDENCE: CIGAR]
- **Save persistence 필요 여부:** 이전 장비 세트를 quest 진행 중 저장해야 할 것으로 추정 [INFERENCE]
- **Known edge cases:** quest outfit이 여러 단계에 걸쳐 변경될 때, 이전 장비 중 일부가 드랍/소실된 경우 [INFERENCE]
- **구현에 필요한 미확인 정보:** 트리거 quest stage/objective 판별 방식, module ownership, SI가 하드코딩된 quest 목록을 갖는지 generic 조건을 사용하는지.

#### IMPLEMENTATION CONTRACT (Quest Contextual)

##### Evidence-backed behavior
shout equip prompt, quest outfit swap prompt, previous equipment 복귀 prompt의 존재 [EVIDENCE: OFFICIAL VIDEO].

##### Parameter-supported hypothesis
해당 없음 — settings.json에 이 기능의 개별 설정이 없음.

##### Unknown
- Module ownership [UNKNOWN]
- 트리거 조건 (어떤 quest의 어떤 stage) [UNKNOWN — 구현을 막음. gate를 정의할 수 없음]
- SI가 바닐라 quest를 하드코딩하는지 generic 조건을 사용하는지 [UNKNOWN — 구현을 막음]

##### CIGAR implementation policy
해당 없음 — gate가 불명이므로 정책 결정 이전 단계.

##### Minimal experiment
Ustengrav에서 Greybeard가 shout을 요구하는 퀘스트 단계에서 SI가 shout equip 프롬프트를 띄우는지 확인. Embassy party quest에서 party clothes 교체 프롬프트가 뜨는지 확인.

### 4d. "TidyUp / 주변 정리하기"

- **Prompt text:** "주변 정리하기" [EVIDENCE: TRANSLATIONS — 문자열 존재만 확인]
- **Module ownership:** [UNKNOWN — QuestActions 소속이라는 직접 근거 없음. translations에 action으로만 존재하며, settings.json의 QuestActions 키에 TidyUp 관련 설정이 별도로 없음]
- **Player-visible behavior:** [UNKNOWN]
- **Trigger condition:** [UNKNOWN]
- **Action performed:** [UNKNOWN]

#### IMPLEMENTATION CONTRACT (TidyUp)

##### Evidence-backed behavior
번역 문자열 "주변 정리하기"의 존재만 확인됨 [EVIDENCE: TRANSLATIONS].

##### Unknown
- 이 프롬프트가 어떤 모듈에 속하는지 [UNKNOWN — 구현을 막음]
- 무엇을 하는지 [UNKNOWN — 구현을 막음]
- 트리거 조건 [UNKNOWN — 구현을 막음]

##### Minimal experiment
SI에서 QuestActions 활성화 후, 던전이나 NPC 사망 현장에서 "주변 정리하기" 프롬프트가 뜨는 상황을 탐색. QuestActions를 끈 상태에서도 뜨는지 비교하여 모듈 소속 확인.

---

## 5. HelmetToggle

- **SI module / setting name:** HelmetToggle (`enabled: false`) [EVIDENCE: SETTINGS]
- **Player-visible behavior:** "투구 표시 전환" 프롬프트 출력 [EVIDENCE: TRANSLATIONS — 문자열 존재만 확인. 이 문자열이 HelmetToggle 모듈에 의해 출력된다는 것은 설정 이름과의 대응에서 추론 [INFERENCE]]
- **Trigger condition:** [UNKNOWN]
- **Suppression condition:** [UNKNOWN]
- **Prompt text/type:** "투구 표시 전환" [EVIDENCE: TRANSLATIONS — 문자열 존재 확인] / Type: [UNKNOWN]
- **Input semantics:** [UNKNOWN]
- **Action performed:** 투구를 Unequip하거나, NiNode를 숨기거나, 또는 다른 메커니즘 [UNKNOWN]
- **State remembered before action:** 투구 장착 상태 [INFERENCE]
- **State restored after action:** [UNKNOWN]
- **Dependencies:** [UNKNOWN]
- **Skyrim engine information required:** [IMPLEMENTATION POLICY — CIGAR 구현 시 후보: Armor Slot 30/31 상태, `ActorEquipManager` 또는 `NiAVObject`]
- **Event-driven 가능 여부:** [UNKNOWN]
- **필요한 최소 polling 주기:** [UNKNOWN]
- **CIGAR에서 재사용 가능한 machinery:** `002-dress` 모듈의 방어구 파악 로직 [EVIDENCE: CIGAR]
- **SI에서 disable해야 하는 setting:** `HelmetToggle.enabled` [EVIDENCE: SETTINGS]
- **Save persistence 필요 여부:** [UNKNOWN — 구현 메커니즘에 따라 달라짐. Unequip 방식이면 Skyrim 자체 장비 상태 저장으로 충분할 수 있고, NiNode/render hide 방식이면 별도 상태 저장이 필요할 수 있음. 현재 SI가 어느 방식을 쓰는지 UNKNOWN]
- **Known edge cases:** [INFERENCE — NiNode Hide 방식을 선택할 경우 머리카락 메시 클리핑 위험이 있을 수 있음. 이는 SI behavior evidence가 아니라 CIGAR 구현 시 고려사항]
- **구현에 필요한 미확인 정보:** Gate 조건, 투구 숨김 방식(Unequip vs render hide).

### IMPLEMENTATION CONTRACT

#### Evidence-backed behavior
없음. 번역 문자열 "투구 표시 전환"의 존재만 확인됨 [EVIDENCE: TRANSLATIONS].

#### Parameter-supported hypothesis
`HelmetToggle.enabled` 설정이 존재하므로 투구 표시를 전환하는 기능이 있을 것으로 추정 [INFERENCE].

#### Unknown
- Gate 조건 [UNKNOWN — 구현을 막음]
- 투구 숨김 방식 (Unequip vs NiNode Hide vs 기타) [UNKNOWN — 구현 방식 결정을 막음]

#### CIGAR implementation policy
Unequip 방식을 선택하면 `002-dress`의 기존 equip/unequip 로직 재사용 가능. NiNode Hide 방식을 선택하면 별도 구현이 필요하고, 선택한 persistence 정책에 따라 co-save가 필요할 수 있으며 머리카락 클리핑 리스크가 존재 [IMPLEMENTATION POLICY].

#### Minimal experiment
SI에서 HelmetToggle 활성화 후, 투구를 쓴 상태에서 프롬프트가 언제 뜨는지 관찰. 투구 숨김 후 인벤토리에서 장착 해제되어 있는지 렌더링만 꺼졌는지 확인.

---

## 6. ItemUse (Weapon Recharge, MakeLight, Equip Context)

- **SI module / setting name:** ItemUse (`enabled_recharge_weapon`, `recharge_weapon_oooc`, `enabled_equip_weapon`, `enabled_equip_armor`, `enabled_equip_spellbook`, `enabled_makelight`, `dont_show_in_combat_makelight`, `time_till_makelight_prompt`, `darkness_threshold`) [EVIDENCE: SETTINGS]
- **Player-visible behavior:**
  - "무기 충전하기" 문자열 존재 [EVIDENCE: TRANSLATIONS]
  - "장착하기" / "시전하기" 문자열 존재 [EVIDENCE: TRANSLATIONS]
- **Trigger condition:**
  - **Recharge:**
    - 설정값: `enabled_recharge_weapon: true`, `recharge_weapon_oooc: true` [EVIDENCE: SETTINGS]
    - 마법무기의 충전량이 일정 수준 이하일 때 소울젬으로 충전을 제안하는 것으로 추정 [INFERENCE]. `recharge_weapon_oooc`은 변수명에서 비전투 시에만 발동하는 조건으로 추정 [INFERENCE — 설정 이름에서 추론. "oooc"이 "out of combat"을 의미한다는 것은 확정되지 않음]
    - 충전량이 정확히 0일 때만 발동하는지, 일정 비율 이하에서 발동하는지 [UNKNOWN]
    - 어떤 소울젬을 선택하는지 [UNKNOWN]
    - 어떤 hand의 weapon을 대상으로 하는지 [UNKNOWN]
  - **Equip:** 공식 영상에서 아이템과 상호작용하거나 습득하는 맥락에서 Equip prompt가 나타나는 사례가 확인됨 [EVIDENCE: OFFICIAL VIDEO/DOC]. 정확한 gate가 Crosshair targeting인지 inventory acquisition event인지는 [UNKNOWN].
  - **MakeLight:**
    - 설정값: `enabled_makelight: false` (현재 비활성), `dont_show_in_combat_makelight: true`, `darkness_threshold: 14.0`, `time_till_makelight_prompt: 5.0` [EVIDENCE: SETTINGS]
    - 조명도가 `darkness_threshold` 미만인 환경에서 `time_till_makelight_prompt` 대기 후 발동으로 추정 [INFERENCE — 설정 이름에서 추론. `darkness_threshold`가 정확히 `PlayerCharacter::GetLightLevel()` 반환값을 기준으로 하는지는 확정되지 않음]
    - `dont_show_in_combat_makelight`은 전투 중 억제로 추정 [INFERENCE — 설정 이름에서 추론]
    - 불빛 수단이 횃불인지, Candlelight/Magelight 마법인지, 또는 둘 다인지 [UNKNOWN]
    - 둘 다 가능할 경우 priority [UNKNOWN]
    - 기존 왼손 아이템을 기억/복원하는지 [UNKNOWN]
    - 은신 중에도 발동하는지 [UNKNOWN]
- **Suppression condition:** 전투 중 억제 옵션이 존재 [EVIDENCE: SETTINGS — `dont_show_in_combat_makelight: true`, `recharge_weapon_oooc: true`]. 정확한 적용 범위는 불명 [INFERENCE]
- **Prompt text/type:** "무기 충전하기", "장착하기", "시전하기" [EVIDENCE: TRANSLATIONS — 문자열 존재 확인] / Type: [UNKNOWN]
- **Input semantics:** [UNKNOWN]
- **Action performed:** [INFERENCE — 소울젬 소모 충전, 타겟 아이템/마법 직접 장착, 불빛 제공 등으로 추정]
- **State remembered before action:** [UNKNOWN — MakeLight에서 왼손 상태를 기억하는지 불명]
- **State restored after action:** [UNKNOWN]
- **Dependencies:** [UNKNOWN]
- **Skyrim engine information required:** [IMPLEMENTATION POLICY — CIGAR 구현 시 후보: 무기 인챈트 차지량(`ExtraEnchantment`), 인벤토리 스캔, 플레이어 조명도(`GetLightLevel`). SI가 이 API를 사용한다는 증거는 없음]
- **Event-driven 가능 여부:** Equip의 정확한 gate는 [UNKNOWN]. CIGAR 구현 시 `CrosshairRefEvent` 또는 `TESContainerChangedEvent`가 후보가 될 수 있음 [IMPLEMENTATION POLICY]. Recharge/MakeLight는 polling 기반 구현이 가능할 것으로 추정 [INFERENCE].
- **필요한 최소 polling 주기:** 1s 이하 [INFERENCE]
- **CIGAR에서 재사용 가능한 machinery:** `015-potion`의 인벤토리 스캔, `010-weapon-swap`의 왼손 복구 로직 [EVIDENCE: CIGAR]
- **SI에서 disable해야 하는 setting:** `ItemUse.enabled_recharge_weapon`, `enabled_makelight`, `enabled_equip_weapon`, `enabled_equip_armor`, `enabled_equip_spellbook` [EVIDENCE: SETTINGS]
- **Save persistence 필요 여부:** [UNKNOWN]
- **Known edge cases:** 아주라의 별 소모 규칙 [INFERENCE], 은신 플레이 시 강제 불빛 팝업 방해 [INFERENCE]

### IMPLEMENTATION CONTRACT (Acquired Gear Equip)

#### Evidence-backed behavior
공식 영상에서 아이템 습득 맥락의 Equip prompt가 확인되며, SI에는 무기와 방어구에 대한 독립 설정이 있다 [EVIDENCE: OFFICIAL VIDEO/DOC, SETTINGS].

#### CIGAR implementation policy
`TESContainerChangedEvent`에서 플레이어 인벤토리로 새로 들어온 playable `WEAP`/`ARMO`만 제안한다. 이벤트 싱크는 FormID만 복사하고 게임 스레드에서 재검증한다. 비전투 행동은 hold이며 15초 동안 유지하고, 전투 중이거나 이동 조작이 막힌 동안 숨긴다. 수락 시 `ActorEquipManager::EquipObject`를 사용한다 [IMPLEMENTATION POLICY].

#### SI ownership boundary
CIGAR는 `enabled_equip_weapon`과 `enabled_equip_armor`만 끈다. `enabled_recharge_weapon`, `enabled_equip_spellbook`, `enabled_makelight`는 아직 SI가 담당한다. Quest Item Equip은 `QuestActions` 전체의 다른 미흡수 기능과 겹치므로 별도 경계가 확보될 때까지 SI 소유로 둔다 [IMPLEMENTATION POLICY].

#### Runtime status
SE+AE+VR build와 deploy 검증 통과. 실제 습득·장착 테스트 대기 [CIGAR].

### IMPLEMENTATION CONTRACT (RechargeWeapon)

#### Evidence-backed behavior
없음. 번역 문자열 "무기 충전하기"의 존재와 설정 key `enabled_recharge_weapon`, `recharge_weapon_oooc`의 존재만 확인됨.

#### Parameter-supported hypothesis
`enabled_recharge_weapon` — 무기 충전 기능의 on/off [INFERENCE]. `recharge_weapon_oooc` — 비전투 시에만 발동 [INFERENCE].

#### Unknown
- 충전 임계값 (charge == 0에서만 발동? 일정 비율 이하?) [UNKNOWN]
- 소울젬 선택 규칙 [UNKNOWN]
- 대상 weapon의 hand (right? left? both?) [UNKNOWN]
- dual wield 처리 [UNKNOWN]
- `recharge_weapon_oooc`의 정확한 semantics [UNKNOWN]

#### CIGAR implementation policy
충전 임계값(예: charge < 10%), 소울젬 선택(예: smallest adequate), 대상 hand 등을 CIGAR가 독립 설계 가능 [IMPLEMENTATION POLICY]. `ExtraEnchantment` API로 충전량 확인, `SoulGem::GetSoulLevel`로 소울젬 선택 구현 가능.

#### Minimal experiment
SI에서 `enabled_recharge_weapon` 활성화 후: (1) 마법무기 충전량을 0으로 만든 후 프롬프트 확인, (2) 충전량이 50%일 때도 뜨는지 확인, (3) 여러 크기의 소울젬 소지 시 어떤 것이 소모되는지 확인.

### IMPLEMENTATION CONTRACT (MakeLight)

#### Evidence-backed behavior
없음. 설정 key들의 존재만 확인됨.

#### Parameter-supported hypothesis
`darkness_threshold: 14.0` — 조명도 기준일 가능성이 높음 [INFERENCE]. `time_till_makelight_prompt: 5.0` — 지연 시간일 가능성이 높음 [INFERENCE]. `dont_show_in_combat_makelight: true` — 전투 중 억제일 가능성이 높음 [INFERENCE].

이 중 어느 것도 설정 이름만으로는 runtime semantics가 확정되지 않음. `darkness_threshold`가 `GetLightLevel()` 반환값과 직접 비교되는지, 자체 스케일을 사용하는지 불명 [UNKNOWN].

#### Unknown
- 불빛 수단: 횃불, Candlelight/Magelight, 또는 둘 다 [UNKNOWN]
- 둘 다 가능할 경우 priority [UNKNOWN]
- 기존 왼손 아이템의 기억/복원 여부 [UNKNOWN]
- 은신 중 발동 여부 [UNKNOWN]
- `darkness_threshold`와 엔진 `GetLightLevel()` 반환값의 관계 [UNKNOWN]

#### CIGAR implementation policy
`GetLightLevel()` + threshold 비교로 gate 구현, 횃불/Candlelight priority와 왼손 처리를 CIGAR 독립 설계 가능 [IMPLEMENTATION POLICY]. 예: 횃불 소지 시 횃불 우선, 없으면 Candlelight, 왼손 복구는 `010-weapon-swap` 패턴 재사용.

#### Minimal experiment
SI에서 `enabled_makelight` 활성화 후: (1) 횃불만 소지한 채 어두운 동굴에서 대기 → 프롬프트 확인, (2) 횃불 없이 Candlelight 마법만 알고 있을 때 프롬프트 확인, (3) 둘 다 있을 때 어떤 것이 제안되는지 확인, (4) 방패를 장착한 상태에서 수락 후 왼손 상태 확인.

### IMPLEMENTATION CONTRACT (Equip / Spellbook)

#### Evidence-backed behavior
공식 영상에서 아이템과 상호작용하거나 습득하는 맥락에서 Equip prompt가 나타나는 사례가 확인됨 [EVIDENCE: OFFICIAL VIDEO/DOC]. 정확한 gate가 Crosshair targeting인지 inventory acquisition event인지는 확인되지 않음 [UNKNOWN].

#### Unknown
- "장착하기"의 대상이 바닥에 있는 아이템(Crosshair)인지 인벤토리 습득 시점(UI Event)인지 [UNKNOWN — 구현을 막음. 엔진 후킹 대상이 완전히 다름]
- 장착 시 왼손/오른손 결정 규칙 [UNKNOWN]

#### Minimal experiment
바닥에 검과 마법책을 버리고 크로스헤어로 바라볼 때 프롬프트가 뜨는지, 'E'로 습득하는 순간 뜨는지 확인.
