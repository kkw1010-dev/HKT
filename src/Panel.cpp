#include "Panel.h"

#include "Module.h"
#include "Prompt.h"
#include "Settings.h"

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
