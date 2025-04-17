#pragma once

// used: [d3d] api
#include <d3d11.h>
#include <dxgi1_2.h>

// used: chookobject
#include "../utilities/detourhook.h"

// used: viewmatrix_t
#include "../sdk/datatypes/matrix.h"
#include "../sdk/datatypes/aggregatesceneobject.h"

namespace VTABLE
{
	namespace D3D
	{
		enum
		{
			PRESENT = 8U,
			RESIZEBUFFERS = 13U,
			RESIZEBUFFERS_CSTYLE = 39U,
		};
	}

	namespace DXGI
	{
		enum
		{
			CREATESWAPCHAIN = 10U,
		};
	}

	namespace CLIENT
	{
		enum
		{
			CREATEMOVE = 5U,
			CAMERA = 7U,
			//MOUSEINPUTENABLED = 16U,
			MOUSEINPUTENABLED = 19U,
			FRAMESTAGENOTIFY = 36U,
		};
	}

	namespace INPUTSYSTEM
	{
		enum
		{
			ISRELATIVEMOUSEMODE = 76U,
		};
	}
}

class CRenderGameSystem;
class IViewRender;
class CCSGOInput;
class CViewSetup;
class CMeshData;
class CUserCmd;
class CEntityInstance;
class C_CSWeaponBase;
class CCSWeaponBaseVData;

class CPlayer_CameraServices;
class CGlowProperty;
class CCSInputMessage;
class CCSGOInputHistoryEntryPB;


class Cheat
{
public:
	bool alive = false;
	bool canShoot = false;
	bool canScope = false;
	bool NoSpread = false;
	bool Init = false;
	bool MapInit = false;
	bool ModulationInit = true;
	bool InThirdperson = false;
	bool IsInFreezeTime = false;
	QAngle_t StoreAngle{};
	Color_t StoreWorldColor{};

	C_CSWeaponBase* m_weapon;
	CCSWeaponBaseVData* m_wpn_data;
};

namespace H
{
	bool Setup();
	void Destroy();

	inline Cheat* cheat = new Cheat();

	/* @section: handlers */
	// d3d11 & wndproc
	HRESULT WINAPI Present(IDXGISwapChain* pSwapChain, UINT uSyncInterval, UINT uFlags);
	HRESULT CS_FASTCALL ResizeBuffers(IDXGISwapChain* pSwapChain, std::uint32_t nBufferCount, std::uint32_t nWidth, std::uint32_t nHeight, DXGI_FORMAT newFormat, std::uint32_t nFlags);
	HRESULT WINAPI CreateSwapChain(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain);
	long CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// game's functions
	ViewMatrix_t* CS_FASTCALL GetMatrixForView(CRenderGameSystem* pRenderGameSystem, IViewRender* pViewRender, ViewMatrix_t* pOutWorldToView, ViewMatrix_t* pOutViewToProjection, ViewMatrix_t* pOutWorldToProjection, ViewMatrix_t* pOutWorldToPixels);
	bool CS_FASTCALL CreateMove5(CCSGOInput* pInput, int nSlot, bool bActive);
	bool CreateMove(CCSGOInput* pInput, int nSlot, CUserCmd* pCmd);
	bool CS_FASTCALL MouseInputEnabled(void* pThisptr);
	void CS_FASTCALL FrameStageNotify(void* rcx, int nFrameStage);
	__int64* CS_FASTCALL LevelInit(void* pClientModeShared, const char* szNewMap);
	__int64 CS_FASTCALL LevelShutdown(void* pClientModeShared);
	void CS_FASTCALL OverrideView(void* pClientModeCSNormal, CViewSetup* pSetup);
	void* CS_FASTCALL ApplyViewPunch(CPlayer_CameraServices* pCameraServices, float* a2, float* a3, float* a4);
	void* CS_FASTCALL OnAddEntity(void* rcx, CEntityInstance* pInstance, int hHandle);
	void* CS_FASTCALL OnRemoveEntity(void* rcx, CEntityInstance* pInstance, int hHandle);
	void CS_FASTCALL DrawObject(void* pAnimatableSceneObjectDesc, void* pDx11, CMeshData* arrMeshDraw, int nDataCount, void* pSceneView, void* pSceneLayer, void* pUnk, void* pUnk2);
	void* IsRelativeMouseMode(void* pThisptr, bool bActive);

	void CS_FASTCALL DrawViewModel(float* a1, float* offsets, float* fov);
	void CS_FASTCALL InputHistory(CCSInputMessage* input_message, CCSGOInputHistoryEntryPB* history, bool has_attack, std::uint64_t a4, std::uint64_t a5, std::uint64_t a6);
	void* CS_FASTCALL DrawSmoke(__int64 a1, __int64 a2, __int64 a3, __int64 a4, __int64 a5, unsigned int* a6);
	void CS_FASTCALL AllowCameraAngleChange(CCSGOInput* pCSGOInput, int a2);
	float CS_FASTCALL FovChanger(CPlayer_CameraServices* a1);
	bool CS_FASTCALL EmitUiPanorama(__int64 a1, __int64 a2, const char* name, float a4);
	void CS_FASTCALL DrawSkyboxArray(std::uintptr_t* a1, std::uintptr_t* a2, std::uintptr_t* a3, int a4, int a5, std::uintptr_t* a6, std::uintptr_t* a7);
	void CS_FASTCALL LightingModulation(__int64 a1, class CAggregateSceneObjectLighting* sceneObject, __int64 a3);
	void* CS_FASTCALL UpdateAggregateSceneObject(CAggregateSceneObject* pAggregateSceneObject, void* a2);
	void CS_FASTCALL DrawAggregateSceneObject(void* a1, void* a2, CBaseSceneData* scene_data, int count, int a5, void* a6, void* a7, void* a8);

	void CS_FASTCALL DrawScope(std::uintptr_t* a1, std::uintptr_t* a2);
	bool CS_FASTCALL ForceCrosshair(std::uintptr_t* a1);


	__int64 CS_FASTCALL OriginalUpdateSkybox(void* v);

	void* CS_FASTCALL DrawGlow(CGlowProperty* glowProperty);
	void* CS_FASTCALL DrawLegs(void* a1, void* a2, void* a3, void* a4, void* a5);
	bool CS_FASTCALL IsGlowing(CGlowProperty* glow);
	void CS_FASTCALL GetGlowColor(CGlowProperty* glow, float* pColorOut);
	//void CS_FASTCALL GetGlowColor(CGlowProperty* glowProperty, float* color);
	//bool CS_FASTCALL IsGlowing(CGlowProperty* glowProperty);






	/* @section: managers */
	inline CBaseHookObject<decltype(&Present)> hkPresent = {};
	inline CBaseHookObject<decltype(&ResizeBuffers)> hkResizeBuffers = {};
	inline CBaseHookObject<decltype(&CreateSwapChain)> hkCreateSwapChain = {};
	inline CBaseHookObject<decltype(&WndProc)> hkWndProc = {};

	inline CBaseHookObject<decltype(&GetMatrixForView)> hkGetMatrixForView = {};
	inline CBaseHookObject<decltype(&CreateMove5)> hkCreateMove5 = {};
	inline CBaseHookObject<decltype(&CreateMove)> hkCreateMove = {};
	inline CBaseHookObject<decltype(&MouseInputEnabled)> hkMouseInputEnabled = {};
	inline CBaseHookObject<decltype(&IsRelativeMouseMode)> hkIsRelativeMouseMode = {};
	inline CBaseHookObject<decltype(&FrameStageNotify)> hkFrameStageNotify = {};
	inline CBaseHookObject<decltype(&LevelInit)> hkLevelInit = {};
	inline CBaseHookObject<decltype(&LevelShutdown)> hkLevelShutdown = {};
	inline CBaseHookObject<decltype(&OverrideView)> hkOverrideView = {};
	inline CBaseHookObject<decltype(&ApplyViewPunch)> hkApplyViewPunch = {};

	inline CBaseHookObject<decltype(&OnAddEntity)> hkOnAddEntity = {};
	inline CBaseHookObject<decltype(&OnRemoveEntity)> hkOnRemoveEntity = {};

	inline CBaseHookObject<decltype(&DrawObject)> hkDrawObject = {};


	inline CBaseHookObject<decltype(&DrawViewModel)> hkDrawViewModel = {};
	inline CBaseHookObject<decltype(&InputHistory)> hkInputHistory = {};
	inline CBaseHookObject<decltype(&DrawSmoke)> hkDrawSmoke = {};
	inline CBaseHookObject<decltype(&AllowCameraAngleChange)> hkAllowCameraAngleChange = {};
	inline CBaseHookObject<decltype(&FovChanger)> hkFovChanger = {};
	inline CBaseHookObject<decltype(&EmitUiPanorama)> hkEmitUiPanorama = {};
	inline CBaseHookObject<decltype(&LightingModulation)> hkLightingModulation = {};
	inline CBaseHookObject<decltype(&DrawSkyboxArray)> hkDrawSkyboxArray = {};
	inline CBaseHookObject<decltype(&UpdateAggregateSceneObject)> hkUpdateAggregateSceneObject = {};
	inline CBaseHookObject<decltype(&DrawAggregateSceneObject)> hkDrawAggregateSceneObject = {};
	inline CBaseHookObject<decltype(&DrawScope)> hkDrawScope = {};
	inline CBaseHookObject<decltype(&OriginalUpdateSkybox)> hkOriginalUpdateSkybox = {};

	inline CBaseHookObject<decltype(&ForceCrosshair)> hkForceCrosshair = {};

	inline CBaseHookObject<decltype(&DrawGlow)> hkDrawGlow = {};
	inline CBaseHookObject<decltype(&DrawLegs)> hkDrawLegs = {};
	inline CBaseHookObject<decltype(&GetGlowColor)> hkGetGlowColor = {};
	inline CBaseHookObject<decltype(&IsGlowing)> hkIsGlowing = {};
}
