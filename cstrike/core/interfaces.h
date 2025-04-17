#pragma once

#include "../common.h"

// used: globalvariables
#include "../sdk/interfaces/iglobalvars.h"

#pragma region sdk_definitons
#define GAME_RESOURCE_SERVICE_CLIENT CS_XOR("GameResourceServiceClientV00")
#define SOURCE2_CLIENT_PREDICTION CS_XOR("Source2ClientPrediction001")
#define SOURCE2_CLIENT CS_XOR("Source2Client00")
#define SCHEMA_SYSTEM CS_XOR("SchemaSystem_00")
#define INPUT_SYSTEM_VERSION CS_XOR("InputSystemVersion00")
#define SOURCE2_ENGINE_TO_CLIENT CS_XOR("Source2EngineToClient00")
#define ENGINE_CVAR CS_XOR("VEngineCvar00")
#define LOCALIZE CS_XOR("Localize_00")
#define NETWORK_CLIENT_SERVICE CS_XOR("NetworkClientService_00")
#define MATERIAL_SYSTEM2 CS_XOR("VMaterialSystem2_00")
#define RESOURCE_SYSTEM CS_XOR("ResourceSystem013")
#define RESOURCE_HANDLE_UTILS CS_XOR("ResourceHandleUtils001")

// @source: master/game/shared/shareddefs.h
#define TICK_INTERVAL 0.015625f
#define TIME_TO_TICKS(TIME) (static_cast<int>(0.5f + static_cast<float>(TIME) / TICK_INTERVAL))
#define TICKS_TO_TIME(TICKS) (TICK_INTERVAL * static_cast<float>(TICKS))
#define ROUND_TO_TICKS(TIME) (TICK_INTERVAL * TIME_TO_TICKS(TIME))
#define TICK_NEVER_THINK (-1)
#pragma endregion

// game interfaces
class ISwapChainDx11;
class IMemAlloc;
class CCSGOInput;
class ISchemaSystem;
class IInputSystem;
class IGameResourceService;
class IDebugOverlayGameSystem;
class ISource2Client;
class IEngineClient;
class IEngineCVar;
class INetworkClientService;
class IMaterialSystem2;
class IResourceSystem;
class CResourceHandleUtils;
class CPVS;
class CGameTraceManager;
class CGameEventManager;
class CCSPlayerController;
class CGameRules;

// [d3d] struct
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;




enum prediction_reason
{
	client_command_tick,
	demo_preentity,
	demo_simulation,
	postnetupdate,
	server_starved_and_added_usercmds,
	client_frame_simulate
};

enum signon_state_t
{
	SIGNONSTATE_NONE,
	SIGNONSTATE_CHALLENGE,
	SIGNONSTATE_CONNECTED,
	SIGNONSTATE_NEW,
	SIGNONSTATE_PRESPAWN,
	SIGNONSTATE_SPAWN,
	SIGNONSTATE_FULL,
	SIGNONSTATE_CHANGELEVEL
};

class CPrediction
{
public:
	void update(int reason, int delta_tick);

public:
	char gap0[48];
	int reason;
	bool in_prediction;
	bool engine_pause;
	char gap36[6];
	int incoming_acknowledged;
	std::uint64_t delta_tick;
	std::uint64_t slot;
	CCSPlayerController* controller_entity;
	char gap58[4];
	int int5C;
	char gap60[32];
	bool m_first_prediction;
	char pad90[15];
	int storage_entity_size;
	char gap94[4];
	void* storage_entitys;
	char gapA0[48];
	int count_penisov;
	char gapD4[4];
	void* position;
	char gapE0[8];
	float real_time;
	char gapEC[12];
	int max_client;
	char gapFC[12];
	std::uintptr_t* unk_class;
	float current_time;
	char gap11C[12];
	int tick_count;
	char gap12C[4];
	char gap130[2];
	char byte132;
	char gap133[3];
	bool has_update;
};

//class CPrediction
//{
//public:
//	void update(int reason, int delta_tick);
//
//public:
//	char gap0[48];
//	int reason;
//	bool InPrediction;
//	bool EnginePause;
//	char gap36[6];
//	int incoming_acknowledged;
//	std::uint64_t delta_tick;
//	std::uint64_t slot;
//	CCSPlayerController* ControllerEntity;
//	char gap58[4];
//	int int5C;
//	char gap60[32];
//	bool bFirstPrediction;
//	char pad90[15];
//	int storage_entity_size;
//	char gap94[4];
//	void* storage_entitys;
//	char gapA0[48];
//	int count_penisov;
//	char gapD4[4];
//	void* position;
//	char gapE0[8];
//	float real_time;
//	char gapEC[12];
//	int max_client;
//	char gapFC[12];
//	std::uintptr_t* unk_class;
//	float current_time;
//	char gap11C[12];
//	int tick_count;
//	char gap12C[4];
//	char gap130[2];
//	char byte132;
//	char gap133[3];
//	bool HasUpdate;
//};




namespace I
{
	bool Setup();

	/* @section: helpers */
	// create and destroy render target view for handling resize
	void CreateRenderTarget();
	void DestroyRenderTarget();

	inline IMemAlloc* MemAlloc = nullptr;
	inline ISwapChainDx11* SwapChain = nullptr;
	inline ID3D11Device* Device = nullptr;
	inline ID3D11DeviceContext* DeviceContext = nullptr;
	inline ID3D11RenderTargetView* RenderTargetView = nullptr;
	inline CGameEventManager* GameEventManager = nullptr;
	inline CCSGOInput* Input = nullptr;
	inline CPrediction* Prediction = nullptr;
	inline ISchemaSystem* SchemaSystem = nullptr;
	inline IGlobalVars* GlobalVars = nullptr;
	inline IInputSystem* InputSystem = nullptr;
	inline IGameResourceService* GameResourceService = nullptr;
	inline ISource2Client* Client = nullptr;
	inline IEngineClient* Engine = nullptr;
	inline IEngineCVar* Cvar = nullptr;
	inline INetworkClientService* NetworkClientService = nullptr;
	inline IMaterialSystem2* MaterialSystem2 = nullptr;
	inline IResourceSystem* ResourceSystem = nullptr;
	inline CResourceHandleUtils* ResourceHandleUtils = nullptr;
	inline CPVS* PVS = nullptr;
	inline CGameTraceManager* GameTraceManager = nullptr;
	inline CGameRules* GameRules = nullptr;

	inline int(__cdecl* RandomSeed)(int seed) = nullptr;
	inline float(__cdecl* RandomFloat)(float min, float max) = nullptr;
}
