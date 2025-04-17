#pragma once
#include "../../common.h"

#include "../../sdk/interfaces/cgametracemanager.h"
#include <unordered_map>
#include <deque>


#define INTERVAL_PER_TICK TICK_INTERVAL


class CUserCmd;
class CBaseUserCmdPB;
class CCSGOInputHistoryEntryPB;

class CCSPlayerController;
class C_CSPlayerPawn;
class C_BaseEntity;

struct QAngle_t;
struct data_t;


struct lag_record_t;
struct hitbox_data_t;
struct aim_point_t;
struct aim_target_t;



namespace F::RAGEBOT
{
	void level_init();
	void level_shutdown();
	void present();


	void OnMove(CUserCmd* pCmd, CBaseUserCmdPB* pBaseCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn);

	void ScaleDamage(TraceHitboxData_t* hitgroup, C_CSPlayerPawn* entity);
	void AutoPeek(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, C_CSPlayerPawn* pLocalPawn);
	bool FireBullet(C_CSPlayerPawn* pPawn, Vector_t eye_pos, Vector_t enemy_pos);
	void AutoStop(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pLocalController);
	void StoreRecords();
	float GetAngularDistanceRage(CBaseUserCmdPB* pCmd, Vector_t vecTarget, C_CSPlayerPawn* pLocal);
	aim_target_t* GetNearestTarget(CBaseUserCmdPB* pUserCmd);
	aim_point_t SelectPoints(lag_record_t* record, float& damage);
	void SelectTarget(CBaseUserCmdPB* pUserCmd);
	lag_record_t* SelectRecord(int handle);
	void FindTargets();
	void RageAim(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pLocalController);
	void LagComp(CUserCmd* cmd, float simulation_time) noexcept;
}
