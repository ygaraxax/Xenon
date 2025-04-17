#pragma once
#include "../../sdk/datatypes/qangle.h"
#include <unordered_map>


class CUserCmd;
class CBaseUserCmdPB;
class CCSGOInputHistoryEntryPB;

class C_CSPlayerPawn;
class CEntityInstance;



namespace F::ANTIAIM
{
	inline QAngle_t angStoredViewBackup;
	inline QAngle_t silentViewAngle;
	inline float bestTargetSimTime;
	inline bool stop = false;


	inline std::unordered_map<int, std::vector<CEntityInstance*>> entities;

	void level_init();
	void level_shutdown();

	std::vector<CEntityInstance*> get(const char* name);

	void add_entity(CEntityInstance* pEntity, int index);

	void remove_entity(CEntityInstance* pEntity, int index);


	void RunAA(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, C_CSPlayerPawn* pLocalPawn);
}
