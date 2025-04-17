#include "features.h"

// used: draw callbacks
#include "utilities/draw.h"
// used: notify
#include "utilities/notify.h"

// used: cheat variables
#include "core/variables.h"
// used: menu
#include "core/menu.h"

#include "sdk/interfaces/cgamerules.h"

// used: features callbacks
#include "features/visuals.h"
#include "features/misc.h"
#include "features/legitbot.h"
#include "features/ragebot/rage.h"

// used: interfaces
#include "core/interfaces.h"
#include "sdk/interfaces/iengineclient.h"
#include "sdk/interfaces/cgameentitysystem.h"
#include "sdk/datatypes/usercmd.h"
#include "sdk/entity.h"
#include "features/visuals/overlay.h"
#include "features/misc/movement.h"
#include "features/antiaim/antiaim.hpp"
#include "sdk/interfaces/inetworkclientservice.h"
#include "core/sdk.h"
#include "sdk/interfaces/imaterialsystem.h"
#include "sdk/interfaces/iresourcesystem.h"




bool F::Setup()
{
	if (!VISUALS::Setup())
	{
		L_PRINT(LOG_ERROR) << CS_XOR("failed to setup visuals");
		return false;
	}

	return true;
}

void F::Destroy()
{
	VISUALS::OnDestroy();
}

void F::OnPresent()
{
	if (!D::bInitialized)
		return;


	D::ResetDrawData();
	{
		if (CCSPlayerController* pLocal = CCSPlayerController::GetLocalPlayerController(); pLocal != nullptr)
		{
			F::VISUALS::OVERLAY::OnFrameStageNotify(pLocal);
		}
	}
	D::SwapDrawData();


	D::NewFrame();
	{
		F::RAGEBOT::present();

		// render watermark
		MENU::RenderWatermark();

		// main window
		ImGui::PushFont(FONT::pMenu[C_GET(int, Vars.nDpiScale)]);

		// render keybinds
		MENU::RenderKeybinds();

		// render spectators
		MENU::RenderSpectators();

		// @note: here you can draw your stuff
		MENU::RenderMainWindow();
		// render notifications
		NOTIFY::Render();
		ImGui::PopFont();
	}
	D::Render();
}







void F::OnFrameStageNotify(int nStage)
{
	F::VISUALS::OnFrame(nStage);
}



void F::OnCreateRage(CUserCmd* pCmd, CBaseUserCmdPB* pBaseCmd, CCSPlayerController* pLocalController)
{
	C_CSPlayerPawn* pLocalPawn = I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(pLocalController->GetPawnHandle());
	if (pLocalPawn == nullptr)
		return;

	F::RAGEBOT::OnMove(pCmd, pBaseCmd, pLocalController, pLocalPawn);
}


class c_eng_pred
{
	struct c_local_data
	{
		float m_absolute_frame_time{}, m_flRealTime{}, m_nTickCount{},
		m_absolute_frame_start_time_std_dev{}, m_spread{}, m_inaccuracy{}, m_player_tick_fraction{}, m_render_tick_fraction{};
		int m_current_time{}, m_current_time2{}, m_tick_count{}, m_tick_base{}, m_player_tick{}, m_render_tick{}, m_shoot_tick{};
		Vector_t m_velocity{}, m_eye_pos{};
		bool command_prediction{}, game_prediction{}, prediction{}, first_prediction{};
	} m_pred_data{};

	bool m_initialized = false;

public:
	void update(CUserCmd* pCmd, C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pLocalController);
	void begin(CUserCmd* pCmd, C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pLocalController);
	void end(C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pLocalController);

	c_local_data* get_local_data()
	{
		return &m_pred_data;
	}
};

inline const auto g_prediction = std::make_unique<c_eng_pred>();

void c_eng_pred::begin(CUserCmd* pCmd, C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pLocalController)
{
	if (!I::GlobalVars)
		return;

	auto pMovementServices = pLocalPawn->GetMovementServices();
	if (!pMovementServices)
		return;

	pMovementServices->set_prediction_command(pCmd);

	/*static auto process_movement = reinterpret_cast<void*(__fastcall*)(void*, CUserCmd*)>(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 48 8B 06 48 8B CE FF 90 ? ? ? ? 44 38 63"), 0x1));
	process_movement(pMovementServices, pCmd);*/


	pLocalController->PhysicsRunThink();
	//pLocalController->m_current_command() = pCmd;

	// 40 55 41 56 48 83 EC ?? 80 B9 engine2.dll
}

void c_eng_pred::update(CUserCmd* pCmd, C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pLocalController)
{
	/* store all predicted info there */
	m_pred_data.m_nTickCount = I::GlobalVars->nTickCount;
	m_pred_data.m_flRealTime = I::GlobalVars->flRealTime;
	m_pred_data.m_absolute_frame_time = I::GlobalVars->flFrameTime;
	m_pred_data.m_absolute_frame_start_time_std_dev = I::GlobalVars->flFrameTime2;
	m_pred_data.m_current_time = I::GlobalVars->flCurrentTime;


	//m_pred_data.command_prediction = pCmd->hasBeenPrediction;
	m_pred_data.game_prediction = I::NetworkClientService->GetNetworkGameClient()->get_prediction();
	m_pred_data.prediction = I::Prediction->in_prediction;
	m_pred_data.first_prediction = I::Prediction->m_first_prediction;

	pCmd->bHasBeenPredicted = true;
	I::NetworkClientService->GetNetworkGameClient()->set_prediction(true);
	I::Prediction->in_prediction = true;
	I::Prediction->m_first_prediction = false;

	/*if (I::NetworkClientService->GetNetworkGameClient()->GetDeltaTick() > 0)
	{
		static auto run_prediction = reinterpret_cast<void*(__fastcall*)(CNetworkGameClient*, int)>(MEM::FindPattern(ENGINE2_DLL, "40 55 41 56 48 83 EC ?? 80 B9"));
		run_prediction(I::NetworkClientService->GetNetworkGameClient(), prediction_reason::client_command_tick);
	}*/

	I::GlobalVars->flCurrentTime = TICKS_TO_TIME(pLocalController->GetTickBase());
	I::GlobalVars->flCurrentTime2 = TICKS_TO_TIME(pLocalController->GetTickBase());

	//if (I::NetworkClientService->GetNetworkGameClient())

	//   48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 0F 29 70 ? 4C 8B FA
}

void c_eng_pred::end(C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pLocalController)
{
	auto pMovementServices = pLocalPawn->GetMovementServices();
	if (!pMovementServices)
		return;

	pMovementServices->reset_prediction_command();

	/* restore all predicted info there */
	I::GlobalVars->nTickCount = m_pred_data.m_nTickCount;
	I::GlobalVars->flRealTime = m_pred_data.m_flRealTime;
	I::GlobalVars->flFrameTime = m_pred_data.m_absolute_frame_time;
	I::GlobalVars->flFrameTime2 = m_pred_data.m_absolute_frame_start_time_std_dev;
	I::GlobalVars->flCurrentTime = m_pred_data.m_current_time;
	I::GlobalVars->flCurrentTime2 = m_pred_data.m_current_time;
	I::Prediction->in_prediction = m_pred_data.prediction;
	I::Prediction->m_first_prediction = m_pred_data.first_prediction;
}



void F::OnCreateMove(CUserCmd* pCmd, CBaseUserCmdPB* pBaseCmd, CCSPlayerController* pLocalController)
{
	C_CSPlayerPawn* pLocalPawn = SDK::LocalPawn;
	if (!pLocalPawn)
		return;

	if (pLocalController == nullptr || pLocalPawn == nullptr)
		return;

	if (!pLocalController->IsPawnAlive())
		return;


	/*if (I::GameRules)
	{
		if (I::GameRules->bFreezePause)
			return;

		if (I::GameRules->iGamePhases == 5)
			return;
	}*/


	g_prediction->update(pCmd, pLocalPawn, pLocalController);
	g_prediction->begin(pCmd, pLocalPawn, pLocalController);


	F::RAGEBOT::AutoPeek(pCmd, pBaseCmd, pLocalPawn);
	F::RAGEBOT::OnMove(pCmd, pBaseCmd, pLocalController, pLocalPawn);
	F::LEGITBOT::OnMove(pCmd, pBaseCmd, pLocalController, pLocalPawn);



	g_prediction->end(pLocalPawn, pLocalController);


	//F::MISC::MOVEMENT::MovementCorrection(pBaseCmd, F::ANTIAIM::angStoredViewBackup);
}

bool F::OnDrawObject(void* pAnimatableSceneObjectDesc, void* pDx11, CMeshData* arrMeshDraw, int nDataCount, void* pSceneView, void* pSceneLayer, void* pUnk, void* pUnk2)
{
	return VISUALS::OnDrawObject(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2);
}
