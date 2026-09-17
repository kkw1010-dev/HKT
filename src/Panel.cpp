#include "Panel.h"

#include "LockOn.h"
#include "Module.h"
#include "Prompt.h"
#include "Settings.h"
#include "Surrender.h"

// The vendored header mixes struct/class and enum types; its warnings are upstream's.
#pragma warning(push)
#pragma warning(disable: 4099 5054)
#include "SKSEMenuFramework/SKSEMenuFramework.h"
#pragma warning(pop)

namespace CIGAR::Panel
{
	namespace
	{
		constexpr auto kSection = "CIGAR";
		constexpr auto kPage = "설정";

		struct Label
		{
			std::string_view module;
			const char* title;
			const char* needs;
		};

		// Modules missing here still get a switch, titled with their own name.
		constexpr std::array kLabels{
			Label{ "Bathe", "목욕", "Bathing in Skyrim - Renewed" },
			Label{ "Dress", "탈의·착용", "" },
			Label{ "BaboKey", "납치 행동 선택", "BaboDialogue" },
			Label{ "LockOn", "록온·그래플", "True Directional Movement, Grapple" },
			Label{ "Deflate", "배출", "Fill Her Up" },
			Label{ "Surrender", "항복", "Acheron (Yamete Kudasai)" },
		};

		const Label* Find(std::string_view a_module)
		{
			for (const auto& label : kLabels) {
				if (label.module == a_module) {
					return &label;
				}
			}
			return nullptr;
		}

		const ImVec4 kDim{ 0.62f, 0.62f, 0.62f, 1.0f };
		const ImVec4 kWarn{ 1.0f, 0.72f, 0.28f, 1.0f };

		struct KeyName
		{
			std::uint32_t code;  // DirectInput scan code
			const char* name;
		};

		// The keys offered for prompt slots: the ones SkyPrompt has icons for and a player can reach
		// without leaving the movement keys for long.
		constexpr std::array kKeys{
			KeyName{ 0x02, "1" }, KeyName{ 0x03, "2" }, KeyName{ 0x04, "3" }, KeyName{ 0x05, "4" },
			KeyName{ 0x06, "5" }, KeyName{ 0x07, "6" }, KeyName{ 0x08, "7" }, KeyName{ 0x09, "8" },
			KeyName{ 0x0A, "9" }, KeyName{ 0x0B, "0" }, KeyName{ 0x0C, "-" }, KeyName{ 0x0D, "=" },
			KeyName{ 0x10, "Q" }, KeyName{ 0x11, "W" }, KeyName{ 0x12, "E" }, KeyName{ 0x13, "R" },
			KeyName{ 0x14, "T" }, KeyName{ 0x15, "Y" }, KeyName{ 0x16, "U" }, KeyName{ 0x17, "I" },
			KeyName{ 0x18, "O" }, KeyName{ 0x19, "P" }, KeyName{ 0x1E, "A" }, KeyName{ 0x1F, "S" },
			KeyName{ 0x20, "D" }, KeyName{ 0x21, "F" }, KeyName{ 0x22, "G" }, KeyName{ 0x23, "H" },
			KeyName{ 0x24, "J" }, KeyName{ 0x25, "K" }, KeyName{ 0x26, "L" }, KeyName{ 0x2C, "Z" },
			KeyName{ 0x2D, "X" }, KeyName{ 0x2E, "C" }, KeyName{ 0x2F, "V" }, KeyName{ 0x30, "B" },
			KeyName{ 0x31, "N" }, KeyName{ 0x32, "M" }, KeyName{ 0x29, "`" }, KeyName{ 0x0F, "Tab" },
			KeyName{ 0x3A, "Caps Lock" }, KeyName{ 0x3B, "F1" }, KeyName{ 0x3C, "F2" }, KeyName{ 0x3D, "F3" },
			KeyName{ 0x3E, "F4" }, KeyName{ 0x3F, "F5" }, KeyName{ 0x40, "F6" }, KeyName{ 0x41, "F7" },
			KeyName{ 0x42, "F8" }, KeyName{ 0x43, "F9" }, KeyName{ 0x44, "F10" }, KeyName{ 0x57, "F11" },
			KeyName{ 0x58, "F12" }, KeyName{ 0x52, "Num 0" }, KeyName{ 0x4F, "Num 1" }, KeyName{ 0x50, "Num 2" },
			KeyName{ 0x51, "Num 3" }, KeyName{ 0x4B, "Num 4" }, KeyName{ 0x4C, "Num 5" }, KeyName{ 0x4D, "Num 6" },
			KeyName{ 0x47, "Num 7" }, KeyName{ 0x48, "Num 8" }, KeyName{ 0x49, "Num 9" },
		};

		std::string NameOf(std::int64_t a_code)
		{
			for (const auto& key : kKeys) {
				if (key.code == a_code) {
					return key.name;
				}
			}
			switch (a_code) {
			case 0x64:
				return "F13";
			case 0x65:
				return "F14";
			case 0x9C:
				return "Num Enter";
			default:
				break;
			}
			if (a_code >= 256 && a_code < 264) {
				return std::format("마우스 {}", a_code - 255);
			}
			return std::format("#{}", a_code);
		}

		void RenderPromptOnlyItem(std::string_view a_target, const char* a_label, const char* a_hidden, void (*a_apply)())
		{
			bool on = Settings::PromptOnly(a_target);
			if (ImGui::Checkbox(a_label, &on)) {
				Settings::SetPromptOnly(a_target, on);
				SKSE::GetTaskInterface()->AddTask(a_apply);
			}
			ImGui::Indent();
			const auto manual = Settings::ManualKey(a_target);
			const auto manualName = manual >= 0 ? NameOf(manual) : std::string("기록 없음");
			if (on) {
				ImGui::TextColored(kDim, "모드 키를 %s(숨김 키)로 옮김. 원래 키 %s는 비어 있음", a_hidden, manualName.c_str());
			} else {
				ImGui::TextColored(kDim, "모드 자체 키 사용. 끌 때 복원한 키: %s", manualName.c_str());
			}
			ImGui::Unindent();
		}

		void RenderPromptOnly()
		{
			RenderPromptOnlyItem("grapple", "그래플: 프롬프트 전용##po-grapple", "F13",
				[] { LockOn::GetSingleton()->CheckKeys(); });
			RenderPromptOnlyItem("surrender", "Acheron 항복: 프롬프트 전용##po-surrender", "F14",
				[] { Surrender::GetSingleton()->ApplyKeyMode(); });
			if (ImGui::Button("모드 키 다시 확인")) {
				SKSE::GetTaskInterface()->AddTask([] {
					LockOn::GetSingleton()->CheckKeys();
					Surrender::GetSingleton()->CheckKey();
				});
			}
			const auto grapple = LockOn::GetSingleton()->GrappleKey();
			const auto surrender = Surrender::GetSingleton()->SurrenderKey();
			ImGui::TextColored(kDim, "현재: 그래플 %s, Acheron 항복 %s",
				grapple >= 0 ? NameOf(grapple).c_str() : "없음", surrender >= 0 ? NameOf(surrender).c_str() : "없음");
			ImGui::PushTextWrapPos(0.0f);
			ImGui::TextColored(kDim, "키는 불러오기 때와 이 버튼을 누를 때만 확인. MCM에서 키를 바꾼 뒤 누를 것. 프롬프트 전용이 켜져 있으면 바꾼 키를 기억하고 숨김 키로 되돌림");
			ImGui::PopTextWrapPos();
		}

		void RenderKeys()
		{
			static const auto names = [] {
				std::array<const char*, kKeys.size()> result{};
				for (std::size_t i = 0; i < kKeys.size(); ++i) {
					result[i] = kKeys[i].name;
				}
				return result;
			}();

			const auto keys = Settings::PromptKeys();
			for (std::size_t slot = 0; slot < keys.size(); ++slot) {
				int current = -1;
				for (std::size_t i = 0; i < kKeys.size(); ++i) {
					if (kKeys[i].code == keys[slot]) {
						current = static_cast<int>(i);
					}
				}
				const auto label = std::format("{}번째 프롬프트 키##key{}", slot + 1, slot);
				ImGui::SetNextItemWidth(160.0f);
				if (ImGui::Combo(label.c_str(), &current, names.data(), static_cast<int>(names.size()), 12) && current >= 0) {
					Settings::SetPromptKey(slot, kKeys[current].code);
				}
				if (current < 0) {
					ImGui::SameLine();
					ImGui::TextColored(kDim, "(설정 파일 값 %s)", NameOf(keys[slot]).c_str());
				}
			}
			if (ImGui::Button("기본값 (1, 2, 3, 4)")) {
				for (std::size_t slot = 0; slot < keys.size(); ++slot) {
					if (keys[slot] != Settings::kDefaultPromptKeys[slot]) {
						Settings::SetPromptKey(slot, Settings::kDefaultPromptKeys[slot]);
					}
				}
			}
			ImGui::TextColored(kDim, "화면에 뜬 순서대로 1번째부터 배정. 게임패드는 SkyPrompt 기본값");

			// A key shared by two slots fires both prompts; a key another mod listens to fires that mod too.
			for (std::size_t a = 0; a < keys.size(); ++a) {
				for (std::size_t b = a + 1; b < keys.size(); ++b) {
					if (keys[a] == keys[b]) {
						ImGui::TextColored(kWarn, "경고: %zu번째와 %zu번째 키가 같음 (%s)", a + 1, b + 1, NameOf(keys[a]).c_str());
					}
				}
			}
			const std::array<std::pair<const char*, std::int64_t>, 2> others{ {
				{ "그래플", LockOn::GetSingleton()->GrappleKey() },
				{ "Acheron 항복", Surrender::GetSingleton()->SurrenderKey() },
			} };
			for (std::size_t slot = 0; slot < keys.size(); ++slot) {
				for (const auto& [who, code] : others) {
					if (code == keys[slot]) {
						ImGui::TextColored(kWarn, "경고: %zu번째 키(%s)가 %s 키와 같음", slot + 1, NameOf(code).c_str(), who);
					}
				}
			}
		}

		void RenderModule(const Module* a_module)
		{
			const std::string_view name = a_module->Name();
			const auto* label = Find(name);
			const std::string id = std::format("{} ({})##{}", label ? label->title : a_module->Name(), name, name);

			bool on = Settings::Enabled(name);
			if (ImGui::Checkbox(id.c_str(), &on)) {
				Settings::SetEnabled(name, on);
			}
			ImGui::Indent();
			if (label && label->needs[0] != '\0') {
				ImGui::TextColored(kDim, "연동: %s (없으면 대기)", label->needs);
			}
			if (on) {
				const auto gate = a_module->ShownGate();
				const auto line = a_module->ShownLine();
				ImGui::PushTextWrapPos(0.0f);
				ImGui::TextColored(kDim, "조건: %s", gate.empty() ? "기록 없음" : gate.c_str());
				ImGui::TextColored(kDim, "최근: %s", line.empty() ? "기록 없음" : line.c_str());
				ImGui::PopTextWrapPos();
			} else {
				ImGui::TextColored(kDim, "꺼짐. 프롬프트 표시 안 함");
			}
			ImGui::Unindent();
			ImGui::Spacing();
		}

		void __stdcall Render()
		{
			static std::once_flag opened;
			std::call_once(opened, [] { logs::info("control panel drawn for the first time"); });

			ImGui::SeparatorText("모듈");
			for (const auto* module : Modules()) {
				RenderModule(module);
			}

			ImGui::SeparatorText("프롬프트 키");
			RenderKeys();

			ImGui::SeparatorText("모드 단축키");
			RenderPromptOnly();

			ImGui::SeparatorText("탈의·착용");
			float range = Settings::PlaceRange();
			if (ImGui::SliderFloat("침대·옷장 유효 거리", &range, Settings::kPlaceRangeMin, Settings::kPlaceRangeMax, "%.0f")) {
				Settings::SetPlaceRange(range);
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) {
				Settings::Save();
			}
			ImGui::TextColored(kDim, "기본 250. 조준한 가구에서 이 거리를 벗어나면 프롬프트 해제");

			ImGui::SeparatorText("상태");
			ImGui::Text("SkyPrompt: %s", Prompts::Available() ? "연결됨" : "없음 (프롬프트 비활성)");
			const auto source = Settings::SourceDescription();
			ImGui::TextColored(kDim, "설정 파일: %s", source.c_str());
			ImGui::TextColored(kDim, "상세 기록: SKSE\\CIGAR.log");
		}
	}

	void Register()
	{
		const auto framework = SKSEMenuFramework_Module();
		if (!framework) {
			logs::info("control panel: SKSE Menu Framework is not loaded; no panel");
			return;
		}
		// The header ignores a missing export silently, so check the one that matters here.
		if (!GetProcAddress(framework, "AddSectionItem") || !GetProcAddress(framework, "igCheckbox")) {
			logs::error("control panel: this SKSE Menu Framework lacks AddSectionItem/igCheckbox; no panel");
			return;
		}
		SKSEMenuFramework::SetSection(kSection);
		SKSEMenuFramework::AddSectionItem(kPage, Render);
		logs::info("control panel: registered as {}/{} in SKSE Menu Framework", kSection, kPage);
	}
}
