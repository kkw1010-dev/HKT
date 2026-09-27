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

- **준비:** FormID는 2026-09-27 현재 로드 오더에서 확인했습니다.
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
  player.additem FEB41810 1
  player.additem FEB41811 1
  player.additem FEB41812 1
  ```
  일식 마법사 세트: 로브, 장화, 장갑, 두건, 어깨·다리·팔 보호대(다리 보호대 `FEB41811`이 60번 슬롯).
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
