# CIGAR: 다음 인게임 시험 (넥서스 피드백)

작성 2026-09-27. 넥서스 댓글에서 나온 두 가지입니다. N1은 **수정 확인**, N2는 **재현**입니다.
배포본은 v3 작성자 빌드 3.0.0이고, N1 수정이 들어 있습니다(2026-09-27 배포).

- 로그: `%USERPROFILE%\OneDrive\Documents\My Games\Skyrim Special Edition\SKSE\CIGAR.log`
- 끝나면 게임을 끄고 **다시 켜지 말고** 알려 주세요. 다시 켜면 로그가 새로 만들어집니다.

## 준비 (두 시험 공통)

리버우드로 이동해서 마을 옆 강으로 걸어 들어갑니다(무기는 넣은 상태).
```
coc Riverwood
```

## N1. 목욕 중에는 용변 프롬프트가 뜨지 않아야 함 (넥서스 #4, 수정 확인)

2026-09-27 수정: 용변 모듈은 Bathing in Skyrim이 목욕 애니메이션을 돌리는 동안 프롬프트를 내지 않습니다
(`Bathe::Washing()`, BiS의 AnimationKeyword 효과 검사).

- **동작:**
  1. 강에 들어가기 전에 방광을 채웁니다.
     ```
     setpqv PNO_Config bladdercontent 600
     setpqv PNO_MainQuest bladder_lastlevel 5
     ```
  2. 물 밖에서 **소변** 프롬프트가 뜨는지 먼저 봅니다(준비가 됐다는 확인).
  3. 강에 들어가 CIGAR **탈의** → **목욕하기**를 누릅니다.
  4. 목욕 애니메이션이 도는 동안 소변 프롬프트가 보이는지 봅니다.
  5. 목욕이 끝난 뒤 소변 프롬프트가 다시 뜨는지 봅니다(방광은 그대로 차 있으므로).
- **기대 결과:**
  - 목욕하는 동안 **소변 프롬프트가 보이지 않습니다.**
  - 목욕이 끝나면 다시 뜹니다.
- **로그:**
  - `[Bathe] prompt event accepted` 이후 목욕이 끝날 때까지 `[Needs] offer` 줄이 **없어야** 합니다.
  - 그 구간의 `[Needs] gate ...` 줄에 `washing=true`가 찍힙니다.
  - 목욕 뒤 `washing=false`와 함께 `[Needs] offer`가 다시 나옵니다.

## N2. 목욕 뒤나 가만히 있을 때 주시하기가 안 되는지 (넥서스 #2)

- **동작 A (목욕 뒤):**
  1. N1의 목욕이 끝나면 옷을 입고 물 밖으로 나옵니다.
  2. 무기를 넣은 채 5초 동안 가만히 사람이나 먼 풍경을 봅니다.
  3. **주시하기**가 뜨면 누르고 있습니다.
- **동작 B (가만히 있을 때):**
  1. 3인칭에서 아무 키도 누르지 않고 **2분 넘게** 서 있습니다. 카메라가 캐릭터 주위를 저절로 돌기 시작하면
     그 상태에서 합니다.
  2. 주시하기를 누르고 있습니다.
- **예상:**
  - 누르는 동안 화면이 부드럽게 확대되고, 떼면 돌아옵니다.
  - 안 되면 어느 경우인지 말로 알려 주세요: 프롬프트가 아예 안 뜸 / 떠서 눌렀는데 확대가 안 됨.
- **로그:**
  - `[Observe] offer event=35 '주시하기...'`: 프롬프트가 떴다
  - `[Observe] observing <대상>: fov A -> B`: 확대를 시작했다
  - `observe ended (...)`: 뗐다
  - `WARN the world FOV is ...` 줄이 있으면 다른 모드가 카메라 FOV를 덮어쓰는 것입니다

## N3. 마네킹 회귀: Another Mannequin Script Fix 다시 켠 뒤 (2026-09-27)

USSEP 시험이 끝나 수정판 스크립트(슬롯 20칸, 속성 방식)가 다시 켜졌습니다. 같은 기능이 이 경로에서도 그대로 되는지,
그리고 어제 고친 60번 슬롯 교환이 이 경로에서도 되는지 봅니다. 새 게임에서 합니다(USSEP 시험 때 세이브는 마네킹
데이터 형식이 다릅니다).

- **준비:** FormID는 2026-09-27 현재 로드 오더에서 확인했습니다(TravelerSetEldenRing.esp 설치로 NirnsChosen 방어구가 FEB41 → FEB42로 밀린 뒤 다시 확인).
  ```
  coc XJKRiverFallCottage
  player.additem 00013952 1
  player.additem 00013951 1
  player.additem 00013953 1
  player.additem 00013954 1
  player.additem 44024D69 1
  player.additem 44024D6A 1
  player.additem 44024D6B 1
  player.additem 44024D6C 1
  player.additem FEB42810 1
  player.additem FEB42811 1
  player.additem FEB42812 1
  ```
  일식 마법사 세트: 로브, 장화, 장갑, 두건, 어깨·다리·팔 보호대(다리 보호대 `FEB42811`이 60번 슬롯).
- **동작:**
  1. 강철 미늘 세트(투구 포함)를 입고 마네킹에 **의상 보관 (길게)**.
  2. 일식 마법사 세트 7부위를 입고 같은 마네킹에 **의상 교환 (길게)**.
  3. 한 번 더 **의상 교환 (길게)**(원래대로 돌아오는지).
- **기대 결과:** 세 번 모두 막히지 않고 옷이 오갑니다. 투구는 벗은 채(보관 상태)로 받습니다.
- **로그 (`[MannequinSwap]`):**
  - 판정 줄에 `slots=properties N/20`: 수정판 스크립트 경로라는 뜻
  - 각 교환마다 `swap done: ... slots properties N/20, 0 problems`
  - `kept item`이나 `WARN` 줄이 없어야 합니다

## N4. 마검사 모드 (The Wizard Warrior 연동, 2026-09-27)

전제: The Wizard Warrior 5.0.1을 2026-09-27에 설치했습니다(`##Magic`, 플러그인 인덱스 0x89, IaM 케이스 037).
CIGAR는 이 모듈이 들어간 빌드로 배포돼 있고 verify_deploy가 WW 스크립트 이름까지 확인했습니다. 플러그인이
추가됐으므로 **새 게임**에서 합니다. 시작 직후 "The Wizard Warrior Initialized" 알림이 뜨면 WW가 준비된 것입니다.

- **준비:** 철 검(현재 로드 오더에서 확인).
  ```
  player.additem 00012EB7 1
  ```
- **동작과 기대 결과:**
  1. 철 검을 장착하고 **무기를 꺼냅니다** → **마검사 모드** 프롬프트가 뜹니다(WW가 꺼져 있을 때).
  2. 누릅니다 → WW가 켜집니다(켜짐 소리와 빛 효과). 휘두르면 기본 그룹 1의 화염구·얼음 가시가 나갑니다.
     켜진 동안에는 무기를 넣었다 다시 꺼내도 프롬프트가 **뜨지 않습니다**.
  3. X로 WW를 끄고 무기를 다시 꺼냅니다 → 프롬프트가 다시 뜹니다.
  4. 프롬프트를 **두 번 눌러 거절**합니다 → 무기를 들고 있는 동안 다시 뜨지 않습니다. 무기를 넣었다 꺼내면
     다시 뜹니다.
- **로그 (`[WizardWarrior]`):**
  - `ready: quest ...`: WW를 찾았다
  - `gate drawn=true on=false allowSwitch=true ...` 다음 `offer event=41`: 프롬프트 표시
  - 누른 뒤 `ToggleAbility queued=true`, 2초 안에 `on: QK_SpellToggle = 1`
  - `WARN`이 있으면 켜기 실패입니다
  - 거절하면 `마검사 모드 dismissed`, 켜진 동안은 판정 줄에 `on=true`

## N5. 마검사 해제 (전투 밖에서 무기를 넣을 때, 2026-09-27)

N4와 같은 게임에서 이어서 해도 됩니다. CIGAR는 이 기능이 들어간 빌드로 배포돼 있습니다.

- **동작과 기대 결과:**
  1. 무기를 꺼내 **마검사 모드**로 WW를 켠 뒤, 전투가 아닐 때 **무기를 넣습니다** → **마검사 해제** 프롬프트가
     뜹니다.
  2. 누릅니다 → WW가 꺼집니다(끄는 소리, 빛 효과 사라짐). 무기를 넣은 채로 있어도 프롬프트는 다시 뜨지 않습니다.
  3. 다시 켜고 무기를 넣은 뒤, 해제 프롬프트를 **두 번 눌러 거절**합니다 → 무기를 넣은 동안 다시 뜨지 않습니다.
     무기를 꺼냈다가 다시 넣으면 다시 뜹니다.
  4. 전투 중에 무기를 넣으면 **뜨지 않아야** 합니다. 진흙게를 불러 싸우는 중에 무기를 넣어 봅니다.
     ```
     player.placeatme 000E4010
     ```
- **로그 (`[WizardWarrior]`):**
  - `offer event=42 '마검사 해제'`: 해제 프롬프트 표시
  - 누른 뒤 `ToggleAbility (turn off) queued=true`, 2초 안에 `off: QK_SpellToggle = 0`
  - `WARN ... still 1`이 있으면 끄기 실패입니다(알림도 뜸)
  - 거절하면 `마검사 해제 dismissed`
  - 전투 중 판정 줄은 `combat=true`이고, 그때 offer event=42가 없어야 합니다
- **참고:** WW 설정에서 무기 마법부여를 켜 두었다면, 마법부여 효과는 꺼진 뒤에도 자기 지속시간까지 남을 수
  있습니다. WW가 끄기에서 마법부여를 일부러 남기는 동작입니다(WW 4.2.6 변경 기록).

## N6. 프롬프트 위치 탐색 (3인칭, 2026-09-27)

탐색 빌드입니다. 3인칭에서 CIGAR 프롬프트를 머리 오른쪽 대신 **캐릭터가 바라보는 방향 앞쪽**에 그립니다.
1인칭은 지금과 같습니다(플레이어에 붙음). 새 게임은 필요 없습니다. 세이브에 보이지 않는 마커가 하나 생깁니다
(탐색 단계에서 사용자 허용).

- **동작:**
  1. 3인칭에서 프롬프트가 뜨게 합니다. 예: 무기를 넣고 가만히 바닥을 보면 앉기·눕기, 무기를 꺼내면 독 바르기나
     마검사 모드.
  2. 기본값은 **전방 40**입니다. CIGAR 제어판의 **"프롬프트 위치 탐색 (3인칭)"**에서 **플레이어(기존) / 전방
     25 / 전방 40 / 전방 60**을 바꿔 가며 봅니다. 바꾸면 떠 있던 프롬프트가 한 번 사라졌다가 새 위치에 다시
     뜹니다.
  3. 카메라를 캐릭터 뒤, 옆, **앞(얼굴 쪽)**으로 돌려 가며 봅니다. 앞에서 보면 마커가 카메라와 얼굴 사이에
     있어서, 얼굴을 가리는지가 확인할 점입니다.
  4. 1인칭으로 바꿨다가 다시 3인칭으로 돌아옵니다(1인칭은 기존 위치).
- **기대 결과:** 프롬프트가 머리를 가리지 않고 얼굴 앞쪽에 보입니다. 어느 후보가 가장 좋은지, 앞에서 볼 때 얼굴을
  가리는지 말로 알려 주세요.
- **로그 (`[PromptAnchor]`):**
  - `marker XXXXXXXX placed (disabled, position written directly): bounds ...`: 마커가 준비됐습니다.
    `WARN ... no bounds`가 있으면 활성 마커로 전환된 것이고, 둘 다 안 되면 플레이어에 붙습니다.
  - `prompts attach to the marker ... (third person)` / `... to the player (first person)`: 부착 전환
  - `distance set to N`: 후보 변경
  - 5초마다 `sample: d=... head (...) marker (...) ... camera ... at N units`: 실제 좌표

## N7. 프롬프트 오른쪽 오프셋 고르기 (3인칭, 2026-09-27, **합격**)

N6 합격 후 본 구현입니다. 전방 거리는 40으로 고정했습니다(사용자 선택). `4a9801e` 빌드로 시험했습니다.

- **결과 (2026-09-27): N7-1~5 모두 합격.** 빠른 이동, 문, 말 타기 뒤에도 프롬프트가 따라왔습니다.
- **사용자 결정:** 값 하나로 고정하지 않고 플레이어가 고르게 둡니다. 후보 0 / 10 / 15 / 20 / 30, 기본 15.
  제어판 `5. 세부 설정` → 프롬프트 위치 → **오른쪽 간격 (3인칭)**으로 옮겼습니다(모든 빌드). PromptAnchor는
  완성입니다. 기록은 `docs/007` "Finished after N7".

- **동작:**
  1. 3인칭에서 프롬프트가 뜨게 합니다(무기를 넣고 바닥을 보면 앉기·눕기, 무기를 꺼내면 독 바르기나 마검사 모드).
  2. CIGAR 제어판 **"프롬프트 오른쪽 오프셋 (N7, 3인칭)"**에서 **0 / 10 / 15(기본) / 20 / 30**을 바꿔 가며 봅니다.
     바꾸면 떠 있던 프롬프트는 다음 순간 새 자리에서 보입니다.
  3. 카메라를 뒤, 옆, 앞으로 돌려 봅니다.
  4. 안정성: 빠른 이동 한 번, 실내↔실외 문 한 번, 말 타기(안장 얹은 말 `EncHorseSaddledBrown`을 불러 탐, FormID 확인 완료). 이동한 뒤에도 프롬프트가 캐릭터 앞에
     따라오는지 봅니다.
     ```
     player.placeatme 00023AB2
     ```
- **기대 결과:** 프롬프트가 얼굴을 가리지 않고 앞쪽 약간 오른쪽에 보입니다. **가장 좋은 값 하나를 골라 알려
  주세요.** 그 값으로 고정하고 제어판 항목은 없앱니다.
- **로그 (`[PromptAnchor]`):**
  - `marker XXXXXXXX reused|placed (load; disabled, ...)`: 준비
  - `prompts attach to the marker ... (third person)` / `the player (first person)`: 부착 전환
  - `right offset set to N`: 후보 변경
  - `WARN`이나 `marker is gone`이 없어야 합니다(있으면 복구 줄이 이어짐)

## P1-P5. NPC 밀기 탐침 (docs/038, 2026-09-28, 작성자 빌드 `cc0b6e2`)

로그만 남기는 탐침입니다. 프롬프트는 뜨지 않습니다. 판정은 `CIGAR.log`의 `[PushProbe] RESULT` 줄로 합니다.
끝나면 게임을 끄고 다시 켜지 마세요(로그가 새로 만들어집니다).

- **P1. 준비:** 여관으로 가서 3인칭으로 바꿉니다. F1 → CIGAR → 5. 세부 설정의 "프롬프트 위치" 바로 아래에
  "탐침: NPC 밀기 (docs/038)"가 보이면 됩니다.
  ```
  coc WhiterunBanneredMare
  ```
- **P2. 걸어서 부딪히기:** 같은 칸의 "자연 부딪힘 관찰"을 켜고 메뉴를 닫습니다. 서 있는 NPC에게 걸어서 2~3초 밀고
  들어갑니다. 3초 기다렸다가 다른 NPC에게 한 번 더 합니다.
- **P3. 달려서 부딪히기:** 관찰을 켠 채로, 서 있는 NPC에게 달려서(질주) 부딪힙니다. 3초 기다렸다가 한 번 더 합니다.
- **P4. 행동 ActionBumpedInto:** NPC를 조준한 채 F1 → "행동 ActionBumpedInto" → 메뉴를 닫고 3초 기다립니다.
  NPC 정면에서 한 번, 옆에서 한 번 합니다.
- **P5. 방향 이벤트:** NPC를 조준하고 "앞", "왼쪽", "오른쪽", "뒤"를 한 번씩 누릅니다(누를 때마다 메뉴를 닫고 3초 대기).

로그에서 보는 것(판정은 Claude가 합니다):
- `natural small|big bump` 줄: 걷기와 질주에서 바닐라가 어떤 부딪힘 상태를 쓰는지
- `RESULT ... DISPLACED / NOT DISPLACED`와 최대 이동 거리: 실제로 비켜났는지
- `anim event` 줄: NPC가 어떤 반응 애니메이션을 탔는지
- `dialogue` 줄: 부딪힌 NPC가 대사를 했는지
- `PROBLEM: HOSTILE / COMBAT / CRIME`: 적대, 전투, 현상금이 생겼는지(없어야 함)

## G1-G7. 밀기 동작 후보 비교 (docs/038 0b단계, 작성자 빌드, 배포 후)

바닐라 클립 4개(A)와 EVG 비집기 2개(C)를 걸으면서 한 번씩 봅니다. 어느 것이 "사람 옆을 비집고 지나가는"
느낌인지는 보시는 분이 고릅니다. 로그는 동작이 걷는 중에 재생되고 멈췄는지만 확인합니다.

- **G1. 준비:** 여관으로 가서 3인칭, 무기는 넣은 상태로 둡니다. 서 있는 NPC 하나 옆을 지나갈 수 있는 자리를 잡습니다.
  ```
  coc WhiterunBanneredMare
  ```
- **G2~G7. 후보 하나씩:** F1 → CIGAR → 5. 세부 설정 "탐침: NPC 밀기" 칸의 "밀기 동작 후보"에서 버튼을 누르고 메뉴를 닫은 뒤,
  NPC 옆을 걸어서 지나갑니다. 걷기 시작하는 순간 동작이 나옵니다(20초 안에 걷지 않으면 취소됩니다).
  - G2: 3801 문 열기 · 왼팔
  - G3: 3802 방패 밀치기 · 왼팔 (0.3초로 짧음)
  - G4: 3803 받기 · 오른팔
  - G5: 3804 건네기 · 오른팔
  - G6: 3805 EVG 비집기 · 상체 (상체 전체가 비틀림)
  - G7: 3806 EVG 비집기 · 왼팔
- **고르기:** 가장 자연스러운 것 하나(없으면 "없음").

로그에서 보는 것(Claude가 판정): `[PushProbe] gesture ... start`의 `bGPMAInstalled true`, 변수 설정과 `OffsetGPMA accepted true`,
`RESULT gesture ... KEPT MOVING`(걷는 속도가 유지됐는지), `stop ... accepted true`와 두 변수가 0으로 돌아왔는지.

## G9-G14, U1. 3805 조합 시험과 유술 개선 (docs/038 0c단계, docs/012, 작성자 빌드, 새 게임)

버튼 위치는 5. 세부 설정의 "프롬프트 위치" 바로 아래 "탐침: NPC 밀기" 칸의 "밀기 동작 후보"입니다. 버튼을 누르고
메뉴를 닫은 뒤 걸으면 재생됩니다. 재생 중에 다른 버튼을 눌러도 이제 앞의 동작이 정리되고 넘어갑니다.

- **G9. 준비:** 여관으로 가서 3인칭, 무기는 넣은 상태. 서 있는 NPC 하나를 찾습니다.
  ```
  coc WhiterunBanneredMare
  ```
- **G10~G12. 3805 끊기:** "3805 상체 · 0.3초에 끊기", "0.5초", "0.8초"를 하나씩 누르고 NPC 옆을 걸어 지나갑니다.
  몸을 반쯤만 틀었다가 돌아오는 동작입니다.
- **G13. 간격별 자동, 정면:** "간격별 자동" 버튼을 누르고 메뉴를 닫은 뒤, 서 있는 NPC를 **정면으로** 향해 걸어갑니다.
  NPC가 앞 120 안에 들어오면 3805(상체 전체)가 나와야 합니다.
- **G14. 간격별 자동, 왼쪽·오른쪽:** 같은 버튼으로, NPC를 **내 왼쪽에** 두고 스쳐 지나갑니다(3806 왼팔이 나와야 함).
  한 번 더 누르고 NPC를 **오른쪽에** 두고 지나갑니다(오른쪽 전용 동작이 없어 3805가 대신 나옴).
- **고르기:** 3805 원래 길이 / 0.3초 / 0.5초 / 0.8초 / 간격별 자동 가운데 가장 자연스러운 것 하나(없으면 "없음").
- **U1. 유술:** 야외에서 도적을 불러 맨손으로 싸웁니다. 도적이 **막을 때** 유술 프롬프트가 뜨고, 도적이 **휘두르기 시작하면
  프롬프트가 바로 사라지는지** 봅니다. 프롬프트가 떠 있을 때 여러 번 눌러 봅니다.
  ```
  player.placeatme 0001BCD8
  ```

로그에서 보는 것(Claude가 판정):
- G: `gesture ... start` / `stop after ... (clip length)` / `RESULT gesture ... KEPT MOVING`, 자동은 `picked ...: auto: <이름> N ahead,
  ±N to the side, straight ahead | on the left | on the right`.
- U1: `[Jujutsu] gate target=- why=target-swinging`이 도적이 휘두를 때 찍히는지, 누른 시도의 `victim[... gAttack=false ...]`,
  거부(`finished (refused)`)가 r5의 7/7보다 줄었는지.

## D0-D8. 게임패드 D-pad와 마우스 버튼 (docs/016 B안, Xbox 패드, 2026-09-28)

Xbox 패드를 연결하고 시작합니다. 기존 세이브로도 됩니다. CIGAR 메뉴는 키보드와 마우스로 엽니다.
설정은 **4. 키** 페이지의 "게임패드 버튼"과 "N번째 프롬프트 키"에 있습니다.
손으로 적을 것은 없습니다. 아래 동작만 하면 결과는 로그로 판정합니다.
중간에 CTD가 나면 거기서 멈추고 알려 주세요. 남은 항목은 모두 끝난 것으로 칩니다.

- **준비(한 번):** 여관에서 무기 두 개와 독을 받습니다.
  ```
  coc WhiterunBanneredMare
  player.additem 00012EB7 1
  player.additem 0003A5A4 5
  ```
  인벤토리에서 **철 검**(00012EB7)을 즐겨찾기로 표시하고(F), 즐겨찾기 메뉴(Q)에서 철 검 위에 커서를 둔 채 키보드
  **1**을 눌러 단축키 1로 지정합니다. 이 모드리스트에서 단축키 1은 D-pad 왼쪽입니다.
- **D0. 기본값 그대로(게임패드 버튼 = "SkyPrompt 설정 따름"):** 철 단검을 받습니다.
  ```
  player.additem 0001397E 1
  ```
  "장착(홀드): 철 단검" 프롬프트가 뜨면 패드로 표시된 버튼을 누르고 있습니다. 이 모드리스트는 1번이 **B**입니다.
  단검이 손에 들리면 됩니다.
- **D1. D-pad로 바꾸기:** 4. 키에서 "게임패드 버튼"을 **D-pad**로 바꿉니다.
- **D2. 1번 = 위:** 메뉴를 닫고 단검을 **뽑습니다**. "독 바르기(홀드)" 프롬프트가 D-pad 위 아이콘으로 뜹니다.
  1. D-pad 위를 **짧게 한 번** 누릅니다. 즐겨찾기가 열리지 않아야 합니다(홀드 프롬프트라 짧게 누르면 아무 일도 없음).
  2. D-pad 위를 **누르고 있습니다.** 독이 발라지고 프롬프트가 사라집니다.
  3. 프롬프트가 없는 상태에서 D-pad 위를 누릅니다. 이번에는 즐겨찾기가 열려야 합니다. 닫습니다.
- **D3. 키보드와 패드 번갈아:** 독을 한 번 더 받습니다(`player.additem 0003A5A4 1`). 단검을 넣었다가 다시 뽑아 "독 바르기"를
  띄웁니다. 키보드를 아무 키나 한 번 누르면 키보드 아이콘(1)으로, 패드를 만지면 D-pad 아이콘으로 바뀌는지 봅니다. 바르지는 않습니다.
- **D4. 2·3·4번 = 아래·왼쪽·오른쪽(전투):** 단검을 뽑고 "독 바르기"가 떠 있는 채로 도적을 부릅니다.
  ```
  player.placeatme 0003DE8A
  ```
  록온, 그래플(가까울 때), 유술(도적이 막을 때), 원거리·근접 무기 전환 같은 프롬프트가 겹쳐 뜹니다. **화면의 D-pad 아이콘대로**
  아래·왼쪽·오른쪽을 한 번씩 눌러 봅니다. 왼쪽을 누를 때 철 검(단축키 1)이 **손에 들리지 않아야** 합니다.
  오른쪽(4번)은 프롬프트가 네 개 떴을 때만 나옵니다. 안 나오면 넘어갑니다.
- **D5. 프롬프트가 없을 때 원래대로:** 싸움이 끝나고 무기를 넣은 뒤, 아무 프롬프트도 없을 때 D-pad **왼쪽**을 누릅니다.
  철 검이 손에 들려야 합니다.
- **D6. 페이지 넘김(있을 때만):** 다른 모드의 SkyPrompt 프롬프트가 CIGAR 프롬프트와 같이 뜨는 일이 있으면, 3·4번이 없을 때
  D-pad 왼쪽·오른쪽으로 페이지가 넘어가는지 봅니다. 그런 상황이 없으면 넘어갑니다.
- **D7. 마우스 옆 버튼(홀드):** 4. 키에서 1번째 프롬프트 키를 **마우스 4**(옆 버튼)로 바꿉니다. 무기를 넣고 가만히 서서 바닥을
  내려다봅니다. "앉기"가 뜨면 옆 버튼으로 앉습니다. "시간 보내기(홀드)"를 옆 버튼으로 **5초 넘게 누르고 있다가** 뗍니다.
  누르는 동안 시간이 계속 흐르고, 떼면 멈춰야 합니다.
- **D8. 되돌리기:** "기본값 (1, 2, 3, 4)" 버튼을 누릅니다. 게임패드 버튼은 원하는 쪽으로 두면 됩니다.

로그에서 보는 것(Claude가 판정):
- 시작: `settings: gamepad buttons ...`, `equip watch registered`.
- D0: 장착 프롬프트의 `offer ... key=2 sent=true`에 `pad=`가 **없음**, `[ItemEquip] ... accepted`, `player equip: on 철 단검`.
- D1: `control panel: gamepad buttons set to D-pad`, 그 뒤 offer 줄에 `pad=266`처럼 슬롯 순서대로 266·267·268·269.
- D2: `[Poison] offer ... slot=1 ... pad=266`, 짧게 누른 동안 `menu 'FavoritesMenu' opened`가 **없음**, 누르고 있은 뒤
  `[Poison] prompt event accepted`와 `applied`, 프롬프트가 없어진 뒤에는 `menu 'FavoritesMenu' opened`가 **있음**.
- D4: 각 프롬프트의 `slot=N ... pad=26x`와 이어지는 `prompt event accepted`, 그리고 그 사이에 `menu 'FavoritesMenu' opened`와
  `player equip: on 철 검`이 **없음**.
- D5: `player equip: on 철 검`.
- D7: `offer ... key=259`, `pass time held (another device; ends on key up)`, `pass time stopped (...): held N s`에서 N이 누른
  시간과 맞음(0.3초에서 끊기지 않음).

## r7 (b26e031, 2026-09-28): r6 CTD 수정 확인, 이어서 D2-D8, K1, U1

r6은 D2에서 CTD로 멈췄습니다. 원인은 SkyPrompt 2.4.0의 그리기 중 경쟁이었고 D-pad 코드와는 무관했습니다(docs/039).
이 빌드는 CIGAR의 SkyPrompt 호출을 모두 SkyPrompt가 그리는 스레드로 옮겼습니다. 순서대로 진행하되, 손으로 적을 것은 없습니다.
CTD가 나면 거기서 멈추고 알려 주세요. 남은 항목은 모두 끝난 것으로 칩니다.

- **T0. 스레드 확인(할 일 없음):** 세이브를 불러오거나 새 게임을 시작하면 로그가 알아서 남습니다.
- **T1. CTD 재현 시도:** 여관에서 무기를 넣은 채 NPC를 바라보고 5초 동안 서 있어 "주시하기"와 "탈의하기" 같은 프롬프트를
  띄웁니다. 그 상태에서 무기 뽑기·넣기를 **10번 빠르게 반복**합니다(키보드 5번, 패드 5번). r6에서 CTD가 난 동작입니다.
- **D2~D8:** 위 "D0-D8" 절의 D2부터 D8까지 그대로 합니다(도적은 0003DE8A로 바뀌었습니다).
- **K1. 세이브를 불러온 직후의 유술(아무도 죽기 전):**
  1. 도적이 없는 곳에서 콘솔로 저장합니다: `save CIGAR_K1`
  2. 바로 불러옵니다: `load CIGAR_K1`
  3. 불러온 뒤 **아무것도 죽이지 말고** 도적을 불러 맨손으로 싸웁니다. 도적이 막을 때 유술 프롬프트를 몇 번 눌러 봅니다.
     ```
     player.placeatme 0003DE8A
     ```
  4. 그다음 그 도적을 아무 방법으로나 쓰러뜨립니다(K1 끝, U1로 이어짐).
- **U1. 유술(첫 죽음 이후):** 도적을 한 명 더 불러 맨손으로 싸웁니다. 막을 때 유술 프롬프트가 뜨고, 휘두르기 시작하면
  바로 사라지는지 봅니다. 떠 있을 때 여러 번 눌러 봅니다.
  ```
  player.placeatme 0003DE8A
  ```

로그에서 보는 것(Claude가 판정):
- T0: `prompt queue: Present hooked (chained: true)`, `prompt queue: first Present call on thread N`,
  `prompt queue: ticks run on thread M, Present on thread N`에서 M이 N과 다르면 전제가 확정됩니다.
  `a tick ran while the render thread was inside Present`가 찍히면 둘이 실제로 겹친다는 직접 증거입니다.
  `WARN prompt queue: no Present call`이나 `ERROR prompt queue: no call at the Present site`가 **없어야** 합니다.
- T1: CTD 없음. 뽑기·넣기마다 Observe·Dress의 offer 줄이 이어집니다.
- D2~D8: 위 절의 기준 그대로입니다.
- K1: 불러온 뒤 첫 죽음 전의 누름에 `known issue: no actor has died since the load`가 찍히면, 그 창이 세이브를 불러와도
  생긴다는 뜻입니다(r5는 새 게임만 봤습니다). `pair started`가 나오면 창이 없는 것입니다.
- U1: 도적이 휘두를 때 `[Jujutsu] gate target=- why=target-swinging`, 누른 시도의 `pair started ...` 대 `finished (refused)`.
  `pair started`가 나오면 21:06 Pandora 출력(SBF 2.0)에서도 유술이 도는 것이므로 `verify_deploy --accept-behaviour`로
  기준을 옮깁니다.

## r8 탐침: 접촉 후 비집고 지나가기 (가)(나) 비교 (작성자 빌드 7822eab, 2026-09-29)

두 버튼은 5. 세부 설정의 "탐침: NPC 밀기" 칸 맨 아래에 있습니다: **"비집기 (가) 내 캡슐 축소"**,
**"비집기 (나) 그 NPC 충돌 끄기"**. 버튼을 누르고 메뉴를 닫은 뒤, 30초 안에 NPC에게 몸을 대고 계속 밀고 걸어가면 적용됩니다.
지나가서 떨어지면(또는 3초가 지나면) 원래대로 돌아갑니다. 비집는 동작(간격별 자동)도 같이 나옵니다. 손으로 적을 것은 없습니다.
CTD가 나면 거기서 멈추고 알려 주세요. 남은 항목은 모두 끝난 것으로 칩니다.

- **준비:** 여관, 3인칭, 무기 넣은 상태. 시험마다 농부를 새로 부릅니다(부르면 내 자리에 나타납니다).
  ```
  coc WhiterunBanneredMare
  player.placeatme 001034E4
  ```
- **Q1. 넓은 곳 (가):** 1층 가운데에서 농부를 부르고 뒤로 서너 걸음 물러난 뒤, (가)를 누르고 농부 정면으로 걸어가 밉니다.
- **Q2. 넓은 곳 (나):** 같은 방법으로 (나).
- **Q3. 좁은 곳 (가):** 문틀(여관 방문이나 출입문 안쪽) 한가운데에 서서 농부를 부릅니다. 문 반대편으로 물러났다가 (가)를 누르고,
  문틀에 선 농부를 밀고 지나갑니다.
- **Q4. 좁은 곳 (나):** Q3과 같은 방법으로 (나).
- **Q5. 벽과 NPC 사이 (가)(나) 각각:** 벽에 등을 붙이고 농부를 부른 뒤 옆으로 비켜났다가, 벽과 농부 사이로 밀고 지나갑니다.
- **고르기(있으면):** (가)와 (나) 중 더 자연스러운 쪽. 없으면 "없음".

로그에서 보는 것(Claude가 판정):
- 시작: `squeeze (가|나) start: touching <이름> ... via <신호>; <interior|exterior> cell ...; nearest door ...; <바꾼 값 전 -> 후>`.
  신호가 `the controller's bumped character`이면 엔진의 접촉 값이 잡힌 것이고, `no controller bump reported`면 거리로 잡은 것입니다.
- 진행: `squeeze +N s: 거리, 지나간 정도, 속도`.
- 복원: `squeeze ... undo (clear of the NPC | time limit 3 s): ...; 값 복원`.
- 결과: `RESULT squeeze ... PASSED | DID NOT PASS`, 복원 뒤 `POPPED | no pop`(예상보다 40 넘게 튀었는지), `N s stuck while moving`(끼임),
  `the NPC moved N (z ±N)`. (나)에서 z가 크게 음수면 NPC가 바닥으로 꺼진 것입니다.
- (가)의 `(the NPC's controller uses this shape too)`가 보이면 캡슐이 NPC와 공유돼서 그 NPC도 같이 작아진 것입니다.

## r8b 판정 (2026-09-29, Claude)

- Q1~Q5: `docs/038` "Stage 0d results". (가)는 한 번도 적용되지 않았습니다(플레이어 충돌 모양이 캡슐이 아님). 그래서 탐침에서 뺐습니다.
  (나)는 7번 중 6번 통과했고, 튐과 바닥 꺼짐은 없었습니다. 사용자가 (나)를 골랐습니다.
- BaboKey: 연결됨(`Babo monitor=C47E22B8 ... ready`).
- 개인판 항목은 CIGAR-Personal `HANDOFF.md`에 있습니다.

## r9: 링 프롬프트와 거절 유지 (docs/042, 2026-09-29)

비전투 프롬프트는 이제 전부 길게 눌러 링을 채워야 실행됩니다. 두 번 톡톡 눌러 거절하면 상황이 바뀔 때까지 다시 나오지 않습니다.
손으로 적을 것은 없습니다. CTD가 나면 거기서 멈추고 알려 주세요.

- **R1. 목욕 링과 거절:** 리버우드 강에 무기를 넣고 들어가 탈의까지 합니다(탈의도 이제 길게 누릅니다).
  ```
  coc Riverwood
  ```
  1. 목욕하기 키를 **한 번 짧게** 누릅니다 → 아무 일도 없어야 합니다.
  2. **두 번 톡톡** 누릅니다 → 목욕하기가 사라지고, 물속에 있는 동안 다시 나오지 않아야 합니다.
  3. 물 밖으로 나갔다가 다시 들어갑니다 → 다시 나와야 합니다. 이번에는 **길게** 눌러 목욕합니다.
- **R2. 앉기 거절:** 평지에 서서 바닥을 내려다봅니다. 앉기·눕기가 뜨면 앉기 키를 **두 번 톡톡** 누릅니다.
  그 자리에서 조금 움직였다 멈춰도 앉기가 다시 나오지 않아야 합니다. 몇 걸음(약 4미터 이상) 떨어진 곳에서 멈추면 다시 나와야 합니다.

로그에서 보는 것(Claude가 판정):
- R1: `event=1 declined: hidden while its condition holds`, 물 밖에서 1초 뒤 `event=1 no longer declined`. 짧게 한 번 누를 때는 `accepted`가 없어야 합니다.
- R2: `event=20 declined: hidden until the player is 300 units away or in another cell`, 멀리 간 뒤 `event=20 no longer declined`.

## r10: 비켜 지나가기 (docs/038 Stage 1, 2026-09-29)

무기를 넣고 3인칭으로 합니다. 손으로 적을 것은 없습니다. CTD가 나면 거기서 멈추고 알려 주세요.
준비: `coc WhiterunBanneredMare`. 시험마다 농부를 새로 부릅니다(`player.placeatme 001034E4`).

- **S1. 넓은 곳:** 농부 정면으로 걸어가 몸을 댑니다. "비켜 지나가기 (누르고 있기)"가 뜨면 누른 채로 계속 걸어 지나갑니다.
- **S2. 문틀:** 문틀 가운데 선 농부를 같은 방법으로 지나갑니다.
- **S3. 벽과 NPC 사이, 옆으로 스치기:** 벽에 붙은 농부의 옆구리를 스치듯 지나갑니다.
- **S4. 두 사람 연속:** 농부를 둘 불러 일렬로 세우고, 한 번 누른 채로 둘 다 지나갑니다.
- **S5. 짧게 한 번:** 프롬프트가 떴을 때 짧게 한 번만 누릅니다. 아무 일도 없어야 합니다.
- **S6. 거절:** 두 번 톡톡 누릅니다. 막힌 채로 있는 동안 다시 나오지 않아야 합니다.

로그에서 보는 것(Claude가 판정):
- 로드 전: `[Squeeze] clip ready: gesture submod ... (31152 bytes, 86 annotations cleared; written)`. 불러온 뒤: `[Squeeze] ready: ...`, `Offset Movement Animation found: on`.
- S1~S4: `ring filled`, `start: <이름> ... (no collision true) ... bump NPC_Bump... accepted true`, `gesture 3810 (upper body|left arm, ...)`,
  `RESULT squeeze on <이름> (past the NPC): PASSED ... read back <원래 값>`. `STILL OVERLAPPING`, `WARN`이 없어야 합니다.
- S4: 두 사람 모두 `start:`가 있고, 두 번째 앞에 `(the next NPC)` 또는 `(past the NPC)`로 첫 사람이 풀립니다.
- S5: `released before the ring filled: nothing changed`, `start:` 없음.
- S6: `event=43 declined: hidden while its condition holds`.
- 프롬프트가 안 뜨면 gate 줄의 `npc=- (이름: 이유)`와 `trace near=... box=... refuse=... speed=... contact=... blocked=...` 줄이 이유를 말합니다.
  `python tools/replay_squeeze_gate.py`로 게임 없이 다시 판정할 수 있습니다(r9 수정, docs/038 "r9").
- 장면·가구 속 NPC도 지나갑니다. 그때 start 줄의 bump는 `skipped (in a scene|in furniture)`입니다.

## r10-P. 물약: 전투 밖은 링, 전투 중은 한 번 누르기 (D18, 2026-09-29)

- **준비:** `player.additem 0003EADE 5`(MAG_RestoreHealth02, 물약 - 체력 회복 하급; 현재 로드 오더에서 Apothecary.esp가 덮어씀), `player.damageav health 60`을 체력이 50% 아래가 될 때까지 반복. 전투는 `player.placeatme 0003DE8A`(EncBandit01WarriorNordM, 산적)로 만듭니다.
- **P1 전투 밖:** "마시기 (길게): ..."가 뜨면 두 번 톡톡 → 사라지고, 체력이 낮은 동안 다시 나오지 않아야 합니다. `player.restoreav health 500`으로 채웠다가 다시 깎으면 다시 뜹니다. 그때 길게 눌러 마십니다. 다시 체력을 깎고 산적을 부른 뒤 전투 중에는 "마시기: ..."를 한 번 눌러 마십니다.
- **로그:** 전투 밖 `offer event=17 '마시기 (길게): 물약 - 체력 회복 하급 (N%)'`, `event=17 declined: hidden while its condition holds`, 이후 `event=17 no longer declined`; 전투 중 `offer event=17 '마시기: ...'`와 `prompt event accepted (0) event=17`.

## r10 로그 확인 (코드 검토 수정분, docs/043, Claude가 평소 플레이 로그로 판정)

별도 행동은 없습니다. 평소처럼 플레이한 로그에서 다음을 봅니다.
- 한 번 누른 프롬프트가 2초 뒤 같은 키로 되살아나지 않는지: 같은 event의 `offer` 줄이 수락 직후 조건 변화 없이 다시 나오지 않아야 합니다.
- Rest gate에 `beast=false`가 찍힙니다(짐승 형태면 true, 앉기·눕기 없음).
- 유술이 중간에 끊긴 경우에만: `pair cut short while the victim is still in its kill move`, 이어서 `the cut-short pair ended: ... the victim stays alive`.
- `WARN`, `notice (log only`, `.bad`가 새로 생기지 않았는지.

## r10 판정 (2026-09-30, Claude)

- 비켜 지나가기 4회 모두 PASSED. 그러나 세이디아(z -479)와 미카엘(z -488)이 바닥으로 꺼졌습니다. 원인: 충돌을 끈 NPC가 움직이면(부딪힘 동작, 걷기) 바닥도 통과합니다.
  고침: 부딪힘 동작 없음, 걷는 NPC는 제외, 8 이상 떨어지면 즉시 원위치와 충돌 복원(`WARN ... dropped`), 시작할 때 옷 스치는 소리(`cloth sound true`).
- 다음 확인(r11): 여관에서 서 있는 사람을 지나갈 때 NPC가 꺼지지 않는지, 소리가 나는지. 걷는 사람에게는 프롬프트가 뜨지 않는 것이 정상입니다(trace의 `refuse=walking`).

## r11: 비켜 지나가기 (D)와 (A) 비교 (D20, 작성자 빌드, 2026-09-30)

한 번의 실행에서 세 방식을 비교합니다. 전환은 제어판 5. 세부 설정의 탐침 칸 맨 아래 "비켜 지나가기 방식"
라디오 버튼입니다: (D) NPC 충돌 끄기, (A) 충돌 그룹 공유, (E) 걸음마다 부딪힌 몸만. 손으로 적을 것은 없습니다. CTD가 나면 거기서 멈추고 알려 주세요.

- **준비:** 무기를 넣고 3인칭으로 합니다.
  ```
  coc WhiterunBanneredMare
  player.placeatme 001034E4
  ```
  (WEFarmerMale, 농부. 부르면 내 자리에 나타납니다.)
- **D1 ((D)):** 서 있는 농부를 비켜 지나가기로 한 번 지나갑니다. 걸어 다니는 여관 사람에게는 프롬프트가 뜨지 않는 것이 정상입니다.
- **A1 ((A)):** 농부를 다시 불러 서 있는 농부를 지나갑니다.
- **A2 ((A)):** 걸어 다니는 여관 사람(세이디아, 훌다 등)을 지나갑니다.
- **A3 ((A)):** 벽과 사람 사이, 옆구리를 스치듯 지나갑니다.

- **E1~E3 ((E)):** A1~A3과 같은 세 가지(서 있는 농부, 걸어 다니는 여관 사람, 벽과 사람 사이)를 (E)로 한 번씩.

로그에서 보는 것(Claude가 판정):
- 시작 직후: `[Squeeze] movement hook installed (chained: true)`. (E)를 쓸 수 있는지는 이 줄로 정해집니다.
- `[Squeeze] way: (A)|(D)|(E) ...`로 전환이 찍힙니다.
- (E): `start: ... (E) per-step no-collision ... (hook installed)`, `RESULT squeeze (E) ... applied in N movement steps`(N이 0보다 커야 함).
- `start: ... standing|walking; (A) NPC group N ...; player controller proxy; phantom XXXXXXXX -> YYYYYYYY ...; cloth sound true`
- `RESULT squeeze (A)|(D) on ...: PASSED`, 그리고 `NPC ... (z ±N); player z ±N; stuck N s while moving; self-bumps N`, `player phantom ... -> ...`(그룹 복원).
- `after the undo: ... (no pop|POPPED)`.
- (A)의 판정 기준: 통과, NPC와 플레이어의 z가 둘 다 ±10 안, self-bumps 0, POPPED 없음, `WARN` 없음.

## r11 판정 (2026-09-30, Claude)

- (D) 실패: 서 있어도 NPC가 13~21 가라앉음. 제거. (E) 실패: 훅은 걸음마다 작동(최대 26번)했지만 플레이어가 막힘. 제거.
- (A) 채택. 로그에서 버그 발견: 플레이어 충돌 그룹이 지나간 뒤 원래대로 안 돌아옴(같은 필터를 두 번 저장·복원). 고쳤습니다.
- 소리: 재생은 됐지만 너무 작은 물리 충돌음이었습니다. 옷 부스럭 소리(ITMClothingUpSD)로 바꿨습니다.
- 책 읽기 "R … 읽기"는 CIGAR가 아니라 Read It Now(넥서스 168337)의 SkyPrompt 프롬프트입니다(그 모드의 키와 위치).

## r12: 비켜 지나가기 (A) 정식, 누르는 즉시, NPC 밀어내기 탐침 (2026-09-30)

준비: 무기를 넣고 3인칭. `coc WhiterunBanneredMare`, `player.placeatme 001034E4`(농부). 손으로 적을 것은 없습니다. CTD가 나면 거기서 멈추고 알려 주세요.

- **S1. 누르는 즉시:** 농부에게 막혀 프롬프트가 뜨면 누르는 순간 지나가기 시작하는지(링 없음), 옷 부스럭 소리가 들리는지.
- **S2. 걷는 사람:** 걸어 다니는 여관 사람을 지나갑니다.
- **N1~N3. 밀어내기:** 제어판 5. 세부 설정 탐침 칸 "비켜 지나가기 뒤 NPC 밀어내기"를 옆으로 8 → 옆으로 15 → 부딪힘 동작으로 바꿔 가며 농부를 한 번씩 지나갑니다. 어느 쪽이 자연스러운지 고르면 됩니다(없으면 "없음").

로그에서 보는 것(Claude가 판정):
- `pressed: squeezing past while held`(누르는 즉시), `start: ... sound 0003E879 valid true played true id N`.
- `RESULT squeeze on ...: ... player phantom XXXXXXXX -> YYYYYYYY`에서 YYYYYYYY가 매번 같은 원래 값(r11의 004A001E 같은)으로 돌아오는지. `body shares the phantom's filter`가 찍히면 정상입니다.
- `nudge RESULT on ...: N aside ... z ±N`, `z MOVED`가 없어야 합니다.
- 비켜 지나가기와 별도로: `PlayerCharacter::Update has not reached CIGAR`가 다시 나오면 그 줄의 `the vtable slot is ...`가 원인 DLL을 말합니다.

## r12 (2026-09-30)

- The user picked the nudge "옆으로 8" (slide 8 units): it is now the default (source only, not built yet).

## r13 판정 (2026-10-01, Claude)

- 사용자 판정: NUDGE, NOTE, NOTE-READ, PASSTIME, LEAN, OFFSET-DEF, OFFSET-WIDE 전부 통과.
- LEAN 메모: 사람들이 원하는 것은 반대 방향(기대기는 두고 앉기·눕기를 끄기). → 앉기, 눕기 스위치 추가.
- 로그(`_test-runs\next\logs-r13\CIGAR.log`, 22:00-22:19): WARN·오류 없음. 앵커 갱신 중단 없음(12분).
  연동 스크립트(BaboKey, Grapple, Bathe) 전부 `script=true`.
- 이른 실행(`logs-r13-early`, 새 게임, 20:52-21:08):
  - 21:04:42 앵커 갱신 중단. **vtable 칸 소유자가 처음 기록됨: `valhallaCombat.dll`** (CIGAR가 훅을 건
    20:56:21보다 뒤에 그 DLL이 칸을 다시 씀). 프롬프트는 플레이어 기준으로 계속 표시됨.
  - 새 게임 시작 순간(20:58:16) 마커를 놓지 못했다가 31초 뒤 복구(`recovery`). Rest의 애니메이션 싱크
    미등록 경고 1건(확인 로그만 빠짐).
  - 프리징: CIGAR 로그의 마지막 줄은 21:08:27 콘솔 열림(`prompts off`). 그 뒤 `Loading Menu` 줄이 없음.
    CIGAR의 경고·오류는 그 앞 6분 동안 없음. CIGAR가 관여했다는 흔적은 없으나, 로그가 멈춘 것만으로
    무관하다고 증명되지는 않음(프리징이면 모든 스레드의 로그가 멈춤).

## r14: 앉기·눕기 스위치와 제어판 2열 (2026-10-01)

준비: 무기를 넣고 3인칭. 제어판은 SKSE Menu Framework 창의 CIGAR.

- R14-PANEL. CIGAR > 2. 비전투 페이지를 연다. 기대: 모듈이 2열로 나오고, 각 항목은 제목, 설명,
  (있으면) 연동 줄만 보임. "조건:"과 "최근:" 줄이 없음. 글자가 열 밖으로 잘리지 않음.
- R14-SIT. "앉기"를 끄고 가만히 서서 바닥을 내려다본다. 기대: "눕기 (길게)"만 뜨고 "앉기"는 안 뜸.
- R14-LIE. "앉기"를 켜고 "눕기"를 끈 뒤 같은 조작. 기대: "앉기 (길게)"만 뜸.
- R14-BOTH. 둘 다 끄고 벽 앞에 선다. 기대: 바닥을 봐도 아무것도 안 뜨고, 벽 앞에서는 "벽에 기대기 (길게)"가 뜸.

### r14 로그 판정: 앵커 갱신 재훅 (사용자 조작 없음, Claude가 `CIGAR.log`로 판정)

평소 플레이만 하면 됨. 볼 줄:
- `[PromptAnchor] PlayerCharacter::Update hooked (chained: true, to <모듈>)`: 처음 훅할 때 칸에 있던 함수의 주인.
- `update hook re-installed (#n of 3): the slot held <주소> in <모듈>; CIGAR now chains to ...`: 칸을 빼앗겨 다시 건 기록.
- 바로 뒤의 `update hook is being called again after re-install #n`: 다시 걸린 뒤 실제로 호출됨(합격).
- `WARN update hook: the slot was taken again ... giving up` 또는 `WARN PlayerCharacter::Update has not reached CIGAR`:
  상한(3회)을 넘었거나 칸이 CIGAR 것인데도 호출이 끊김(불합격, 원인 다름).
- 재훅 줄이 한 번도 없고 WARN도 없으면: 이번 실행에서는 문제가 재현되지 않음(판정 보류).

## r14 판정 (2026-10-02, Claude)

- 사용자 판정: PANEL, SIT, LIE, BOTH 전부 통과.
- PANEL 메모: Rest가 전체 켜기/끄기인지, 딸린 스위치를 하부 메뉴처럼 보이게. → 코드 확인: 맞음(Rest가 꺼지면
  main의 틱 루프가 Rest를 건너뛰어 앉기·눕기·기대기·손 녹이기·시간 보내기가 전부 멈춤). r15 빌드에서 묶어 그림.
- 로그(`_test-runs\next\logs-r14\CIGAR.log`, 00:01-00:17, 새 게임): 재훅 줄 없음, `has not reached CIGAR` 없음.
  **앵커 정지가 재현되지 않아 재훅은 판정 보류.** 처음 훅할 때 칸의 주인은 `BeamWalking.dll`.
  경고 2건은 r13 이른 실행과 같은 새 게임 시작 순간의 것(마커를 10초 뒤 복구, Rest 애니메이션 싱크 미등록).

## r15: 휴식 스위치 묶음 (2026-10-02)

- R15-REST. CIGAR > 2. 비전투에서 "휴식 전체 켜기/끄기 (앉기·눕기·기대기)"를 찾는다. 기대: 그 아래에 앉기, 눕기,
  기대기, 시간 보내기가 한 단계 들여쓰기로 붙어 있고, 2열 배치가 깨지지 않음.
- R15-REST-OFF. 그 스위치를 끈다. 기대: 딸린 네 스위치가 흐리게(비활성) 보이고 눌리지 않음. 다시 켜면 원래
  켜짐/꺼짐 상태 그대로 돌아옴.
- 앵커 재훅은 계속 로그 판정(조작 없음, r14 항목과 같은 줄).


## r15 판정 (2026-10-02, Claude)

- 사용자 판정: R15-REST, R15-REST-OFF 통과. 메모: 휴식 칸이 길어 오른쪽 "무기 충전" 아래에 빈 공간. → 두 열이
  행을 공유하지 않고 각자 위에서부터 쌓이도록 바꿈.
- 로그(`_test-runs\next\logs-r15\CIGAR.log`, 00:35-00:43, 세이브 로드): 경고·오류 없음, 로드 직후 마커 재사용
  정상. 재훅 줄 없음(8분 실행, 앵커 정지 미재현: 판정 보류 유지).

## r16: 제어판 열별 쌓기 (2026-10-02)

- R16-PANEL. CIGAR > 2. 비전투. 기대: 왼쪽 열의 휴식 칸 옆 오른쪽 열에 빈 공간이 없고 항목이 위에서부터 이어짐.
  읽는 순서는 왼쪽 열을 다 내려간 뒤 오른쪽 열. 오른쪽 끝에서 글자가 잘리지 않음(창 가장자리에서 줄이 바뀜).
  1. 전투, 3. 모드 연동 페이지도 같은 배치.

## r16 판정 (2026-10-02, Claude)

- 사용자 판정: R16-PANEL 통과(오른쪽 열 공백 없음, 글자 안 잘림). 로그(01:47-01:53): 경고·오류 없음, 재훅 줄 없음.

## r17: 다대일 칸 프로토타입 (휩쓸어 베기, 워 스톰프; 브랜치 `cleave-proto`, 작성자 빌드 전용)

빌드: `C:\TAKEALOOK\_test-runs\cigar-cleave-proto\CIGAR.dll` (3.1.2 전부 포함 + 프로토타입, sha256 19FC8F97...C893,
`cleave-proto` d1fd004). **2026-10-02 배포됨**(verify_deploy 통과); r17 뒤 3.1.2 작성자 빌드로 되돌림. 필요: Precision(휩쓸기), Bow Rapid Combo V3와 Hot Key
Skill(스톰프). 전부 게임에서 처음 실행되는 것이라 "안 뜬다/안 나간다"도 결과임. 판정은 대부분 `CIGAR.log`로 함.

콘솔 배치(시작할 때 한 번; 순간이동 없음, 야외 평지에서):
```
player.additem 0001359D 1
player.additem 00012EB7 1
player.modav health 5000
```
(`tgm`은 쓰지 않음: 무한 스태미나라 강공격의 스태미나 차감을 로그로 볼 수 없게 됨.) 적 소환은 항목마다:
`player.placeatme 0003DE8A 2` (도적, 현재 로드오더에서 확인) / 중립 NPC `player.placeatme 00013BBF 1` (나짐).

프롬프트가 뜨는 조건(사용자가 만들 수 있게 한 줄씩):
- 휩쓸어 베기: 전투 중, 근접 무기(검·도끼·대검·양손도끼·망치)를 뽑고 공격 중이 아닐 때, **도적 2명 이상이 정면
  (좌우 약 35도 안에 1명, 나머지는 60도 안)에서 서로 붙어(옆으로 120, 앞뒤 80 이내) 무기 닿는 거리 안에 0.3초**
  있으면 뜸. 그 부채꼴 안에 적대가 아닌 사람이 있으면 안 뜸. 한 무리를 등지고 서면 안 뜸(정면이어야 함).
- 워 스톰프: 전투 중, 활이 아닌 무기를 뽑고, **적 2명 이상이 200 이내(약 세 걸음)에서 앞뒤·좌우로 둘러싸(150도
  이상 퍼져) 0.3초** 있으면 뜸. 200 안에 적대가 아닌 사람이 있으면 안 뜸. 둘러싸이면 휩쓸어 베기 대신 이것이 뜸.

- R17-CLEAVE-2H. 철 대검을 들고 도적 2명 소환, 둘이 앞에 붙어 오게 둔다. "휩쓸어 베기"가 뜨면 누른다. 다시 뜨면
  또 눌러 **세 번** 누른다(누를 때마다 시작 방식이 다름). 기대: 누를 때 강공격 한 번이 나가고 두 명이 함께 맞음.
  로그: `crowd: CLEAVE ...`, `swing #n by action|event|idle ...: started true|false`, `swing state after 0.3 s: ... power
  attacking ...; attack data ...; stamina ... (spent ...)`, `RESULT cleave (...): CLEAVED | ONE HIT ONLY | NO HIT; hits ...`.
- R17-CLEAVE-1H. 철 검으로 같은 것(세 번).
- R17-NO-PROMPT. 도적 1명만 남았을 때, 그리고 나짐을 소환해 도적들과 함께 앞에 있을 때. 기대: 휩쓸어 베기가 안 뜸.
  로그: `trace no group: one hostile only` / `a non-hostile actor in the arc (...)`.
- R17-STOMP. 근접 무기를 든 채 도적 3명 소환(플레이어 위치에 생겨 둘러쌈). 기대: "워 스톰프"가 뜨고, 누르면 발차기
  동작 + 쿵 소리 + 먼지, 주변 도적이 피해 없이 비틀거림. 한 번 쓴 뒤에는 둘러싼 적 구성이 바뀔 때까지 다시 안 뜸.
  눈으로 볼 것: 팔 모양이 어색한지(활용 동작임), 동작 뒤 전투 자세로 돌아오는지.
  로그: `[Cleave] war stomp clip ready|off: ...`(게임 시작 시), `crowd: STOMP ...`, `stomp: CustomStartC sent, accepted
  true|false`, `RESULT stomp: ALL|SOME|NOBODY STAGGERED; n of m ...` 또는 `RESULT stomp: NOT PLAYED (...)`.
- 조작 없는 로그 판정: 푸스로다는 프롬프트 없이 `crowd: FUS RO DAH (log only, no prompt)` 줄만 남김(적이 멀리 앞에 둘
  이상 있을 때). 앵커 재훅 줄도 계속 봄.

## r17b 판정 (2026-10-03, Claude; 원본 CIGAR.log 11:59-12:18)

- 사용자: L1, CLEAVE-2H, CLEAVE-1H, NO-PROMPT 통과, STOMP 실패(발동은 됨, 먼지·소리 없음), STOMP-LOOK 자연스러움.
- 로그: 스톰프 5회 모두 "spell cast, sound played, dust spawned"이고 4~5명 비틀거림 → 효과가 작거나 안 들린 것.
  휩쓸기 action 경로 5회 = 강공격(CLEAVED 2, ONE HIT 3), event 경로 5회 = 강공격 아님·돌진·NO HIT 4, idle 0회 시작.
- 조치: 스톰프 소리 따라가기+볼륨, 먼지 3배 + 꼬리 내려찍기 충격 효과; 휩쓸기는 action 경로만; 전투 중 독 바르기 한 번
  누르기; NPC 그래플 끔(Grapple INI, IAM Case 040).

## r17c (프로토타입 DLL 05E399EE..., `cleave-proto` 53bad15, 2026-10-03 배포)

콘솔 배치와 소환은 r17과 같음.
- R17C-STOMP. 도적 3명에게 둘러싸여 "워 스톰프". 기대: 발차기 때 쿵 소리와 먼지가 보이고 들림.
  로그: `RESULT stomp: ...; stagger spell cast, sound played, dust spawned, impact set played|not played`.
- R17C-CLEAVE. 휩쓸어 베기를 세 번. 기대: 세 번 모두 강공격(돌진하는 다른 기술이 나오지 않음).
  로그: 매번 `swing #n by action`, `power attacking true`.
- R17C-POISON. 독을 가진 채 전투 중 무기를 뽑음. 기대: "독 바르기: ..."(길게 없음)가 뜨고 한 번 누르면 바로 발라짐.
  전투 밖에서는 "독 바르기 (길게)" 링 그대로.
- R17C-GRAPPLE. 전투 내내 NPC가 플레이어에게 그래플을 걸지 않음(조작 없음).
- 로그만: `PlayerCharacter::Update hooked on runtime 1-6-1170-0 (slot AD; ...)` 줄(1.7 안전장치가 1.6.1170을 그대로 통과).

## r18: 용언 장착 (VoiceAnswer, 2026-10-03 배포, DLL 949A8D46...)

준비: 거침없는 힘을 한 단어 이상 앎, 외침 칸에 다른 외침이나 종족 능력을 둔 상태(콘솔로 만들 때:
`player.teachword 00013E22` 후 `player.unlockword 00013E22` — Fus, Skyrim.esm). 적 소환: `player.placeatme 0003DE8A 2`.
- R18-SHOUT. 도적 둘이 보이는 전투에서 목소리가 쉬고 있지 않을 때. 기대: "장착하기: 거침없는 힘"이 한 번 누르기로
  뜨고, 누르면 외침 칸이 거침없는 힘으로 바뀜(외침 키로 바로 쓸 수 있음). 이미 장착돼 있거나 외침 직후 대기 중이면 안 뜸.
  로그: `[VoiceAnswer] equipped 거침없는 힘 (two or more enemies within range): voice slot was ..., now 거침없는 힘`.
- 로그만: 전투가 끝나도 외침 칸을 되돌리지 않음(D40).

### r18 판정 (2026-10-03, Claude; 원본 CIGAR.log 13:47-13:58)

- 사용자: SHOW·EQUIP·GONE 실패(프롬프트가 안 뜸).
- 로그: 전투 중 매 틱 `enemies=0 ... (not two or more enemies in sight)`. 적 수를 세는 단계에서 0이었고, 어느 조건(적대·전투·시야)에서
  빠졌는지는 로그에 없었음. 시야 판정(`HasLineOfSight`)이 원인으로 유력. 외침 대기 중에는 적을 세지도 않았음.
- 수정(빌드 대기): 시야를 조건에서 뺌(적대 + 전투 중 + 1,000 이내 둘 이상), 줄에 단계별 수(near/hostile/enemies)와 양방향 시야 수를
  남기고, 외침 대기 중에도 셈. 다음 실행은 r18 항목 그대로 다시(조작 같음).

### r19 판정 (2026-10-03, Claude; 원본 CIGAR.log 14:37-14:54)

- 사용자: SHOW·EQUIP·GONE 실패. 로그: 전투 내내 `near=0 hostile=0 enemies=0`, 같은 틱에 WeaponSwap은 `Util::NearbyHostiles`로
  목표를 73 거리에서 찾음 → VoiceAnswer 자체 반복문이 아무도 세지 못함(정확한 원인은 코드 판독으로 특정 못 함).
- 수정(빌드 대기): 적 수를 WeaponSwap과 같은 `Util::NearbyHostiles`로 셈. 줄에 목록 크기(listed)·적대·빈사·전투 중 수를 남김.
  다음 실행은 r18 항목 그대로.

### r23 판정 (2026-10-04, Claude; 원본 CIGAR.log 23:09 실행)

- 사용자: SHOW·EQUIP·GONE 통과. 로그: 전투 중 `listed=38 hostile=2 bleeding=0 enemies=2` → 프롬프트 표시 → 수락 →
  `voice slot was 시간 왜곡, now 거침없는 힘` → 이후 `already equipped`, 적 1명이 되자 `not two or more enemies`. 의도대로. 경고·오류 0.

## r25: 용언 장착 확장 (D57; 빌드 대기 중, 커밋 6d30177)

콘솔 배치 파일: `C:\TAKEALOOK\_test-runs
ext25-console\` (스카이림 폴더에 복사해 `bat r25words` 식으로 실행).
`r25words`: 거침없는 힘(Fus)·시간 왜곡(Tiid)·카인의 평화(Kaan)·공포(Faas Ru Maar) 단어 익힘 + 철 대검 + 체력 5000.
외침 대기 중이면 안 뜸(시간 왜곡은 Stormcrown 기준 180초 이상) — 항목 사이에 외침을 쓰지 않으면 바로 이어 볼 수 있음.
- R25-WOLVES. `bat r25wolves`(늑대 3). 기대: "장착하기: 카인의 평화". 로그 `rule animal-pack`.
- R25-FRIEND. `bat r25friend`(도적 2 + 나짐 1), 나짐이 정면에 오게. 기대: "장착하기: 시간 왜곡". 로그 `rule friend-ahead`.
- R25-CENTURION. `bat r25centurion`(드워프 백부장 1 + 도적 1). 기대: "장착하기: 시간 왜곡". 로그 `rule unshakeable`.
- R25-BANDITS. `bat r25bandits`(도적 2). 기대: 도적 레벨이 24 이하면 "공포", 넘으면 "거침없는 힘". 로그 `rule low-level-crowd` 또는 `crowd`,
  게이트 줄의 `maxLevel=`·`dismayCap=`이 어느 쪽인지 말해 줌.
- 판정은 `[VoiceAnswer] gate ... rules: friend-ahead: ... | animal-pack: ... | ...` 줄로 함(규칙별 성립 여부와 이유).

