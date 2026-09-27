# CIGAR

스카이림에서 자주 하는 행동을 **화면에 뜨는 프롬프트**로 바꿔 주는 SKSE 플러그인입니다.
지금 할 수 있는 행동이 조건이 맞을 때만 화면에 뜨고, 키 하나로 실행됩니다.
다른 모드의 단축키를 외울 필요가 없습니다.

플러그인(.esp/.esl)이 없습니다. 설치해도 로드 오더가 늘지 않고, 언제든 빼도 됩니다.
문구는 게임 언어에 따라 한국어 또는 영어로 나옵니다.

## 요구 사항

- [SKSE64](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- [SkyPrompt](https://www.nexusmods.com/skyrimspecialedition/mods/148703) — 프롬프트를 그리는 모드입니다. 없으면 아무것도 표시되지 않습니다.
  SkyPrompt에는 SKSE Menu Framework와 ImGui Icons가 필요합니다.
- [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) — 게임 안에서 설정을 바꾸는 CIGAR 패널도 여기에 생깁니다. 패널이 없어도 기능은 그대로 동작합니다.

나머지는 모두 선택입니다. 아래 "연동 모드"를 보세요. Skyrim 1.6.1170(Anniversary Edition),
SkyPrompt 2.4.0과 2.3.15에서 시험했습니다.
게임패드는 SkyPrompt의 패드 버튼으로 동작합니다. CIGAR의 키 설정은 키보드 키만 바꿉니다.

## 설치

Mod Organizer 2나 Vortex로 압축 파일을 그대로 설치하면 됩니다. 새 게임은 필요 없습니다.

## 프롬프트 사용법

- 조건이 맞는 동안 계속 떠 있고, 조건이 사라지면 저절로 없어집니다.
- 한 번에 최대 네 개까지 뜨며, 각각 다른 키가 배정됩니다. 기본 키는 1, 2, 3, 4입니다.
- 전투 밖 행동은 대부분 **길게 누르기**입니다. 실수로 눌러 상태가 바뀌지 않게 하기 위해서입니다.
- 키를 **두 번 빠르게 눌러 닫으면**, 시간 보내기·투구·의자 음주·주시하기·마검사 모드는 상황이 바뀔 때까지 다시 뜨지 않습니다.
- 3인칭에서는 프롬프트가 캐릭터 머리 바로 앞, 약간 오른쪽에 뜹니다(오른쪽 간격은 설정에서 바꿉니다).
  1인칭에서는 캐릭터 기준으로 뜹니다.
- 메뉴(인벤토리, 지도, 대화 등)가 열려 있는 동안은 숨습니다.

## 프롬프트 목록

모드 이름이 붙은 프롬프트는 그 모드가 있어야 뜹니다. 나머지는 바닐라만으로 동작합니다.

### 생활

| 프롬프트 | 언제 |
|---|---|
| 목욕하기 / 샤워하기 | 벗은 채 물에 들어갔을 때, 폭포 아래에 있을 때. *Bathing in Skyrim - Renewed* |
| 탈의하기 / 착용하기 | 침대·옷장 앞, 물속. 벗은 옷을 기억했다가 그대로 입습니다 |
| 먹기: <음식> | 배가 고플 때. 가장 싼 음식부터, 날고기·술·상한 음식은 빼고. *서바이벌 모드* |
| 소변 보기 / 대변 보기 | 방광·장 수치가 찼을 때. 목욕하는 동안은 뜨지 않습니다. *Private Needs - Orgasm* |
| 배출 (길게) | 몸에 찬 것이 있을 때. *Fill Her Up Baka Edition* |
| 앉기 / 눕기 | 무기를 넣고 가만히 서서 바닥을 내려다볼 때. 움직이면 천천히 일어납니다 |
| 기대기 | 벽·탁자·난간을 정면으로 볼 때 |
| 손 녹이기 | 모닥불·화로 앞 |
| 시간 보내기 (누르고 있기) | 앉거나 누운 동안, 의자에 앉은 동안. 누르는 동안 시간이 빨리 흐릅니다 |
| 마시기: <술> | 여관·집 의자에 앉아 있고 술이 있을 때 |
| 주시하기 (누르고 있기) | 무기를 넣고 5초 동안 인물이나 먼 풍경을 바라볼 때. 시야가 확대됩니다 |

### 장비·아이템

| 프롬프트 | 언제 |
|---|---|
| 장착하기 (길게): <장비> | 새 무기·방어구를 얻었을 때 (15초). 방어구는 지금 입은 것보다 방어력이 높을 때만, 입은 것이 마법부여 장비면 뜨지 않습니다 |
| 읽기 (길게): <책> | 모르는 주문서나 퀘스트 쪽지·책을 얻었을 때 (15초) |
| 투구 벗기 (길게) / 투구 쓰기 | 얼굴이 보이도록 투구를 벗고 다니는 것이 기본입니다. 던전 밖에서 투구를 쓰고 있으면 벗기가, 전투가 시작되면 쓰기가 뜹니다 |
| 충전하기: <무기> | 전투 밖에서 손에 든 마법 무기의 충전량이 25% 이하일 때. 가장 알맞은 소울젬을 씁니다 |
| 독 바르기 (길게): <독> | 무기를 꺼냈고, 무기에 독이 없고, 독을 가지고 있을 때 |
| 마시기: <물약> | 체력·기력·마나가 부족하거나, 중독·질병 상태이거나, 물속에 잠겼을 때 |
| 의상 교환 (길게) | 마네킹을 볼 때. 내가 입은 옷과 마네킹의 옷을 맞바꿉니다. 마법부여·강화는 그대로 옮겨지고, 마네킹의 투구는 벗은 채로 받습니다. 한쪽이 비어 있으면 의상 보관 또는 의상 착용으로 뜹니다 |

### 퀘스트

| 프롬프트 | 언제 |
|---|---|
| 추적하기 (길게): <퀘스트> | 추적하지 않던 퀘스트에 새 목표가 생겼을 때 (15초) |
| 장착하기: <샤우트> | 그레이비어드가 샤우트를 보여 달라고 할 때 |
| 파티 의상 입기 / 원래 장비로 | 탈모어 대사관 연회 퀘스트 |
| 행동 선택 | BaboDialogue의 납치 방에 있을 때. *BaboDialogue* |

### 전투

| 프롬프트 | 언제 |
|---|---|
| 록온 | 전투 중 적을 록온하지 않았을 때. *True Directional Movement* |
| 그래플 | 전투 중 가까운 적이 있을 때. *Grapple* |
| 원거리 무기 / 근접 무기 | 적과의 거리가 지금 든 무기와 맞지 않을 때 (화살이 있는 활·석궁만) |
| 처형 | 스태거가 깨진 적이 있을 때. *Valhalla Combat* |
| 유술 | 가드 중인 인간형 적이 가까이 있을 때. 넘어뜨리기 네 가지와, 적을 죽이는 목 꺾기 |
| 항복 (길게) | 전투 중 체력이 40% 아래로 떨어졌을 때. *Acheron* |
| 마검사 모드 / 마검사 해제 | 꺼진 상태에서 무기를 꺼낼 때, 켜진 상태에서 전투 밖에서 무기를 넣을 때. *The Wizard Warrior* |

## 연동 모드

CIGAR는 아래 모드가 **있으면 쓰고, 없으면 그 프롬프트만 조용히 쉽니다.**
이 모드들의 파일은 넣지 않습니다.

| 프롬프트 | 모드 | 받는 곳 |
|---|---|---|
| 목욕하기, 샤워하기 | Bathing in Skyrim - Renewed | [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/135288) |
| 록온 | True Directional Movement | [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/51614) |
| 그래플 | Grapple (Smooth) | Smooth의 Patreon |
| 처형 | Valhalla Combat | [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/64741) |
| 항복 | Acheron | [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/108159) |
| 항복 (3분 규칙 반영) | Yamete Kudasai | [LoversLab](https://www.loverslab.com/files/file/23123-yamete-kudasai/) |
| 먹기 | 서바이벌 모드(크리에이션 클럽, 현재 게임에 무료 포함)와 [Survival Mode Improved - SKSE](https://www.nexusmods.com/skyrimspecialedition/mods/78244) | |
| 먹기 (Gourmet의 식사 아닌 음식 제외) | Gourmet | [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/96876) |
| 배출 | Fill Her Up Baka Edition (BakaFactory) | LoversLab / SubscribeStar |
| 소변 보기, 대변 보기 | Private Needs - Orgasm | [LoversLab](https://www.loverslab.com/files/file/39023-private-needs-orgasm/) |
| 행동 선택 | BaboDialogue (BakaFactory) | LoversLab / SubscribeStar |
| 마검사 모드, 마검사 해제 | The Wizard Warrior - Spellsword Magic Combat Evolved | [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/14890) |

손 녹이기에는 서바이벌 모드 크리에이션 클럽 파일이 필요합니다.

### CIGAR가 바꾸는 다른 모드 설정

키 하나가 두 가지 일을 하지 않도록, CIGAR는 다른 모드의 단축키를 넘겨받을 수 있습니다.
단축키 페이지의 **프롬프트 전용** 스위치이며, **기본으로 켜져 있습니다.** 끄면 원래 키를 돌려줍니다.

| 모드 | 프롬프트 전용이 켜져 있는 동안 |
|---|---|
| Grapple | 단축키를 F13으로 옮깁니다 |
| Acheron | 항복 키를 F14로 옮깁니다 |
| Valhalla Combat | 처형 키를 F15로 옮깁니다 |
| Fill Her Up Baka Edition | 배출 단축키를 비웁니다 |
| Private Needs - Orgasm | 단축키 여섯 개를 비웁니다 (메뉴 키, 수치 확인 키 포함) |

True Directional Movement가 있으면 Grapple의 록온 키를 TDM의 록온 키로 맞춥니다.

다른 모드의 메뉴에서 이 키들을 바꿨다면 단축키 페이지의 **모드 키 다시 확인**을 한 번 눌러 주세요.

## 설정 (SKSE Menu Framework)

게임 안 메뉴의 **CIGAR** 항목에 다섯 페이지가 있습니다.

1. **전투**, 2. **비전투**, 3. **모드 연동** — 기능별 켜고 끄기. 끄면 그 프롬프트는 바로 사라집니다.
   모드 연동 기능은 그 모드가 있을 때만 동작하고, 없으면 조용히 기다립니다.
4. **단축키** — 프롬프트 키 네 개와 위의 프롬프트 전용 스위치. 다른 모드와 겹치면 경고가 표시됩니다.
5. **세부 설정** — SkyPrompt 연결 상태, **언어**(자동, 한국어, English), **프롬프트 위치**(3인칭에서
   프롬프트가 오른쪽으로 비켜나는 정도: 0, 10, 15, 20, 30, 기본 15), 그리고 프롬프트가 뜨기 시작하는
   기준값입니다. 물약의 비율, 먹기의 허기 단계, 용변 수치, 무기 전환 거리, 유술 거리, 침대·옷장 유효 거리, 시간 보내기 속도. **유술 피해**(넘어진 적이 잃는
   최대 기력 비율, 기본 100%, 최대 체력 비율, 기본 5%)도 여기서 바꿉니다. 넘어뜨리기는 체력을 최소 1
   남기고, 목 꺾기만 적을 죽입니다.

바꾼 값은 `SKSE/Plugins/CIGAR.json`에 저장됩니다.

## 알아 둘 것

- **의자 음주**는 기본 게임과 DLC의 술 29종, 그리고 다른 모드가 술로 표시한 음료(Gourmet, Object
  Categorization Framework, SunHelm)를 압니다.
- **장면:** SexLab이나 OStim 장면이 진행되는 동안에는 전투·장비·용변 프롬프트가 뜨지 않습니다.
  (실행 중에 감지하며, 두 프레임워크 모두 필수가 아닙니다.)
- **투구**는 동작 없이 바로 벗습니다.
- **프롬프트 위치:** 프롬프트를 놓으려고 세이브마다 보이지 않는 마커 하나를 둡니다. CIGAR를 빼도 해가 없습니다.
- **마네킹:** 비공식 패치와 Another Mannequin Script Fix에서 시험했습니다. 마네킹이 받을 수 없는 의상이면
  아무것도 옮기지 않고 이유를 알림으로 알려 줍니다.
- 설정 패널의 페이지 이름은 다음 실행부터 언어가 바뀝니다. 나머지는 바로 바뀝니다.

## 알려진 문제

- **유술: 게임을 불러온 뒤 아직 아무도 죽지 않았으면 발동하지 않습니다.** 처음 누군가 죽고 나면
  정상적으로 발동합니다.
- **처형: 첫 입력을 가끔 놓칠 수 있습니다.** 다시 누르면 나갑니다.
- **마검사 해제 뒤에도 무기 마법부여가 남습니다.** 지속 시간이 끝나면 사라집니다. The Wizard Warrior 자체의
  설계이며, CIGAR는 건드리지 않습니다.

## 문제가 생기면

`Documents/My Games/Skyrim Special Edition/SKSE/CIGAR.log`에 실행할 때마다 기록이 남습니다.
프롬프트가 안 뜨거나 동작이 안 나가는 이유와, 위 연동 모드 중 무엇을 찾았는지가 적혀 있으니,
문의할 때 이 파일을 같이 올려 주세요.

## 라이선스

CommonLibSSE-NG를 사용하므로 **GPL-3.0-or-later**입니다.
소스는 <https://github.com/kkw1010-dev/HKT> 에 있습니다.
