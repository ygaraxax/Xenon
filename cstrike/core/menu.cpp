#include "menu.h"

// used: config variables
#include "variables.h"

// used: iinputsystem
#include "interfaces.h"
#include "../sdk/interfaces/iengineclient.h"
#include "../sdk/interfaces/inetworkclientservice.h"
#include "../sdk/interfaces/cgameentitysystem.h"
#include "../sdk/interfaces/iglobalvars.h"
#include "../sdk/interfaces/ienginecvar.h"

// used: overlay's context
#include "../features/visuals/overlay.h"

// used: notifications
#include "../utilities/notify.h"
#include <format>
#include "sdk.h"
#include "../features/antiaim/antiaim.hpp"
#include "../utilities/inputsystem.h"

#pragma region menu_array_entries
static constexpr const char* arrMiscDpiScale[] = {
	"100%",
	"125%",
	"150%",
	"175%",
	"200%"
};

static const std::pair<const char*, const std::size_t> arrColors[] = {
	{ "[accent] - main", Vars.colAccent0 },
	{ "[accent] - dark (hover)", Vars.colAccent1 },
	{ "[accent] - darker (active)", Vars.colAccent2 },
	{ "[primitive] - text", Vars.colPrimtv0 },
	{ "[primitive] - background", Vars.colPrimtv1 },
	{ "[primitive] - disabled", Vars.colPrimtv2 },
	{ "[primitive] - frame background", Vars.colPrimtv3 },
	{ "[primitive] - border", Vars.colPrimtv4 },
};

static constexpr const char* arrMenuAddition[] = {
	"dim",
	"particle",
	"glow"
};


static constexpr const char* arrHitboxes[] = {
	"HEAD",
	"NECK",
	"CHEST",
	"STOMACH",
	"CENTER",
	"PELVIS",
	"LEG",
	"FEET",
};
#pragma endregion


bool active = false;
float size_child = 0;


void MENU::RenderMainWindow()
{
	ImGuiIO& io = ImGui::GetIO();
	ImGuiStyle& style = ImGui::GetStyle();

	// @test: we should always update the animation?
	animMenuDimBackground.Update(io.DeltaTime, style.AnimationSpeed);
	if (!bMainWindowOpened)
		return;

	const ImVec2 vecScreenSize = io.DisplaySize;
	const float flBackgroundAlpha = animMenuDimBackground.GetValue(1.f);
	flDpiScale = D::CalculateDPI(C_GET(int, Vars.nDpiScale));

	// @note: we call this every frame because we utilizing rainbow color as well! however it's not really performance friendly?
	UpdateStyle(&style);


	if (flBackgroundAlpha > 0.f)
	{
		if (C_GET(unsigned int, Vars.bMenuAdditional) & MENU_ADDITION_DIM_BACKGROUND)
			D::AddDrawListRect(ImGui::GetBackgroundDrawList(), ImVec2(0, 0), vecScreenSize, C_GET(ColorPickerVar_t, Vars.colPrimtv1).colValue.Set<COLOR_A>(125 * flBackgroundAlpha), DRAW_RECT_FILLED);

		if (C_GET(unsigned int, Vars.bMenuAdditional) & MENU_ADDITION_BACKGROUND_PARTICLE)
			menuParticle.Render(ImGui::GetBackgroundDrawList(), vecScreenSize, flBackgroundAlpha);
	}


	// handle main window get out of screen bound
	// @note: we call this here so it will override the previous SetNextWindowPos
	if (ImGuiWindow* pMainWindow = ImGui::FindWindowByName(CS_XOR("Xenon")); pMainWindow != nullptr)
	{
		bool bRequireClamp = false;
		ImVec2 vecWindowPos = pMainWindow->Pos;
		if (pMainWindow->Pos.x < 0.0f)
		{
			bRequireClamp = true;
			vecWindowPos.x = 0.0f;
		}
		else if (pMainWindow->Size.x + pMainWindow->Pos.x > vecScreenSize.x)
		{
			bRequireClamp = true;
			vecWindowPos.x = vecScreenSize.x - pMainWindow->Size.x;
		}
		if (pMainWindow->Pos.y < 0.0f)
		{
			bRequireClamp = true;
			vecWindowPos.y = 0.0f;
		}
		else if (pMainWindow->Size.y + pMainWindow->Pos.y > vecScreenSize.y)
		{
			bRequireClamp = true;
			vecWindowPos.y = vecScreenSize.y - pMainWindow->Size.y;
		}

		if (bRequireClamp) // Necessary to prevent window from constantly undocking itself if docked.
			ImGui::SetNextWindowPos(vecWindowPos, ImGuiCond_Always);
	}


	static const CTab arrTabs[] = {
			{ "Ragebot", &T::RageBot },
			{ "Anti Aim", &T::AntiAim },
			{ "Legitbot", &T::LegitBot },
			{ "Players", &T::VisualsPlayers },
			{ "Weapons", &T::VisualsWeapons },
			{ "Grenades", &T::VisualsGrenades },
			{ "World", &T::VisualsWorld },
			{ "View", &T::VisualsView },
			{ "Main", &T::MiscellaneousMain },
			{ "Miscellaneous", &T::MiscellaneousInventory },
			{ "Miscellaneous", &T::MiscellaneousConfigs }
	};


	static ImVec2 vecMenuPos;
	static ImVec2 vecMenuSize;


	static bool settings_window = false;


	UpdateStyle();


	if (settings_window)
	{
		ImGui::SetNextWindowPos(ImVec2(vecScreenSize.x * 0.5f, vecScreenSize.y * 0.5f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(400 * flDpiScale, 300 * flDpiScale), ImGuiCond_Always);
		// render main window
		ImGui::Begin(CS_XOR("Settings"), &settings_window, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);
		{
			vecMenuPos = ImGui::GetWindowPos();
			vecMenuSize = ImGui::GetWindowSize();
			ImDrawList* pDrawList = ImGui::GetWindowDrawList();

			if (C_GET(unsigned int, Vars.bMenuAdditional) & MENU_ADDITION_GLOW)
				D::AddDrawListShadowRect(ImGui::GetBackgroundDrawList(), vecMenuPos, vecMenuPos + vecMenuSize, C_GET(ColorPickerVar_t, Vars.colAccent0).colValue, 64.f * flDpiScale, style.WindowRounding, ImDrawFlags_ShadowCutOutShapeBackground);

			static int nCurrentSettingSubTab = 0;
			ImGui::BeginChildSection(CS_XOR("Misc Settings"), ImVec2(0, ImGui::GetContentRegionAvail().y), true);
			{
				static int nSelectedColor = 0;

				ImGui::HotKey(CS_XOR("Menu key"), &C_GET(unsigned int, Vars.nMenuKey));

				if (ImGui::BeginCombo(CS_XOR("Dpi scale"), arrMiscDpiScale[C_GET(int, Vars.nDpiScale)]))
				{
					for (int i = 0; i < IM_ARRAYSIZE(arrMiscDpiScale); i++)
					{
						if (ImGui::Selectable(arrMiscDpiScale[i], (C_GET(int, Vars.nDpiScale) == i)))
							C_GET(int, Vars.nDpiScale) = i;
					}

					ImGui::EndCombo();
				}

				if (ImGui::Button(CS_XOR("Unload")))
				{
					bool* unload = &C_GET(bool, Vars.bUnloadCheat);
					*unload = true;
				}

				ImGui::MultiCombo(CS_XOR("Additional settings"), &C_GET(unsigned int, Vars.bMenuAdditional), arrMenuAddition, CS_ARRAYSIZE(arrMenuAddition));

				ImGui::SliderFloat(CS_XOR("Animation speed"), &C_GET(float, Vars.flAnimationSpeed), 1.f, 10.f);

				ImGui::SeparatorText(CS_XOR("Colors pallete"));

				ImGui::PushItemWidth(-1);

				if (ImGui::BeginListBox(CS_XOR("##themes.select"), CS_ARRAYSIZE(arrColors), 5))
				{
					for (std::size_t i = 0U; i < CS_ARRAYSIZE(arrColors); i++)
					{
						const char* szColorName = arrColors[i].first;

						if (ImGui::Selectable(szColorName, (i == nSelectedColor)))
							nSelectedColor = (int)i;
					}

					ImGui::EndListBox();
				}

				ImGui::ColorEdit4(CS_XOR("##themes.picker"), &C_GET(ColorPickerVar_t, arrColors[nSelectedColor].second), ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_DisplayRGB);
				ImGui::PopItemWidth();
			}
			ImGui::EndChildSection();
		}
		ImGui::End();
	}



	/*    */



	ImGui::SetNextWindowPos(ImVec2(vecScreenSize.x * 0.5f, vecScreenSize.y * 0.5f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(900 * flDpiScale, 624 * flDpiScale), ImGuiCond_Always);
	// render main window
	ImGui::Begin(CS_XOR("Xenon"), &bMainWindowOpened, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);
	{
		vecMenuSize = ImGui::GetWindowSize();

		ImVec2 P1, P2;
		const auto& p = ImGui::GetWindowPos();
		const auto& pWindowDrawList = ImGui::GetWindowDrawList();
		const auto& pBackgroundDrawList = ImGui::GetBackgroundDrawList();
		const auto& pForegroundDrawList = ImGui::GetForegroundDrawList();

	

		if (C_GET(unsigned int, Vars.bMenuAdditional) & MENU_ADDITION_GLOW)
			D::AddDrawListShadowRect(ImGui::GetBackgroundDrawList(), p, p + vecMenuSize, C_GET(ColorPickerVar_t, Vars.colAccent0).colValue, 64.f * flDpiScale, style.WindowRounding, ImDrawFlags_ShadowCutOutShapeBackground);


		pBackgroundDrawList->AddRectFilled(p, ImVec2(905 * flDpiScale + p.x, 624 * flDpiScale + p.y), ImColor(9, 9, 9, 210), 10); // Background

		pWindowDrawList->AddRectFilled(ImVec2(189.000f * flDpiScale + p.x, 75.000f * flDpiScale + p.y), ImVec2(903 * flDpiScale + p.x, 76 * flDpiScale + p.y), ImColor(25, 25, 25, 180), 10); // bar line


		ImGui::SetCursorPos(ImVec2(800 * flDpiScale, 21 * flDpiScale));

		if (ImGui::OptButton("L", ImVec2(30 * flDpiScale, 30 * flDpiScale), false));

		ImGui::SameLine(840 * flDpiScale);

		if (ImGui::OptButton("B", ImVec2(30 * flDpiScale, 30 * flDpiScale), true))
			settings_window = !settings_window;


		pWindowDrawList->AddRectFilled(p + ImVec2(5, 5), ImVec2(190 * flDpiScale + p.x, 624 * flDpiScale + p.y - 5), ImGui::GetColorU32(ImGuiCol_ChildBg), 10, ImDrawFlags_RoundCornersLeft); // bar line


		const int vtx_idx_1 = pWindowDrawList->VtxBuffer.Size;

		pWindowDrawList->AddText(FONT::ico_main_logo, 40.f * flDpiScale, ImVec2(5.000f * flDpiScale + p.x, 15.000f * flDpiScale + p.y), ImColor(1.0f, 1.0f, 1.0f, 0.7f), "U");
		pWindowDrawList->AddText(FONT::pCheatLogo, 35.f * flDpiScale, ImVec2(70.000f * flDpiScale + p.x, 15.000f * flDpiScale + p.y), ImColor(0.60f, 0.60f, 0.60f, 0.70f), "Xenon");
		//pWindowDrawList->AddText(FONT::pCheatLogo, 35.f * flDpiScale, ImVec2(20.000f * flDpiScale + p.x, 15.000f * flDpiScale + p.y), ImColor(0.60f, 0.60f, 0.60f, 0.70f), "Xenon");
		//pWindowDrawList->AddText(FONT::pCheatLogo, 35.f * flDpiScale, ImVec2(20.000f * flDpiScale + p.x, 15.000f * flDpiScale + p.y), ImColor(0.60f, 0.60f, 0.60f, 0.70f), "Xenon");
		pWindowDrawList->AddRectFilled(ImVec2(70.000f * flDpiScale + p.x, 51.000f * flDpiScale + p.y), ImVec2(180 * flDpiScale + p.x, 52 * flDpiScale + p.y), ImColor(0.60f, 0.60f, 0.60f, 0.70f), 10); // bar line

		const int vtx_idx_2 = pWindowDrawList->VtxBuffer.Size;


		ImGui::ShadeVertsLinearColorGradientKeepAlpha(pWindowDrawList, vtx_idx_1, vtx_idx_2, ImVec2(p.x, p.y), ImVec2(200 * flDpiScale + p.x, 20 * flDpiScale + p.y), ImColor(0.25f, 0.25f, 0.25f, 0.50f), ImColor(0.60f, 0.60f, 0.60f, 1.00f));




		ImGui::SetCursorPosY(80 * flDpiScale);


		// "D", "Players", flDpiScale, nCurrentMainTab == 3


		if (ImGui::TabButton("N", "RageBot", ImVec2(190 * flDpiScale, 40 * flDpiScale)) && nCurrentMainTab != 0)
		{
			nCurrentMainTab = 0;
			active = true;
		}

		if (ImGui::TabButton("Q", "AntiAim", ImVec2(190 * flDpiScale, 40 * flDpiScale)) && nCurrentMainTab != 1)
		{
			nCurrentMainTab = 1;
			active = true;
		}

		if (ImGui::TabButton("P", "LegitBot", ImVec2(190 * flDpiScale, 40 * flDpiScale)) && nCurrentMainTab != 2)
		{
			nCurrentMainTab = 2;
			active = true;
		}


		if (ImGui::TabButton("I", "Visuals", ImVec2(190 * flDpiScale, 40)) && nCurrentMainTab != 3)
		{
			nCurrentMainTab = 3;
			active = true;
		}

		if (ImGui::TabButton("O", "Misc", ImVec2(190 * flDpiScale, 40 * flDpiScale)) && nCurrentMainTab != 4)
		{
			nCurrentMainTab = 4;
			active = true;
		}

		/*if (ImGui::TabButton("R", "PlayerList", ImVec2(190, 40)) && nCurrentMainTab != 5)
		{
			nCurrentMainTab = 5;
			active = true;
		}

		if (ImGui::TabButton("T", "Skins", ImVec2(190, 40)) && nCurrentMainTab != 6)
		{
			nCurrentMainTab = 6;
			active = true;
		}

		if (ImGui::TabButton("J", "Lua", ImVec2(190, 40)) && nCurrentMainTab != 7)
		{
			nCurrentMainTab = 7;
			active = true;
		}*/

		if (ImGui::TabButton("S", "Config", ImVec2(190, 40)) && nCurrentMainTab != 8)
		{
			nCurrentMainTab = 8;
			active = true;
		}


		if (active)
		{
			if (size_child <= 10)
				size_child += 1 / ImGui::GetIO().Framerate * 60.f;
			else
			{
				active = false;
			};
		}
		else
		{
			if (size_child >= 0)
				size_child -= 1 / ImGui::GetIO().Framerate * 60.f;
		}



		pWindowDrawList->AddCircleFilled(ImVec2(57.000f * flDpiScale + p.x, 570.000f * flDpiScale + p.y), 20.000f * flDpiScale, ImColor(10, 9, 10, 255), 30);

		pWindowDrawList->AddCircle(ImVec2(57.000f * flDpiScale + p.x, 570.000f * flDpiScale + p.y), 22.000f * flDpiScale, ImColor(20, 19, 20, 255), 30, 4.000f);


		const int vtx_idx_3 = pWindowDrawList->VtxBuffer.Size;

		// Account
		pWindowDrawList->AddText(FONT::pCheatLogo, 19.f * flDpiScale, ImVec2(90.000f * flDpiScale + p.x, 555.000f * flDpiScale + p.y), ImColor(0.40f, 0.40f, 0.40f, 0.50f), "Username\nLifetime");

		const int vtx_idx_4 = pWindowDrawList->VtxBuffer.Size;

		ImGui::ShadeVertsLinearColorGradientKeepAlpha(pWindowDrawList, vtx_idx_3, vtx_idx_4, ImVec2(97.000f * flDpiScale + p.x, 547.000f * flDpiScale + p.y), ImVec2(200.000f * flDpiScale + p.x, 567.000f * flDpiScale + p.y), ImColor(0.35f, 0.35f, 0.35f, 0.50f), ImColor(0.90f, 0.90f, 0.90f, 1.00f));



		ImGui::SetCursorPos(ImVec2(203 * flDpiScale, 88 * flDpiScale - size_child * flDpiScale));


		switch (nCurrentMainTab) {
		case 0:
			T::RageBot();
			break;

		case 1:
			T::AntiAim();
			break;

		case 2:
			T::LegitBot();
			break;

		case 3:
			T::Visuals();
			break;

		case 4:
			T::Miscellaneous();
			break;

		case 5:
			// Player List
			break;

		case 6:
			// Skins
			break;

		case 7:
			// Lue
			break;

		case 8:
			T::Configs();
			break;

		default:
			break;
		}


		/*if (nCurrentMainTab <= sizeof(arrTabs))
		{
			if (arrTabs[nCurrentMainTab].pRenderFunction != nullptr)
				arrTabs[nCurrentMainTab].pRenderFunction();
		}*/

		/* ImGui::SetCursorPos(ImVec2(10, 10 * flDpiScale));
		if (ImGui::Button(CS_XOR("Save"), ImVec2(50 * flDpiScale, 15 * flDpiScale)))
			C::SaveFile(T::nSelectedConfig);

		ImGui::SetCursorPos(ImVec2(10 + 50 * flDpiScale + 10, 10 * flDpiScale));
		if (ImGui::Button(CS_XOR("Settings"), ImVec2(50 * flDpiScale, 15 * flDpiScale)))
			settings_window = !settings_window;


		ImGui::Spacing();

		if (nCurrentMainTab <= sizeof(arrTabs))
		{
			if (arrTabs[nCurrentMainTab].pRenderFunction != nullptr)
				arrTabs[nCurrentMainTab].pRenderFunction();
		}*/
	}
	ImGui::End();



	/*   */



	/* ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 10));

	
	ImGui::SetNextWindowPos(ImVec2(vecMenuPos.x - 130 * flDpiScale, vecMenuPos.y), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(130 * flDpiScale, 350 * flDpiScale), ImGuiCond_Always);
	ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);
	{
		ImVec2 vecLeftMenuPos = ImGui::GetWindowPos();
		ImVec2 vecLeftMenuSize = ImGui::GetWindowSize();

		ImDrawList* pDrawList = ImGui::GetWindowDrawList();

		if (C_GET(unsigned int, Vars.bMenuAdditional) & MENU_ADDITION_GLOW)
			D::AddDrawListShadowRect(ImGui::GetBackgroundDrawList(), vecLeftMenuPos, vecLeftMenuPos + vecLeftMenuSize, C_GET(ColorPickerVar_t, Vars.colAccent0).colValue, 64.f * flDpiScale, style.WindowRounding, ImDrawFlags_ShadowCutOutShapeBackground);

		static ImVec2 vecTitleSize = FONT::pCheatLogo->CalcTextSizeA(25 * flDpiScale, 100, -1.f, CS_XOR("XENON"));
		float width = ImGui::CalcItemWidth();
		pDrawList->AddText(FONT::pCheatLogo, 25 * flDpiScale, ImVec2(vecLeftMenuPos.x + style.WindowPadding.x + (130 / 2) - vecTitleSize.x / 2 - 2, vecLeftMenuPos.y + style.WindowPadding.y / 2 - 2), C_GET(ColorPickerVar_t, Vars.colAccent0).colValue.GetU32(), CS_XOR("XENON"));
		pDrawList->AddText(FONT::pCheatLogo, 25 * flDpiScale, ImVec2(vecLeftMenuPos.x + style.WindowPadding.x + (130 / 2) - vecTitleSize.x / 2, vecLeftMenuPos.y + style.WindowPadding.y / 2), ImGui::GetColorU32(ImGuiCol_Text), CS_XOR("XENON"));


		ImGui::NewLine();
		ImGui::NewLine();

		ImGui::BeginGroup();
		{
			ImGui::SetCursorPosX(25);
			ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_Text), "Aimbot");
			if (ImGui::TabButton("A", "Ragebot", flDpiScale, nCurrentMainTab == 0))
				nCurrentMainTab = 0;
			if (ImGui::TabButton("B", "Anti Aim", flDpiScale, nCurrentMainTab == 1))
				nCurrentMainTab = 1;
			if (ImGui::TabButton("C", "Legitbot", flDpiScale, nCurrentMainTab == 2))
				nCurrentMainTab = 2;

			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();

			ImGui::SetCursorPosX(25);
			ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_Text), "Visuals");
			if (ImGui::TabButton("D", "Players", flDpiScale, nCurrentMainTab == 3))
				nCurrentMainTab = 3;
			if (ImGui::TabButton("E", "Weapons", flDpiScale, nCurrentMainTab == 4))
				nCurrentMainTab = 4;
			if (ImGui::TabButton("F", "Grenades", flDpiScale, nCurrentMainTab == 5))
				nCurrentMainTab = 5;
			if (ImGui::TabButton("G", "World", flDpiScale, nCurrentMainTab == 6))
				nCurrentMainTab = 6;
			if (ImGui::TabButton("H", "View", flDpiScale, nCurrentMainTab == 7))
				nCurrentMainTab = 7;

			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();

			ImGui::SetCursorPosX(25);
			ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_Text), "Miscellaneous");
			if (ImGui::TabButton("I", "Main", flDpiScale, nCurrentMainTab == 8))
				nCurrentMainTab = 8;
			if (ImGui::TabButton("J", "Inventory", flDpiScale, nCurrentMainTab == 9))
				nCurrentMainTab = 9;
			if (ImGui::TabButton("L", "Configs", flDpiScale, nCurrentMainTab == 10))
				nCurrentMainTab = 10;
		}
		ImGui::EndGroup();
	}
	ImGui::End();


	ImGui::PopStyleColor();*/
}

void MENU::RenderOverlayPreviewWindow()
{
	using namespace F::VISUALS::OVERLAY;

	ImGuiStyle& style = ImGui::GetStyle();
	// @note: call this function inside rendermainwindow, else expect a crash...
	const ImVec2 vecMenuPos = ImGui::GetWindowPos();
	const ImVec2 vecMenuSize = ImGui::GetWindowSize();

	const ImVec2 vecOverlayPadding = ImVec2(30.f * flDpiScale, 50.f * flDpiScale);

	ImGui::SetNextWindowPos(ImVec2(vecMenuPos.x + vecMenuSize.x + style.WindowPadding.x, vecMenuPos.y), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(240 * flDpiScale, vecMenuSize.y), ImGuiCond_Always);
	ImGui::Begin(CS_XOR("Preview"), &bMainWindowOpened, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground);
	{
		const ImVec2 vecWindowPos = ImGui::GetWindowPos();
		const ImVec2 vecWindowSize = ImGui::GetWindowSize();

		const auto& p = ImGui::GetWindowPos();
		const auto& pBackgroundDrawList = ImGui::GetBackgroundDrawList();

		if (C_GET(unsigned int, Vars.bMenuAdditional) & MENU_ADDITION_GLOW)
			D::AddDrawListShadowRect(ImGui::GetBackgroundDrawList(), p, p + ImVec2(240 * flDpiScale, vecMenuSize.y), C_GET(ColorPickerVar_t, Vars.colAccent0).colValue, 64.f * flDpiScale, style.WindowRounding, ImDrawFlags_ShadowCutOutShapeBackground);

		pBackgroundDrawList->AddRectFilled(p, ImVec2(240 * flDpiScale + p.x, vecMenuSize.y + p.y), ImColor(9, 9, 9, 180), 10, ImDrawFlags_RoundCornersAll); // Background


		ImDrawList* pDrawList = ImGui::GetWindowDrawList();
		Context_t context;

		ImVec4 vecBox = {
			vecWindowPos.x + vecOverlayPadding.x,
			vecWindowPos.y + vecOverlayPadding.y,
			vecWindowPos.x + vecWindowSize.x - vecOverlayPadding.x,
			vecWindowPos.y + vecWindowSize.y - vecOverlayPadding.y
		};

		if (const auto& boxOverlayConfig = C_GET(FrameOverlayVar_t, Vars.overlayBox); boxOverlayConfig.bEnable)
		{
			const bool bHovered = context.AddBoxComponent(pDrawList, vecBox, 1, boxOverlayConfig.flThickness, boxOverlayConfig.flRounding, boxOverlayConfig.colPrimary, boxOverlayConfig.colOutline);
			if (bHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				ImGui::OpenPopup(CS_XOR("context##box.component"));
		}

		if (const auto& nameOverlayConfig = C_GET(TextOverlayVar_t, Vars.overlayName); nameOverlayConfig.bEnable)
			context.AddComponent(new CTextComponent(true, SIDE_TOP, DIR_TOP, FONT::pVisual, CS_XOR("Xenon"), Vars.overlayName));

		if (const auto& healthOverlayConfig = C_GET(BarOverlayVar_t, Vars.overlayHealthBar); healthOverlayConfig.bEnable)
		{
			const float flFactor = M_SIN(ImGui::GetTime() * 5.f) * 0.55f + 0.45f;
			context.AddComponent(new CBarComponent(true, SIDE_LEFT, vecBox, flFactor, Vars.overlayHealthBar));
		}

		if (const auto& armorOverlayConfig = C_GET(BarOverlayVar_t, Vars.overlayArmorBar); armorOverlayConfig.bEnable)
		{
			const float flArmorFactor = M_SIN(ImGui::GetTime() * 5.f) * 0.55f + 0.45f;
			context.AddComponent(new CBarComponent(false, SIDE_BOTTOM, vecBox, flArmorFactor, Vars.overlayArmorBar));
		}

		// only render context preview if overlay is enabled
		context.Render(pDrawList, vecBox);

		if (ImGui::BeginPopup(CS_XOR("context##box.component"), ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, 0));

			ImGui::ColorEdit4(CS_XOR("primary color##box.component"), &C_GET(FrameOverlayVar_t, Vars.overlayBox).colPrimary);
			ImGui::ColorEdit4(CS_XOR("outline color##box.component"), &C_GET(FrameOverlayVar_t, Vars.overlayBox).colOutline);
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.75f);
			ImGui::SliderFloat(CS_XOR("thickness##box.component"), &C_GET(FrameOverlayVar_t, Vars.overlayBox).flThickness, 1.f, 5.f, CS_XOR("%.1f"), ImGuiSliderFlags_AlwaysClamp);
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.75f);
			ImGui::SliderFloat(CS_XOR("rounding##box.component"), &C_GET(FrameOverlayVar_t, Vars.overlayBox).flRounding, 1.f, 5.f, CS_XOR("%.1f"), ImGuiSliderFlags_AlwaysClamp);

			ImGui::PopStyleVar();

			ImGui::EndPopup();
		}
	}
	ImGui::End();
}

void MENU::RenderWatermark()
{
	if (!C_GET(bool, Vars.bWatermark) || !bMainWindowOpened)
		return;

	ImGuiStyle& style = ImGui::GetStyle();

	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.f, 0.f, 0.f, 0.03f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.03f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.03f));
	ImGui::PushFont(FONT::pExtra);
	ImGui::BeginMainMenuBar();
	{
		ImGui::Dummy(ImVec2(1, 1));

#ifdef _DEBUG
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), CS_XOR("debug"));
#endif
		if (CRT::StringString(GetCommandLineW(), CS_XOR(L"-insecure")) != nullptr)
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), CS_XOR("insecure"));

		if (I::Engine->IsInGame())
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), CS_XOR("in-game"));

		static ImVec2 vecNameSize = ImGui::CalcTextSize(CS_XOR("Xenon | " __DATE__ " " __TIME__));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - vecNameSize.x - style.FramePadding.x);
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), CS_XOR("Xenon | " __DATE__ " " __TIME__));
	}
	ImGui::EndMainMenuBar();
	ImGui::PopFont();
	ImGui::PopStyleColor(3);
}


inline const char* GetExtractedWeaponNameMenu(C_CSWeaponBase* weapon)
{
	if (!weapon)
		return "";

	auto weapon_data = weapon->GetWeaponVData();
	if (!weapon_data)
		return "";

	const char* szWeaponName = weapon_data->GetName();
	const char* weaponPrefix = ("weapon_");
	const char* weaponNameStart = strstr(szWeaponName, weaponPrefix);
	const char* extractedWeaponName = weaponNameStart ? weaponNameStart + strlen(weaponPrefix) : szWeaponName;

	return extractedWeaponName;
}


int GetWeaponIndex()
{
	auto active_weapon = SDK::LocalPawn->GetActiveWeaponFromPlayer();
	if (active_weapon != nullptr)
	{
		auto pWeaponVData = active_weapon->GetWeaponVData();
		if (pWeaponVData != nullptr)
		{
			const char* extractedWeaponName = GetExtractedWeaponNameMenu(active_weapon);

			bool has_awp = strcmp(extractedWeaponName, CS_XOR("awp")) == 0;
			bool has_heavy_pistols = strcmp(extractedWeaponName, CS_XOR("revolver")) == 0 || strcmp(extractedWeaponName, CS_XOR("deagle")) == 0;
			bool has_scout = strcmp(extractedWeaponName, CS_XOR("ssg08")) == 0;

			if (has_awp)
			{
				return 6;
			}

			else if (has_scout)
			{
				return 5;
			}

			else if (has_heavy_pistols)
			{
				return 2;
			}

			else if (pWeaponVData->GetWeaponType() == WEAPONTYPE_PISTOL && !has_heavy_pistols)
			{
				return 1;
			}

			else if (pWeaponVData->GetWeaponType() == WEAPONTYPE_MACHINEGUN)
			{
				return 3;
			}

			else if (pWeaponVData->GetWeaponType() == WEAPONTYPE_RIFLE)
			{
				return 3;
			}

			else if (pWeaponVData->GetWeaponType() == WEAPONTYPE_SNIPER_RIFLE)
			{
				return 4;
			}

			else
			{
				return 0;
			}
		}
	}

	return -1;
}

//  client.dll   48 85 C9 74 32 48 8B 41 10 48 85 C0 74 29 44
unsigned int get_handle_entity(C_CSPlayerPawn* entity)
{
	if (!entity)
		return -1;

	using fn_get_handle_entity = int(__fastcall*)(C_CSPlayerPawn*);
	static fn_get_handle_entity fn = reinterpret_cast<fn_get_handle_entity>(MEM::FindPattern(CLIENT_DLL, "48 85 C9 74 32 48 8B 41 10 48 85 C0 74 29 44"));


	return fn(entity);
}

void MENU::RenderKeybinds()
{
	if (!C_GET(bool, Vars.bKeybindList))
		return;

	ImGuiIO& io = ImGui::GetIO();
	ImGuiStyle& style = ImGui::GetStyle();

	// @note: we call this every frame because we utilizing rainbow color as well! however it's not really performance friendly?
	UpdateStyle(&style);

	const float flBackgroundAlpha = animMenuDimBackground.GetValue(1.f);


	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(150, -1));
	ImGui::Begin("Binds", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
	{
		ImVec2 vecLeftMenuPos = ImGui::GetWindowPos();
		ImVec2 vecLeftMenuSize = ImGui::GetWindowSize();

		if (C_GET(unsigned int, Vars.bMenuAdditional) & MENU_ADDITION_GLOW)
			D::AddDrawListShadowRect(ImGui::GetBackgroundDrawList(), vecLeftMenuPos, vecLeftMenuPos + vecLeftMenuSize, C_GET(ColorPickerVar_t, Vars.colAccent0).colValue, 64.f * flDpiScale, style.WindowRounding, ImDrawFlags_ShadowCutOutShapeBackground);


		if (C_GET(bool, Vars.bLegitbot))
			ImGui::Text(CS_XOR("Aim Assist"));

		if (C_GET(bool, Vars.bThirdPerson))
			ImGui::Text(CS_XOR("Third person"));

		if (C_GET(bool, Vars.bAutoFire))
			ImGui::Text(CS_XOR("Autofire"));

		if (IPT::GetBindState(C_GET(KeyBind_t, Vars.nBodyAim)))
			ImGui::Text(CS_XOR("Body Aim"));

		try
		{
			int weaponIndex = GetWeaponIndex();
			if (weaponIndex != -1)
			{
				if (C_GET(bool, Vars.bEnableOverrideDamage))
					ImGui::Text(std::format("Override damage [{}]", C_GET_ARRAY(int, 7, Vars.iOverrideDamage, weaponIndex)).c_str());
				else
					ImGui::Text(std::format("Min damage [{}]", C_GET_ARRAY(int, 7, Vars.iMinDamage, weaponIndex)).c_str());
			}
		}
		catch (...)
		{ }

		if (C_GET(KeyBind_t, Vars.nAutoPeekKey).bEnable)
			ImGui::Text(CS_XOR("Auto Peek"));
	}

	ImGui::End();
	ImGui::PopStyleVar();
}

void MENU::RenderSpectators()
{
	if (!C_GET(bool, Vars.bSpectatorList))
		return;

	CCSPlayerController* pLocalController = CCSPlayerController::GetLocalPlayerController();
	if (pLocalController == nullptr)
		return;

	C_CSPlayerPawn* pLocalPawn = I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(pLocalController->GetRefEHandle());
	if (pLocalPawn == nullptr)
		return;

	ImGuiStyle& style = ImGui::GetStyle();

	// @note: we call this every frame because we utilizing rainbow color as well! however it's not really performance friendly?
	UpdateStyle(&style);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(150, -1));
	ImGui::Begin("Spectators", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
	{
		ImVec2 vecLeftMenuPos = ImGui::GetWindowPos();
		ImVec2 vecLeftMenuSize = ImGui::GetWindowSize();

		if (C_GET(unsigned int, Vars.bMenuAdditional) & MENU_ADDITION_GLOW)
			D::AddDrawListShadowRect(ImGui::GetBackgroundDrawList(), vecLeftMenuPos, vecLeftMenuPos + vecLeftMenuSize, C_GET(ColorPickerVar_t, Vars.colAccent0).colValue, 64.f * flDpiScale, style.WindowRounding, ImDrawFlags_ShadowCutOutShapeBackground);


		auto handle_local = get_handle_entity(pLocalPawn);

		const auto& entity_list = F::ANTIAIM::get("CCSPlayerController");
		for (auto& entity : entity_list)
		{
			// Get the entity
			C_BaseEntity* pEntity = I::GameResourceService->pGameEntitySystem->Get(entity->GetRefEHandle());
			if (pEntity == nullptr)
				continue;

			// Get the class info
			SchemaClassInfoData_t* pClassInfo = nullptr;
			pEntity->GetSchemaClassInfo(&pClassInfo);
			if (pClassInfo == nullptr)
				continue;

			// Get the hashed name
			const FNV1A_t uHashedName = FNV1A::Hash(pClassInfo->szName);
			if (uHashedName != FNV1A::HashConst("CCSPlayerController"))
				continue;

			// Cast to player controller
			CCSPlayerController* pPlayer = reinterpret_cast<CCSPlayerController*>(pEntity);
			if (pPlayer == nullptr || pPlayer == pLocalController)
				continue;

			C_CSPlayerPawn* pawn = I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(pPlayer->GetPawnHandle());
			if (!pawn)
				continue;

			if (pawn == pLocalPawn)
				continue;

			if (pPlayer->IsPawnAlive())
				continue;



			auto obs_service = pawn->GetObserverServices();
			if (!obs_service)
				continue;



			C_BaseEntity* obs_target_pawn = I::GameResourceService->pGameEntitySystem->Get(obs_service->GetObserverTarget());
			if (!obs_target_pawn)
				continue;


			/*L_PRINT(LOG_INFO) << obs_target_pawn->GetRefEHandle().GetEntryIndex();
			L_PRINT(LOG_INFO) << obs_target_pawn->GetRefEHandle().ToInt();
			L_PRINT(LOG_INFO) << "";
			L_PRINT(LOG_INFO) << pLocalPawn->GetRefEHandle().GetEntryIndex();
			L_PRINT(LOG_INFO) << pLocalPawn->GetRefEHandle().ToInt();
			L_PRINT(LOG_INFO) << pLocalController->GetObserverPawnHandle().GetEntryIndex();
			L_PRINT(LOG_INFO) << pLocalController->GetObserverPawnHandle().ToInt();
			L_PRINT(LOG_INFO) << "";
			L_PRINT(LOG_INFO) << "";*/

			if (obs_target_pawn->GetRefEHandle().GetEntryIndex() != pLocalPawn->GetRefEHandle().GetEntryIndex())
				continue;

			ImGui::Text(pPlayer->GetPlayerName());
		}
	}
	ImGui::End();
	ImGui::PopStyleVar();
}

void MENU::UpdateStyle(ImGuiStyle* pStyle)
{
	ImGuiStyle& style = pStyle != nullptr ? *pStyle : ImGui::GetStyle();

	//style.WindowPadding = ImVec2(15, 20);
	//style.WindowRounding = 0;
	/*style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.WindowRounding = 5;
	style.FramePadding = ImVec2(5, 5);
	style.FrameRounding = 4;
	style.ItemSpacing = ImVec2(12, 8);
	style.ItemInnerSpacing = ImVec2(8, 6);
	style.IndentSpacing = 25;
	style.ScrollbarSize = 15;
	style.ScrollbarRounding = 9;
	style.GrabMinSize = 5;
	style.GrabRounding = 3;*/

	style.Colors[ImGuiCol_Text] = C_GET(ColorPickerVar_t, Vars.colPrimtv0).colValue.GetVec4(1.f); // primtv 0
	style.Colors[ImGuiCol_TextDisabled] = C_GET(ColorPickerVar_t, Vars.colPrimtv2).colValue.GetVec4(0.85f); // primtv 2
	style.Colors[ImGuiCol_TextSelectedBg] = C_GET(ColorPickerVar_t, Vars.colAccent1).colValue.GetVec4(0.85f); // accent 1

	style.Colors[ImGuiCol_WindowBg] = C_GET(ColorPickerVar_t, Vars.colPrimtv3).colValue.GetVec4(0.95f); // primtv 3
	style.Colors[ImGuiCol_ChildBg] = C_GET(ColorPickerVar_t, Vars.colPrimtv1).colValue.GetVec4(.6f); // primtv 1
	style.Colors[ImGuiCol_PopupBg] = C_GET(ColorPickerVar_t, Vars.colPrimtv1).colValue.GetVec4(1.0f); // primtv 1
	style.Colors[ImGuiCol_WindowShadow] = C_GET(ColorPickerVar_t, Vars.colAccent2).colValue.GetVec4(0.80f); // accent 3

	style.Colors[ImGuiCol_Border] = C_GET(ColorPickerVar_t, Vars.colPrimtv4).colValue.GetVec4(0.10f); // primtv 4
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // clear

	style.Colors[ImGuiCol_FrameBg] = C_GET(ColorPickerVar_t, Vars.colPrimtv3).colValue.GetVec4(1.0f); // primtv 3
	style.Colors[ImGuiCol_FrameBgHovered] = C_GET(ColorPickerVar_t, Vars.colAccent1).colValue.GetVec4(0.8f); // accent 1
	style.Colors[ImGuiCol_FrameBgActive] = C_GET(ColorPickerVar_t, Vars.colAccent2).colValue.GetVec4(0.6f); // accent 0

	style.Colors[ImGuiCol_TitleBg] = C_GET(ColorPickerVar_t, Vars.colAccent0).colValue.GetVec4(0.20f); // accent 0
	style.Colors[ImGuiCol_TitleBgActive] = C_GET(ColorPickerVar_t, Vars.colAccent2).colValue.GetVec4(0.50f); // accent 1
	style.Colors[ImGuiCol_TitleBgCollapsed] = C_GET(ColorPickerVar_t, Vars.colAccent1).colValue.GetVec4(0.20f); // accent 1

	style.Colors[ImGuiCol_MenuBarBg] = C_GET(ColorPickerVar_t, Vars.colPrimtv1).colValue.GetVec4(0.70f); // primtv 1

	style.Colors[ImGuiCol_ScrollbarBg] = C_GET(ColorPickerVar_t, Vars.colPrimtv3).colValue.GetVec4(0.30f); // primtv 3
	style.Colors[ImGuiCol_ScrollbarGrab] = C_GET(ColorPickerVar_t, Vars.colAccent2).colValue.GetVec4(1.00f); // accent 3
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = C_GET(ColorPickerVar_t, Vars.colAccent1).colValue.GetVec4(0.90f); // primtv 5
	style.Colors[ImGuiCol_ScrollbarGrabActive] = C_GET(ColorPickerVar_t, Vars.colAccent2).colValue.GetVec4(0.50f); // primtv 2

	style.Colors[ImGuiCol_CheckMark] = C_GET(ColorPickerVar_t, Vars.colAccent0).colValue.GetVec4(1.00f); // primtv 0

	style.Colors[ImGuiCol_SliderGrab] = C_GET(ColorPickerVar_t, Vars.colAccent0).colValue.GetVec4(1.0f); // accent 0
	style.Colors[ImGuiCol_SliderGrabActive] = C_GET(ColorPickerVar_t, Vars.colAccent1).colValue.GetVec4(0.8f); // accent 1

	style.Colors[ImGuiCol_Button] = C_GET(ColorPickerVar_t, Vars.colPrimtv3).colValue.GetVec4(1.0f); // primtv 3
	style.Colors[ImGuiCol_ButtonHovered] = C_GET(ColorPickerVar_t, Vars.colAccent1).colValue.GetVec4(1.00f); // primtv 5
	style.Colors[ImGuiCol_ButtonActive] = C_GET(ColorPickerVar_t, Vars.colAccent2).colValue.GetVec4(1.00f); // accent 0

	style.Colors[ImGuiCol_Header] = C_GET(ColorPickerVar_t, Vars.colAccent0).colValue.GetVec4(1.00f); // accent 0
	style.Colors[ImGuiCol_HeaderHovered] = C_GET(ColorPickerVar_t, Vars.colAccent1).colValue.GetVec4(1.00f); // primtv 5
	style.Colors[ImGuiCol_HeaderActive] = C_GET(ColorPickerVar_t, Vars.colAccent2).colValue.GetVec4(1.0f); // primtv 3

	style.Colors[ImGuiCol_Separator] = C_GET(ColorPickerVar_t, Vars.colPrimtv3).colValue.GetVec4(1.0f); // primtv 3
	style.Colors[ImGuiCol_SeparatorHovered] = C_GET(ColorPickerVar_t, Vars.colAccent1).colValue.GetVec4(1.00f); // primtv 5
	style.Colors[ImGuiCol_SeparatorActive] = C_GET(ColorPickerVar_t, Vars.colAccent2).colValue.GetVec4(1.00f); // accent 0

	style.Colors[ImGuiCol_ResizeGrip] = C_GET(ColorPickerVar_t, Vars.colAccent0).colValue.GetVec4(1.00f); // accent 0
	style.Colors[ImGuiCol_ResizeGripHovered] = C_GET(ColorPickerVar_t, Vars.colAccent1).colValue.GetVec4(0.70f); // primtv 5
	style.Colors[ImGuiCol_ResizeGripActive] = C_GET(ColorPickerVar_t, Vars.colAccent2).colValue.GetVec4(1.0f); // accent 1

	style.Colors[ImGuiCol_Tab] = C_GET(ColorPickerVar_t, Vars.colPrimtv1).colValue.GetVec4(0.80f); // primtv 1
	style.Colors[ImGuiCol_TabHovered] = C_GET(ColorPickerVar_t, Vars.colAccent1).colValue.GetVec4(0.80f); // primtv 5
	style.Colors[ImGuiCol_TabActive] = C_GET(ColorPickerVar_t, Vars.colAccent2).colValue.GetVec4(0.70f); // accent 0
	style.Colors[ImGuiCol_TabUnfocused] = C_GET(ColorPickerVar_t, Vars.colAccent1).colValue.GetVec4(0.70f); // primtv 5
	style.Colors[ImGuiCol_TabUnfocusedActive] = C_GET(ColorPickerVar_t, Vars.colAccent2).colValue.GetVec4(0.60f); // accent 0

	style.Colors[ImGuiCol_PlotLines] = C_GET(ColorPickerVar_t, Vars.colAccent0).colValue.GetVec4(1.00f); // accent 0
	style.Colors[ImGuiCol_PlotLinesHovered] = C_GET(ColorPickerVar_t, Vars.colAccent0).colValue.GetVec4(0.50f); // accent 0
	style.Colors[ImGuiCol_PlotHistogram] = C_GET(ColorPickerVar_t, Vars.colAccent0).colValue.GetVec4(1.00f); // accent 0
	style.Colors[ImGuiCol_PlotHistogramHovered] = C_GET(ColorPickerVar_t, Vars.colAccent0).colValue.GetVec4(0.50f); // accent 0

	style.Colors[ImGuiCol_DragDropTarget] = C_GET(ColorPickerVar_t, Vars.colAccent2).colValue.GetVec4(0.80f); // accent 3

	style.Colors[ImGuiCol_ModalWindowDimBg] = C_GET(ColorPickerVar_t, Vars.colPrimtv4).colValue.GetVec4(0.25f); // primtv 4

	style.Colors[ImGuiCol_ControlBg] = C_GET(ColorPickerVar_t, Vars.colPrimtv2).colValue.GetVec4(1.0f); // primtv 3
	style.Colors[ImGuiCol_ControlBg] = C_GET(ColorPickerVar_t, Vars.colPrimtv4).colValue.GetVec4(1.0f); // primtv 5
	style.Colors[ImGuiCol_ControlBg] = C_GET(ColorPickerVar_t, Vars.colPrimtv1).colValue.GetVec4(0.10f); // primtv 2

	style.Colors[ImGuiCol_Triangle] = C_GET(ColorPickerVar_t, Vars.colPrimtv0).colValue.GetVec4(1.f); // accent 0

	C_GET(ColorPickerVar_t, Vars.colPrimtv0).UpdateRainbow(); // (text)
	C_GET(ColorPickerVar_t, Vars.colPrimtv1).UpdateRainbow(); // (background)
	C_GET(ColorPickerVar_t, Vars.colPrimtv2).UpdateRainbow(); // (disabled)
	C_GET(ColorPickerVar_t, Vars.colPrimtv3).UpdateRainbow(); // (control bg)
	C_GET(ColorPickerVar_t, Vars.colPrimtv4).UpdateRainbow(); // (border)

	C_GET(ColorPickerVar_t, Vars.colAccent0).UpdateRainbow(); // (main)
	C_GET(ColorPickerVar_t, Vars.colAccent1).UpdateRainbow(); // (dark)
	C_GET(ColorPickerVar_t, Vars.colAccent2).UpdateRainbow(); // (darker)

	// update animation speed
	style.AnimationSpeed = C_GET(float, Vars.flAnimationSpeed) / 10.f;
}


/*
			ImGui::ToggleButton(CS_XOR("Remove Legs"), &C_GET(bool, Vars.bRemoveLegs));
*/


#pragma region menu_tabs

void T::Render(const char* szTabBar, const CTab* arrTabs, const unsigned long long nTabsCount, int* nCurrentTab, ImGuiTabBarFlags flags)
{
	if (ImGui::BeginTabBar(szTabBar, flags))
	{
		for (std::size_t i = 0U; i < nTabsCount; i++)
		{
			// add tab
			if (ImGui::BeginTabItem(arrTabs[i].szName))
			{
				// set current tab index
				*nCurrentTab = (int)i;
				ImGui::EndTabItem();
			}
		}

		// render inner tab
		if (arrTabs[*nCurrentTab].pRenderFunction != nullptr)
			arrTabs[*nCurrentTab].pRenderFunction();

		ImGui::EndTabBar();
	}
}

void T::LegitBot()
{
	ImGui::BeginChildSection(CS_XOR("Aimbot"), ImVec2(339 * MENU::flDpiScale, 253 * MENU::flDpiScale), true);
	{
		ImGui::ToggleButton(CS_XOR("Enable"), &C_GET(bool, Vars.bLegitbot));
		ImGui::SliderInt(CS_XOR("Aim range"), &C_GET(int, Vars.iAimRange), 1.f, 135.f);
		ImGui::SliderFloat(CS_XOR("Smoothing"), &C_GET(float, Vars.flSmoothing), 1.f, 100.f, "%.2f");
	}
	ImGui::EndChildSection();

	ImGui::SetCursorPos(ImVec2(555 * MENU::flDpiScale, 88 * MENU::flDpiScale - size_child * MENU::flDpiScale));

	ImGui::BeginChildSection("Binds", ImVec2(339 * MENU::flDpiScale, 253 * MENU::flDpiScale), true);
	{
		ImGui::ToggleButton(CS_XOR("Always on##aimbot"), &C_GET(bool, Vars.bLegitbotAlwaysOn));
		ImGui::BeginDisabled(C_GET(bool, Vars.bLegitbotAlwaysOn));
		{
			ImGui::HotKey(CS_XOR("Toggle key"), &C_GET(unsigned int, Vars.nLegitbotActivationKey));
		}
		ImGui::EndDisabled();
	}
	ImGui::EndChildSection();
}

void T::RageBot()
{
	ImGui::SetCursorPos(ImVec2(339 * MENU::flDpiScale - 130 * MENU::flDpiScale, 15 * MENU::flDpiScale));

	float buttonWidth = 110;

	auto currentWeapon = C_GET(int, Vars.iWeaponTypeSelected);

	const char* weapons[7]{ CS_XOR("Default"), CS_XOR("Pistols"), CS_XOR("Heavy Pistols"), CS_XOR("Assult Rifles"), CS_XOR("Auto"), CS_XOR("Scout"), CS_XOR("Awp") };
	//ImGui::Combo(CS_XOR(" ##weapontypeselect"), &C_GET(int, Vars.iWeaponTypeSelected), weapons, IM_ARRAYSIZE(weapons), 6);

	for (int i = 0; i < IM_ARRAYSIZE(weapons) / 2; i++)
	{
		if (ImGui::Button(weapons[i], ImVec2(buttonWidth * MENU::flDpiScale, 30 * MENU::flDpiScale)))
			C_GET(int, Vars.iWeaponTypeSelected) = i;

		ImGui::SetCursorPos(ImVec2(339 * MENU::flDpiScale - 130 * MENU::flDpiScale + buttonWidth * MENU::flDpiScale * (i + 1) + 5 * (i + 1) * MENU::flDpiScale, 15 * MENU::flDpiScale));
	}

	for (int i = IM_ARRAYSIZE(weapons) / 2; i < IM_ARRAYSIZE(weapons); i++)
	{
		if (ImGui::Button(weapons[i], ImVec2(buttonWidth * MENU::flDpiScale, 30 * MENU::flDpiScale)))
			C_GET(int, Vars.iWeaponTypeSelected) = i;

		ImGui::SetCursorPos(ImVec2(339 * MENU::flDpiScale - 130 * MENU::flDpiScale + buttonWidth * MENU::flDpiScale * (i - (IM_ARRAYSIZE(weapons) / 2)) + 5 * (i - IM_ARRAYSIZE(weapons) / 2) * MENU::flDpiScale, 50 * MENU::flDpiScale));
	}


	ImGui::SetCursorPos(ImVec2(203 * MENU::flDpiScale, 88 * MENU::flDpiScale - size_child * MENU::flDpiScale));


	ImGui::BeginChildSection(CS_XOR("Main"), ImVec2(339 * MENU::flDpiScale, 253 * MENU::flDpiScale), true);
	{
		ImGui::ToggleButton(CS_XOR("Enable Ragebot##ragebot"), &C_GET(bool, Vars.bRageBot));
		ImGui::ToggleButton(CS_XOR("Silentaim"), &C_GET(bool, Vars.bSilentAim));
		ImGui::ToggleButton(CS_XOR("NoSpread"), &C_GET(bool, Vars.bNoSpread));
		ImGui::SliderInt(CS_XOR("FOV"), &C_GET(int, Vars.iRageAimRange), 1, 180);
		ImGui::ToggleButton(CS_XOR("Autowall"), &C_GET(bool, Vars.bAutoWall));
		ImGui::ToggleButton(CS_XOR("Auto Fire"), &C_GET(bool, Vars.bAutoFire));
		ImGui::NewLine();
		ImGui::HotKey(CS_XOR("Body Aim"), &C_GET(KeyBind_t, Vars.nBodyAim));
	}
	ImGui::EndChildSection();

	ImGui::SetCursorPos(ImVec2(555 * MENU::flDpiScale, 88 * MENU::flDpiScale - size_child * MENU::flDpiScale));

	ImGui::BeginChildSection(CS_XOR("Min damage"), ImVec2(339 * MENU::flDpiScale, 253 * MENU::flDpiScale), true);
	{
		ImGui::SliderInt(CS_XOR("Penetration Count"), &C_GET_ARRAY(int, 7, Vars.iPenetrationCount, currentWeapon), 1, 30);
		ImGui::HotKey(CS_XOR("Min Damage Key"), &C_GET(unsigned int, Vars.nOverrideDamageActivationKey));
		ImGui::SliderInt(CS_XOR("Override Damage"), &C_GET_ARRAY(int, 7, Vars.iOverrideDamage, currentWeapon), 1, 200);
		ImGui::SliderInt(CS_XOR("Min Damage"), &C_GET_ARRAY(int, 7, Vars.iMinDamage, currentWeapon), 1, 200);

		ImGui::ToggleButton(CS_XOR("Multipoint"), &C_GET_ARRAY(bool, 7, Vars.bMultiPoint, currentWeapon));
	}
	ImGui::EndChildSection();

	ImGui::SetCursorPos(ImVec2(203 * MENU::flDpiScale, 353 * MENU::flDpiScale - size_child * MENU::flDpiScale));

	ImGui::BeginChildSection(CS_XOR("Accuracy"), ImVec2(339 * MENU::flDpiScale, 258 * MENU::flDpiScale), true);
	{
		ImGui::SliderInt(CS_XOR("Hit Chance"), &C_GET_ARRAY(int, 7, Vars.iHitChance, currentWeapon), 0, 100);
		ImGui::MultiCombo(CS_XOR("Hitboxes"), &C_GET_ARRAY(unsigned int, 7, Vars.bRageHitboxes, currentWeapon), arrHitboxes, CS_ARRAYSIZE(arrHitboxes));
		//ImGui::HotKey(CS_XOR("Auto Fire Key"), &C_GET(unsigned int, Vars.nAutoFireKey));
		ImGui::Separator();
		ImGui::ToggleButton(CS_XOR("Auto Peek"), &C_GET(bool, Vars.bAutoPeek));
		ImGui::HotKey(CS_XOR("Auto Peek Key"), &C_GET(KeyBind_t, Vars.nAutoPeekKey));
	}
	ImGui::EndChildSection();
}

void T::AntiAim()
{
	ImGui::BeginChildSection(CS_XOR("Antiaim"), ImVec2(339 * MENU::flDpiScale, 253 * MENU::flDpiScale), true);
	{
		ImGui::ToggleButton(CS_XOR("Enable##antiaim"), &C_GET(bool, Vars.bAntiaim));
		ImGui::ToggleButton(CS_XOR("Freestading"), &C_GET(bool, Vars.bFreestading));
		ImGui::BeginDisabled(!C_GET(bool, Vars.bAntiaim));
		{
			/*static const char* AntiaimTypeItems[]{ "DEFAULT", "ADAPTIVE" };
			ImGui::Combo(CS_XOR("Antiaim Type"), &C_GET(int, Vars.bAntiaimType), AntiaimTypeItems, IM_ARRAYSIZE(AntiaimTypeItems));
*/
			static const char* AntiaimPitchItems[]{ "UP", "DOWN", "ZERO" };
			ImGui::Combo(CS_XOR("Pitch"), &C_GET(int, Vars.iAntiaimPitch), AntiaimPitchItems, IM_ARRAYSIZE(AntiaimPitchItems));

			ImGui::ToggleButton(CS_XOR("Yaw"), &C_GET(bool, Vars.bAntiaimYaw));

		}
		ImGui::EndDisabled();
	}
	ImGui::EndChildSection();


	ImGui::SetCursorPos(ImVec2(203 * MENU::flDpiScale, 353 * MENU::flDpiScale - size_child * MENU::flDpiScale));


	ImGui::BeginChildSection(CS_XOR("Angle"), ImVec2(339 * MENU::flDpiScale, 253 * MENU::flDpiScale), true);
	{
		//ImGui::ToggleButton(CS_XOR("Jitter"), &C_GET(bool, Vars.bJitter));
		//ImGui::BeginDisabled(!C_GET(bool, Vars.bJitter));	
		//{
		static const char* AntiaimJitterTypeItems[]{ "NONE", "CENTER", "SPINBOT", "3-WAY" };
		ImGui::Combo(CS_XOR("Jitter Type"), &C_GET(int, Vars.iJitterType), AntiaimJitterTypeItems, IM_ARRAYSIZE(AntiaimJitterTypeItems));

		ImGui::SliderInt(CS_XOR("Jitter Angle"), &C_GET(int, Vars.iJitterAngle), 1, 89);
		//}
		//ImGui::EndDisabled();
	}
	ImGui::EndChildSection();


	ImGui::SetCursorPos(ImVec2(555 * MENU::flDpiScale, 88 * MENU::flDpiScale - size_child * MENU::flDpiScale));


	ImGui::BeginChildSection(CS_XOR("manuals.antiaim"), ImVec2(339 * MENU::flDpiScale, 253 * MENU::flDpiScale), true);
	{
		if (ImGui::BeginMenuBar())
		{
			ImGui::TextUnformatted(CS_XOR("Manuals"));
			ImGui::EndMenuBar();
		}

		ImGui::HotKey(CS_XOR("Left Key"), &C_GET(KeyBind_t, Vars.nLeft));
		ImGui::HotKey(CS_XOR("Right Key"), &C_GET(KeyBind_t, Vars.nRight));
		ImGui::HotKey(CS_XOR("Backward Key"), &C_GET(KeyBind_t, Vars.nBackward));
	}
	ImGui::EndChildSection();
}


void T::Visuals()
{

	const auto& p = ImGui::GetWindowPos();
	const auto& pWindowDrawList = ImGui::GetWindowDrawList();


	static int nVisualTab = 0;


	//pWindowDrawList->AddRectFilled(ImVec2(190 * MENU::flDpiScale + p.x, p.y + 10 * MENU::flDpiScale), ImVec2(600 * MENU::flDpiScale + p.x, 50 * MENU::flDpiScale + p.y), ImGui::GetColorU32(ImGuiCol_ChildBg), 0, ImDrawFlags_RoundCornersLeft); // bar line


	ImGui::SetCursorPos(ImVec2(339 * MENU::flDpiScale - 100 * MENU::flDpiScale, 21 * MENU::flDpiScale));

	if (ImGui::Button("Players", ImVec2(90, 40)) && nVisualTab != 0)
	{
		nVisualTab = 0;
	}

	ImGui::SetCursorPos(ImVec2(339 * MENU::flDpiScale, 21 * MENU::flDpiScale));

	if (ImGui::Button("Weapons", ImVec2(90, 40)) && nVisualTab != 1)
	{
		nVisualTab = 1;
	}

	ImGui::SetCursorPos(ImVec2(339 * MENU::flDpiScale + 100 * MENU::flDpiScale, 21 * MENU::flDpiScale));

	if (ImGui::Button("Grenades", ImVec2(90, 40)) && nVisualTab != 2)
	{
		nVisualTab = 2;
	}

	ImGui::SetCursorPos(ImVec2(339 * MENU::flDpiScale + 100 * 2 * MENU::flDpiScale, 21 * MENU::flDpiScale));

	if (ImGui::Button("World", ImVec2(90, 40)) && nVisualTab != 3)
	{
		nVisualTab = 3;
	}

	ImGui::SetCursorPos(ImVec2(339 * MENU::flDpiScale + 100 * 3 * MENU::flDpiScale, 21 * MENU::flDpiScale));

	if (ImGui::Button("View", ImVec2(90, 40)) && nVisualTab != 4)
	{
		nVisualTab = 4;
	}



	ImGui::SetCursorPos(ImVec2(203 * MENU::flDpiScale, 88 * MENU::flDpiScale - size_child * MENU::flDpiScale));



	switch (nVisualTab)
	{
	case 0:
		T::VisualsPlayers();
		break;
	case 1:
		T::VisualsWeapons();
		break;
	case 2:
		T::VisualsGrenades();
		break;
	case 3:
		T::VisualsWorld();
		break;
	case 4:
		T::VisualsView();
		break;
	default:
		break;
	}

}

void T::VisualsPlayers()
{
	MENU::RenderOverlayPreviewWindow();

	ImGui::BeginChildSection(CS_XOR("Player Esp"), ImVec2(339 * MENU::flDpiScale, 253 * MENU::flDpiScale), true);
	{
		ImGui::ToggleButton(CS_XOR("Enable##player.esp"), &C_GET(bool, Vars.bVisualOverlay));

		ImGui::BeginDisabled(!C_GET(bool, Vars.bVisualOverlay));
		{
			ImGui::ToggleButton(CS_XOR("Bounding box"), &C_GET(FrameOverlayVar_t, Vars.overlayBox).bEnable);
			ImGui::ToggleButton(CS_XOR("Name"), &C_GET(TextOverlayVar_t, Vars.overlayName).bEnable);
			ImGui::ToggleButton(CS_XOR("Weapon"), &C_GET(TextOverlayVar_t, Vars.overlayWeaponName).bEnable);
			ImGui::ToggleButton(CS_XOR("Health bar"), &C_GET(BarOverlayVar_t, Vars.overlayHealthBar).bEnable);
			ImGui::ToggleButton(CS_XOR("Armor bar"), &C_GET(BarOverlayVar_t, Vars.overlayArmorBar).bEnable);
		}
		ImGui::EndDisabled();
	}
	ImGui::EndChildSection();

	ImGui::SetCursorPos(ImVec2(203 * MENU::flDpiScale, 353 * MENU::flDpiScale - size_child * MENU::flDpiScale));

	ImGui::BeginChildSection(CS_XOR("Player Glow"), ImVec2(339 * MENU::flDpiScale, 253 * MENU::flDpiScale), true);
	{
		ImGui::ToggleButton(CS_XOR("Enable##player.glow"), &C_GET(bool, Vars.bVisualGlow));
		ImGui::BeginDisabled(!C_GET(bool, Vars.bVisualGlow));
		{
			ImGui::ColorEdit4(CS_XOR("Enemy color"), &C_GET(Color_t, Vars.colGlowEnemy));
			ImGui::ColorEdit4(CS_XOR("Team color"), &C_GET(Color_t, Vars.colGlowTeam));
		}
		ImGui::EndDisabled();
	}
	ImGui::EndChildSection();

	ImGui::SetCursorPos(ImVec2(555 * MENU::flDpiScale, 88 * MENU::flDpiScale - size_child * MENU::flDpiScale));

	ImGui::BeginChildSection(CS_XOR("Player Chams"), ImVec2(339 * MENU::flDpiScale, 253 * MENU::flDpiScale), true);
	{
		ImGui::ToggleButton(CS_XOR("Player Chams"), &C_GET(bool, Vars.bPlayerChams));
		ImGui::ToggleButton(CS_XOR("Ragdoll Chams"), &C_GET(bool, Vars.bRagdollChams));
		ImGui::ToggleButton(CS_XOR("Team Chams"), &C_GET(bool, Vars.bTeamChams));
		ImGui::ToggleButton(CS_XOR("Invisible chams"), &C_GET(bool, Vars.bVisualChamsIgnoreZ));

		ImGui::Separator();

		ImGui::Combo(CS_XOR("Visible material"), &C_GET(int, Vars.nVisualChamsVisibleMaterial), CS_XOR("white\0illuminate\0latex\0glow\0glass\0"));
		if (C_GET(bool, Vars.bVisualChamsIgnoreZ))
			ImGui::Combo(CS_XOR("Invisible materials"), &C_GET(int, Vars.nVisualChamsInvisibleMaterial), CS_XOR("white\0illuminate\0latex\0glow\0glass\0"));

		ImGui::Separator();

		ImGui::ColorEdit4(CS_XOR("Enemy visible color"), &C_GET(Color_t, Vars.colVisualChamsEnemy));
		if (C_GET(bool, Vars.bVisualChamsIgnoreZ))
			ImGui::ColorEdit4(CS_XOR("Enemy invisible color"), &C_GET(Color_t, Vars.colVisualChamsEnemyIgnoreZ));

		ImGui::ColorEdit4(CS_XOR("Team visible color"), &C_GET(Color_t, Vars.colVisualChamsTeam));
		if (C_GET(bool, Vars.bVisualChamsIgnoreZ))
			ImGui::ColorEdit4(CS_XOR("Team invisible color"), &C_GET(Color_t, Vars.colVisualChamsTeamIgnoreZ));
	}
	ImGui::EndChildSection();
}

void T::VisualsWeapons()
{
	ImGui::BeginChildSection(CS_XOR("Weapon Esp"), ImVec2(339 * MENU::flDpiScale, 253 * MENU::flDpiScale), true);
	{
		ImGui::BeginDisabled(true);
		{
			ImGui::Text(CS_XOR("Coming soon..."));
		}
		ImGui::EndDisabled();
	}
	ImGui::EndChildSection();


	ImGui::SetCursorPos(ImVec2(555 * MENU::flDpiScale, 88 * MENU::flDpiScale - size_child * MENU::flDpiScale));


	ImGui::BeginChildSection(CS_XOR("Weapon Chams"), ImVec2(339 * MENU::flDpiScale, 253 * MENU::flDpiScale), true);
	{
		ImGui::ToggleButton(CS_XOR("Weapon Chams"), &C_GET(bool, Vars.bWeaponChams));
		if (C_GET(bool, Vars.bWeaponChams))
		{
			ImGui::Combo(CS_XOR("Weapon Material"), &C_GET(int, Vars.nVisualChamsWeaponMaterial), CS_XOR("white\0illuminate\0latex\0glow\0glass\0"));
			ImGui::ColorEdit4(CS_XOR("Weapon Color"), &C_GET(Color_t, Vars.colVisualChamsWeapon));
		}

		ImGui::Separator();


		ImGui::ToggleButton(CS_XOR("C4 Chams"), &C_GET(bool, Vars.bC4Chams));
		if (C_GET(bool, Vars.bC4Chams))
		{
			ImGui::Combo(CS_XOR("C4 Material"), &C_GET(int, Vars.nVisualChamsC4Material), CS_XOR("white\0illuminate\0latex\0glow\0glass\0"));
			ImGui::ColorEdit4(CS_XOR("C4 Color"), &C_GET(Color_t, Vars.colVisualChamsC4));
		}
	}
	ImGui::EndChildSection();
}

void T::VisualsGrenades()
{
	//ImGuiStyle& style = ImGui::GetStyle();

	//ImGui::Columns(2, CS_XOR("##visuals_grenades_collumns"), false);
	//{
	//	ImGui::BeginChild(CS_XOR("grenades.esp"), ImVec2(0, ImGui::GetContentRegionAvail().y / 2.f), true, ImGuiWindowFlags_MenuBar);
	//	{
	//		if (ImGui::BeginMenuBar())
	//		{
	//			ImGui::TextUnformatted("Grenades Esp");
	//			ImGui::EndMenuBar();
	//		}

	//		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, 0));

	//		ImGui::BeginDisabled(true);
	//		{
	//			ImGui::Text(CS_XOR("Coming soon..."));
	//		}
	//		ImGui::EndDisabled();

	//		ImGui::PopStyleVar();
	//	}
	//	ImGui::EndChild();
	//}
	//ImGui::NextColumn();
	//{
	//	// Glow
	//}
	//ImGui::Columns(1);
}

void T::VisualsWorld()
{
	ImGui::BeginChildSection(CS_XOR("Visual World"), ImVec2(339 * MENU::flDpiScale, 253 * MENU::flDpiScale), true);
	{
		ImGui::ToggleButton(CS_XOR("Night Mode"), &C_GET(bool, Vars.bNightMode));
		ImGui::NewLine();

		ImGui::ColorEdit4(CS_XOR("World Light"), &C_GET(Color_t, Vars.colLight));
		ImGui::SliderInt(CS_XOR("Light Itensivity"), &C_GET(int, Vars.nLightingIntensity), 0, 600);

		ImGui::SliderInt(CS_XOR("Brightness"), &C_GET(int, Vars.nBrightness), 1, 200);

		ImGui::ColorEdit4(CS_XOR("World color"), &C_GET(Color_t, Vars.colWorld));
		ImGui::ColorEdit4(CS_XOR("Skybox color"), &C_GET(Color_t, Vars.colSkybox));
		ImGui::NewLine();
		ImGui::ToggleButton(CS_XOR("Show Smoke"), &C_GET(bool, Vars.bShowSmoke));
	}
	ImGui::EndChildSection();
}

void T::VisualsView()
{
	ImGui::BeginChildSection(CS_XOR("Visuals Camera"), ImVec2(339 * MENU::flDpiScale, 253 * MENU::flDpiScale), true);
	{

		ImGui::SliderInt(CS_XOR("Fov"), &C_GET(int, Vars.iFov), 50, 160);
		ImGui::ToggleButton(CS_XOR("Static fov in scope?"), &C_GET(bool, Vars.bStaticFovInScope));
		ImGui::ToggleButton(CS_XOR("Force cosshair"), &C_GET(bool, Vars.bForceCrosshair));
		ImGui::ToggleButton(CS_XOR("Remove Visual Punch"), &C_GET(bool, Vars.bRemoveVisualPunch));

	}
	ImGui::EndChildSection();


	ImGui::SetCursorPos(ImVec2(203 * MENU::flDpiScale, 353 * MENU::flDpiScale - size_child * MENU::flDpiScale));


	ImGui::BeginChildSection(CS_XOR("Thirdperson"), ImVec2(339 * MENU::flDpiScale, 253 * MENU::flDpiScale), true);
	{
		ImGui::ToggleButton(CS_XOR("Enable"), &C_GET(bool, Vars.bThirdPerson));
		ImGui::ToggleButton(CS_XOR("Thirdperson in spec"), &C_GET(bool, Vars.bThirdPersonInSpec));
		ImGui::HotKey(CS_XOR("Enable Key"), &C_GET(unsigned int, Vars.nThirdPersonActivationKey));
		ImGui::SliderInt(CS_XOR("Distance"), &C_GET(int, Vars.iThirdPersonDistance), 50.0f, 400.0f);
	}
	ImGui::EndChildSection();


	ImGui::SetCursorPos(ImVec2(555 * MENU::flDpiScale, 88 * MENU::flDpiScale - size_child * MENU::flDpiScale));


	ImGui::BeginChildSection(CS_XOR("Viewmodel"), ImVec2(339 * MENU::flDpiScale, 253 * MENU::flDpiScale), true);
	{
		ImGui::SliderFloat(CS_XOR("Viewmodel x"), &C_GET(float, Vars.flViewModelOffsetX), -15.f, 15.f, "%.2f");
		ImGui::SliderFloat(CS_XOR("Viewmodel y"), &C_GET(float, Vars.flViewModelOffsetY), -15.f, 15.f, "%.2f");
		ImGui::SliderFloat(CS_XOR("Viewmodel z"), &C_GET(float, Vars.flViewModelOffsetZ), -15.f, 15.f, "%.2f");
	}
	ImGui::EndChildSection();


	ImGui::SetCursorPos(ImVec2(555 * MENU::flDpiScale, 353 * MENU::flDpiScale - size_child * MENU::flDpiScale));


	ImGui::BeginChildSection(CS_XOR("Visual Removals"), ImVec2(339 * MENU::flDpiScale, 253 * MENU::flDpiScale), true);
	{
		ImGui::ToggleButton(CS_XOR("Remove Legs"), &C_GET(bool, Vars.bRemoveLegs));
	}
	ImGui::EndChildSection();
}






void T::Miscellaneous()
{
	ImGui::BeginChildSection(CS_XOR("Misc General"), ImVec2(339 * MENU::flDpiScale, 253 * MENU::flDpiScale), true);
	{
		//ImGui::ToggleButton(CS_XOR("Watermark"), &C_GET(bool, Vars.bWatermark));
		ImGui::ToggleButton(CS_XOR("AntiUntrusted mod"), &C_GET(bool, Vars.bAntiUntrusted));
		ImGui::ToggleButton(CS_XOR("Debug Shoot Hitbox"), &C_GET(bool, Vars.bDebugShootHitbox));
		ImGui::ToggleButton(CS_XOR("Auto Stop"), &C_GET(bool, Vars.bAutoStop));
		ImGui::ToggleButton(CS_XOR("Auto Scope"), &C_GET(bool, Vars.bAutoScope));
		ImGui::ToggleButton(CS_XOR("Draw Scope"), &C_GET(bool, Vars.bDrawScope));
		ImGui::ToggleButton(CS_XOR("Hit Marker"), &C_GET(bool, Vars.bHitmarker));
		ImGui::NewLine();
		ImGui::ToggleButton(CS_XOR("Spectator List"), &C_GET(bool, Vars.bSpectatorList));
		ImGui::ToggleButton(CS_XOR("KeyBind list"), &C_GET(bool, Vars.bKeybindList));


		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 0.f, 1.f));
		if (ImGui::Button(CS_XOR("unlock hidden cvars"), ImVec2(-1, 15 * MENU::flDpiScale)))
		{
			I::Cvar->UnlockHiddenCVars();
			NOTIFY::Push({ N_TYPE_INFO, CS_XOR("unlocked all hidden cvars") });
		}

		ImGui::PopStyleColor();
	}
	ImGui::EndChildSection();


	ImGui::SetCursorPos(ImVec2(555 * MENU::flDpiScale, 88 * MENU::flDpiScale - size_child * MENU::flDpiScale));


	ImGui::BeginChildSection(CS_XOR("Misc Movement"), ImVec2(339 * MENU::flDpiScale, 253 * MENU::flDpiScale), true);
	{
		ImGui::ToggleButton(CS_XOR("Auto bunny-hopping"), &C_GET(bool, Vars.bAutoBHop));
		if (C_GET(bool, Vars.bAutoBHop))
			ImGui::SliderInt(CS_XOR("Chance"), &C_GET(int, Vars.nAutoBHopChance), 0, 100, CS_XOR("%d%%"));

		ImGui::ToggleButton(CS_XOR("Auto strafe"), &C_GET(bool, Vars.bAutoStrafe));

	}
	ImGui::EndChildSection();
}

void T::MiscellaneousInventory()
{
	ImGui::BeginDisabled(true);
	{
		ImGui::Text(CS_XOR("Coming soon..."));
	}
	ImGui::EndDisabled();
}

void T::Configs()
{
	ImGui::BeginChildSection(CS_XOR("MIsc Configs"), ImVec2(670 * MENU::flDpiScale, 253 * 2 * MENU::flDpiScale - size_child * MENU::flDpiScale), true);
	{
		ImGui::Columns(2, CS_XOR("#CONFIG"), false);
		{
			ImGui::PushItemWidth(-1);

			// check selected configuration for magic value
			if (nSelectedConfig == ~1U)
			{
				// set default configuration as selected on first use
				for (std::size_t i = 0U; i < C::vecFileNames.size(); i++)
				{
					if (CRT::StringCompare(C::vecFileNames[i], CS_XOR(CS_CONFIGURATION_DEFAULT_FILE_NAME CS_CONFIGURATION_FILE_EXTENSION)) == 0)
						nSelectedConfig = i;
				}
			}

			if (ImGui::BeginListBox(CS_XOR("##config.list"), C::vecFileNames.size(), 5))
			{
				for (std::size_t i = 0U; i < C::vecFileNames.size(); i++)
				{
					// @todo: imgui cant work with wstring
					const wchar_t* wszFileName = C::vecFileNames[i];

					char szFileName[MAX_PATH] = {};
					CRT::StringUnicodeToMultiByte(szFileName, CS_ARRAYSIZE(szFileName), wszFileName);

					if (ImGui::Selectable(szFileName, (nSelectedConfig == i)))
						nSelectedConfig = i;
				}

				ImGui::EndListBox();
			}

			ImGui::PopItemWidth();
		}
		ImGui::NextColumn();
		{
			ImGui::PushItemWidth(-1);
			if (ImGui::InputTextWithHint(CS_XOR("##config.file"), CS_XOR("Create new..."), szConfigFile, sizeof(szConfigFile), ImGuiInputTextFlags_EnterReturnsTrue))
			{
				// check if the filename isn't empty
				if (const std::size_t nConfigFileLength = CRT::StringLength(szConfigFile); nConfigFileLength > 0U)
				{
					CRT::WString_t wszConfigFile(szConfigFile);

					if (C::CreateFile(wszConfigFile.Data()))
						// set created config as selected @todo: dependent on current 'C::CreateFile' behaviour, generally it must be replaced by search
						nSelectedConfig = C::vecFileNames.size() - 1U;

					// clear string
					CRT::MemorySet(szConfigFile, 0U, sizeof(szConfigFile));
				}
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(CS_XOR("press enter to create new configuration"));

			if (ImGui::Button(CS_XOR("Save"), ImVec2(-1, 15 * MENU::flDpiScale)))
				C::SaveFile(nSelectedConfig);

			if (ImGui::Button(CS_XOR("Load"), ImVec2(-1, 15 * MENU::flDpiScale)))
				C::LoadFile(nSelectedConfig);

			if (ImGui::Button(CS_XOR("Remove"), ImVec2(-1, 15 * MENU::flDpiScale)))
				ImGui::OpenPopup(CS_XOR("Confirmation Config"));

			if (ImGui::Button(CS_XOR("Refresh"), ImVec2(-1, 15 * MENU::flDpiScale)))
				C::Refresh();

			ImGui::PopItemWidth();
		}
		ImGui::Columns(1);

		if (ImGui::BeginPopupModal(CS_XOR("Confirmation Config"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
		{
			CRT::String_t<MAX_PATH> szCurrentConfig(C::vecFileNames[nSelectedConfig]);

			ImGui::Text(CS_XOR("Are you sure you want to remove \"%s\" configuration?"), szCurrentConfig);
			ImGui::Spacing();

			if (ImGui::Button(CS_XOR("No"), ImVec2(ImGui::GetContentRegionAvail().x / 2.f, 0)))
				ImGui::CloseCurrentPopup();

			ImGui::Separator();

			if (ImGui::Button(CS_XOR("Yes"), ImVec2(ImGui::GetContentRegionAvail().x, 0)))
			{
				C::RemoveFile(nSelectedConfig);

				// reset selected configuration index
				nSelectedConfig = ~0U;

				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}
	ImGui::EndChildSection();
}






void T::SkinsChanger()
{

}

#pragma endregion

#pragma region menu_particle



float maxRadius = 15.f;


float min(float a, float b)
{
	return (((a) < (b)) ? (a) : (b));
}

float max(float a, float b)
{
	return (((a) > (b)) ? (a) : (b));
}

float Constrain(float n, float low, float high)
{
	return max(min(n, high), low);
}

float Map(float n, float start1, float stop1, float start2, float stop2, bool withinBounds = false)
{
	const float newVal = (n - start1) / (stop1 - start1) * (stop2 - start2) + start2;
	if (!withinBounds)
		return newVal;

	if (start2 < stop2)
		return Constrain(newVal, start2, stop2);
	else
		return Constrain(newVal, stop2, start2);
}

float RandomFloat(float a, float b)
{
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = b - a;
	float r = random * diff;
	return a + r;
}

float GetRandomSize(float min, float max)
{
	float r = pow(RandomFloat(0.f, 1.f), 3);
	return Constrain(r * max, min, max);
}


void MENU::ParticleContext_t::Render(ImDrawList* pDrawList, const ImVec2& vecScreenSize, const float flAlpha)
{
	if (this->vecParticles.empty())
	{
		for (int i = 0; i < 100; i++)
			this->AddParticle(ImGui::GetIO().DisplaySize);
	}

	for (auto& particle : this->vecParticles)
	{
		//this->DrawParticle(pDrawList, particle, C_GET(ColorPickerVar_t, Vars.colAccent0).colValue.Set<COLOR_A>(flAlpha * 255));
		this->DrawParticle(pDrawList, particle, Color_t(255, 255, 255).Set<COLOR_A>((particle.radius / maxRadius) * 255));
		this->UpdatePosition(particle, vecScreenSize);
		//this->FindConnections(pDrawList, particle, C_GET(ColorPickerVar_t, Vars.colAccent2).colValue.Set<COLOR_A>(flAlpha * 255), 200.f);
	}
}


static constexpr float flSpeed = 150.f;
ImVec2 gravity = { 0.f, 0.005f };

void MENU::ParticleContext_t::AddParticle(const ImVec2& vecScreenSize)
{
	// exceeded limit
	if (this->vecParticles.size() >= 400UL)
		return;

	// @note: random speed value
	this->vecParticles.emplace_back(
		ImVec2(MATH::fnRandomFloat(0.f, vecScreenSize.x), MATH::fnRandomFloat(-100.f, vecScreenSize.y)),
		ImVec2(0, 0),
		GetRandomSize(5.f, maxRadius)
	);
}

void MENU::ParticleContext_t::DrawParticle(ImDrawList* pDrawList, ParticleData_t& particle, const Color_t& colPrimary)
{
	D::AddDrawListCircle(pDrawList, particle.vecPosition, particle.radius, colPrimary, 25, DRAW_CIRCLE_FILLED);
}

void MENU::ParticleContext_t::FindConnections(ImDrawList* pDrawList, ParticleData_t& particle, const Color_t& colPrimary, float flMaxDistance)
{
	for (auto& currentParticle : this->vecParticles)
	{
		// skip current particle
		if (&currentParticle == &particle)
			continue;

		/// @note: calcuate length distance 2d, return FLT_MAX if failed
		const float flDistance = ImLength(particle.vecPosition - currentParticle.vecPosition, FLT_MAX);
		if (flDistance <= flMaxDistance)
			this->DrawConnection(pDrawList, particle, currentParticle, (flMaxDistance - flDistance) / flMaxDistance, colPrimary);
	}
}

void MENU::ParticleContext_t::DrawConnection(ImDrawList* pDrawList, ParticleData_t& particle, ParticleData_t& otherParticle, float flAlpha, const Color_t& colPrimary) const
{
	D::AddDrawListLine(pDrawList, particle.vecPosition, otherParticle.vecPosition, colPrimary.Set<COLOR_A>(flAlpha * 255), 1.f);
}

void MENU::ParticleContext_t::UpdatePosition(ParticleData_t& particle, const ImVec2& vecScreenSize) const
{
	this->ResolveScreenCollision(particle, vecScreenSize);

	ImGuiStyle& style = ImGui::GetStyle();

	ImVec2 f = gravity;
	f *= particle.radius;
	particle.vecAccelaretion += f;



	float xOff = particle.vecPosition.x / vecScreenSize.x;
	float yOff = particle.vecPosition.y / vecScreenSize.y;
	float wx = Map(ImGui::GetMousePos().x, 0, vecScreenSize.x, -0.002f, 0.002f, true);
	ImVec2 wind = ImVec2(wx + (xOff * .002f), (yOff * .002f));
	wind *= .5f;

	ImVec2 windf = wind;
	wind *= particle.radius;
	particle.vecAccelaretion += windf;

	particle.vecVelocity += particle.vecAccelaretion;
	particle.vecPosition += particle.vecVelocity * 22.f * ImGui::GetIO().DeltaTime;
	particle.vecAccelaretion *= 0.f;

	// move particle
	//particle.vecPosition.x += (particle.vecVelocity.x * style.AnimationSpeed * 10.f) * ImGui::GetIO().DeltaTime;
	//particle.vecPosition.y += (particle.vecVelocity.y * style.AnimationSpeed * 10.f) * ImGui::GetIO().DeltaTime;
}

void MENU::ParticleContext_t::ResolveScreenCollision(ParticleData_t& particle, const ImVec2& vecScreenSize) const
{
	if (particle.vecPosition.y + particle.vecVelocity.y > vecScreenSize.y + particle.radius)
	{
		//particle.vecVelocity.y = -particle.vecVelocity.y;
		particle.vecPosition = ImVec2(MATH::fnRandomFloat(0.f, vecScreenSize.x), MATH::fnRandomFloat(-100.f, -50.f));
		particle.vecVelocity = ImVec2(0, 0);
		particle.radius = GetRandomSize(5.f, maxRadius);
	}
}

#pragma endregion
