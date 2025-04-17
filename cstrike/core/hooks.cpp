#include "hooks.h"

// used: variables
#include "variables.h"

// used: game's sdk
#include "../sdk/interfaces/iswapchaindx11.h"
#include "../sdk/interfaces/iviewrender.h"
#include "../sdk/interfaces/cgameentitysystem.h"
#include "../sdk/interfaces/ccsgoinput.h"
#include "../sdk/interfaces/iinputsystem.h"
#include "../sdk/interfaces/iengineclient.h"
#include "../sdk/interfaces/inetworkclientservice.h"
#include "../sdk/interfaces/iglobalvars.h"
#include "../sdk/interfaces/imaterialsystem.h"
#include "../sdk/interfaces/ipvs.h"
#include "../sdk/interfaces/cgameevents.h"
#include "../sdk/interfaces/iresourcesystem.h"
#include "../sdk/interfaces/cgameentitysystem.h"


// used: viewsetup
#include "../sdk/datatypes/viewsetup.h"

// used: entity
#include "../sdk/entity.h"

// used: get virtual function, find pattern, ...
#include "../utilities/memory.h"
// used: inputsystem
#include "../utilities/inputsystem.h"
// used: draw
#include "../utilities/draw.h"

// used: features callbacks
#include "../features.h"
// used: CRC rebuild
#include "../features/CRC.h"

// used: game's interfaces
#include "interfaces.h"
#include "sdk.h"

// used: menu
#include "menu.h"
#include "../sdk/interfaces/cgametracemanager.h"
#include <DirectXMath.h>
#include "../features/antiaim/antiaim.hpp"
#include "../features/ragebot/rage.h"
#include "../features/misc.h"
#include "../features/misc/movement.h"
#include <algorithm>


bool H::Setup()
{
	if (MH_Initialize() != MH_OK)
	{
		L_PRINT(LOG_ERROR) << CS_XOR("failed to initialize minhook");

		return false;
	}
	L_PRINT(LOG_INFO) << CS_XOR("minhook initialization completed");

	if (!hkPresent.Create(MEM::GetVFunc(I::SwapChain->pDXGISwapChain, VTABLE::D3D::PRESENT), reinterpret_cast<void*>(&Present)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"Present\" hook has been created");

	if (!hkResizeBuffers.Create(MEM::GetVFunc(I::SwapChain->pDXGISwapChain, VTABLE::D3D::RESIZEBUFFERS), reinterpret_cast<void*>(&ResizeBuffers)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"ResizeBuffers\" hook has been created");

	// creat swap chain hook
	IDXGIDevice* pDXGIDevice = NULL;
	I::Device->QueryInterface(IID_PPV_ARGS(&pDXGIDevice));

	IDXGIAdapter* pDXGIAdapter = NULL;
	pDXGIDevice->GetAdapter(&pDXGIAdapter);

	IDXGIFactory* pIDXGIFactory = NULL;
	pDXGIAdapter->GetParent(IID_PPV_ARGS(&pIDXGIFactory));

	if (!hkCreateSwapChain.Create(MEM::GetVFunc(pIDXGIFactory, VTABLE::DXGI::CREATESWAPCHAIN), reinterpret_cast<void*>(&CreateSwapChain)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"CreateSwapChain\" hook has been created");

	pDXGIDevice->Release();
	pDXGIDevice = nullptr;
	pDXGIAdapter->Release();
	pDXGIAdapter = nullptr;
	pIDXGIFactory->Release();
	pIDXGIFactory = nullptr;

	// @ida: class CViewRender->OnRenderStart call GetMatricesForView
	if (!hkGetMatrixForView.Create(MEM::FindPattern(CLIENT_DLL, CS_XOR("40 53 48 81 EC ? ? ? ? 49 8B C1")), reinterpret_cast<void*>(&GetMatrixForView)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"GetMatrixForView\" hook has been created");


	// @ida: #STR: cl: CreateMove clamped invalid attack history index %d in frame history to -1. Was %d, frame history size %d.\n
	//if (!hkCreateMove.Create(MEM::GetVFunc(I::Input, VTABLE::CLIENT::CREATEMOVE), reinterpret_cast<void*>(&CreateMove)))
	/*if (!hkCreateMove5.Create(MEM::FindPattern(CLIENT_DLL, CS_XOR("85 D2 0F 85 ? ? ? ? 48 8B C4 44 88 40")), reinterpret_cast<void*>(&CreateMove5)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"CreateMove\" hook has been created");*/

	if(!hkCreateMove.Create(MEM::FindPattern(CLIENT_DLL, CS_XOR("48 8B C4 4C 89 40 ? 48 89 48 ? 55 53 56 57 48 8D A8")), reinterpret_cast<void*>(&CreateMove)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"CreateMove\" hook has been created");


	if (!hkMouseInputEnabled.Create(MEM::GetVFunc(I::Input, VTABLE::CLIENT::MOUSEINPUTENABLED), reinterpret_cast<void*>(&MouseInputEnabled)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"MouseInputEnabled\" hook has been created");

	if (!hkFrameStageNotify.Create(MEM::GetVFunc(I::Client, VTABLE::CLIENT::FRAMESTAGENOTIFY), reinterpret_cast<void*>(&FrameStageNotify)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"FrameStageNotify\" hook has been created");

	// @ida: ClientModeShared -> #STR: "mapname", "transition", "game_newmap"
	if (!hkLevelInit.Create(MEM::FindPattern(CLIENT_DLL, CS_XOR("48 89 5C 24 ? 56 48 83 EC ? 48 8B 0D ? ? ? ? 48 8B F2")), reinterpret_cast<void*>(&LevelInit)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"LevelInit\" hook has been created");

	// @ida: ClientModeShared -> #STR: "map_shutdown"
	if (!hkLevelShutdown.Create(MEM::FindPattern(CLIENT_DLL, CS_XOR("48 83 EC ? 48 8B 0D ? ? ? ? 48 8D 15 ? ? ? ? 45 33 C9 45 33 C0 48 8B 01 FF 50 ? 48 85 C0 74 ? 48 8B 0D ? ? ? ? 48 8B D0 4C 8B 01 41 FF 50 ? 48 83 C4")), reinterpret_cast<void*>(&LevelShutdown)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"LevelShutdown\" hook has been created");

	// @note: seems to do nothing for now...
	// @ida: ClientModeCSNormal->OverrideView idx 15
	//v21 = flSomeWidthSize * 0.5;
	//v22 = *flSomeHeightSize * 0.5;
	//*(float*)(pSetup + 0x49C) = v21; // m_OrthoRight
	//*(float*)(pSetup + 0x494) = -v21; // m_OrthoLeft
	//*(float*)(pSetup + 0x498) = -v22; // m_OrthoTop
	//*(float*)(pSetup + 0x4A0) = v22; // m_OrthoBottom
	if (!hkOverrideView.Create(MEM::FindPattern(CLIENT_DLL, CS_XOR("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 48 8B FA E8")), reinterpret_cast<void*>(&OverrideView)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"OverrideView\" hook has been created");

	if (!hkDrawObject.Create(MEM::FindPattern(SCENESYSTEM_DLL, CS_XOR("48 8B C4 48 89 50 ? 53")), reinterpret_cast<void*>(&DrawObject)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"DrawObject\" hook has been created");

	if (!hkIsRelativeMouseMode.Create(MEM::GetVFunc(I::InputSystem, VTABLE::INPUTSYSTEM::ISRELATIVEMOUSEMODE), reinterpret_cast<void*>(&IsRelativeMouseMode)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"IsRelativeMouseMode\" hook has been created");

	if (!hkDrawViewModel.Create(MEM::FindPattern(CLIENT_DLL, CS_XOR("48 89 5C 24 ? 48 89 74 24 ? 55 57 41 54 41 56 41 57 48 8B EC 48 83 EC ? 4D 8B E0")), reinterpret_cast<void*>(&DrawViewModel)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"DrawViewModel\" hook has been created");

	/*if (!hkInputHistory.Create(MEM::FindPattern(CLIENT_DLL, CS_XOR("4C 89 4C 24 ? 55 53 57 41 56 48 8D 6C 24")), reinterpret_cast<void*>(&InputHistory)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"InputHistory\" hook has been created");*/

	if (!hkDrawSmoke.Create(MEM::FindPattern(CLIENT_DLL, CS_XOR("48 89 54 24 ? 55 41 55 48 8D AC 24")), reinterpret_cast<void*>(&DrawSmoke)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"DrawSmoke\" hook has been created");


	if (!hkApplyViewPunch.Create(MEM::FindPattern(CLIENT_DLL, CS_XOR("40 53 55 56 41 56 48 81 EC ? ? ? ? 48 8B D9")), reinterpret_cast<void*>(&ApplyViewPunch)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"ApplyViewPunch\" hook has been created");


	if (!hkAllowCameraAngleChange.Create(MEM::FindPattern(CLIENT_DLL, CS_XOR("85 D2 0F 85 DC 00 00 00 48")), reinterpret_cast<void*>(&AllowCameraAngleChange)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"AllowCameraAngleChange\" hook has been created");

	if (!hkFovChanger.Create(MEM::FindPattern(CLIENT_DLL, CS_XOR("40 53 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 85 C0 74 ? 48 8B C8 48 83 C4")), reinterpret_cast<void*>(&FovChanger)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"Fov Changer\" hook has been created");




	if (!hkOnAddEntity.Create(MEM::FindPattern(CLIENT_DLL, CS_XOR("48 89 74 24 10 57 48 83 EC 20 48 8B F9 41 8B C0 B9")), reinterpret_cast<void*>(&OnAddEntity)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"On Add Entity\" hook has been created");

	if (!hkOnRemoveEntity.Create(MEM::FindPattern(CLIENT_DLL, CS_XOR("48 89 74 24 10 57 48 83 EC 20 48 8B F9 41 8B C0 25")), reinterpret_cast<void*>(&OnRemoveEntity)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"On Add Entity\" hook has been created");



	/*if (!hkDrawLegs.Create(MEM::FindPattern(CLIENT_DLL, CS_XOR("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 01")), reinterpret_cast<void*>(&DrawLegs)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"Draw Legs\" hook has been created");*/

	/*if (!hkEmitUiPanorama.Create(MEM::FindPattern(CLIENT_DLL, CS_XOR("40 53 57 48 83 EC 48 48 8B 0D")), reinterpret_cast<void*>(&EmitUiPanorama)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"EmitUiPanorama\" hook has been created");*/



	/*if (!hkDrawSkyboxArray.Create(MEM::FindPattern(SCENESYSTEM_DLL, CS_XOR("45 85 C9 0F 8E ? ? ? ? 4C 8B DC 55")), reinterpret_cast<void*>(&DrawSkyboxArray)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"DrawSkyboxArray\" hook has been created");*/


	if (!hkLightingModulation.Create(MEM::FindPattern(SCENESYSTEM_DLL, CS_XOR("48 89 54 24 ? 53 41 56 41 57")), reinterpret_cast<void*>(&LightingModulation)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"LightingModulation\" hook has been created");

	/*if (!hkUpdateAggregateSceneObject.Create(MEM::FindPattern(SCENESYSTEM_DLL, CS_XOR("48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC ? 4C 8B F9")), reinterpret_cast<void*>(&UpdateAggregateSceneObject)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"UpdateAggregateSceneObject\" hook has been created");*/


	/*if (!hkDrawAggregateSceneObject.Create(MEM::FindPattern(SCENESYSTEM_DLL, CS_XOR("48 89 54 24 ? 55 57 41 55 48 8D AC 24 ? ? ? ? B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 49 63 F9")), reinterpret_cast<void*>(&DrawAggregateSceneObject)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"DrawAggregateSceneObject\" hook has been created");*/



	// Draw scope           4C 8B DC 53 56 57 48 83 EC           client.dll
	if (!hkDrawScope.Create(MEM::FindPattern(CLIENT_DLL, CS_XOR("4C 8B DC 53 56 57 48 83 EC")), reinterpret_cast<void*>(&DrawScope)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"DrawScope\" hook has been created");

	if (!hkForceCrosshair.Create(MEM::FindPattern(CLIENT_DLL, CS_XOR("48 89 5C 24 08 57 48 83 EC 20 48 8B D9 E8 ? ? ? ? 48 85")), reinterpret_cast<void*>(&ForceCrosshair)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"ForceCrosshair\" hook has been created");




	if (Events->Intilization())
		L_PRINT(LOG_INFO) << CS_XOR("Event listeners added");

	/*if (!hkOriginalUpdateSkybox.Create(MEM::FindPattern(CLIENT_DLL, CS_XOR("48 8B C4 48 89 58 18 48 89 70 20 55 57 41 54 41 55")), reinterpret_cast<void*>(&OriginalUpdateSkybox)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"OriginalUpdateSkybox\" hook has been created");*/

	/*if (!hkIsGlowing.Create(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, CS_XOR("E8 ? ? ? ? 33 DB 84 C0 0F 84 ? ? ? ? 48 8B")), 0x1), reinterpret_cast<void*>(&IsGlowing)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"IsGlowing\" hook has been created");

	if (!hkGetGlowColor.Create(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, CS_XOR("E8 ? ? ? ? F3 0F 10 BE")), 0x1), reinterpret_cast<void*>(&GetGlowColor)))
		return false;
	L_PRINT(LOG_INFO) << CS_XOR("\"GetGlowColor\" hook has been created");*/


	return true;
}

void H::Destroy()
{
	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);

	MH_Uninitialize();
}

HRESULT __stdcall H::Present(IDXGISwapChain* pSwapChain, UINT uSyncInterval, UINT uFlags)
{
	const auto oPresent = hkPresent.GetOriginal();

	// recreate it if it's not valid
	if (I::RenderTargetView == nullptr)
		I::CreateRenderTarget();

	// set our render target
	if (I::RenderTargetView != nullptr)
		I::DeviceContext->OMSetRenderTargets(1, &I::RenderTargetView, nullptr);

	F::OnPresent();

	return oPresent(I::SwapChain->pDXGISwapChain, uSyncInterval, uFlags);
}

HRESULT CS_FASTCALL H::ResizeBuffers(IDXGISwapChain* pSwapChain, std::uint32_t nBufferCount, std::uint32_t nWidth, std::uint32_t nHeight, DXGI_FORMAT newFormat, std::uint32_t nFlags)
{
	const auto oResizeBuffer = hkResizeBuffers.GetOriginal();

	auto hResult = oResizeBuffer(pSwapChain, nBufferCount, nWidth, nHeight, newFormat, nFlags);
	if (SUCCEEDED(hResult))
		I::CreateRenderTarget();

	return hResult;
}

HRESULT __stdcall H::CreateSwapChain(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
{
	const auto oCreateSwapChain = hkCreateSwapChain.GetOriginal();

	I::DestroyRenderTarget();
	L_PRINT(LOG_INFO) << CS_XOR("render target view has been destroyed");

	return oCreateSwapChain(pFactory, pDevice, pDesc, ppSwapChain);
}

long H::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (D::OnWndProc(hWnd, uMsg, wParam, lParam))
		return 1L;

	return ::CallWindowProcW(IPT::pOldWndProc, hWnd, uMsg, wParam, lParam);
}

ViewMatrix_t* CS_FASTCALL H::GetMatrixForView(CRenderGameSystem* pRenderGameSystem, IViewRender* pViewRender, ViewMatrix_t* pOutWorldToView, ViewMatrix_t* pOutViewToProjection, ViewMatrix_t* pOutWorldToProjection, ViewMatrix_t* pOutWorldToPixels)
{
	const auto oGetMatrixForView = hkGetMatrixForView.GetOriginal();
	ViewMatrix_t* matResult = oGetMatrixForView(pRenderGameSystem, pViewRender, pOutWorldToView, pOutViewToProjection, pOutWorldToProjection, pOutWorldToPixels);

	// get view matrix
	SDK::ViewMatrix = *pOutWorldToProjection;
	// get camera position
	// @note: ida @GetMatrixForView(global_pointer, pRenderGameSystem + 16, ...)
	SDK::CameraPosition = pViewRender->vecOrigin;

	return matResult;
}



Vector_t CalculateCameraPosition(Vector_t anchorPos, float distance, QAngle_t viewAngles)
{
	float yaw = DirectX::XMConvertToRadians(viewAngles.y);
	float pitch = DirectX::XMConvertToRadians(viewAngles.x);

	float x = anchorPos.x + distance * cosf(yaw) * cosf(pitch);
	float y = anchorPos.y + distance * sinf(yaw) * cosf(pitch);
	float z = anchorPos.z + distance * sinf(pitch);

	return Vector_t{ x, y, z };
}

QAngle_t CalcAngle(Vector_t viewPos, Vector_t aimPos)
{
	QAngle_t angle = { 0, 0, 0 };

	Vector_t delta = aimPos - viewPos;

	angle.x = -asin(delta.z / delta.Length()) * (180.0f / 3.141592654f);
	angle.y = atan2(delta.y, delta.x) * (180.0f / 3.141592654f);

	return angle;
}

QAngle_t NormalizeAngles(QAngle_t angles)
{
	while (angles.x > 89.0f)
		angles.x -= 180.0f;
	while (angles.x < -89.0f)
		angles.x += 180.0f;
	while (angles.y > 180.0f)
		angles.y -= 360.0f;
	while (angles.y < -180.0f)
		angles.y += 360.0f;
	angles.z = 0.0f;
	return angles;
}


CUserCmd* GetUserCmd(CCSPlayerController* local_controller)
{
	static auto get_command_index = reinterpret_cast<void*(__fastcall*)(void*, int*)>(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 8B 8D ? ? ? ? 8D 51"), 1, 0));
	if (!get_command_index)
		return nullptr;

	int index = 0;
	get_command_index(local_controller, &index);
	int command_index = index - 1;

	if (command_index == -1)
		command_index = 0xFFFFFFFFLL;

	static auto get_user_cmd_base = reinterpret_cast<void*(__fastcall*)(void*, int)>(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 48 8B CF 4C 8B E8 44 8B B8"), 1, 0));
	if (!get_user_cmd_base)
		return nullptr;

	static void* cmd_base_address = *reinterpret_cast<void**>(MEM::ResolveRelativeAddress(MEM::FindPattern(CLIENT_DLL, "48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B CF 4C 8B E8"), 0x3, 0x7));
	if (!cmd_base_address)
		return nullptr;

	auto user_cmd_base = get_user_cmd_base(cmd_base_address, command_index);
	if (!user_cmd_base)
		return nullptr;

	DWORD sequence_number = *reinterpret_cast<DWORD*>((uintptr_t)user_cmd_base + 0x5C00);

	static auto get_user_cmd = reinterpret_cast<CUserCmd*(__fastcall*)(void*, DWORD)>(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 48 8B 0D ? ? ? ? 45 33 E4 48 89 44 24"), 1, 0));
	if (!get_user_cmd)
		return nullptr;

	auto user_cmd = get_user_cmd(local_controller, sequence_number);
	return user_cmd;
}

CUserCmd* GetUserCmdDon()
{
	CCSPlayerController* local_controller = CCSPlayerController::GetLocalPlayerController();
	if (!local_controller)
		return nullptr;

	static auto get_command_index = reinterpret_cast<void*(__fastcall*)(void*, int*)>(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 8B 8D ? ? ? ? 8D 51"), 0x1, 0x0));
	if (!get_command_index)
		return nullptr;

	int index = 0;
	get_command_index(local_controller, &index);
	unsigned int command_index = index - 1;

	if (command_index == -1)
		command_index = 0xFFFFFFFF;


	static void* cmd_base_address = MEM::ResolveRelativeAddress(MEM::FindPattern(CLIENT_DLL, "48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B CF 4C 8B E8"), 0x3, 0x7);
	if (!cmd_base_address)
		return nullptr;

	static auto get_user_cmd_base = reinterpret_cast<void*(__fastcall*)(void*, int)>(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 48 8B CF 4C 8B E8 44 8B B8"), 0x1, 0x0));
	if (!get_user_cmd_base)
		return nullptr;

	void* user_cmd_base = get_user_cmd_base(cmd_base_address, command_index);
	if (!user_cmd_base)
		return nullptr;

	auto sequence_number = (DWORD)(*(__int64*)user_cmd_base + 0x5C00);

	static auto get_user_cmd = reinterpret_cast<CUserCmd*(__fastcall*)(void*, DWORD)>(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 48 8B 0D ? ? ? ? 45 33 E4 48 89 44 24"), 0x1, 0x0));
	if (!get_user_cmd)
		return nullptr;

	CUserCmd* user_cmd = get_user_cmd(local_controller, sequence_number);
	return user_cmd;
}



bool CS_FASTCALL H::CreateMove5(CCSGOInput* pInput, int nSlot, bool bActive)
{
	const auto oCreateMove = hkCreateMove5.GetOriginal();
	const bool bResult = oCreateMove(pInput, nSlot, bActive);

	if (!I::Engine->IsConnected() || !I::Engine->IsInGame())
		return bResult;

	CCSPlayerController* pLocalController = SDK::LocalController = CCSPlayerController::GetLocalPlayerController();
	if (!pLocalController)
		return bResult;

	C_CSPlayerPawn* pLocalPawn = SDK::LocalPawn = I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(pLocalController->GetPawnHandle());
	if (!pLocalPawn)
		return bResult;

	auto pCmd = SDK::Cmd = GetUserCmd(pLocalController);
	if (pCmd == nullptr)
		return bResult;

	auto pBaseCmd = pCmd->csgoUserCmd.pBaseCmd;
	if (pBaseCmd == nullptr)
		return bResult;


	if (pBaseCmd->pViewAngles)
		F::ANTIAIM::angStoredViewBackup = pBaseCmd->pViewAngles->angValue;

	cheat->m_weapon = pLocalPawn->GetActiveWeaponFromPlayer();
	if (cheat->m_weapon != nullptr)
		cheat->m_wpn_data = cheat->m_weapon->GetWeaponVData();
	else
		cheat->m_wpn_data = nullptr;

	if (cheat->m_weapon == nullptr || cheat->m_wpn_data == nullptr)
		return bResult;


	F::ANTIAIM::RunAA(pCmd, pBaseCmd, pLocalPawn);
	F::MISC::MOVEMENT::MovementCorrection(pBaseCmd, F::ANTIAIM::angStoredViewBackup);

	F::MISC::OnMove(pCmd, pBaseCmd, pLocalController, pLocalPawn);


	F::OnCreateMove(pCmd, pBaseCmd, SDK::LocalController);

	CRC::Save(pBaseCmd, pCmd);
	if (CRC::CalculateCRC(pBaseCmd) == true)
		CRC::Apply(pCmd);

	return bResult;
}


bool H::CreateMove(CCSGOInput* pInput, int nSlot, CUserCmd* pCmd)
{
	const auto oCreateMove = hkCreateMove.GetOriginal();
	const bool bResult = oCreateMove(pInput, nSlot, pCmd);

	if (!I::Engine->IsConnected() || !I::Engine->IsInGame())
		return bResult;

	if (!pCmd)
		return bResult;

	SDK::Cmd = pCmd;

	CCSPlayerController* pLocalController = SDK::LocalController = CCSPlayerController::GetLocalPlayerController();
	if (!pLocalController)
		return bResult;

	C_CSPlayerPawn* pLocalPawn = SDK::LocalPawn = I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(pLocalController->GetPredictedPawnHandle());
	if (!pLocalPawn)
		return bResult;

	auto pBaseCmd = pCmd->csgoUserCmd.pBaseCmd;
	if (pBaseCmd == nullptr)
		return bResult;

	if (pBaseCmd->pViewAngles)
		F::ANTIAIM::angStoredViewBackup = pBaseCmd->pViewAngles->angValue;

	cheat->m_weapon = pLocalPawn->GetActiveWeaponFromPlayer();
	if (cheat->m_weapon != nullptr)
		cheat->m_wpn_data = cheat->m_weapon->GetWeaponVData();
	else
		cheat->m_wpn_data = nullptr;

	if (cheat->m_weapon == nullptr || cheat->m_wpn_data == nullptr)
		return bResult;

	F::ANTIAIM::RunAA(pCmd, pBaseCmd, pLocalPawn);
	F::MISC::MOVEMENT::MovementCorrection(pBaseCmd, F::ANTIAIM::angStoredViewBackup);

	F::MISC::OnMove(pCmd, pBaseCmd, pLocalController, pLocalPawn);

	F::OnCreateMove(pCmd, pBaseCmd, SDK::LocalController);

	CRC::Save(pBaseCmd, pCmd);
	if (CRC::CalculateCRC(pBaseCmd) == true)
		CRC::Apply(pCmd);

	return bResult;
}


bool CS_FASTCALL H::MouseInputEnabled(void* pThisptr)
{
	const auto oMouseInputEnabled = hkMouseInputEnabled.GetOriginal();
	return MENU::bMainWindowOpened ? false : oMouseInputEnabled(pThisptr);
}


void CS_FASTCALL H::FrameStageNotify(void* rcx, int nFrameStage)
{
	const auto oFrameStageNotify = hkFrameStageNotify.GetOriginal();
	F::OnFrameStageNotify(nFrameStage);


	if (IPT::IsKeyReleased(C_GET(unsigned int, Vars.nThirdPersonActivationKey)))
	{
		bool* v = &C_GET(bool, Vars.bThirdPerson);
		*v = !(*v);
	}

	/*if (IPT::IsKeyReleased(C_GET(unsigned int, Vars.nAutoFireKey)))
	{
		bool* v = &C_GET(bool, Vars.bAutoFire);
		*v = !(*v);
	}*/

	if (IPT::IsKeyReleased(C_GET(unsigned int, Vars.nOverrideDamageActivationKey)))
	{
		bool* v = &C_GET(bool, Vars.bEnableOverrideDamage);
		*v = !(*v);
	}


	return oFrameStageNotify(rcx, nFrameStage);
}

__int64* CS_FASTCALL H::LevelInit(void* pClientModeShared, const char* szNewMap)
{
	const auto oLevelInit = hkLevelInit.GetOriginal();
	// if global variables are not captured during I::Setup or we join a new game, recapture it
	if (I::GlobalVars == nullptr)
		I::GlobalVars = *reinterpret_cast<IGlobalVars**>(MEM::ResolveRelativeAddress(MEM::FindPattern(CLIENT_DLL, CS_XOR("48 89 15 ? ? ? ? 48 89 42")), 0x3, 0x7));

	F::ANTIAIM::level_init();
	F::RAGEBOT::level_init();

	// disable model occlusion
	I::PVS->Set(false);

	return oLevelInit(pClientModeShared, szNewMap);
}

__int64 CS_FASTCALL H::LevelShutdown(void* pClientModeShared)
{
	const auto oLevelShutdown = hkLevelShutdown.GetOriginal();
	// reset global variables since it got discarded by the game
	I::GlobalVars = nullptr;

	F::ANTIAIM::level_shutdown();
	F::RAGEBOT::level_shutdown();

	return oLevelShutdown(pClientModeShared);
}

void CS_FASTCALL H::OverrideView(void* pClientModeCSNormal, CViewSetup* pSetup)
{
	const auto oOverrideView = hkOverrideView.GetOriginal();

	if (!I::Engine->IsConnected() || !I::Engine->IsInGame() || !SDK::LocalController || !SDK::LocalPawn || !pSetup)
		return oOverrideView(pClientModeCSNormal, pSetup);

	if (!cheat->m_wpn_data)
		return oOverrideView(pClientModeCSNormal, pSetup);


	if (!SDK::LocalController->IsPawnAlive() && C_GET(bool, Vars.bThirdPersonInSpec) && C_GET(bool, Vars.bThirdPerson))
	{
		auto pObserverServices = SDK::LocalPawn->GetObserverServices();
		if (!pObserverServices)
			return oOverrideView(pClientModeCSNormal, pSetup);

		if (pObserverServices->GetObserverMode() == ObserverMode_t::OBS_MODE_IN_EYE)
		{
			pObserverServices->GetObserverMode() = C_GET(bool, Vars.bThirdPersonInSpec) ? ObserverMode_t::OBS_MODE_CHASE : ObserverMode_t::OBS_MODE_IN_EYE;
			pObserverServices->GetObserverChaseDistance() = C_GET(int, Vars.iThirdPersonDistance);
			pObserverServices->GetObserverChaseDistanceCalcTime() = 0.f;
		}

		return oOverrideView(pClientModeCSNormal, pSetup);
	}

	if (!SDK::LocalController->IsPawnAlive())
		return oOverrideView(pClientModeCSNormal, pSetup);


	auto WeaponType = cheat->m_wpn_data->GetWeaponType();
	if (WeaponType == WEAPONTYPE_GRENADE)
		return oOverrideView(pClientModeCSNormal, pSetup);

	if (C_GET(bool, Vars.bThirdPerson))
	{
		QAngle_t adjusted_cam_view_angle = F::ANTIAIM::angStoredViewBackup;
		adjusted_cam_view_angle.x = -adjusted_cam_view_angle.x;

		Vector_t newCameraPos = CalculateCameraPosition(SDK::LocalPawn->GetEyePosition(), -C_GET(int, Vars.iThirdPersonDistance), adjusted_cam_view_angle);
		pSetup->vecOrigin =  newCameraPos;

		Ray_t ray{};
		GameTrace_t trace{};
		TraceFilter_t filter{ 0x1C3003, SDK::LocalPawn, NULL, 4 };


		if (I::GameTraceManager->TraceShape(&ray, SDK::LocalPawn->GetEyePosition(), pSetup->vecOrigin, &filter, &trace))
		{
			if (trace.m_pHitEntity != nullptr)
				pSetup->vecOrigin = trace.m_vecPosition;
		}

		QAngle_t p = NormalizeAngles(CalcAngle(pSetup->vecOrigin, SDK::LocalPawn->GetEyePosition()));


		pSetup->angView = QAngle_t{ p.x, p.y, };
	}


	return oOverrideView(pClientModeCSNormal, pSetup);
}


void* CS_FASTCALL H::ApplyViewPunch(CPlayer_CameraServices* pCameraServices, float* a2, float* a3, float* a4)
{
	const auto oApplyViewPunch = hkApplyViewPunch.GetOriginal();

	if (C_GET(bool, Vars.bRemoveVisualPunch))
		pCameraServices->GetViewPunchAngle() = Vector_t();

	return oApplyViewPunch(pCameraServices, a2, a3, a4);
}



void* CS_FASTCALL H::OnAddEntity(void* rcx, CEntityInstance* pInstance, int hHandle)
{
	const auto original = hkOnAddEntity.GetOriginal();

	F::ANTIAIM::add_entity(pInstance, hHandle);

	return original(rcx, pInstance, hHandle);
}

void* CS_FASTCALL H::OnRemoveEntity(void* rcx, CEntityInstance* pInstance, int hHandle)
{
	const auto original = hkOnRemoveEntity.GetOriginal();

	F::ANTIAIM::remove_entity(pInstance, hHandle);

	return original(rcx, pInstance, hHandle);
}



void* CS_FASTCALL H::DrawSmoke(__int64 a1, __int64 a2, __int64 a3, __int64 a4, __int64 a5, unsigned int* a6)
{
	const auto oDrawSmoke = hkDrawSmoke.GetOriginal();
	if (C_GET(bool, Vars.bShowSmoke))
		return oDrawSmoke(a1, a2, a3, a4, a5, a6);

	return NULL;
}

void CS_FASTCALL H::AllowCameraAngleChange(CCSGOInput* pCSGOInput, int a2)
{
	const auto oAllowCameraAngleChange = hkAllowCameraAngleChange.GetOriginal();

	if (!I::Engine->IsInGame() || !I::Engine->IsConnected() || !pCSGOInput)
		return oAllowCameraAngleChange(pCSGOInput, a2);


	CUserCmd* pCmd = SDK::Cmd;
	if (pCmd == nullptr)
		return oAllowCameraAngleChange(pCSGOInput, a2);

	CBaseUserCmdPB* pBaseCmd = pCmd->csgoUserCmd.pBaseCmd;
	if (pBaseCmd == nullptr)
		return oAllowCameraAngleChange(pCSGOInput, a2);

	if (pBaseCmd->pViewAngles == nullptr)
		return oAllowCameraAngleChange(pCSGOInput, a2);

	QAngle_t angOriginalAngle = pBaseCmd->pViewAngles->angValue;

	pBaseCmd->pViewAngles->angValue = F::ANTIAIM::angStoredViewBackup;
	oAllowCameraAngleChange(pCSGOInput, a2);
	pBaseCmd->pViewAngles->angValue = angOriginalAngle;

	/*QAngle_t angOriginalAngle = pCSGOInput->GetViewAngles();

	pCSGOInput->SetViewAngle(F::ANTIAIM::angStoredViewBackup);
	oAllowCameraAngleChange(pCSGOInput, a2);
	pCSGOInput->SetViewAngle(angOriginalAngle);*/
}

float CS_FASTCALL H::FovChanger(CPlayer_CameraServices* a1)
{
	const auto oFovChanger = hkFovChanger.GetOriginal();

	if (!I::Engine->IsInGame() || !I::Engine->IsConnected())
		return oFovChanger(a1);

	if (SDK::LocalController == nullptr || SDK::LocalPawn == nullptr)
		return oFovChanger(a1);

	if (!SDK::LocalController->IsPawnAlive())
		return oFovChanger(a1);

	if (!cheat->m_wpn_data)
		return oFovChanger(a1);

	if (cheat->m_wpn_data->GetWeaponType() == WEAPONTYPE_RIFLE || cheat->m_wpn_data->GetWeaponType() == WEAPONTYPE_SNIPER_RIFLE)
	{
		if (SDK::LocalPawn->IsScoped())
		{
			if (C_GET(bool, Vars.bStaticFovInScope))
				return C_GET(int, Vars.iFov);

			else
				return oFovChanger(a1);
		}
	}

	return C_GET(int, Vars.iFov);
}

bool CS_FASTCALL H::EmitUiPanorama(__int64 a1, __int64 a2, const char* name, float a4)
{
	const auto oEmitUiPanorama = hkEmitUiPanorama.GetOriginal();

	return oEmitUiPanorama(a1, a2, name, a4);

	L_PRINT(LOG_INFO) << CS_XOR("In EmitUiPanorama");

	if (!name)
		return oEmitUiPanorama(a1, a2, name, a4);

	L_PRINT(LOG_INFO) << name;


	if (FNV1A::Hash(name) == FNV1A::Hash("popup_accept_match_found"))
	{
		using fnSetViewAngle = bool(CS_FASTCALL*)(void*, const char*);
		static auto fn = reinterpret_cast<fnSetViewAngle>(MEM::FindPattern(CLIENT_DLL, CS_XOR("40 53 48 83 EC 20 48 8B DA 48 8D 15 ?? ?? ?? ?? 48 8B CB FF 15 ?? ?? ?? ?? 85 C0 75 12")));

		fn(nullptr, "");
	}

	return oEmitUiPanorama(a1, a2, name, a4);
}

void CS_FASTCALL H::DrawObject(void* pAnimatableSceneObjectDesc, void* pDx11, CMeshData* arrMeshDraw, int nDataCount, void* pSceneView, void* pSceneLayer, void* pUnk, void* pUnk2)
{
	const auto oDrawObject = hkDrawObject.GetOriginal();

	if (!I::Engine->IsConnected() || !I::Engine->IsInGame())
		return oDrawObject(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2);

	if (!SDK::LocalController || !SDK::LocalPawn)
		return oDrawObject(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2);

	if (!F::OnDrawObject(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2))
		return oDrawObject(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2);
}

void* H::IsRelativeMouseMode(void* pThisptr, bool bActive)
{
	const auto oIsRelativeMouseMode = hkIsRelativeMouseMode.GetOriginal();

	MENU::bMainActive = bActive;

	if (MENU::bMainWindowOpened)
		return oIsRelativeMouseMode(pThisptr, false);

	return oIsRelativeMouseMode(pThisptr, bActive);
}


void CS_FASTCALL H::DrawViewModel(float* a1, float* offsets, float* fov)
{
	const auto oDrawViewModel = hkDrawViewModel.GetOriginal();

	oDrawViewModel(a1, offsets, fov);

	if (!I::Engine->IsInGame() || !I::Engine->IsConnected())
		return;

	if (!SDK::LocalController || !SDK::LocalPawn)
		return;

	if (!SDK::LocalController->IsPawnAlive())
		return;

	offsets[0] = C_GET(float, Vars.flViewModelOffsetX);
	offsets[1] = C_GET(float, Vars.flViewModelOffsetY);
	offsets[2] = C_GET(float, Vars.flViewModelOffsetZ);
}


// 4C 89 4C 24 ? 55 53 57 41 56 48 8D 6C 24
void CS_FASTCALL H::InputHistory(CCSInputMessage* input_message, CCSGOInputHistoryEntryPB* history, bool has_attack, std::uint64_t a4, std::uint64_t a5, std::uint64_t a6)
{
	if (F::ANTIAIM::bestTargetSimTime != 0)
	{
		const int best_tick = TIME_TO_TICKS(F::ANTIAIM::bestTargetSimTime) + 1;

		if (history && input_message && SDK::Cmd)
		{
			if (!history)
				history = SDK::Cmd->AddInputHistory();


			/*if (history->cl_interp != nullptr)
			{
				history->cl_interp->flFraction = 0.999f;
				history->SetBits(EInputHistoryBits::INPUT_HISTORY_BITS_CL_INTERP);
			}*/

			if (history->sv_interp0 != nullptr)
			{
				history->sv_interp0->nSrcTick = best_tick;
				history->sv_interp0->nDstTick = best_tick + 1;
				history->sv_interp0->flFraction = 0.f;
				history->SetBits(EInputHistoryBits::INPUT_HISTORY_BITS_SV_INTERP0);
			}

			if (history->sv_interp1 != nullptr)
			{
				history->sv_interp1->nSrcTick = best_tick + 1;
				history->sv_interp1->nDstTick = best_tick + 2;
				history->sv_interp1->flFraction = 0.f;
				history->SetBits(EInputHistoryBits::INPUT_HISTORY_BITS_SV_INTERP1);
			}

			//if (history->player_interp != nullptr)
			//{
			//	//pInputEntry->sv_interp1 = pInputEntry->create_new_interp();
			//	history->player_interp->nSrcTick = best_tick + 1;
			//	history->player_interp->nDstTick = best_tick + 2;
			//	history->player_interp->flFraction = 0.f;
			//	history->SetBits(EInputHistoryBits::INPUT_HISTORY_BITS_PLAYER_INTERP);
			//}

			/*if (history->pViewAngles)
			{
				history->pViewAngles->angValue = F::ANTIAIM::silentViewAngle.Clamp();
				history->SetBits(EInputHistoryBits::INPUT_HISTORY_BITS_VIEWANGLES);
			}*/

			/*history->nRenderTickCount = best_tick + 1;
			history->SetBits(EInputHistoryBits::INPUT_HISTORY_BITS_RENDERTICKCOUNT);*/


			if (SDK::LocalController)
			{
				history->nPlayerTickCount = SDK::LocalController->GetTickBase();
				history->SetBits(EInputHistoryBits::INPUT_HISTORY_BITS_PLAYERTICKCOUNT);

				//input_message->m_player_tick_count = SDK::LocalController->GetTickBase();
			}

			//input_message->m_view_angles = Vector_t(F::ANTIAIM::silentViewAngle.x, F::ANTIAIM::silentViewAngle.y, F::ANTIAIM::silentViewAngle.z);

			//input_message->m_view_angles =
		}
	}

	return hkInputHistory.GetOriginal()(input_message, history, true, a4, a5, a6);
}



// scenesystem.dll 45 85 C9 0F 8E ? ? ? ? 4C 8B DC 55
void CS_FASTCALL H::DrawSkyboxArray(std::uintptr_t* a1, std::uintptr_t* a2, std::uintptr_t* a3, int a4, int a5, std::uintptr_t* a6, std::uintptr_t* a7)
{
	try
	{
		I::MaterialSystem2->SetColor(a3, Color_t(0, 0, 255));
	}
	catch (...)
	{
		return hkDrawSkyboxArray.GetOriginal()(a1, a2, a3, a4, a5, a6, a7);
	}
	//
}

void CS_FASTCALL H::LightingModulation(__int64 a1, CAggregateSceneObjectLighting* sceneObject, __int64 a3)
{
	const auto oLightingModulation = hkLightingModulation.GetOriginal();

	if (sceneObject == nullptr)
		return oLightingModulation(a1, sceneObject, a3);

	if (!C_GET(bool, Vars.bNightMode))
		return oLightingModulation(a1, sceneObject, a3);

	float flIntensity = C_GET(int, Vars.nLightingIntensity) / 50.f;
	Color_t color = C_GET(Color_t, Vars.colLight);

	sceneObject->red = color.r / 255.0f * flIntensity;
	sceneObject->green = color.g / 255.0f * flIntensity;
	sceneObject->blue = color.b / 255.0f * flIntensity;

	oLightingModulation(a1, sceneObject, a3);
}

void* CS_FASTCALL H::UpdateAggregateSceneObject(CAggregateSceneObject* pAggregateSceneObject, void* a2)
{
	const auto oUpdateAggregateSceneObject = hkUpdateAggregateSceneObject.GetOriginal();

	CAggregateSceneObject* pAggregateSceneObjectBackup = pAggregateSceneObject;

	if (cheat->Init)
	{
		try
		{
			if (pAggregateSceneObject == nullptr)
				return oUpdateAggregateSceneObject(pAggregateSceneObject, a2);

			if (pAggregateSceneObject->array == nullptr)
				return oUpdateAggregateSceneObject(pAggregateSceneObject, a2);


			if (cheat->ModulationInit)
			{
				C_GET(Color_t, Vars.colWorld) = Color_t(pAggregateSceneObject->array->r, pAggregateSceneObject->array->g, pAggregateSceneObject->array->b);
				cheat->StoreWorldColor = Color_t(pAggregateSceneObject->array->r, pAggregateSceneObject->array->g, pAggregateSceneObject->array->b);
				cheat->ModulationInit = false;
			}

			if (cheat->MapInit)
			{
				cheat->StoreWorldColor = Color_t(pAggregateSceneObject->array->r, pAggregateSceneObject->array->g, pAggregateSceneObject->array->b);
				cheat->MapInit = false;
			}


			Color_t nightModeColor = C_GET(bool, Vars.bNightMode) ? C_GET(Color_t, Vars.colWorld) : cheat->StoreWorldColor;

			pAggregateSceneObject->array->r = nightModeColor.r;
			pAggregateSceneObject->array->g = nightModeColor.g;
			pAggregateSceneObject->array->b = nightModeColor.b;

			return oUpdateAggregateSceneObject(pAggregateSceneObject, a2);
		}
		catch (...)
		{ }
	}

	return oUpdateAggregateSceneObject(pAggregateSceneObjectBackup, a2);
}


enum e_model_type : int
{
	MODEL_SUN,
	MODEL_EFFECTS,
	MODEL_OTHER,
};

int get_model_type(const std::string_view& name)
{
	if (name.find("sun") != std::string::npos || name.find("clouds") != std::string::npos)
		return MODEL_SUN;

	if (name.find("effects") != std::string::npos)
		return MODEL_EFFECTS;

	return MODEL_OTHER;
}

void CS_FASTCALL H::DrawAggregateSceneObject(void* a1, void* a2, CBaseSceneData* scene_data, int count, int a5, void* a6, void* a7, void* a8)
{
	auto oDrawAggregateSceneObject = hkDrawAggregateSceneObject.GetOriginal();


	if (!C_GET(bool, Vars.bNightMode))
		return oDrawAggregateSceneObject(a1, a2, scene_data, count, a5, a6, a7, a8);

	try
	{
		if (!scene_data)
			return oDrawAggregateSceneObject(a1, a2, scene_data, count, a5, a6, a7, a8);

		if (!scene_data->material)
			return oDrawAggregateSceneObject(a1, a2, scene_data, count, a5, a6, a7, a8);

		int type = get_model_type(scene_data->material->GetName());

		Color_t color{ 0, 0, 0 };

		switch (type)
		{
		case MODEL_SUN:
			color = { 0, 0, 0 };
			break;
		case MODEL_EFFECTS:
		case MODEL_OTHER:
			color = C_GET(Color_t, Vars.colWorld);
			break;
		}

		for (int i = 0; i < count; ++i)
		{
			auto scene = &scene_data[i];
			if (!scene)
				continue;

			scene->r = color.r;
			scene->g = color.g;
			scene->b = color.b;
			scene->a = 255;
		}

		oDrawAggregateSceneObject(a1, a2, scene_data, count, a5, a6, a7, a8);
	}
	catch (...)
	{ }

	
}

void CS_FASTCALL H::DrawScope(std::uintptr_t* a1, std::uintptr_t* a2)
{
	const auto oDrawScope = hkDrawScope.GetOriginal();

	if (!C_GET(bool, Vars.bDrawScope))
		return;

	return oDrawScope(a1, a2);
}

bool CS_FASTCALL H::ForceCrosshair(std::uintptr_t* a1)
{
	const auto oForceCrosshair = hkForceCrosshair.GetOriginal();

	if (!SDK::LocalPawn)
		return oForceCrosshair(a1);

	if (SDK::LocalPawn->GetHealth() <= 0)
		return oForceCrosshair(a1);

	if (C_GET(bool, Vars.bForceCrosshair) && !SDK::LocalPawn->IsScoped())
		return true;

	return oForceCrosshair(a1);
}




__int64 CS_FASTCALL H::OriginalUpdateSkybox(void* v)
{
	const auto oUpdateAggregateSceneObject = hkOriginalUpdateSkybox.GetOriginal();
	return oUpdateAggregateSceneObject(v);
}




//void CS_FASTCALL H::GetGlowColor(CGlowProperty* glowProperty, float* color)
//{
//	const auto oGetGlowColor = hkGetGlowColor.GetOriginal();
//
//
//	if (!I::Engine->IsInGame() || !I::Engine->IsConnected())
//		return oGetGlowColor(glowProperty, color);
//
//
//	try
//	{
//		auto hOwner = glowProperty->m_pOwnerHandle;
//		if (hOwner == nullptr)
//			return oGetGlowColor(glowProperty, color);
//
//
//		C_CSPlayerPawn* pPawn = I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(hOwner->GetRefEHandle());
//		if (pPawn == nullptr)
//			return oGetGlowColor(glowProperty, color);
//
//		if (pPawn->GetHealth() <= 0)
//			return oGetGlowColor(glowProperty, color);
//
//
//
//		if (SDK::LocalPawn && (SDK::LocalPawn == pPawn))
//			return oGetGlowColor(glowProperty, color);
//
//
//		Color_t color;
//		if (SDK::LocalPawn->IsOtherEnemy(pPawn))
//			color = C_GET(Color_t, Vars.colGlowEnemy);
//		else
//			color = C_GET(Color_t, Vars.colGlowTeam);
//
//		color[0] = color.r;
//		color[1] = color.g;
//		color[2] = color.b;
//	}
//	catch (...)
//	{
//		
//	}
//
//	return oGetGlowColor(glowProperty, color);
//}
//
//bool CS_FASTCALL H::IsGlowing(CGlowProperty* glowProperty)
//{
//	if (glowProperty == nullptr)
//		return false;
//
//	return glowProperty->m_pOwnerHandle && glowProperty->m_bGlowing;
//}


void* CS_FASTCALL H::DrawLegs(void* a1, void* a2, void* a3, void* a4, void* a5)
{
	const auto oDrawLegs = hkDrawLegs.GetOriginal();

	if (!I::Engine->IsInGame() || I::Engine->IsConnected())
		return DrawLegs(a1, a2, a3, a4, a5);

	if (C_GET(bool, Vars.bRemoveLegs))
		return nullptr;

	return DrawLegs(a1, a2, a3, a4, a5);
}



bool CS_FASTCALL H::IsGlowing(CGlowProperty* glow)
{
	if (!I::Engine->IsInGame())
		return false;

	CBasePlayerController* con = SDK::LocalController;

	if (!con)
		return false;

	C_CSPlayerPawn* lp = SDK::LocalPawn;

	if (!lp)
		return false;

	if (!glow)
		return false;

	if (!glow->Owner())
		return false;

	if (glow->m_bGlowing())
		return true;

	CEntityIdentity* identify = reinterpret_cast<CEntityInstance*>(glow->Owner())->GetIdentity();

	if (!identify)
		return false;

	const char* class_name = identify->GetDesignerName();
	if (!class_name)
		return false;

	std::string sName = class_name;
	if (sName.find("player") != std::string::npos && (C_GET(bool, Vars.bVisualGlow)))
	{
		CCSPlayerController* pPlayer = reinterpret_cast<CCSPlayerController*>(glow->Owner());
		if (pPlayer == nullptr)
			return false;

		C_CSPlayerPawn* pawn = I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(pPlayer->GetPawnHandle());
		if (pawn == nullptr)
			return false;

		if (pawn == lp)
		{
			/*if (g_Config::selfGlow && g_Config::bThirdPerson)
				return true;*/
			return false;
		}

		if (pawn->GetHealth() > 0.f && (pawn->GetTeam() != lp->GetTeam()))
			return true;
	}
	//else if (sName.find("weapon") != std::string::npos && settings::visuals::esp::items::glow) {
	//	CBasePlayerWeapon* weapon = reinterpret_cast<CBasePlayerWeapon*>(glow->m_pOwner);
	//
	//	if (weapon->GetHandleOwner() == 0xFFFFFFFF)
	//		return true;
	//}

	return false;
}

void CS_FASTCALL H::GetGlowColor(CGlowProperty* glow, float* pColorOut)
{
	const auto oGetGlowColor = hkGetGlowColor.GetOriginal();
	if (!glow)
		return oGetGlowColor(glow, pColorOut);

	CEntityIdentity* identify = reinterpret_cast<CEntityInstance*>(glow->Owner())->GetIdentity();

	if (!identify)
		return oGetGlowColor(glow, pColorOut);

	const char* class_name = identify->GetDesignerName();
	if (!class_name)
		return oGetGlowColor(glow, pColorOut);

	std::string sName = class_name;
	if (sName.find("player") != std::string::npos && (C_GET(bool, Vars.bVisualGlow)))
	{
		CCSPlayerController* pPlayer = reinterpret_cast<CCSPlayerController*>(glow->Owner());
		if (pPlayer == nullptr)
			return;

		C_CSPlayerPawn* pawn = I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(pPlayer->GetPawnHandle());
		if (pawn == nullptr)
			return;


		if (pawn == SDK::LocalPawn)
			return;

		if (pawn->GetHealth() > 0.f)
		{
			pColorOut[0] = C_GET(Color_t, Vars.colGlowTeam).r / 255.f;
			pColorOut[1] = C_GET(Color_t, Vars.colGlowTeam).g / 255.f;
			pColorOut[2] = C_GET(Color_t, Vars.colGlowTeam).b / 255.f;
			pColorOut[3] = C_GET(Color_t, Vars.colGlowTeam).a / 255.f;
		}
	}
	/*else if (sName.find("player") != std::string::npos && g_Config::selfGlow)
	{
		C_CSPlayerPawn* pawn = reinterpret_cast<C_CSPlayerPawn*>(glow->Owner());

		if (pawn != SDK::LocalPawn)
			return;

		if (pawn->GetHealth() > 0.f)
		{
			pColorOut[0] = g_Config::selfGlowColor.r / 255.f;
			pColorOut[1] = g_Config::selfGlowColor.g / 255.f;
			pColorOut[2] = g_Config::selfGlowColor.b / 255.f;
			pColorOut[3] = g_Config::selfGlowColor.a / 255.f;
		}
	}*/
	else
	{
		return oGetGlowColor(glow, pColorOut);
	}

	//else if (sName.find("weapon") != std::string::npos && settings::visuals::esp::items::glow) {
	//	CBasePlayerWeapon* weapon = reinterpret_cast<CBasePlayerWeapon*>(glow->m_pOwner);
	//
	//	if (weapon->GetHandleOwner() == 0xFFFFFFFF)
	//		memcpy(pColorOut, settings::visuals::esp::items::glow_color, sizeof(float) * 4);
	//}
}
