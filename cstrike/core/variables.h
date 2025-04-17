#pragma once

#include "config.h"
#include "../sdk/entity.h"

#pragma region variables_combo_entries
using VisualOverlayBox_t = int;

enum EVisualOverlayBox : VisualOverlayBox_t
{
	VISUAL_OVERLAY_BOX_NONE = 0,
	VISUAL_OVERLAY_BOX_FULL,
	VISUAL_OVERLAY_BOX_CORNERS,
	VISUAL_OVERLAY_BOX_MAX
};

using VisualChamMaterial_t = int;
enum EVisualsChamMaterials : VisualChamMaterial_t
{
	VISUAL_MATERIAL_PRIMARY_WHITE = 0,
	VISUAL_MATERIAL_ILLUMINATE,
	VISUAL_MATERIAL_LATEX,
	VISUAL_MATERIAL_GLOW,
	VISUAL_MATERIAL_GLASS,
	VISUAL_MATERIAL_MAX
};

using MiscDpiScale_t = int;

enum EMiscDpiScale : MiscDpiScale_t
{
	MISC_DPISCALE_DEFAULT = 0,
	MISC_DPISCALE_125,
	MISC_DPISCALE_150,
	MISC_DPISCALE_175,
	MISC_DPISCALE_200,
	MISC_DPISCALE_MAX
};

#pragma endregion

#pragma region variables_multicombo_entries
using MenuAddition_t = unsigned int;
enum EMenuAddition : MenuAddition_t
{
	MENU_ADDITION_NONE = 0U,
	MENU_ADDITION_DIM_BACKGROUND = 1 << 0,
	MENU_ADDITION_BACKGROUND_PARTICLE = 1 << 1,
	MENU_ADDITION_GLOW = 1 << 2,
	MENU_ADDITION_ALL = MENU_ADDITION_DIM_BACKGROUND | MENU_ADDITION_BACKGROUND_PARTICLE | MENU_ADDITION_GLOW,
};
#pragma endregion


#pragma region variables_multicombo_entries
enum AntiAim
{
	AA_UP = 0,
	AA_DOWN = 1,
	AA_CENTER = 2
};

enum AntiAimType
{
	JITTER_NONE = 0,
	JITTER_CENTER = 1,
	JITTER_SPINBOT = 2,
	JITTER_3_WAY = 3
};


enum HitboxAddition
{
	HITBOX_HEAD = 1 << 0,
	HITBOX_NECK = 1 << 1,
	HITBOX_CHEST = 1 << 2,
	HITBOX_STOMACH = 1 << 3,
	HITBOX_CENTER = 1 << 4,
	HITBOX_PELVIS = 1 << 5,
	HITBOX_LEG = 1 << 6,
	HITBOX_FEET = 1 << 7,
	HITBOXES_ALL = HITBOX_HEAD | HITBOX_NECK | HITBOX_CHEST | HITBOX_STOMACH | HITBOX_CENTER | HITBOX_PELVIS | HITBOX_LEG | HITBOX_FEET,
};
#pragma endregion



struct Variables_t
{
#pragma region variables_visuals
	C_ADD_VARIABLE(bool, bVisualOverlay, false);

	C_ADD_VARIABLE(FrameOverlayVar_t, overlayBox, FrameOverlayVar_t(false));
	C_ADD_VARIABLE(TextOverlayVar_t, overlayName, TextOverlayVar_t(false));
	C_ADD_VARIABLE(TextOverlayVar_t, overlayWeaponName, TextOverlayVar_t(false));
	C_ADD_VARIABLE(BarOverlayVar_t, overlayHealthBar, BarOverlayVar_t(false, false, true, 1.5f, Color_t(0, 255, 0), Color_t(255, 0, 0)));
	C_ADD_VARIABLE(BarOverlayVar_t, overlayArmorBar, BarOverlayVar_t(false, false, false, 1.5f, Color_t(0, 255, 255), Color_t(255, 0, 0)));


	C_ADD_VARIABLE(bool, bPlayerChams, false);
	C_ADD_VARIABLE(bool, bRagdollChams, false);
	C_ADD_VARIABLE(bool, bTeamChams, false);
	C_ADD_VARIABLE(bool, bVisualChamsIgnoreZ, false); // invisible chams


	C_ADD_VARIABLE(int, nVisualChamsVisibleMaterial, VISUAL_MATERIAL_PRIMARY_WHITE);
	C_ADD_VARIABLE(int, nVisualChamsInvisibleMaterial, VISUAL_MATERIAL_PRIMARY_WHITE);

	C_ADD_VARIABLE(Color_t, colVisualChamsEnemy, Color_t(255, 0, 0));
	C_ADD_VARIABLE(Color_t, colVisualChamsEnemyIgnoreZ, Color_t(0, 0, 0));
	C_ADD_VARIABLE(Color_t, colVisualChamsTeam, Color_t(0, 255, 0));
	C_ADD_VARIABLE(Color_t, colVisualChamsTeamIgnoreZ, Color_t(255, 255, 255));


	C_ADD_VARIABLE(bool, bVisualGlow, false);
	C_ADD_VARIABLE(Color_t, colGlowEnemy, Color_t(255, 0, 0));
	C_ADD_VARIABLE(Color_t, colGlowTeam, Color_t(0, 255, 0));


	C_ADD_VARIABLE(bool, bC4Chams, false);
	C_ADD_VARIABLE(int, nVisualChamsC4Material, VISUAL_MATERIAL_ILLUMINATE);
	C_ADD_VARIABLE(Color_t, colVisualChamsC4, Color_t(255, 255, 255));


	C_ADD_VARIABLE(bool, bWeaponChams, false);
	C_ADD_VARIABLE(int, nVisualChamsWeaponMaterial, VISUAL_MATERIAL_ILLUMINATE);
	C_ADD_VARIABLE(Color_t, colVisualChamsWeapon, Color_t(255, 255, 255));


	C_ADD_VARIABLE(bool, bRemoveLegs, false);
#pragma endregion

#pragma region variables_visuals_world
	C_ADD_VARIABLE(bool, bNightMode, false);
	C_ADD_VARIABLE(Color_t, colLight, Color_t(0, 0, 0));
	C_ADD_VARIABLE(int, nLightingIntensity, 50);
	C_ADD_VARIABLE(int, nBrightness, 50);
	C_ADD_VARIABLE(Color_t, colWorld, Color_t(0, 0, 0));
	C_ADD_VARIABLE(Color_t, colSkybox, Color_t(0, 0, 0));
#pragma endregion

#pragma region variables_misc
	C_ADD_VARIABLE(bool, bAntiUntrusted, true);
	C_ADD_VARIABLE(bool, bWatermark, false);
	C_ADD_VARIABLE(bool, bDebugShootHitbox, false);

	C_ADD_VARIABLE(bool, bAutoBHop, false);
	C_ADD_VARIABLE(int, nAutoBHopChance, 100);

	C_ADD_VARIABLE(bool, bAutoStrafe, false);


	C_ADD_VARIABLE(bool, bSpectatorList, false);
	C_ADD_VARIABLE(bool, bKeybindList, false);
#pragma endregion

#pragma region variables_menu
	C_ADD_VARIABLE(unsigned int, nMenuKey, VK_INSERT);
	C_ADD_VARIABLE(unsigned int, nPanicKey, VK_END);
	C_ADD_VARIABLE(bool, bUnloadCheat, false);
	C_ADD_VARIABLE(int, nDpiScale, 0);

	/*
	 * color navigation:
	 * [definition N][purpose]
	 * 1. primitive:
	 * - primtv 0 (text)
	 * - primtv 1 (background)
	 * - primtv 2 (disabled)
	 * - primtv 3 (control bg)
	 * - primtv 4 (border)
	 * - primtv 5 (hover)
	 *
	 * 2. accents:
	 * - accent 0 (main)
	 * - accent 1 (dark)
	 * - accent 2 (darker)
	 */
	C_ADD_VARIABLE(unsigned int, bMenuAdditional, MENU_ADDITION_NONE);
	C_ADD_VARIABLE(float, flAnimationSpeed, 3.f);


	C_ADD_VARIABLE(ColorPickerVar_t, colPrimtv0, ColorPickerVar_t(255, 255, 255)); // (text)
	C_ADD_VARIABLE(ColorPickerVar_t, colPrimtv1, ColorPickerVar_t(34, 42, 52)); // (background)
	C_ADD_VARIABLE(ColorPickerVar_t, colPrimtv2, ColorPickerVar_t(58, 47, 47)); // (disabled)
	C_ADD_VARIABLE(ColorPickerVar_t, colPrimtv3, ColorPickerVar_t(26, 34, 45)); // (frame bg)
	C_ADD_VARIABLE(ColorPickerVar_t, colPrimtv4, ColorPickerVar_t(0, 0, 0)); // (border)

	C_ADD_VARIABLE(ColorPickerVar_t, colAccent0, ColorPickerVar_t(0, 163, 255)); // (main)
	C_ADD_VARIABLE(ColorPickerVar_t, colAccent1, ColorPickerVar_t(86, 86, 86)); // (dark)
	C_ADD_VARIABLE(ColorPickerVar_t, colAccent2, ColorPickerVar_t(255, 255, 255)); // (darker)
#pragma endregion
#pragma region variables_legitbot
	C_ADD_VARIABLE(bool, bLegitbot, false);
	C_ADD_VARIABLE(int, iAimRange, 10);
	C_ADD_VARIABLE(float, flSmoothing, 10.0f);
	C_ADD_VARIABLE(bool, bLegitbotAlwaysOn, false);
	C_ADD_VARIABLE(unsigned int, nLegitbotActivationKey, VK_HOME);
#pragma endregion



#pragma region variables_ragebot
	C_ADD_VARIABLE(int, iWeaponTypeSelected, 0);

	C_ADD_VARIABLE(bool, bRageBot, false);
	C_ADD_VARIABLE(bool, bSilentAim, false);
	C_ADD_VARIABLE(bool, bNoSpread, false);
	C_ADD_VARIABLE(int, iRageAimRange, 20);

	C_ADD_VARIABLE_ARRAY(int, 7, iHitChance, 70);
	C_ADD_VARIABLE_ARRAY(unsigned int, 7, bRageHitboxes, HITBOXES_ALL);
	C_ADD_VARIABLE_ARRAY(bool, 7, bMultiPoint, false);
	C_ADD_VARIABLE_ARRAY(int, 7, iPenetrationCount, 4);
	C_ADD_VARIABLE_ARRAY(int, 7, iMinDamage, 75);
	C_ADD_VARIABLE_ARRAY(int, 7, iOverrideDamage, 10);


	C_ADD_VARIABLE(bool, bAutoWall, false);
	C_ADD_VARIABLE(bool, bAutoPeek, false);
	C_ADD_VARIABLE(KeyBind_t, nAutoPeekKey, 0);

	C_ADD_VARIABLE(bool, bEnableOverrideDamage, false);
	C_ADD_VARIABLE(unsigned int, nOverrideDamageActivationKey, VK_XBUTTON1);


	C_ADD_VARIABLE(bool, bAutoFire, false);
	C_ADD_VARIABLE(unsigned int, nAutoFireKey, VK_MBUTTON);

	C_ADD_VARIABLE(bool, bDoubleTap, false);
	C_ADD_VARIABLE(KeyBind_t, nBodyAim, 0);

	C_ADD_VARIABLE(int, iBacktrackTime, 200);

	C_ADD_VARIABLE(bool, bAutoStop, false);
	C_ADD_VARIABLE(bool, bAutoScope, false);


	//           Anti Aim
	C_ADD_VARIABLE(bool, bAntiaim, false);
	C_ADD_VARIABLE(bool, bFreestading, false);
	C_ADD_VARIABLE(KeyBind_t, nLeft, 0);
	C_ADD_VARIABLE(KeyBind_t, nRight, 0);
	C_ADD_VARIABLE(KeyBind_t, nBackward, 0);
	
	C_ADD_VARIABLE(int, iAntiaimPitch, 0);
	C_ADD_VARIABLE(bool, bAntiaimYaw, false);

	C_ADD_VARIABLE(bool, bJitter, false);
	C_ADD_VARIABLE(int, iJitterType, 0);
	C_ADD_VARIABLE(int, iJitterAngle, 35);
#pragma endregion



#pragma region variables_viewmodel
	C_ADD_VARIABLE(float, flViewModelOffsetX, 2.5f);
	C_ADD_VARIABLE(float, flViewModelOffsetY, 2.0f);
	C_ADD_VARIABLE(float, flViewModelOffsetZ, -2.5f);

	C_ADD_VARIABLE(int, iFov, 90);
	C_ADD_VARIABLE(bool, bStaticFovInScope, false);

	C_ADD_VARIABLE(bool, bThirdPerson, false);
	C_ADD_VARIABLE(bool, bThirdPersonInSpec, false);
	C_ADD_VARIABLE(unsigned int, nThirdPersonActivationKey, VK_HOME);
	C_ADD_VARIABLE(int, iThirdPersonDistance, 150);

	C_ADD_VARIABLE(bool, bForceCrosshair, false);
	C_ADD_VARIABLE(bool, bRemoveVisualPunch, false);
	C_ADD_VARIABLE(bool, bHitmarker, false);

	//C_ADD_VARIABLE(unsigned int, bRemovals, HITBOXES_ALL);

	C_ADD_VARIABLE(bool, bShowSmoke, true);
	C_ADD_VARIABLE(bool, bDrawScope, true);
#pragma endregion

};

inline Variables_t Vars = {};
