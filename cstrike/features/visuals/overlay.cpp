// used: [stl] vector
#include <vector>
// used: [stl] sort
#include <algorithm>

#include "overlay.h"

// used: cheat variables
#include "../../core/variables.h"

// used: entity
#include "../../sdk/entity.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"
#include "../../sdk/interfaces/cgametracemanager.h"

// used: sdk variables
#include "../../core/sdk.h"

// used: l_print
#include "../../utilities/log.h"
// used: inputsystem
#include "../../utilities/inputsystem.h"
// used: draw system
#include "../../utilities/draw.h"

// used: mainwindowopened
#include "../../core/menu.h"
#include <map>
#include <stdexcept>
#include <thread>
#include <mutex>
#include "../antiaim/antiaim.hpp"
#include "../../sdk/interfaces/cgameevents.h"
#include "../../core/hooks.h"



std::mutex mtxVisual;



using namespace F::VISUALS;

#pragma region visual_overlay_components

ImVec2 OVERLAY::CBaseComponent::GetBasePosition(const ImVec4& box) const
{
	return { box[this->nSide == SIDE_RIGHT ? SIDE_RIGHT : SIDE_LEFT], box[this->nSide == SIDE_BOTTOM ? SIDE_BOTTOM : SIDE_TOP] };
}

ImVec2 OVERLAY::CBaseDirectionalComponent::GetBasePosition(const ImVec4& box) const
{
	ImVec2 vecBasePosition = {};

	if (this->nSide == SIDE_TOP || this->nSide == SIDE_BOTTOM)
	{
		//CS_ASSERT(this->nDirection != (this->nSide ^ SIDE_BOTTOM) + 1); // this direction isn't supported for this side
		vecBasePosition = { (box[SIDE_LEFT] + box[SIDE_RIGHT]) * 0.5f, box[this->nSide] };
	}
	else if (this->nSide == SIDE_LEFT || this->nSide == SIDE_RIGHT)
	{
		//CS_ASSERT(this->nDirection != (this->nSide ^ SIDE_RIGHT)); // this direction isn't supported for this side
		vecBasePosition = { box[this->nSide], box[this->nDirection == DIR_TOP ? SIDE_BOTTOM : SIDE_TOP] };
	}
	else
	{
		L_PRINT(LOG_ERROR) << CS_XOR("CBaseDirectionalComponent::GetBasePosition: invalid side: ") << this->nSide;
		//CS_ASSERT(false); // this side isn't supported for this component
		return vecBasePosition;
	}

	if (this->nSide != SIDE_RIGHT && this->nDirection != DIR_RIGHT)
		vecBasePosition.x -= this->vecSize.x * ((static_cast<std::uint8_t>(this->nDirection) == static_cast<std::uint8_t>(this->nSide) && (this->nSide & 1U) == 1U) ? 0.5f : 1.0f);

	if (this->nSide == SIDE_TOP || this->nDirection == DIR_TOP)
		vecBasePosition.y -= this->vecSize.y;

	return vecBasePosition;
}

OVERLAY::CBarComponent::CBarComponent(const bool bIsMenuItem, const EAlignSide nAlignSide, const ImVec4& vecBox, const float flProgressFactor, const std::size_t uOverlayVarIndex) :
	bIsMenuItem(bIsMenuItem), uOverlayVarIndex(uOverlayVarIndex), flProgressFactor(MATH::Clamp(flProgressFactor, 0.f, 1.f))
{
	this->nSide = nAlignSide;

	const bool bIsHorizontal = ((nAlignSide & 1U) == 1U);

	const BarOverlayVar_t& overlayConfig = C_GET(BarOverlayVar_t, uOverlayVarIndex);
	this->vecSize = { (bIsHorizontal ? vecBox[SIDE_RIGHT] - vecBox[SIDE_LEFT] : overlayConfig.flThickness), (bIsHorizontal ? overlayConfig.flThickness : vecBox[SIDE_BOTTOM] - vecBox[SIDE_TOP]) };
}

void OVERLAY::CBarComponent::Render(ImDrawList* pDrawList, const ImVec2& vecPosition)
{
	BarOverlayVar_t& overlayConfig = C_GET(BarOverlayVar_t, uOverlayVarIndex);
	const ImVec2 vecThicknessOffset = { overlayConfig.flThickness, overlayConfig.flThickness };
	ImVec2 vecMin = vecPosition, vecMax = vecPosition + this->vecSize;

	// background glow
	pDrawList->AddShadowRect(vecMin, vecMax, overlayConfig.colBackground.GetU32(), 1.f, ImVec2(0, 0));
	// outline
	pDrawList->AddRect(vecMin, vecMax, overlayConfig.colOutline.GetU32(), 0.f, ImDrawFlags_None, overlayConfig.flThickness);

	// account outline offset
	vecMin += vecThicknessOffset;
	vecMax -= vecThicknessOffset;

	const ImVec2 vecLineSize = vecMax - vecMin;

	// modify active side axis by factor
	if ((this->nSide & 1U) == 0U)
		vecMin.y += vecLineSize.y * (1.0f - this->flProgressFactor);
	else
		vecMax.x -= vecLineSize.x * (1.0f - this->flProgressFactor);

	// bar
	if (overlayConfig.bGradient && !overlayConfig.bUseFactorColor)
	{
		if (this->nSide == SIDE_LEFT || this->nSide == SIDE_RIGHT)
			pDrawList->AddRectFilledMultiColor(vecMin, vecMax, overlayConfig.colPrimary.GetU32(), overlayConfig.colPrimary.GetU32(), overlayConfig.colSecondary.GetU32(), overlayConfig.colSecondary.GetU32());
		else
			pDrawList->AddRectFilledMultiColor(vecMin, vecMax, overlayConfig.colSecondary.GetU32(), overlayConfig.colPrimary.GetU32(), overlayConfig.colPrimary.GetU32(), overlayConfig.colSecondary.GetU32());
	}
	else
	{
		const ImU32 u32Color = overlayConfig.bUseFactorColor ? Color_t::FromHSB((flProgressFactor * 120.f) / 360.f, 1.0f, 1.0f).GetU32() : overlayConfig.colPrimary.GetU32();
		pDrawList->AddRectFilled(vecMin, vecMax, u32Color, 0.f, ImDrawFlags_None);
	}

	// only open menu item if menu is opened and overlay is enabled
	bIsMenuItem &= (MENU::bMainWindowOpened && overlayConfig.bEnable);
	if (bIsMenuItem)
	{
		// @note: padding 2.f incase the thickness is too small
		this->bIsHovered = ImRect(vecPosition - ImVec2(2.f, 2.f), vecPosition + this->vecSize + ImVec2(2.f, 2.f)).Contains(ImGui::GetIO().MousePos);
		// if component is hovered + right clicked
		if (this->bIsHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			ImGui::OpenPopup(CS_XOR("context##component.bar"));
		
		if (ImGui::BeginPopup(CS_XOR("context##component.bar"), ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, -1));

			ImGui::Checkbox(CS_XOR("use factor color##component.bar"), &overlayConfig.bUseFactorColor);
			if (!overlayConfig.bUseFactorColor)
			ImGui::Checkbox(CS_XOR("use gradient##component.bar"), &overlayConfig.bGradient);

			ImGui::ColorEdit3(CS_XOR("primary color##component.bar"), &overlayConfig.colPrimary);
			if (overlayConfig.bGradient && !overlayConfig.bUseFactorColor)
				ImGui::ColorEdit3(CS_XOR("secondary color##component.bar"), &overlayConfig.colSecondary);

			ImGui::ColorEdit4(CS_XOR("outline color##component.bar"), &overlayConfig.colOutline);
			ImGui::ColorEdit4(CS_XOR("background color##component.bar"), &overlayConfig.colBackground);

			ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.75f);
			ImGui::SliderFloat(CS_XOR("thickness##component.bar"), &overlayConfig.flThickness, 1.0f, 10.0f, CS_XOR("%.1f"), ImGuiSliderFlags_NoInput);
			ImGui::PopStyleVar();

			ImGui::EndPopup();
		}
	}
	else
		// dont process hovered on menu close...
		this->bIsHovered = false;
}

OVERLAY::CTextComponent::CTextComponent(const bool bIsMenuItem, const EAlignSide nAlignSide, const EAlignDirection nAlignDirection, const ImFont* pFont, const char* szText, const std::size_t uOverlayVarIndex) :
	bIsMenuItem(bIsMenuItem), pFont(pFont), uOverlayVarIndex(uOverlayVarIndex)
{
	// allocate own buffer to safely store a copy of the string
	this->szText = new char[CRT::StringLength(szText) + 1U];
	CRT::StringCopy(this->szText, szText);

	this->nSide = nAlignSide;
	this->nDirection = nAlignDirection;
	this->vecSize = pFont->CalcTextSizeA(pFont->FontSize, FLT_MAX, 0.0f, szText) + C_GET(TextOverlayVar_t, uOverlayVarIndex).flThickness;
}

OVERLAY::CTextComponent::~CTextComponent()
{
	// deallocate buffer of the copied string
	delete[] this->szText;
}

void OVERLAY::CTextComponent::Render(ImDrawList* pDrawList, const ImVec2& vecPosition)
{
	TextOverlayVar_t& overlayConfig = C_GET(TextOverlayVar_t, this->uOverlayVarIndex);

	const ImVec2 vecOutlineOffset = { overlayConfig.flThickness, overlayConfig.flThickness };

	// @test: used for spacing debugging
	//pDrawList->AddRect(vecPosition, vecPosition + this->vecSize, IM_COL32(255, 255, 255, 255));

	// @todo: fix this cringe shit after gui merge
	if (overlayConfig.flThickness >= 1.0f)
	{
		pDrawList->AddText(this->pFont, this->pFont->FontSize, vecPosition, overlayConfig.colOutline.GetU32(), this->szText);
		pDrawList->AddText(this->pFont, this->pFont->FontSize, vecPosition + vecOutlineOffset * 2.0f, overlayConfig.colOutline.GetU32(), this->szText);
	}

	pDrawList->AddText(this->pFont, this->pFont->FontSize, vecPosition + vecOutlineOffset, overlayConfig.colPrimary.GetU32(), this->szText);


	// only open menu item if menu is opened and overlay is enabled
	bIsMenuItem &= MENU::bMainWindowOpened && overlayConfig.bEnable; 
	if (bIsMenuItem)
	{
		this->bIsHovered = ImRect(vecPosition, vecPosition + this->vecSize).Contains(ImGui::GetIO().MousePos);
		//pDrawList->AddRect(vecPosition, vecPosition + this->vecSize, IM_COL32(this->bIsHovered ? 0 : 255, this->bIsHovered ? 255 : 0, 0, 255));

		// if component is hovered + right clicked
		if (this->bIsHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			ImGui::OpenPopup(CS_XOR("context##component.text"));

		if (ImGui::BeginPopup(CS_XOR("context##component.text")))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, -1));

			ImGui::ColorEdit3(CS_XOR("primary color##component.bar"), &overlayConfig.colPrimary);
			ImGui::ColorEdit4(CS_XOR("outline color##component.bar"), &overlayConfig.colOutline);

			ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.75f);
			ImGui::SliderFloat(CS_XOR("outline thickness##component.bar"), &overlayConfig.flThickness, 1.0f, 10.0f, CS_XOR("%.1f"), ImGuiSliderFlags_NoInput);

			ImGui::PopStyleVar();

			ImGui::EndPopup();
		}
	}
}

#pragma endregion

#pragma region visual_overlay_context

bool OVERLAY::Context_t::AddBoxComponent(ImDrawList* pDrawList, const ImVec4& vecBox, const int nType, float flThickness, float flRounding, const Color_t& colPrimary, const Color_t& colOutline)
{
	flThickness = std::floorf(flThickness);
	const ImVec2 vecThicknessOffset = { flThickness, flThickness };

	switch (nType)
	{
	case VISUAL_OVERLAY_BOX_FULL:
	{
		const ImVec2 vecBoxMin = { vecBox[SIDE_LEFT], vecBox[SIDE_TOP] };
		const ImVec2 vecBoxMax = { vecBox[SIDE_RIGHT], vecBox[SIDE_BOTTOM] };

		// inner outline
		pDrawList->AddRect(vecBoxMin + vecThicknessOffset * 2.0f, vecBoxMax - vecThicknessOffset * 2.0f, colOutline.GetU32(), flRounding, ImDrawFlags_RoundCornersAll, flThickness);
		// primary box
		pDrawList->AddRect(vecBoxMin + vecThicknessOffset, vecBoxMax - vecThicknessOffset, colPrimary.GetU32(), flRounding, ImDrawFlags_RoundCornersAll, flThickness);
		// outer outline
		pDrawList->AddRect(vecBoxMin, vecBoxMax, colOutline.GetU32(), flRounding, ImDrawFlags_RoundCornersAll, flThickness);

		break;
	}
	case VISUAL_OVERLAY_BOX_CORNERS:
	{
		// corner part of the whole line
		constexpr float flPartRatio = 0.25f;

		const float flCornerWidth = ((vecBox[SIDE_RIGHT] - vecBox[SIDE_LEFT]) * flPartRatio);
		const float flCornerHeight = ((vecBox[SIDE_BOTTOM] - vecBox[SIDE_TOP]) * flPartRatio);

		const ImVec2 arrCornerPoints[4][3] = {
			// top-left
			{ ImVec2(vecBox[SIDE_LEFT], vecBox[SIDE_TOP] + flCornerHeight) + vecThicknessOffset, ImVec2(vecBox[SIDE_LEFT], vecBox[SIDE_TOP]) + vecThicknessOffset, ImVec2(vecBox[SIDE_LEFT] + flCornerWidth, vecBox[SIDE_TOP]) + vecThicknessOffset },

			// top-right
			{ ImVec2(vecBox[SIDE_RIGHT] - flCornerWidth - vecThicknessOffset.x, vecBox[SIDE_TOP] + vecThicknessOffset.y * 2.0f), ImVec2(vecBox[SIDE_RIGHT] - vecThicknessOffset.x, vecBox[SIDE_TOP] + vecThicknessOffset.y * 2.0f), ImVec2(vecBox[SIDE_RIGHT] - vecThicknessOffset.x, vecBox[SIDE_TOP] + flCornerHeight + vecThicknessOffset.y * 2.0f) },

			// bottom-left
			{ ImVec2(vecBox[SIDE_LEFT] + flCornerWidth + vecThicknessOffset.x, vecBox[SIDE_BOTTOM] - vecThicknessOffset.y * 2.0f), ImVec2(vecBox[SIDE_LEFT] + vecThicknessOffset.x, vecBox[SIDE_BOTTOM] - vecThicknessOffset.y * 2.0f), ImVec2(vecBox[SIDE_LEFT] + vecThicknessOffset.x, vecBox[SIDE_BOTTOM] - flCornerHeight - vecThicknessOffset.y * 2.0f) },

			// bottom-right
			{ ImVec2(vecBox[SIDE_RIGHT], vecBox[SIDE_BOTTOM] - flCornerHeight) - vecThicknessOffset, ImVec2(vecBox[SIDE_RIGHT], vecBox[SIDE_BOTTOM]) - vecThicknessOffset, ImVec2(vecBox[SIDE_RIGHT] - flCornerWidth, vecBox[SIDE_BOTTOM]) - vecThicknessOffset }
		};

		for (std::size_t i = 0U; i < CS_ARRAYSIZE(arrCornerPoints); i++)
		{
			const auto& arrLinePoints = arrCornerPoints[i];
			const ImVec2 vecHalfPixelOffset = ((i & 1U) == 1U ? ImVec2(-0.5f, -0.5f) : ImVec2(0.5f, 0.5f));

			// @todo: we can even do not clear path and reuse it
			pDrawList->PathLineTo(arrLinePoints[0] + vecHalfPixelOffset);
			pDrawList->PathLineTo(arrLinePoints[1] + vecHalfPixelOffset);
			pDrawList->PathLineTo(arrLinePoints[2] + vecHalfPixelOffset);
			pDrawList->PathStroke(colOutline.GetU32(), false, flThickness + 1.0f);

			pDrawList->PathLineTo(arrLinePoints[0] + vecHalfPixelOffset);
			pDrawList->PathLineTo(arrLinePoints[1] + vecHalfPixelOffset);
			pDrawList->PathLineTo(arrLinePoints[2] + vecHalfPixelOffset);
			pDrawList->PathStroke(colPrimary.GetU32(), false, flThickness);
		}

		break;
	}
	default:
		break;
	}

	// accumulate spacing for next side/directional components
	for (float& flSidePadding : this->arrSidePaddings)
		flSidePadding += this->flComponentSpacing;

	return ImRect(vecBox).Contains(ImGui::GetIO().MousePos);
}

ImVec4 OVERLAY::Context_t::AddFrameComponent(ImDrawList* pDrawList, const ImVec2& vecScreen, const EAlignSide nSide, const Color_t& colBackground, const float flRounding, const ImDrawFlags nRoundingCorners)
{
	// calculate frame size by previously added components on active side
	const ImVec2 vecFrameSize = this->GetTotalDirectionalSize(nSide);

	ImVec2 vecFrameMin = { vecScreen.x - vecFrameSize.x * 0.5f, vecScreen.y - vecFrameSize.y };
	ImVec2 vecFrameMax = { vecScreen.x + vecFrameSize.x * 0.5f, vecScreen.y };

	pDrawList->AddRectFilled(vecFrameMin - this->flComponentSpacing, vecFrameMax + this->flComponentSpacing, colBackground.GetU32(), flRounding, nRoundingCorners);

	// accumulate spacing for next side/directional components
	for (float& flSidePadding : this->arrSidePaddings)
		flSidePadding += this->flComponentSpacing;

	return { vecFrameMin.x, vecFrameMin.y, vecFrameMax.x, vecFrameMax.y };
}

/*
 * @todo: currently not well designed, make it more flexible for use cases where we need e.g. previous frame bar factor etc
 * also to optimize this, allocate components at stack instead of heap + make all context units static and do not realloc components storage every frame, but reset (like memset idk) it at the end of frame
 */
void OVERLAY::Context_t::AddComponent(CBaseComponent* pComponent)
{
	// guarantee that first directional component on each side is in the primary direction
	if (pComponent->IsDirectional())
	{
		CBaseDirectionalComponent* pDirectionalComponent = static_cast<CBaseDirectionalComponent*>(pComponent);

		// check if it's not an exception direction and there are no components in the primary direction
		if (((pDirectionalComponent->nSide & 1U) == 1U || pDirectionalComponent->nDirection != DIR_TOP) && this->arrSideDirectionPaddings[pDirectionalComponent->nSide][pDirectionalComponent->nSide] == 0.0f)
			pDirectionalComponent->nDirection = static_cast<EAlignDirection>(pDirectionalComponent->nSide);
	}

	float& flSidePadding = this->arrSidePaddings[pComponent->nSide];

	if (pComponent->IsDirectional())
	{
		CBaseDirectionalComponent* pDirectionalComponent = static_cast<CBaseDirectionalComponent*>(pComponent);
		float(&arrDirectionPaddings)[DIR_MAX] = this->arrSideDirectionPaddings[pDirectionalComponent->nSide];

		// directional components don't change side paddings, but take them into account
		pComponent->vecOffset[pDirectionalComponent->nSide & 1U] += ((pDirectionalComponent->nSide < 2U) ? -flSidePadding : flSidePadding);

		// check if the component is in the same direction as the side and it's the first component in this direction
		if (static_cast<std::uint8_t>(pDirectionalComponent->nDirection) == static_cast<std::uint8_t>(pDirectionalComponent->nSide) && arrDirectionPaddings[pDirectionalComponent->nDirection] == 0.0f)
		{
			// accumulate paddings for sub-directions
			for (std::uint8_t nSubDirection = DIR_LEFT; nSubDirection < DIR_MAX; nSubDirection++)
			{
				/*
				 * exclude conflicting sub-directions
				 *
				 * SIDE_LEFT[0]: DIR_LEFT[0], DIR_BOTTOM[3] | ~2 & ~1
				 * SIDE_TOP[1]: DIR_LEFT[0], DIR_TOP[1], DIR_RIGHT[2] | ~3
				 * SIDE_RIGHT[2]: DIR_RIGHT[2], DIR_BOTTOM[3] | ~0 & ~1
				 * SIDE_BOTTOM[3]: DIR_LEFT[0], DIR_RIGHT[2], DIR_BOTTOM[3] | ~1
				 */
				if (nSubDirection == pDirectionalComponent->nSide || nSubDirection == ((pDirectionalComponent->nSide + 2U) & 3U) || (nSubDirection == DIR_TOP && (pDirectionalComponent->nSide & 1U) == 0U))
					continue;

				arrDirectionPaddings[nSubDirection] += (pDirectionalComponent->vecSize[nSubDirection == DIR_BOTTOM ? SIDE_TOP : SIDE_LEFT] * (((pDirectionalComponent->nSide & 1U) == 1U) ? 0.5f : 1.0f) + this->flComponentSpacing);
			}
		}

		float& flSideDirectionPadding = arrDirectionPaddings[pDirectionalComponent->nDirection];

		// append direction padding to offset
		pComponent->vecOffset[pDirectionalComponent->nDirection & 1U] += ((pDirectionalComponent->nDirection < 2U) ? -flSideDirectionPadding : flSideDirectionPadding);

		// accumulate direction padding for next component
		flSideDirectionPadding += pDirectionalComponent->vecSize[pDirectionalComponent->nDirection & 1U];

		// accumulate spacing for next directional components
		flSideDirectionPadding += this->flComponentSpacing;
	}
	else
	{
		// append side padding to offset
		pComponent->vecOffset[pComponent->nSide & 1U] += ((pComponent->nSide < 2U) ? -(flSidePadding + pComponent->vecSize[pComponent->nSide]) : flSidePadding);

		// accumulate side padding for next component
		flSidePadding += pComponent->vecSize[pComponent->nSide & 1U];

		// accumulate spacing for next components
		flSidePadding += this->flComponentSpacing;
	}

	this->vecComponents.push_back(pComponent);
}

ImVec2 OVERLAY::Context_t::GetTotalDirectionalSize(const EAlignSide nSide) const
{
	ImVec2 vecSideSize = {};

	// @todo: we should peek max of bottom + side or top directions at horizontal sides
	const float(&arrDirectionPaddings)[DIR_MAX] = this->arrSideDirectionPaddings[nSide];
	for (std::uint8_t nSubDirection = DIR_LEFT; nSubDirection < DIR_MAX; nSubDirection++)
		vecSideSize[nSubDirection & 1U] += arrDirectionPaddings[nSubDirection];

	return vecSideSize;
}

void OVERLAY::Context_t::Render(ImDrawList* pDrawList, const ImVec4& vecBox) const
{
	bool bCenteredFirstSideDirectional[SIDE_MAX] = {};

	for (CBaseComponent* const pComponent : this->vecComponents)
	{
		ImVec2 vecPosition = pComponent->GetBasePosition(vecBox);

		// check if the component is in the side that supports multi-component centering
		if (pComponent->nSide == SIDE_TOP || pComponent->nSide == SIDE_BOTTOM)
		{
			// check if the component is directional
			if (CBaseDirectionalComponent* const pDirectionalComponent = static_cast<CBaseDirectionalComponent*>(pComponent); pDirectionalComponent->IsDirectional())
			{
				const float(&arrDirectionPaddings)[DIR_MAX] = this->arrSideDirectionPaddings[pComponent->nSide];

				// check if the component has horizontal direction
				if (static_cast<std::uint8_t>(pDirectionalComponent->nDirection) != static_cast<std::uint8_t>(pDirectionalComponent->nSide))
					// add centering offset to the component's offset
					pDirectionalComponent->vecOffset.x += (arrDirectionPaddings[DIR_LEFT] - arrDirectionPaddings[DIR_RIGHT]) * 0.5f;
				// otherwise check if it's the first component in direction as side
				else if (!bCenteredFirstSideDirectional[pDirectionalComponent->nSide])
				{
					// add centering offset to the component's offset
					pDirectionalComponent->vecOffset.x += (arrDirectionPaddings[DIR_LEFT] - arrDirectionPaddings[DIR_RIGHT]) * 0.5f;

					bCenteredFirstSideDirectional[pDirectionalComponent->nSide] = true;
				}
			}
		}

		// add final component offset to the base position
		vecPosition += pComponent->vecOffset;

		pComponent->Render(pDrawList, vecPosition);
	}
}

#pragma endregion



enum ESortEntityType : int
{
	SORT_ENTITY_NONE = -1,
	SORT_ENTITY_PLAYER = 0,
};

struct SortEntityObject_t
{
	SortEntityObject_t(C_BaseEntity* pEntity, CBaseHandle hEntity, float flDistance, ESortEntityType nSortType) :
		pEntity(pEntity), hEntity(hEntity), flDistance(flDistance), nSortType(nSortType) { }

	C_BaseEntity* pEntity;
	CBaseHandle hEntity;
	float flDistance;
	ESortEntityType nSortType;
};



int lastEspPlayersCount = 1;




void OVERLAY::OnFrameStageNotify(CCSPlayerController* pLocalController)
{
	// only render when in-game
	if (!I::Engine->IsConnected() || !I::Engine->IsInGame())
		return;


	// Scope Line
	DrawScope(D::pDrawListActive);
	DrawFovCircle(D::pDrawListActive);


	Events->OnPaint();


	if (!C_GET(bool, Vars.bVisualOverlay))
		return;


	const int nHighestIndex = I::Engine->GetMaxClients();

	std::vector<SortEntityObject_t> vecSortedEntities = {};
	vecSortedEntities.reserve(nHighestIndex);

	const auto& entity_list = F::ANTIAIM::get("CCSPlayerController");

	for (auto& entity : entity_list)
	{
		// Get the entity
		C_BaseEntity* pEntity = I::GameResourceService->pGameEntitySystem->Get(entity->GetRefEHandle());
		if (pEntity == nullptr)
			continue;

		SchemaClassInfoData_t* pClassInfo = nullptr;
		pEntity->GetSchemaClassInfo(&pClassInfo);
		if (pClassInfo == nullptr)
			continue;

		const FNV1A_t uHashedName = FNV1A::Hash(pClassInfo->szName);

		ESortEntityType nEntityType = SORT_ENTITY_NONE;
		Vector_t vecOrigin = Vector_t();

		if (uHashedName == FNV1A::HashConst("CCSPlayerController"))
		{
			nEntityType = SORT_ENTITY_PLAYER;
			CCSPlayerController* pPlayer = reinterpret_cast<CCSPlayerController*>(pEntity);
			if (pPlayer == nullptr)
				continue;

			vecOrigin = pPlayer->GetPawnOrigin();
		}

		// only add sortable entities
		if (nEntityType != SORT_ENTITY_NONE)
			vecSortedEntities.emplace_back(pEntity, pEntity->GetRefEHandle(), SDK::CameraPosition.DistTo(vecOrigin), nEntityType);
	}



	// sort entities by distance to draw them from the farthest to the nearest
	std::ranges::sort(vecSortedEntities.begin(), vecSortedEntities.end(), std::ranges::greater{}, &SortEntityObject_t::flDistance);

	for (auto& [pEntity, hEntity, flDistance, nSortType] : vecSortedEntities)
	{
		// if the handle is invalid, skip this entity
		if (!hEntity.IsValid())
			continue;

		switch (nSortType)
		{
		case SORT_ENTITY_PLAYER:
		{
			CCSPlayerController* pPlayer = I::GameResourceService->pGameEntitySystem->Get<CCSPlayerController>(hEntity);
			if (pPlayer == nullptr)
				break;

			if (!pPlayer->IsPawnAlive())
				break;

			Player(pLocalController, pPlayer, flDistance);

			break;
		}
		default:
			break;
		}
	}
}

void OVERLAY::DrawScope(ImDrawList* pDrawList)
{
	ImGuiIO& io = ImGui::GetIO();
	const ImVec2 vecScreenSize = io.DisplaySize;

	if (SDK::LocalController == nullptr)
		return;

	if (SDK::LocalPawn == nullptr)
		return;

	if (!SDK::LocalController->IsPawnAlive())
		return;

	auto active_weapon = SDK::LocalPawn->GetActiveWeaponFromPlayer();
	if (active_weapon == nullptr)
		return;

	auto v_data = active_weapon->GetWeaponVData();
	if (v_data == nullptr)
		return;

	if (!(v_data->GetWeaponType() == WEAPONTYPE_RIFLE || v_data->GetWeaponType() == WEAPONTYPE_SNIPER_RIFLE))
		return;

	if (SDK::LocalPawn->IsScoped())
	{
		pDrawList->AddLine(ImVec2(0, vecScreenSize.y / 2), ImVec2(vecScreenSize.x, vecScreenSize.y / 2), IM_COL32(0, 0, 0, 255), 1);
		pDrawList->AddLine(ImVec2(vecScreenSize.x / 2, 0), ImVec2(vecScreenSize.x / 2, vecScreenSize.y), IM_COL32(0, 0, 0, 255), 1);
	}
}

void OVERLAY::DrawFovCircle(ImDrawList* pDrawList)
{
	if (!C_GET(bool, Vars.bLegitbot) && !C_GET(bool, Vars.bRageBot))
		return;

	return;

	if (C_GET(bool, Vars.bLegitbot))
		pDrawList->AddCircle(ImVec2(1920 / 2, 1080 / 2), C_GET(int, Vars.iAimRange), IM_COL32(0, 0, 0, 255), 512, 1.2f);

	if (C_GET(bool, Vars.bRageBot))
		pDrawList->AddCircle(ImVec2(1920 / 2, 1080 / 2), C_GET(int, Vars.iRageAimRange), IM_COL32(0, 0, 0, 255), 512, 1.2f);
}

bool OVERLAY::GetEntityBoundingBox(C_CSPlayerPawn* pEntity, ImVec4* pVecOut)
{
	CCollisionProperty* pCollision = pEntity->GetCollision();
	if (pCollision == nullptr)
		return false;

	CGameSceneNode* pGameSceneNode = pEntity->GetGameSceneNode();
	if (pGameSceneNode == nullptr)
		return false;

	CTransform nodeToWorldTransform = pGameSceneNode->GetNodeToWorld();
	const Matrix3x4_t matTransform = nodeToWorldTransform.quatOrientation.ToMatrix(nodeToWorldTransform.vecPosition);

	const Vector_t vecMins = pCollision->GetMins();
	const Vector_t vecMaxs = pCollision->GetMaxs();

	pVecOut->x = pVecOut->y = std::numeric_limits<float>::max();
	pVecOut->z = pVecOut->w = -std::numeric_limits<float>::max();

	for (int i = 0; i < 8; ++i)
	{
		const Vector_t vecPoint{ 
			i & 1 ? vecMaxs.x : vecMins.x, 
			i & 2 ? vecMaxs.y : vecMins.y,
			i & 4 ? vecMaxs.z : vecMins.z 
		};
		ImVec2 vecScreen;
		if (!D::WorldToScreen(vecPoint.Transform(matTransform), &vecScreen))
			return false;

		pVecOut->x = MATH::Min(pVecOut->x, vecScreen.x);
		pVecOut->y = MATH::Min(pVecOut->y, vecScreen.y);
		pVecOut->z = MATH::Max(pVecOut->z, vecScreen.x);
		pVecOut->w = MATH::Max(pVecOut->w, vecScreen.y);
	}

	return true;
}


namespace WeaponsIcons
{
	std::map<std::string, const wchar_t*> gunIcons = {
		{ "weapon_p90", L"P" },
		{ "weapon_mp9", L"O" },
		{ "weapon_mp5sd", L"O" },
		{ "weapon_m4a4", L"M" },
		{ "weapon_knife", L"]" },
		{ "weapon_knife_ct", L"]" },
		{ "weapon_knife_t", L"]" },
		{ "weapon_deagle", L"A" },
		{ "weapon_elite", L"B" },
		{ "weapon_fiveseven", L"C" },
		{ "weapon_glock", L"D" },
		{ "weapon_revolver", L"J" },
		{ "weapon_hkp2000", L"E" },
		{ "weapon_p250", L"F" },
		{ "weapon_usp_silencer", L"G" },
		{ "weapon_tec9", L"H" },
		{ "weapon_cz75a", L"I" },
		{ "weapon_mac10", L"K" },
		{ "weapon_ump45", L"L" },
		{ "weapon_bizon", L"M" },
		{ "weapon_mp7", L"N" },
		{ "weapon_galilar", L"Q" },
		{ "weapon_famas", L"R" },
		{ "weapon_m4a1_silencer", L"T" },
		{ "weapon_m4a1", L"S" },
		{ "weapon_aug", L"U" },
		{ "weapon_sg556", L"V" },
		{ "weapon_ak47", L"W" },
		{ "weapon_g3sg1", L"X" },
		{ "weapon_scar20", L"Y" },
		{ "weapon_awp", L"Z" },
		{ "weapon_ssg08", L"a" },
		{ "weapon_xm1014", L"b" },
		{ "weapon_sawedoff", L"c" },
		{ "weapon_mag7", L"d" },
		{ "weapon_nova", L"e" },
		{ "weapon_negev", L"f" },
		{ "weapon_m249", L"g" },
		{ "weapon_taser", L"h" },
		{ "weapon_flashbang", L"i" },
		{ "weapon_hegrenade", L"j" },
		{ "weapon_smokegrenade", L"k" },
		{ "weapon_molotov", L"l" },
		{ "weapon_decoy", L"m" },
		{ "weapon_incgrenade", L"n" },
		{ "weapon_c4", L"o" },
		{ "weapon_bayonet", L"]" },
		{ "weapon_knife_survival_bowie", L"]" },
		{ "weapon_knife_butterfly", L"]" },
		{ "weapon_knife_canis", L"]" },
		{ "weapon_knife_cord", L"]" },
		{ "weapon_knife_css", L"]" },
		{ "weapon_knife_falchion", L"]" },
		{ "weapon_knife_flip", L"]" },
		{ "weapon_knife_gut", L"]" },
		{ "weapon_knife_karambit", L"]" },
		{ "weapon_knife_twinblade", L"]" },
		{ "weapon_knife_kukri", L"]" },
		{ "weapon_knife_m9_bayonet", L"]" },
		{ "weapon_knife_outdoor", L"]" },
		{ "weapon_knife_push", L"]" },
		{ "weapon_knife_skeleton", L"]" },
		{ "weapon_knife_stiletto", L"]" },
		{ "weapon_knife_tactical", L"]" },
		{ "weapon_knife_widowmaker", L"]" },
		{ "weapon_knife_ursus", L"]" }
	};

	wchar_t get(std::string designerName)
	{
		try
		{
			return *gunIcons.at(designerName);
		}
		catch (std::out_of_range& const e)
		{
			return NULL;
		}
	}
};



void OVERLAY::Player(CCSPlayerController* pLocal, CCSPlayerController* pPlayer, const float flDistance)
{
	if (pLocal == nullptr || pPlayer == nullptr)
		return;

	if (!pPlayer->IsPawnAlive())
		return;

	if (!pLocal->GetPawnHandle().IsValid() || !pPlayer->GetPawnHandle().IsValid())
		return;

	C_CSPlayerPawn* pLocalPawn = I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(pLocal->GetPawnHandle());
	C_CSPlayerPawn* pPlayerPawn = I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(pPlayer->GetPawnHandle());

	if (pLocalPawn == nullptr || pPlayerPawn == nullptr)
		return;


	bool bIsEnemy = (pLocalPawn->IsOtherEnemy(pPlayerPawn));

	// @note: only enemy overlay for now
	if (!bIsEnemy)
		return;

	ImVec4 vecBox = {};
	if (!GetEntityBoundingBox(pPlayerPawn, &vecBox))
		return;

	Context_t context;


	if (const auto& frameOverlayConfig = C_GET(FrameOverlayVar_t, Vars.overlayBox); frameOverlayConfig.bEnable)
		context.AddBoxComponent(D::pDrawListActive, vecBox, 1, frameOverlayConfig.flThickness, frameOverlayConfig.flRounding, frameOverlayConfig.colPrimary, frameOverlayConfig.colOutline);

	if (const auto& nameOverlayConfig = C_GET(TextOverlayVar_t, Vars.overlayName); nameOverlayConfig.bEnable)
	{
		const char* szPlayerName = pPlayer->GetPlayerName();
		context.AddComponent(new CTextComponent(false, SIDE_TOP, DIR_TOP, FONT::pVisual, szPlayerName, Vars.overlayName));
	}

	if (const auto& weaponNameOverlayConfig = C_GET(TextOverlayVar_t, Vars.overlayWeaponName); weaponNameOverlayConfig.bEnable)
	{
		try
		{
			auto weapon = pPlayerPawn->GetActiveWeaponFromPlayer();
			if (weapon)
			{
				auto weaponData = weapon->GetWeaponVData();
				if (weaponData)
				{
					auto szWeaponName = weaponData->GetName();

					const char weaponIconsName = WeaponsIcons::get(szWeaponName);

					context.AddComponent(new CTextComponent(false, SIDE_RIGHT, DIR_RIGHT, FONT::pEspIcons, &weaponIconsName, Vars.overlayWeaponName));
				}
			}
		}
		catch (...)
		{ }
	}


	if (const auto& healthOverlayConfig = C_GET(BarOverlayVar_t, Vars.overlayHealthBar); healthOverlayConfig.bEnable)
	{
		// @note: pPlayerPawn->GetMaxHealth() sometime return 0.f
		const float flHealthFactor = pPlayerPawn->GetHealth() / 100.f;
		context.AddComponent(new CBarComponent(false, SIDE_LEFT, vecBox, flHealthFactor, Vars.overlayHealthBar));
	}

	if (const auto& armorOverlayConfig = C_GET(BarOverlayVar_t, Vars.overlayArmorBar); armorOverlayConfig.bEnable)
	{
		const float flArmorFactor = pPlayerPawn->GetArmorValue() / 100.f;
		context.AddComponent(new CBarComponent(false, SIDE_BOTTOM, vecBox, flArmorFactor, Vars.overlayArmorBar));
	}

	// render all the context
	context.Render(D::pDrawListActive, vecBox);
}
