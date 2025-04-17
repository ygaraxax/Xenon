#include "rage.h"

#include "../../sdk/entity.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/inetworkclientservice.h"
#include "../../sdk/interfaces/iengineclient.h"
// used: cusercmd
#include "../../sdk/datatypes/usercmd.h"

// used: activation button
#include "../../utilities/inputsystem.h"

// used: cheat variables
#include "../../core/variables.h"

#include "../../sdk/interfaces/ienginecvar.h"

#include "../antiaim/antiaim.hpp"

#include "../../core/sdk.h"
#include <list>
#include <algorithm>
#include "../../core/hooks.h"
#include <unordered_map>
#include "../../utilities/draw.h"
#include <mutex>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "../../core/hooks.h"
#include "../misc/movement.h"
#define M_PI_F ((float)(M_PI))

#ifndef RAD2DEG
#define RAD2DEG(x) ((float)(x) * (float)(180.f / M_PI_F))
#endif

#ifndef DEG2RAD
#define DEG2RAD(x) ((float)(x) * (float)(M_PI_F / 180.f))
#endif


#define MAX_STUDIO_BONES 128


struct data_t
{
	float m_dmg;
	bool isValid;
	C_CSPlayerPawn* m_local;
	CCSPlayerController* m_local_controller;
	C_CSPlayerPawn* m_target;
	CCSWeaponBaseVData* m_wpn_data;

	int hitChance;
	unsigned int hitboxes;
	bool multipoint;
	int penetrationCount;
	int minDamage;
	int overrideDamage;
};


using namespace MATH;

std::mutex mtx;
data_t m_data;

std::vector<int> m_hitboxes;
lag_record_t* m_best_target;


static bool stop = false;

static bool active_retract = false;
static bool quickpeek_retract = false;
static bool quickpeek_retract_shoot = false;
static Vector_t retractPos = {};

inline QAngle_t CalcAngles(Vector_t viewPos, Vector_t aimPos)
{
	QAngle_t angle = { 0, 0, 0 };

	Vector_t delta = aimPos - viewPos;

	angle.x = -asin(delta.z / delta.Length()) * (180.0f / 3.141592654f);
	angle.y = atan2(delta.y, delta.x) * (180.0f / 3.141592654f);

	return angle;
}



static constexpr std::uint32_t PENMASK = 0x1C300Bu; // mask_shot_hull | contents_hitbox?

class backtrack_entity;


struct lag_compensation_data_t
{
	std::uint64_t m_tick{};
	float m_time{};
	MEM_PAD(0x24);
}; // size: 0x40

struct data_info
{
	std::uint64_t tick;
	float time;
	char pad[0x24];
};

float get_delta_time(int tick_entity)
{
	int tick_count = I::GlobalVars->nTickCount;
	static float maxunlag_val = I::Cvar->Find(FNV1A::Hash("sv_maxunlag"))->value.fl;
	;

	float mod_value = fmodf(maxunlag_val, 0.015625);
	float unlag_difference = maxunlag_val - mod_value;
	float mod_ticks = mod_value * 64.0;
	int unlag_ticks = (int)(float)((float)(unlag_difference * 64.0) + 0.5);

	data_info info{};
	info.tick = tick_entity;
	info.time = tick_entity * 0.015625f;

	int v20{};
	float v19{}, v21{};

	if (mod_ticks < (float)(1.0 - 0.0099999998))
	{
		v20 = tick_count - unlag_ticks;
		if (mod_ticks > 0.0099999998)
		{
			v19 = -mod_ticks;
			v21 = v19;
			if (v19 < 0.0f)
				v19 = v19 + 1.0f;
		}
		else
		{
			v19 = 0.0f;
			v21 = 0.0;
		}
	}
	else // high ping 100+
	{
		v19 = 0.0f;
		v20 = tick_count - (unlag_ticks + 1);
		v21 = 0.0;
	}

	auto v22 = v20 - 1;

	data_info v40{};

	v40.time = v19;
	if (v21 >= 0.0)
		v22 = v20;
	v40.tick = v22;
	if (v19 < (float)(1.0 - 0.0099999998))
	{
		if (v19 <= 0.0099999998)
		{
			v40.time = 0.0;
		}
	}
	else
	{
		v40.tick = v22 + 1;
		v40.time = 0.0;
	}

	auto tick = v40.tick;
	auto v24 = (float)info.tick;
	auto v38 = v40;
	int new_tick{};
	data_info v39{};

	if ((float)((float)(v24 + info.time) * 0.015625) <= 0.0)
	{
		auto cmd_tick_count = tick_entity;
		v40.tick = cmd_tick_count;
		v40.time = 0.0;
		new_tick = v40.tick;
		tick = v38.tick;
		v39 = v40;
	}
	else
	{
		new_tick = info.tick;
		v39 = info;
	}

	int v32{};
	float v30{};
	int v33;
	int v31;

	if (tick <= new_tick && (tick < new_tick || v39.time > v38.time))
	{
		v33 = new_tick - tick;
		v30 = v39.time - v38.time;
		if ((float)(v39.time - v38.time) < 0.0)
			v30 = v30 + 1.0;
		v32 = v33 - 1;
		if ((float)(v39.time - v38.time) >= 0.0)
			v32 = v33;
		if (v30 >= (float)(1.0 - 0.0099999998))
		{
			++v32;
			goto LABEL_48;
		}
	LABEL_47:
		if (v30 > 0.0099999998)
			goto LABEL_49;
		goto LABEL_48;
	}
	v30 = v38.time - v39.time;
	v31 = tick - new_tick;
	if ((float)(v38.time - v39.time) < 0.0)
		v30 = v30 + 1.0;
	v32 = v31 - 1;
	if ((float)(v38.time - v39.time) >= 0.0)
		v32 = v31;
	if (v30 < (float)(1.0 - 0.0099999998))
		goto LABEL_47;
	++v32;
LABEL_48:
	v30 = 0.0;
LABEL_49:
	auto delta_time = (float)((float)v32 + v30) * 0.015625;

	return delta_time;
}

float get_las_valid_sim_time() noexcept
{
	if (I::GlobalVars == nullptr || I::NetworkClientService == nullptr || I::Cvar == nullptr)
		return FLT_MAX;

	static auto sv_maxunlag = I::Cvar->Find(FNV1A::Hash("sv_maxunlag"))->value.fl;

	CNetworkGameClient* networkGameClient = I::NetworkClientService->GetNetworkGameClient();
	if (!networkGameClient)
		return FLT_MAX;

	c_net_chan* netChan = networkGameClient->get_net_channel2();
	if (!netChan)
		return FLT_MAX;

	const auto latency = netChan->get_latency(flow::FLOW_OUTGOING) + netChan->get_latency(flow::FLOW_INCOMING);
	const float correct = std::clamp(latency, 0.0f, sv_maxunlag);
	//const float max_delta = std::min((sv_maxunlag - correct), 0.2f) / 1000.f;
	const float max_delta = std::min((sv_maxunlag - correct), 200 / 1000.f);

	try
	{
		return I::GlobalVars->flCurrentTime - max_delta;
	}
	catch (...)
	{
		return FLT_MAX;
	}
}



//std::unordered_map<int, std::unique_ptr<backtrack_entity>> records{};


struct record
{
public:
	record() = default;

	/*inline record()
	{
	}*/

	CSkeletonInstance* m_skeleton = {};
	Matrix2x4_t m_bone_data[128];
	Matrix2x4_t m_bone_data_backup[128];
	bool last_valid = {};
	int bone_count = {};

	float m_simulation_time{};
	Vector_t m_vec_mins{};
	Vector_t m_vec_maxs{};
	Vector_t m_vec_velocity{};

	Vector_t vecOrigin;
	Vector_t vecAbsOrigin;
	Vector_t velocity;
	Vector_t absVelocity;
	QAngle_t angRotation;
	QAngle_t angAbsRotation;
	QAngle_t angEyeAngles;

	bool m_throwing{};

	Vector_t best_position{};
	int best_hitbox{};


	void store(C_CSPlayerPawn* pawn)
	{

	}

	void apply(C_CSPlayerPawn* pawn)
	{
		if (pawn == nullptr)
			return;

		if (pawn->GetHealth() <= 0)
			return;

		auto scene_node = pawn->GetGameSceneNode();
		if (scene_node == nullptr)
			return;

		vecOrigin = scene_node->GetOrigin();
		vecAbsOrigin = scene_node->GetAbsOrigin();
		angRotation = scene_node->GetAngleRotation();
		angAbsRotation = scene_node->GetAbsAngleRotation();
		angEyeAngles = pawn->GetAngEyeAngles();
		m_vec_velocity = pawn->GetVecVelocity();
		absVelocity = pawn->GetAbsVelocity();
		m_simulation_time = pawn->GetSimulationTime();

		//std::memcpy(m_bone_data_backup, skeleton_instance->pBoneCache, sizeof(Matrix2x4_t) * skeleton_instance->nBoneCount);
		std::memmove(m_bone_data_backup, m_skeleton->pBoneCache, sizeof(Matrix2x4_t) * m_skeleton->nBoneCount);
		//CRT::MemoryCopy(m_bone_data_backup, skeleton_instance->pBoneCache, sizeof(Matrix2x4_t) * skeleton_instance->nBoneCount);

		scene_node->GetOrigin() = vecOrigin;
		scene_node->GetAbsOrigin() = vecAbsOrigin;
		scene_node->GetAngleRotation() = angRotation;
		scene_node->GetAbsAngleRotation() = angAbsRotation;
		pawn->GetAngEyeAngles() = angEyeAngles;
		pawn->GetAbsVelocity() = absVelocity;
		pawn->GetVecVelocity() = m_vec_velocity;
		pawn->GetSimulationTime() = m_simulation_time;

		//std::memcpy(skeleton_instance->pBoneCache, m_bone_data, sizeof(Matrix2x4_t) * skeleton_instance->nBoneCount);
		//std::memmove(m_skeleton->pBoneCache, m_bone_data, sizeof(Matrix2x4_t) * m_skeleton->nBoneCount);
		std::memmove(m_skeleton->pBoneCache, m_bone_data, sizeof(Matrix2x4_t) * m_skeleton->nBoneCount);
		//CRT::MemoryCopy(skeleton_instance->pBoneCache, m_bone_data, sizeof(Matrix2x4_t) * skeleton_instance->nBoneCount);
	}

	void reset(C_CSPlayerPawn* pawn)
	{
		if (pawn == nullptr)
			return;

		if (pawn->GetHealth() <= 0)
			return;

		auto scene_node = pawn->GetGameSceneNode();
		if (scene_node == nullptr)
			return;


		scene_node->GetOrigin() = vecOrigin;
		scene_node->GetAbsOrigin() = vecAbsOrigin;
		scene_node->GetAngleRotation() = angRotation;
		scene_node->GetAbsAngleRotation() = angAbsRotation;
		pawn->GetAngEyeAngles() = angEyeAngles;
		pawn->GetAbsVelocity() = absVelocity;
		pawn->GetVecVelocity() = m_vec_velocity;
		pawn->GetSimulationTime() = m_simulation_time;

		//std::memcpy(skeleton_instance->pBoneCache, m_bone_data_backup, sizeof(Matrix2x4_t) * skeleton_instance->nBoneCount);
		std::memmove(m_skeleton->pBoneCache, m_bone_data_backup, sizeof(Matrix2x4_t) * m_skeleton->nBoneCount);
		//CRT::MemoryCopy(skeleton_instance->pBoneCache, m_bone_data_backup, sizeof(Matrix2x4_t) * skeleton_instance->nBoneCount);
	}
};

struct record_set
{
public:
	record* record1;
	record* record2;
	float fraction;
};

class backtrack_entity
{
public:
	C_CSPlayerPawn* m_pawn = {};
	std::deque<record> records = {};

	backtrack_entity() = default;

	backtrack_entity(C_CSPlayerPawn* pPawn)
	{
		this->m_pawn = pPawn;
		this->save_record();
	}

	void save_record()
	{
		if (m_pawn == nullptr)
			return;

		if (m_pawn->GetHealth() <= 0)
			return;

		auto scene_node = m_pawn->GetGameSceneNode();
		if (scene_node == nullptr)
			return;

		auto skeleton_instance = scene_node->GetSkeletonInstance();
		if (skeleton_instance == nullptr)
			return;


		/*CSkeletonInstance* skeleton_instance;

		try
		{
			skeleton_instance = scene_node->GetSkeletonInstance();
			if (skeleton_instance == nullptr)
				return;
		}
		catch (...)
		{
			return;
		}*/

		auto collision = m_pawn->GetCollision();
		if (collision == nullptr)
			return;

		auto& rec = this->records.emplace_front(record());
		//auto& rect = this->records.front();


		//static auto calc_world_space_bone = reinterpret_cast<void(__fastcall*)(void*, uint32_t)>(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 49 8B 96 ? ? ? ? 48 8B CF"), 0x1));
		static auto calc_world_space_bone = reinterpret_cast<void(__fastcall*)(void*, uint32_t)>((MEM::FindPattern(CLIENT_DLL, "40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC D0"))); // 40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC D0

		//calc_world_space_bone(skeleton_instance, FLAG_HITBOX);

		rec.m_skeleton = skeleton_instance;
		rec.vecOrigin = scene_node->GetAbsOrigin();

		calc_world_space_bone(rec.m_skeleton, FLAG_ALL_BONE_FLAGS);

		rec.bone_count = skeleton_instance->nBoneCount;
		//rec.bone_count = 128;

		rec.vecOrigin = scene_node->GetOrigin();
		rec.vecAbsOrigin = scene_node->GetAbsOrigin();
		rec.angRotation = scene_node->GetAngleRotation();
		rec.angAbsRotation = scene_node->GetAbsAngleRotation();
		rec.angEyeAngles = m_pawn->GetAngEyeAngles();

		rec.m_simulation_time = m_pawn->GetSimulationTime();
		rec.m_vec_mins = collision->GetMins();
		rec.m_vec_maxs = collision->GetMaxs();
		rec.m_vec_velocity = m_pawn->GetVecVelocity();
		rec.absVelocity = m_pawn->GetAbsVelocity();

		//m_throwing = pawn->is_throwing();
		rec.m_throwing = false;

		//std::memcpy(m_bone_data, skeleton_instance->pBoneCache, sizeof(Matrix2x4_t) * skeleton_instance->nBoneCount);
		std::memmove(rec.m_bone_data, skeleton_instance->pBoneCache, sizeof(Matrix2x4_t) * skeleton_instance->nBoneCount);
		//CRT::MemoryCopy(m_bone_data, skeleton_instance->pBoneCache, sizeof(Matrix2x4_t) * bone_count);
	}

	float closest_record(Vector_t start, Vector_t end, float las_valid_sim_time)
	{
		float closest = FLT_MAX;

		for (auto& rec : records)
		{
			if (rec.m_simulation_time <= las_valid_sim_time)
				continue;

			float dist = get_distance(rec.vecOrigin, start, end);
			if (dist < closest)
				closest = dist;
		}

		return closest;
	}

	float get_distance(Vector_t p, Vector_t a, Vector_t b)
	{
		Vector_t ap = p - a;
		Vector_t ab = b - a;

		float ab2 = ab.DotProduct(ab);
		float ap_ab = ap.DotProduct(ab);
		float t = ap_ab / ab2;

		// ignore if player is behind ur or too far away
		if (t < 0.0f || t > 1.0f)
			return std::numeric_limits<float>::max();

		Vector_t nearest = a + ab * t;
		return (p - nearest).Length();
	}

	record_set find_best_records(float las_valid_sim_time)
	{
		record* last_record = nullptr;
		record* first_record = nullptr;
		float last_record_time = FLT_MAX;
		float first_record_time = 0;

		if (this->records.size() < 2)
			return { first_record, last_record, 0.15f };

		for (auto& rec : this->records)
		{
			if (rec.m_simulation_time <= las_valid_sim_time)
				continue;

			if (rec.m_simulation_time > first_record_time)
			{
				first_record_time = rec.m_simulation_time;
				first_record = &rec;
			}


			if (rec.m_simulation_time < last_record_time)
			{
				last_record_time = rec.m_simulation_time;
				last_record = &rec;
			}
		}


		if (last_record == nullptr)
			last_record = &this->records.back();

		if (first_record == nullptr)
			first_record = &this->records.front();

		return { first_record, last_record, 0.15f };
	}
};

/* struct lag_record_t
{
	Vector_t m_origin{};
	C_CSPlayerPawn* m_pawn = nullptr;
	CSkeletonInstance* m_skeleton = nullptr;
	Matrix2x4_t m_bone_data[128];
	Matrix2x4_t m_bone_data_backup[128];

	int bone_count;

	float m_simulation_time{};
	Vector_t m_vec_mins{};
	Vector_t m_vec_maxs{};
	Vector_t m_vec_velocity{};

	Vector_t vecOrigin;
	Vector_t vecAbsOrigin;
	Vector_t velocity;
	Vector_t absVelocity;
	QAngle_t angRotation;
	QAngle_t angAbsRotation;
	QAngle_t angEyeAngles;

	bool m_throwing{};

	Vector_t best_position{};
	int best_hitbox{};


	lag_record_t() = default;

	inline lag_record_t(C_CSPlayerPawn* pawn)
	{
		this->store(pawn);
	}

	void store(C_CSPlayerPawn* pawn)
	{
		if (pawn == nullptr)
			return;

		if (pawn->GetHealth() <= 0)
			return;

		auto scene_node = pawn->GetGameSceneNode();
		if (scene_node == nullptr)
			return;

		auto skeleton_instance = scene_node->GetSkeletonInstance();
		if (skeleton_instance == nullptr)
			return;

		auto collision = pawn->GetCollision();
		if (collision == nullptr)
			return;

		//static auto setup_bones = reinterpret_cast<void(__fastcall*)(void*, int)>(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 49 8B 96 ? ? ? ? 48 8B CF"), 0x1));
		static auto setup_bones = reinterpret_cast<void(__fastcall*)(void*, int)>((MEM::FindPattern(CLIENT_DLL, "40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC D0"))); // 40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC D0

		//setup_bones(skeleton_instance, FLAG_HITBOX);

		m_pawn = pawn;
		m_skeleton = skeleton_instance;
		m_origin = scene_node->GetAbsOrigin();

		setup_bones(m_skeleton, FLAG_HITBOX);


		bone_count = skeleton_instance->nBoneCount;

		vecOrigin = scene_node->GetOrigin();
		vecAbsOrigin = scene_node->GetAbsOrigin();
		angRotation = scene_node->GetAngleRotation();
		angAbsRotation = scene_node->GetAbsAngleRotation();
		angEyeAngles = pawn->GetAngEyeAngles();

		m_simulation_time = pawn->GetSimulationTime();
		m_vec_mins = collision->GetMins();
		m_vec_maxs = collision->GetMaxs();
		m_vec_velocity = pawn->GetVecVelocity();
		absVelocity = pawn->GetAbsVelocity();

		//m_throwing = pawn->is_throwing();
		m_throwing = false;


		//std::memcpy(m_bone_data, skeleton_instance->pBoneCache, sizeof(Matrix2x4_t) * skeleton_instance->nBoneCount);
		std::memmove(m_bone_data, skeleton_instance->pBoneCache, sizeof(Matrix2x4_t) * skeleton_instance->nBoneCount);
		//CRT::MemoryCopy(m_bone_data, skeleton_instance->pBoneCache, sizeof(Matrix2x4_t) * bone_count);
	}

	void apply(C_CSPlayerPawn* pawn)
	{
		if (pawn == nullptr)
			return;

		if (pawn->GetHealth() <= 0)
			return;

		auto scene_node = pawn->GetGameSceneNode();
		if (scene_node == nullptr)
			return;


		CSkeletonInstance* skeleton_instance;

		try
		{
			skeleton_instance = scene_node->GetSkeletonInstance();
			if (skeleton_instance == nullptr)
				return;
		}
		catch (...)
		{
			return;
		}


		vecOrigin = scene_node->GetOrigin();
		vecAbsOrigin = scene_node->GetAbsOrigin();
		angRotation = scene_node->GetAngleRotation();
		angAbsRotation = scene_node->GetAbsAngleRotation();
		angEyeAngles = pawn->GetAngEyeAngles();
		m_vec_velocity = pawn->GetVecVelocity();
		absVelocity = pawn->GetAbsVelocity();
		m_simulation_time = pawn->GetSimulationTime();


		//std::memcpy(m_bone_data_backup, skeleton_instance->pBoneCache, sizeof(Matrix2x4_t) * skeleton_instance->nBoneCount);
		std::memmove(m_bone_data_backup, skeleton_instance->pBoneCache, sizeof(Matrix2x4_t) * skeleton_instance->nBoneCount);
		//CRT::MemoryCopy(m_bone_data_backup, skeleton_instance->pBoneCache, sizeof(Matrix2x4_t) * skeleton_instance->nBoneCount);

		scene_node->GetOrigin() = vecOrigin;
		scene_node->GetAbsOrigin() = vecAbsOrigin;
		scene_node->GetAngleRotation() = angRotation;
		scene_node->GetAbsAngleRotation() = angAbsRotation;
		pawn->GetAngEyeAngles() = angEyeAngles;
		pawn->GetAbsVelocity() = absVelocity;
		pawn->GetVecVelocity() = m_vec_velocity;
		pawn->GetSimulationTime() = m_simulation_time;


		//std::memcpy(skeleton_instance->pBoneCache, m_bone_data, sizeof(Matrix2x4_t) * skeleton_instance->nBoneCount);
		std::memmove(skeleton_instance->pBoneCache, m_bone_data, sizeof(Matrix2x4_t) * skeleton_instance->nBoneCount);
		//CRT::MemoryCopy(skeleton_instance->pBoneCache, m_bone_data, sizeof(Matrix2x4_t) * skeleton_instance->nBoneCount);
	}

	void reset(C_CSPlayerPawn* pawn)
	{
		if (pawn == nullptr)
			return;

		if (pawn->GetHealth() <= 0)
			return;




		auto scene_node = pawn->GetGameSceneNode();
		if (scene_node == nullptr)
			return;

		CSkeletonInstance* skeleton_instance;
		try
		{
			skeleton_instance = scene_node->GetSkeletonInstance();
			if (skeleton_instance == nullptr)
				return;
		}
		catch (...)
		{
			return;
		}

		scene_node->GetOrigin() = vecOrigin;
		scene_node->GetAbsOrigin() = vecAbsOrigin;
		scene_node->GetAngleRotation() = angRotation;
		scene_node->GetAbsAngleRotation() = angAbsRotation;
		pawn->GetAngEyeAngles() = angEyeAngles;
		pawn->GetAbsVelocity() = absVelocity;
		pawn->GetVecVelocity() = m_vec_velocity;
		pawn->GetSimulationTime() = m_simulation_time;

		//std::memcpy(skeleton_instance->pBoneCache, m_bone_data_backup, sizeof(Matrix2x4_t) * skeleton_instance->nBoneCount);
		std::memmove(skeleton_instance->pBoneCache, m_bone_data_backup, sizeof(Matrix2x4_t) * skeleton_instance->nBoneCount);
		//CRT::MemoryCopy(skeleton_instance->pBoneCache, m_bone_data_backup, sizeof(Matrix2x4_t) * skeleton_instance->nBoneCount);
	}

	bool is_valid()
	{
		if (!I::GlobalVars)
			return false;

		static auto sv_maxunlag_var = I::Cvar->Find(FNV1A::Hash("sv_maxunlag"));

		const auto mod{ fmodf(sv_maxunlag_var->value.fl, 0.2f) };

		const auto maxDelta{ TIME_TO_TICKS(sv_maxunlag_var->value.fl - mod) };

		const auto overlap{ 64.f * mod };
		auto lastValid{ TIME_TO_TICKS(I::GlobalVars->flCurrentTime) - maxDelta };
		if (overlap < 1.f - 0.01f)
		{
			if (overlap <= 0.01f)
				lastValid++;
		}

		lastValid--;

		return lastValid < TIME_TO_TICKS(this->m_simulation_time);
	}
};*/


void vector_angles(Vector_t vec_forward, Vector_t& vec_angles)
{
	if (vec_forward.x == 0.f && vec_forward.y == 0.f)
	{
		vec_angles.y = 0.f;
		vec_angles.x = vec_forward.z > 0.f ? 270.f : 90.f;
	}
	else
	{
		vec_angles.y = RAD2DEG(std::atan2(vec_forward.y, vec_forward.x));
		if (vec_angles.y < 0.f)
			vec_angles.y += 360.f;

		vec_angles.x = RAD2DEG(std::atan2(-vec_forward.z, vec_forward.Length2D()));
		if (vec_angles.x < 0.f)
			vec_angles.x += 360.f;
	}

	vec_angles.x = std::remainder(vec_angles.x, 360.f);
	vec_angles.y = std::remainder(vec_angles.y, 360.f);
	vec_angles.z = std::remainder(vec_angles.z, 360.f);
}

void vec_angles(Vector_t forward, Vector_t* angles)
{
	float tmp, yaw, pitch;

	if (forward.y == 0.f && forward.x == 0.f)
	{
		yaw = 0;
		if (forward.z > 0)
		{
			pitch = 270;
		}
		else
		{
			pitch = 90.f;
		}
	}
	else
	{
		yaw = (float)(atan2(forward.y, forward.x) * 180.f / 3.14159265358979323846f);
		if (yaw < 0)
		{
			yaw += 360.f;
		}
		tmp = (float)sqrt(forward.x * forward.x + forward.y * forward.y);
		pitch = (float)(atan2(-forward.z, tmp) * 180.f / 3.14159265358979323846f);
		if (pitch < 0)
		{
			pitch += 360.f;
		}
	}
	angles->x = pitch;
	angles->y = yaw;
	angles->z = 0.f;
}



std::unordered_map<int, std::unique_ptr<backtrack_entity>> records{};



void F::RAGEBOT::level_init()
{
	if (m_best_target)
		m_best_target = nullptr;

	records.clear();
}

void F::RAGEBOT::level_shutdown()
{
	if (m_best_target)
		m_best_target = nullptr;

	records.clear();
}

void F::RAGEBOT::OnMove(CUserCmd* pCmd, CBaseUserCmdPB* pBaseCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn)
{
	if (!C_GET(bool, Vars.bRageBot))
		return;

	if (!pLocalPawn->IsValidMoveType())
		return;


	F::ANTIAIM::bestTargetSimTime = 0;
	F::ANTIAIM::stop = false;


	m_data.m_local = pLocalPawn;
	m_data.m_local_controller = pLocalController;


	/*F::ANTIAIM::RunAA(pCmd, pBaseCmd, pLocalPawn);
	F::MISC::MOVEMENT::MovementCorrection(pBaseCmd, F::ANTIAIM::angStoredViewBackup);*/

	F::RAGEBOT::RageAim(pCmd, pBaseCmd, pLocalPawn, pLocalController);
	


	m_data.m_local = nullptr;
	m_data.m_local_controller = nullptr;
	m_data.m_wpn_data = nullptr;
	m_data.m_target = nullptr;
	m_data.m_dmg = 0;
}


QAngle_t NormalizeAnglesRage(QAngle_t angles)
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

QAngle_t GetAngularDifferenceRage(CBaseUserCmdPB* pCmd, Vector_t vecTarget, C_CSPlayerPawn* pLocal)
{
	// The current position
	Vector_t vecCurrent = pLocal->GetEyePosition();

	// The new angle
	QAngle_t vNewAngle = (vecTarget - vecCurrent).ToAngles();
	vNewAngle.Normalize(); // Normalise it so we don't jitter about

	// Store our current angles
	QAngle_t vCurAngle = pCmd->pViewAngles->angValue;

	// Find the difference between the two angles (later useful when adding smoothing)
	vNewAngle -= vCurAngle;

	return vNewAngle;
}

QAngle_t GetAngularDifferenceRageCurAngle(CBaseUserCmdPB* pCmd, Vector_t vecTarget, C_CSPlayerPawn* pLocal, QAngle_t vCurAngle)
{
	// The current position
	Vector_t vecCurrent = pLocal->GetEyePosition();

	// The new angle
	QAngle_t vNewAngle = (vecTarget - vecCurrent).ToAngles();
	vNewAngle.Normalize(); // Normalise it so we don't jitter about


	// Find the difference between the two angles (later useful when adding smoothing)
	vNewAngle -= vCurAngle;

	return vNewAngle;
}

QAngle_t GetAngularDifferenceRageView(CBaseUserCmdPB* pCmd, Vector_t vecTarget, C_CSPlayerPawn* pLocal)
{
	if (pCmd == nullptr)
		return { 0, 0, 0 };

	if (pLocal == nullptr)
		return { 0, 0, 0 };

	// The current position
	Vector_t vecCurrent;
	vecCurrent = pLocal->GetEyePosition();

	// The new angle
	QAngle_t vNewAngle = NormalizeAnglesRage((vecTarget - vecCurrent).ToAngles());

	QAngle_t vCurAngle = F::ANTIAIM::angStoredViewBackup;
	/*if (pCmd->pViewAngles)
		vCurAngle = NormalizeAnglesRage(pCmd->pViewAngles->angValue);*/

	// Find the difference between the two angles (later useful when adding smoothing)
	vNewAngle -= vCurAngle;

	return vNewAngle;
}


bool MultiPoints(Matrix2x4_t* pBoneCache, int hitbox_index, std::vector<Vector_t>& points, CHitBox* hitbox)
{
	Vector_t center = pBoneCache->GetOrigin(hitbox_index);


	if (IPT::GetBindState(C_GET(KeyBind_t, Vars.nBodyAim)) && hitbox_index != HITBOXES::CENTER)
		return false;


	points.push_back(center);


	if (!hitbox)
		return true;


	if (m_data.multipoint)
	{
		points.push_back(center + Vector_t{ 0.f, 0.f, hitbox->ShapeRadius() });
		points.push_back(center - Vector_t{ 0.f, 0.f, hitbox->ShapeRadius() });

		points.push_back(center + Vector_t{ 0.f, hitbox->ShapeRadius(), 0.f });
		points.push_back(center - Vector_t{ 0.f, hitbox->ShapeRadius(), 0.f });

		points.push_back(center + Vector_t{ hitbox->ShapeRadius(), 0.f, 0.f });
		points.push_back(center - Vector_t{ hitbox->ShapeRadius(), 0.f, 0.f });
	}

	return true;
}


Vector_t ExtrapolatePosition(Vector_t pos, Vector_t vel)
{
	float ticks = 1.f;
	const float intervalpertick = INTERVAL_PER_TICK;

	CNetworkGameClient* networkGameClient = I::NetworkClientService->GetNetworkGameClient();
	if (!networkGameClient)
		return pos;

	c_net_chan* netChan = networkGameClient->get_net_channel2();

	if (!netChan)
		return pos;

	float magic = static_cast<float>((netChan->get_network_latency() * 64.f) / 1000.f);

	return (pos) + (vel * intervalpertick * static_cast<float>(ticks)) + (vel * magic);
}

float F::RAGEBOT::GetAngularDistanceRage(CBaseUserCmdPB* pCmd, Vector_t vecTarget, C_CSPlayerPawn* pLocal)
{
	return GetAngularDifferenceRage(pCmd, vecTarget, pLocal).Length2D();
}

void F::RAGEBOT::ScaleDamage(TraceHitboxData_t* hitgroup, C_CSPlayerPawn* entity)
{
	if (entity == nullptr)
		return;

	if (hitgroup == nullptr)
		return;

	if (m_data.m_wpn_data == nullptr)
		return;

	auto pWeaponServices = entity->GetItemServices();
	if (pWeaponServices == nullptr)
		return;

	//ida: server.dll; 80 78 42 00 74 08 F3 0F 59 35 ?? ?? ?? ?? 80 BE 04 0D 00 00 00
	static CConVar *mp_damage_scale_ct_head = I::Cvar->Find(
				   FNV1A::HashConst("mp_damage_scale_ct_head")),
				   *mp_damage_scale_t_head = I::Cvar->Find(
				   FNV1A::HashConst("mp_damage_scale_t_head")),
				   *mp_damage_scale_ct_body = I::Cvar->Find(
				   FNV1A::HashConst("mp_damage_scale_ct_body")),
				   *mp_damage_scale_t_body = I::Cvar->Find(
				   FNV1A::HashConst("mp_damage_scale_t_body"));

	const auto damage_scale_ct_head = mp_damage_scale_ct_head->value.fl,
			   damage_scale_t_head = mp_damage_scale_t_head->value.fl,
			   damage_scale_ct_body = mp_damage_scale_ct_body->value.fl,
			   damage_scale_t_body = mp_damage_scale_t_body->value.fl;

	if (entity->GetLocalPlayer() == nullptr)
		return;

	const bool is_ct = entity->GetLocalPlayer()->GetTeam() == 3, is_t = entity->GetLocalPlayer()->GetTeam() == 2;

	float head_damage_scale = is_ct ? damage_scale_ct_head : is_t ? damage_scale_t_head : 1.0f;
	const float body_damage_scale = is_ct ? damage_scale_ct_body : is_t ? damage_scale_t_body : 1.0f;

	// william: magic values u can see here: ida: server.dll; F3 0F 10 35 ?? ?? ?? ?? 0F 29 7C 24 30 44 0F 29 44 24
	// xref: mp_heavybot_damage_reduction_scale
	if (pWeaponServices->HasHeavyArmor())
		head_damage_scale *= 0.5f;

	switch (hitgroup->m_nHitGroup)
	{
	case HitGroup_t::HITGROUP_HEAD:
		m_data.m_dmg *= m_data.m_wpn_data->GetHeadshotMultiplier() * head_damage_scale;
		break;
	case HitGroup_t::HITGROUP_CHEST:
	case HitGroup_t::HITGROUP_LEFTARM:
	case HitGroup_t::HITGROUP_RIGHTARM:
	case HitGroup_t::HITGROUP_NECK:
		m_data.m_dmg *= body_damage_scale;
		break;
	case HitGroup_t::HITGROUP_STOMACH:
		m_data.m_dmg *= 1.25f * body_damage_scale;
		break;
	case HitGroup_t::HITGROUP_LEFTLEG:
	case HitGroup_t::HITGROUP_RIGHTLEG:
		m_data.m_dmg *= 0.75f * body_damage_scale;
		break;
	default:
		break;
	}

	if (entity->HasArmor(hitgroup->m_nHitGroup))
	{
		float heavy_armor_bonus = 1.0f, armor_bonus = 0.5f, armor_ratio = m_data.m_wpn_data->GetArmorRatio() * 0.5f;

		if (pWeaponServices->HasHeavyArmor())
		{
			heavy_armor_bonus = 0.25f;
			armor_bonus = 0.33f;
			armor_ratio *= 0.20f;
		}

		float damage_to_health = m_data.m_dmg * armor_ratio;
		const float damage_to_armor = (m_data.m_dmg - damage_to_health) * (heavy_armor_bonus * armor_bonus);

		const float iArmor = static_cast<float>(entity->GetArmorValue());

		if (damage_to_armor > iArmor)
		{
			damage_to_health = m_data.m_dmg - iArmor / armor_bonus;
		}

		m_data.m_dmg = damage_to_health;
	}
}

bool F::RAGEBOT::FireBullet(C_CSPlayerPawn* pPawn, Vector_t eye_pos, Vector_t enemy_pos)
{
	//CS_ASSERT(m_data.m_local != nullptr || m_data.m_target != nullptr || m_data.m_wpn_data != nullptr);
	if (m_data.m_wpn_data == nullptr)
		return false;

	if (m_data.m_local == nullptr || pPawn == nullptr)
		return false;

	if (pPawn->GetHealth() <= 0 || m_data.m_local->GetHealth() <= 0)
		return false;

	float weaponRange = m_data.m_wpn_data->GetRange();
	float weaponRangeModifier = m_data.m_wpn_data->GetRangeModifier();
	int weaponDamage = m_data.m_wpn_data->GetDamage();

	Vector_t direction = enemy_pos - eye_pos;
	Vector_t end_pos = direction * weaponRange;

	int pen_count = m_data.penetrationCount;

	TraceData_t trace_data = TraceData_t();
	trace_data.m_arr_pointer = &trace_data.m_arr;

	TraceFilter_t filter;

	I::GameTraceManager->Init(filter, m_data.m_local, PENMASK, 0x3, 0x7);
	void* data_pointer = &trace_data;

	I::GameTraceManager->CreateTrace(&trace_data, eye_pos, end_pos, filter, pen_count);

	handle_bullet_data_t handle_bullet_data(
	static_cast<float>(weaponDamage),
	m_data.m_wpn_data->GetPenetration(),
	weaponRange,
	weaponRangeModifier,
	pen_count,
	false);

	m_data.m_dmg = static_cast<float>(weaponDamage);

	float flTraceLength = 0.f;
	float flMaxRange = weaponRange;


	int pPawnEntryIndex = pPawn->GetRefEHandle().GetEntryIndex();

	//sv_cheats 1; bot_kick; bot_stop 1; bot_add;
	if (trace_data.m_num_update > 0)
	{
		for (int i = 0; i < trace_data.m_num_update; i++)
		{
			if (!trace_data.m_pointer_update_value)
				break;

			if (trace_data.m_arr.data() == nullptr)
				break;

			UpdateValue_t* value = reinterpret_cast<UpdateValue_t* const>(reinterpret_cast<std::uintptr_t>(trace_data.m_pointer_update_value) + i * sizeof(UpdateValue_t));
			if (value == nullptr)
				break;

			GameTrace_t game_trace = GameTrace_t{};
			I::GameTraceManager->InitializeTraceInfo(&game_trace);
			I::GameTraceManager->GetTraceInfo(
			&trace_data, &game_trace, 0.0f,
			reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(trace_data.m_arr.data()) + sizeof(TraceArrElement_t) * (value->handleIdx & ENT_ENTRY_MASK))); //ENT_ENTRY_MASK = 0x7FFF

			flMaxRange -= flTraceLength;

			/*if (game_trace.m_flFraction == 1.0f)
				break;*/

			flTraceLength += game_trace.m_flFraction * flMaxRange;
			m_data.m_dmg *= std::powf(weaponRangeModifier, flTraceLength / 500.f);

			if (flTraceLength > 3000.f)
				break;


			if (game_trace.m_pHitEntity != nullptr)
			{
				if (!game_trace.m_pHitEntity->GetRefEHandle().IsValid())
					return false;

				if (game_trace.m_pHitEntity->GetRefEHandle().GetEntryIndex() == pPawnEntryIndex)
				{
					if (m_data.m_dmg < 1.0f)
						continue;

					ScaleDamage(game_trace.m_pHitboxData, pPawn);
					return true;
				}
			}

			if (I::GameTraceManager->handle_bullet_penetration(&trace_data, &handle_bullet_data, value, false))
				return false;

			m_data.m_dmg = handle_bullet_data.m_dmg;
		}
	}

	return false;
}

auto GetAimPunch(C_CSPlayerPawn* pLocalPawn)
{
	using GetAimPunch_t = float(__fastcall*)(void*, Vector_t*, float, bool);
	static GetAimPunch_t GetAimPunch = reinterpret_cast<GetAimPunch_t>(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 48 8D 4E ? 48 8D 54 24 ? E8 ? ? ? ? F2 0F 10 44 24"), 0x1));

	Vector_t AimPunchAngle;
	GetAimPunch(pLocalPawn, &AimPunchAngle, 0.0f, true);

	return QAngle_t(AimPunchAngle.x, AimPunchAngle.y, AimPunchAngle.z);
}

void SetupHitboxes()
{
	if (m_data.hitboxes & HITBOX_HEAD)
		m_hitboxes.push_back(HITBOXES::HEAD);

	if (m_data.hitboxes & HITBOX_NECK)
		m_hitboxes.push_back(HITBOXES::NECK);

	if (m_data.hitboxes & HITBOX_CHEST)
	{
		m_hitboxes.push_back(HITBOXES::CHEST);
		m_hitboxes.push_back(HITBOXES::RIGHT_CHEST);
		m_hitboxes.push_back(HITBOXES::LEFT_CHEST);
	}

	if (m_data.hitboxes & HITBOX_STOMACH)
		m_hitboxes.push_back(HITBOXES::STOMACH);

	if (m_data.hitboxes & HITBOX_CENTER)
		m_hitboxes.push_back(HITBOXES::CENTER);

	if (m_data.hitboxes & HITBOX_PELVIS)
		m_hitboxes.push_back(HITBOXES::PELVIS);

	if (m_data.hitboxes & HITBOX_LEG)
	{
		m_hitboxes.push_back(HITBOXES::R_LEG);
		m_hitboxes.push_back(HITBOXES::L_LEG);
	}

	if (m_data.hitboxes & HITBOX_FEET)
	{
		m_hitboxes.push_back(HITBOXES::R_FEET);
		m_hitboxes.push_back(HITBOXES::L_FEET);
	}
}


Vector_t calculate_spread_angles(Vector_t angle, int random_seed, float weapon_inaccuarcy, float weapon_spread)
{
	float r1, r2, r3, r4, s1, c1, s2, c2;

	I::RandomSeed(random_seed + 1);

	r1 = I::RandomFloat(0.f, 1.f);
	r2 = I::RandomFloat(0.f, MATH::_2PI);
	r3 = I::RandomFloat(0.f, 1.f);
	r4 = I::RandomFloat(0.f, MATH::_2PI);

	//if (item_def_index == CS_XOR("weapon_revoler") && revolver2)
	//{
	//	r1 = 1.f - (r1 * r1);
	//	r3 = 1.f - (r3 * r3);
	//}

	//// negev spread.
	//else if (item_def_index == CS_XOR("weapon_negev") && recoil_index < 3.f)
	//{
	//	for (int i{ 3 }; i > recoil_index; --i)
	//	{
	//		r1 *= r1;
	//		r3 *= r3;
	//	}

	//	r1 = 1.f - r1;
	//	r3 = 1.f - r3;
	//}

	c1 = std::cos(r2);
	c2 = std::cos(r4);
	s1 = std::sin(r2);
	s2 = std::sin(r4);

	Vector_t spread = {
		(c1 * (r1 * weapon_inaccuarcy)) + (c2 * (r3 * weapon_spread)),
		(s1 * (r1 * weapon_inaccuarcy)) + (s2 * (r3 * weapon_spread))
	};

	Vector_t vec_forward, vec_right, vec_up;
	AngleVectors(angle, vec_forward, vec_right, vec_up);

	return (vec_forward + (vec_right * spread.x) + (vec_up * spread.y)).Normalized();
}


void VectorAngles(const Vector_t& vecForward, QAngle_t& vecAngles)
{
	Vector_t vecView;
	if (vecForward.y == 0 && vecForward.x == 0)
	{
		vecView.x = 0.f;
		vecView.y = 0.f;
	}
	else
	{
		vecView.y = atan2(vecForward.y, vecForward.x) * 180.f / 3.14f;

		if (vecView.y < 0.f)
			vecView.y += 360.f;

		auto tmp = vecForward.Length2D();

		vecView.x = atan2(-vecForward.z, tmp) * 180.f / 3.14f;

		if (vecView.x < 0.f)
			vecView.x += 360.f;
	}

	vecAngles.x = vecView.x;
	vecAngles.y = vecView.y;
	vecAngles.z = 0.f;
}


bool NewHitchance(C_CSPlayerPawn* pPawn, C_CSWeaponBase* pWeapon, Vector_t vecTarget)
{
	if (I::Cvar->Find(FNV1A::HashConst("weapon_accuracy_nospread"))->value.i1 || C_GET(bool, Vars.bNoSpread)) // todo: nospread
		return true;

	if (!pPawn)
		return false;

	const Vector_t vecSource = SDK::LocalPawn->GetEyePosition();

	const int nWantedHitCount = 255;
	Vector_t vecForward = Vector_t(0, 0, 0), vecRight = Vector_t(0, 0, 0), vecUp = Vector_t(0, 0, 0);
	CalcAngles(vecSource, vecTarget).ToDirections(&vecForward, &vecRight, &vecUp);

	int nHits = 0;
	const int nNeededHits = static_cast<int>(nWantedHitCount * (m_data.hitChance / 100.f));

	pWeapon->update_accuracy_penality();
	const float flWeaponSpread = pWeapon->get_spread();
	const float flWeaponInaccuracy = pWeapon->get_inaccuracy();

	//TraceFilter_t filter(0x1C3003, SDK::LocalPawn, nullptr, 4);
	TraceFilter_t filter(0x1C3003, SDK::LocalPawn, NULL, 4);

	for (int i = 0; i < nWantedHitCount; i++)
	{
		float a = I::RandomFloat(0.f, 1.f);
		float b = I::RandomFloat(0.f, 2.f * _PI);
		const float c = I::RandomFloat(0.f, 1.f);
		const float d = I::RandomFloat(0.f, 2.f * _PI);

		const float flInaccuracy = a * flWeaponInaccuracy;
		const float flSpread = c * flWeaponSpread;

		if (auto attManager = pWeapon->GetAttributeManager())
		{
			if (auto item = attManager->GetItem())
			{
				if (item->GetItemDefinitionIndex() == WEAPON_R8_REVOLVER)
				{
					a = 1.f - a * a;
					a = 1.f - c * c;
				}
			}
		}

		Vector_t vecSpreadView((cos(b) * flInaccuracy) + (cos(d) * flSpread), (sin(b) * flInaccuracy) + (sin(d) * flSpread), 0), vecDirection = Vector_t(0, 0, 0);

		vecDirection.x = vecForward.x + (vecSpreadView.x * vecRight.x) + (vecSpreadView.y * vecUp.x);
		vecDirection.y = vecForward.y + (vecSpreadView.x * vecRight.y) + (vecSpreadView.y * vecUp.y);
		vecDirection.z = vecForward.z + (vecSpreadView.x * vecRight.z) + (vecSpreadView.y * vecUp.z);
		vecDirection = vecDirection.Normalized();

		QAngle_t angViewSpread{};
		VectorAngles(vecDirection, angViewSpread);
		angViewSpread = angViewSpread.Normalize();

		Vector_t vecViewForward;
		angViewSpread.ToDirections(&vecViewForward);
		vecViewForward.NormalizeInPlace();
		vecViewForward = vecSource + (vecViewForward * m_data.m_wpn_data->GetRange());

		Ray_t ray = Ray_t();
		GameTrace_t trace = GameTrace_t();
		I::GameTraceManager->ClipRayToEntity(&ray, vecSource, vecViewForward, pPawn, &filter, &trace);

		if (trace.m_pHitEntity == pPawn)//&& trace.m_pHitboxData->m_nHitboxId == m_best_target->best_hitbox)
			nHits++;

		const int nHitChance = static_cast<int>((static_cast<float>(nHits) / nWantedHitCount) * 100.f);
		if (nHitChance >= m_data.hitChance)
			return true;

		if ((nWantedHitCount - i + nHits) < nNeededHits)
			return false;
	}
	return false;
}



bool NoSpread(CUserCmd* pCmd, Vector_t bestPoint, C_CSPlayerPawn* player, int index)
{
	auto weapon = m_data.m_local->GetActiveWeaponFromPlayer();
	if (!weapon)
		return false;

	auto weapon_data = weapon->GetWeaponVData();

	if (!weapon_data)
		return false;

	float acc = weapon->get_inaccuracy();
	float spread = weapon->get_spread();
	auto recoil_index = weapon->GetRecoilIndex();

	float range = weapon_data->GetRange();

	using fn_get_hash_seed = float(__fastcall*)(C_CSPlayerPawn*, Vector_t, int);
	static fn_get_hash_seed fn = reinterpret_cast<fn_get_hash_seed>(MEM::FindPattern(CLIENT_DLL, "48 89 5C 24 ? 57 48 81 EC ? ? ? ? F3 0F 10 0A"));
	int m_shoot_tick;
	auto spread_angle = calculate_spread_angles(bestPoint, fn(m_data.m_local, bestPoint, m_shoot_tick + 1), acc, spread);

	Vector_t result = spread_angle * m_data.m_wpn_data->GetRange() + m_data.m_local->GetEyePosition();

	Ray_t ray{};
	GameTrace_t trace{};
	TraceFilter_t filter{};
	I::GameTraceManager->Init(filter, m_data.m_local, PENMASK, 3, 7);

	I::GameTraceManager->TraceShape(&ray, m_data.m_local->GetEyePosition(), result, &filter, &trace);
	I::GameTraceManager->ClipRayToEntity(&ray, m_data.m_local->GetEyePosition(), result, player, &filter, &trace);

	if (trace.m_pHitEntity)
	{
		if (trace.m_pHitEntity->GetRefEHandle().GetEntryIndex() == player->GetRefEHandle().GetEntryIndex())
			return true;
	}

	return false;
}

#include "../../sdk/interfaces/ccsgoinput.h"

void ProcessShoot(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd)
{
	// pCmd->csgoUserCmd.inputHistoryField.nCurrentSize - 1

	if (C_GET(bool, Vars.bAutoFire))
	{
		if (!(pCmd->nButtons.nValue & IN_ATTACK))
		{
			pCmd->nButtons.nValue |= IN_ATTACK;
			pUserCmd->pInButtonState->nValue |= IN_ATTACK;
		}

		pCmd->SetAttackHistory(pCmd->csgoUserCmd.inputHistoryField.nCurrentSize - 1);
		I::Input->SetAttackHistory(pCmd->csgoUserCmd.inputHistoryField.nCurrentSize - 1);

	}

	//ProcessBacktrack(pCmd, pUserCmd, bestTarget);
}

bool CanShoot(CCSPlayerController* pLocalController, C_CSWeaponBase* active_weapon, int tick)
{
	if (!active_weapon->GetClip1())
		return false;

	if (m_data.m_local->IsWaitForNoAttack())
		return false;

	if (active_weapon->GetNextPrimaryAttackTick() > tick)
		return false;

	return true;
}

void LimitSpeed(CUserCmd* pCmd, C_CSPlayerPawn* pLocalPawn, C_CSWeaponBase* active_weapon, float max_speed)
{
	if (pCmd->csgoUserCmd.pBaseCmd == nullptr)
		return;

	if (pCmd->csgoUserCmd.pBaseCmd->pViewAngles == nullptr)
		return;

	CPlayer_MovementServices* movement_services = pLocalPawn->GetMovementServices();
	if (!movement_services)
		return;

	Vector_t velocity = pLocalPawn->GetAbsVelocity();

	float cmd_speed = std::sqrt(
		(pCmd->csgoUserCmd.pBaseCmd->flSideMove * pCmd->csgoUserCmd.pBaseCmd->flSideMove) +
		(pCmd->csgoUserCmd.pBaseCmd->flForwardMove * pCmd->csgoUserCmd.pBaseCmd->flForwardMove)
	);

	float speed_2d = velocity.Length2D();

	if (cmd_speed <= 50.f && speed_2d <= 50.f)
		return;

	float accelerate = I::Cvar->Find(FNV1A::HashConst("sv_accelerate"))->value.fl;

	Vector_t view_angles = {
		pCmd->csgoUserCmd.pBaseCmd->pViewAngles->angValue.x,
		pCmd->csgoUserCmd.pBaseCmd->pViewAngles->angValue.y,
		pCmd->csgoUserCmd.pBaseCmd->pViewAngles->angValue.z
	};

	Vector_t forward{}, right{}, up{};
	AngleVectors(view_angles, &forward, &right, &up);

	float diff = speed_2d - max_speed;
	float wish_speed = max_speed;

	Vector_t direction = {
		forward.x * pCmd->csgoUserCmd.pBaseCmd->flForwardMove + right.x * pCmd->csgoUserCmd.pBaseCmd->flSideMove,
		forward.y * pCmd->csgoUserCmd.pBaseCmd->flForwardMove + right.y * pCmd->csgoUserCmd.pBaseCmd->flSideMove,
		0.f
	};

	const float max_accelerate = accelerate * INTERVAL_PER_TICK * std::max(250.f, movement_services->GetMaxspeed() * movement_services->GetSurfaceFriction());

	if (diff - max_accelerate <= 0.f || speed_2d - max_accelerate - 3.f <= 0.f)
		wish_speed = max_speed;
	else
	{
		direction = velocity;
		wish_speed = -1.f;
	}

	if (pCmd->csgoUserCmd.pBaseCmd->flForwardMove > 0)
		pCmd->csgoUserCmd.pBaseCmd->flForwardMove = wish_speed;
	else if (pCmd->csgoUserCmd.pBaseCmd->flForwardMove < 0)
		pCmd->csgoUserCmd.pBaseCmd->flForwardMove = -wish_speed;

	if (pCmd->csgoUserCmd.pBaseCmd->flSideMove > 0)
		pCmd->csgoUserCmd.pBaseCmd->flSideMove = wish_speed;
	else if (pCmd->csgoUserCmd.pBaseCmd->flSideMove < 0)
		pCmd->csgoUserCmd.pBaseCmd->flSideMove = -wish_speed;

	pCmd->csgoUserCmd.pBaseCmd->SetBits(EBaseCmdBits::BASE_BITS_FORWARDMOVE);
	pCmd->csgoUserCmd.pBaseCmd->SetBits(EBaseCmdBits::BASE_BITS_LEFTMOVE);
}





//uint32_t compute_random_seed(QAngle_t* angViewAngles, int nPlayerTickCount)
//{
//	using fn_t = uint32_t(__fastcall*)(void* a1, QAngle_t* angViewAngles, int nPlayerTickCount);
//	static fn_t fn = (fn_t)GetSigPtr(FNV1A::HashConst("COMPUTE_RANDOM_SEED"));
//	return fn(nullptr, angViewAngles, nPlayerTickCount);
//}
//
//
//Vector2D_t CalcSpread(int nRandomSeed, float flInAccuracy, float flSpread)
//{
//	using fn_t = void(__fastcall*)(int16_t, int, int, std::uint32_t, float, float, float, float*, float*);
//	static auto pFn = (fn_t)GetSigPtr(FNV1A::HashConst("CALC_SPREAD"));
//
//	Vector2D_t vecOut;
//	pFn(SDK::LocalPawn->GetActiveWeapon()->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex(), /*num bullets*/ 1, 0, nRandomSeed, flInAccuracy, flSpread, SDK::LocalPawn->GetActiveWeapon()->m_flRecoilIndex(), &vecOut.x, &vecOut.y);
//
//	return vecOut;
//
//}



//struct NoSpreadResult
//{
//	bool found;
//	QAngle_t adjusted;
//	int foundAfter;
//	int seed;
//};
//
//NoSpreadResult nospread(QAngle_t& angle)
//{
//	NoSpreadResult res{};
//	res.found = false;
//	CUserCmd* pCmd = SDK::Cmd;
//
//	C_CSWeaponBase* weapon = H::cheat->m_weapon;
//
//	unsigned int nCurrentTick = SDK::LocalController->GetTickBase();
//	float flInAccuracy = weapon->get_inaccuracy();
//	float flSpread = weapon->get_spread();
//
//	for (int i = 0; i < 720; i++)
//	{
//		QAngle_t angTemp = QAngle_t((float)i / 2.0f, angle.y, 0.0f);
//
//		int nTempSeed = compute_random_seed(&angTemp, nCurrentTick);
//
//		Vector_t vecSpread = CalcSpread(nTempSeed + 1, flInAccuracy, flSpread);
//
//		QAngle_t angNoSpreadView = angle;
//		angNoSpreadView.x += DirectX::XMConvertToDegrees(atan(sqrt((vecSpread.x * vecSpread.x) + (vecSpread.y * vecSpread.y))));
//		angNoSpreadView.z = -DirectX::XMConvertToDegrees(atan2(vecSpread.x, vecSpread.y));
//
//		if (compute_random_seed(&angNoSpreadView, nCurrentTick) == nTempSeed)
//		{
//			res.adjusted = angNoSpreadView;
//			res.seed = nTempSeed;
//			res.found = true;
//			break;
//		}
//	}
//
//	return res;
//}



void clamp_movement(CUserCmd* cmd, float speed)
{
	float final_speed = speed;
	if (!cmd)
		return;

	cmd->nButtons.nValueScroll &= ~IN_SPRINT;

	auto f = cmd->csgoUserCmd.pBaseCmd->flForwardMove * 250.f;
	auto f2 = cmd->csgoUserCmd.pBaseCmd->flSideMove * 250.f;

	float squirt = std::sqrtf((f * f) + (f2 * f2));

	if (squirt > speed)
	{
		float aye = squirt / speed;
		cmd->csgoUserCmd.pBaseCmd->flForwardMove /= aye;
		cmd->csgoUserCmd.pBaseCmd->SetBits(EBaseCmdBits::BASE_BITS_FORWARDMOVE);
		cmd->csgoUserCmd.pBaseCmd->flSideMove /= aye;
		cmd->csgoUserCmd.pBaseCmd->SetBits(EBaseCmdBits::BASE_BITS_LEFTMOVE);
	}
}




void AutoStop(CUserCmd* pCmd, C_CSPlayerPawn* pLocalPawn, C_CSWeaponBase* active_weapon)
{

	/*auto remove_button = [&](int button)
	{
		pCmd->nButtons.nValue &= ~button;
		pCmd->nButtons.nValueChanged &= ~button;
		pCmd->nButtons.nValueScroll &= ~button;
	};

	remove_button(IN_FORWARD);
	remove_button(IN_BACK);
	remove_button(IN_LEFT);
	remove_button(IN_RIGHT);
	remove_button(IN_MOVELEFT);
	remove_button(IN_MOVERIGHT);*/

	return;

	CPlayer_MovementServices* movement_services = pLocalPawn->GetMovementServices();
	if (!movement_services)
		return;

	Vector_t velocity = pLocalPawn->GetVecVelocity();

	float max_speed = active_weapon->get_max_speed() * 0.2f;
	float speed = velocity.Length2D();

	float sv_friction = I::Cvar->Find(FNV1A::Hash("sv_friction"))->value.fl;

	auto newspeed = speed - ((speed * 0.17f) * sv_friction) * movement_services->GetSurfaceFriction();
	auto max_accelspeed = fabsf((0.17f * movement_services->GetSurfaceFriction()) * (active_weapon->get_max_speed() * sv_friction));
	auto accelspeed = fabsf(fminf((0.17f * movement_services->GetSurfaceFriction()) * ((speed * 0.8f) * sv_friction),
		(speed * 0.8f) - newspeed));


	auto new_speed = max_accelspeed / accelspeed;
	new_speed *= INTERVAL_PER_TICK; // interval per subtick

	if (speed <= max_speed)
	{
		clamp_movement(pCmd, max_speed);
	}


	else
	{
		Vector_t angle;
		vector_angles(velocity, angle);

		float speed = velocity.Length2D();

		angle.y = pCmd->csgoUserCmd.pBaseCmd->pViewAngles->angValue.y - angle.y;

		Vector_t direction;
		AngleVectors(angle, &direction);

		Vector_t stop = direction * -speed;

		if (speed > 13.f)
		{
			pCmd->csgoUserCmd.pBaseCmd->flForwardMove = stop.x * new_speed;
			pCmd->csgoUserCmd.pBaseCmd->SetBits(EBaseCmdBits::BASE_BITS_FORWARDMOVE);
			pCmd->csgoUserCmd.pBaseCmd->flSideMove = stop.y * new_speed;
			pCmd->csgoUserCmd.pBaseCmd->SetBits(EBaseCmdBits::BASE_BITS_LEFTMOVE);
		}
		else
		{
			pCmd->csgoUserCmd.pBaseCmd->flForwardMove = 0.f;
			pCmd->csgoUserCmd.pBaseCmd->SetBits(EBaseCmdBits::BASE_BITS_FORWARDMOVE);
			pCmd->csgoUserCmd.pBaseCmd->flSideMove = 0.f;
			pCmd->csgoUserCmd.pBaseCmd->SetBits(EBaseCmdBits::BASE_BITS_LEFTMOVE);
		}
	}

	movement_services->run_command(pCmd);
}

void FullAutoStop(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, C_CSPlayerPawn* pLocalPawn, C_CSWeaponBase* active_weapon)
{
	if (!(pLocalPawn->GetFlags() & FL_ONGROUND))
		return;

	auto remove_button = [&](int button)
	{
		pUserCmd->pInButtonState->nValue &= ~button;
		pUserCmd->pInButtonState->nValueChanged &= ~button;
		pUserCmd->pInButtonState->nValueScroll &= ~button;
		pUserCmd->pInButtonState->SetBits(EButtonStatePBBits::BUTTON_STATE_PB_BITS_BUTTONSTATE1);
		pUserCmd->pInButtonState->SetBits(EButtonStatePBBits::BUTTON_STATE_PB_BITS_BUTTONSTATE2);
		pUserCmd->pInButtonState->SetBits(EButtonStatePBBits::BUTTON_STATE_PB_BITS_BUTTONSTATE3);
	};

	remove_button(IN_SPRINT);

	float wish_speed = active_weapon->get_max_speed() * 0.6f;

	LimitSpeed(pCmd, pLocalPawn, active_weapon, wish_speed);
}

void QuickStop(CUserCmd* pCmd, C_CSPlayerPawn* pLocalPawn, C_CSWeaponBase* active_weapon)
{
	CPlayer_MovementServices* movement_services = pLocalPawn->GetMovementServices();
	if (!movement_services)
		return;

	Vector_t velocity = pLocalPawn->GetAbsVelocity();

	const auto speed = velocity.Length2D();
	if (speed < 1.f)
	{
		pCmd->csgoUserCmd.pBaseCmd->flForwardMove = pCmd->csgoUserCmd.pBaseCmd->flSideMove = 0.f;
		return;
	}

	static const auto accelerate{ I::Cvar->Find(FNV1A::HashConst("sv_accelerate")) };
	static const auto max_speed{ I::Cvar->Find(FNV1A::HashConst("sv_maxspeed")) };

	const auto surface_friction{ movement_services->GetSurfaceFriction() };

	const auto max_accelspeed{ accelerate->value.fl * max_speed->value.fl * surface_friction };

	auto get_maximum_accelerate_speed = [&]
	{
		const auto speed_ratio{ speed / (accelerate->value.fl) };
		return speed - max_accelspeed <= -1.f ? max_accelspeed / speed_ratio : max_accelspeed;
	};

	Vector_t velocity_angle{};
	vec_angles(velocity * -1.0f, &velocity_angle);
	velocity_angle.y = pCmd->csgoUserCmd.pBaseCmd->pViewAngles->angValue.y - velocity_angle.y;
}

#include <chrono>


void backtrack_render();

void F::RAGEBOT::AutoPeek(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, C_CSPlayerPawn* pLocalPawn)
{
	active_retract = false;
	if (!C_GET(bool, Vars.bAutoPeek))
		return;

	bool isMovementKeyPressed = (pCmd->nButtons.nValue & IN_FORWARD) || (pCmd->nButtons.nValue & IN_BACK) || (pCmd->nButtons.nValue & IN_MOVELEFT) || (pCmd->nButtons.nValue & IN_MOVERIGHT);

	if (!isMovementKeyPressed)
		quickpeek_retract = true;
	else
		quickpeek_retract = false;

	if (pCmd->nButtons.nValue & IN_ATTACK)
		quickpeek_retract_shoot = true;

	if (!IPT::GetBindState(C_GET(KeyBind_t, Vars.nAutoPeekKey)))
	{
		quickpeek_retract = false;
		retractPos = Vector_t{};
	}

	if (retractPos.IsZero() && IPT::GetBindState(C_GET(KeyBind_t, Vars.nAutoPeekKey)))
		retractPos = pLocalPawn->GetGameSceneNode()->GetAbsOrigin();

	if ((quickpeek_retract || quickpeek_retract_shoot) && !retractPos.IsZero())
	{
		float dist = pLocalPawn->GetGameSceneNode()->GetAbsOrigin().DistTo(retractPos);
		if (dist <= 1.f)
		{
			quickpeek_retract = false;
			quickpeek_retract_shoot = false;
			return;
		}

		pUserCmd->flForwardMove = 1.f;
		pUserCmd->flSideMove = 0.f;
		Vector_t* move = (Vector_t*)&pUserCmd->flForwardMove;
		Vector_t move_backup = *move;
		const QAngle_t& current_angles = pUserCmd->pViewAngles->angValue;

		QAngle_t angle = CalcAngles(pLocalPawn->GetGameSceneNode()->GetAbsOrigin(), retractPos);

		const float delta = remainderf(angle.y - current_angles.y, 360.f);
		const float yaw = DEG2RAD(delta);

		move->x = move_backup.x * std::cos(yaw) - move_backup.y * std::sin(yaw);
		move->y = move_backup.x * std::sin(yaw) + move_backup.y * std::cos(yaw);

		float mul = 1.f;

		if (std::fabsf(move->x) > 1.f)
			mul = 1.f / std::fabsf(move->x);
		else if (std::fabsf(move->y) > 1.f)
			mul = 1.f / std::fabsf(move->y);

		move->x *= mul;
		move->y *= mul;
		move->z = 0.f;

		float mul2 = 1.f;
		static float trigger_dist = 10.f;
		if (dist < trigger_dist)
			mul2 = dist / trigger_dist;

		pUserCmd->SetBits(EBaseCmdBits::BASE_BITS_FORWARDMOVE);
		pUserCmd->SetBits(EBaseCmdBits::BASE_BITS_LEFTMOVE);
		pUserCmd->flForwardMove = move->x * mul2;
		pUserCmd->flSideMove = move->y * mul2;
		active_retract = true;
	}
}

Color_t get_rainbow(float alpha, float offset)
{
	// Get the current time in seconds
	auto now = std::chrono::steady_clock::now();
	auto duration = now.time_since_epoch();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() / 1000.0f;

	// Adjust the time by the speed and offset
	float adjustedTime = time * (100 / 50.f);
	adjustedTime += offset;

	// Compute the RGB components based on the adjusted time
	float r = 0.5f + 0.5f * std::sin(adjustedTime + 0.0f); // Red component
	float g = 0.5f + 0.5f * std::sin(adjustedTime + 2.0f * M_PI / 3.0f); // Green component
	float b = 0.5f + 0.5f * std::sin(adjustedTime + 4.0f * M_PI / 3.0f); // Blue componentwb

	// Return the color as an RGB structure
	return Color_t{ r, g, b, alpha };
}

void SpinningCurve3D(Vector_t& origin, float radius, ImColor color, float angleOffset, float arcLength)
{
	int numSegments = 20;
	float step = arcLength / numSegments;

	float startAngle = angleOffset;
	float endAngle = angleOffset + arcLength;

	for (float angle = startAngle; angle < endAngle; angle += step)
	{
		Vector_t start(radius * cosf(angle) + origin.x, radius * sinf(angle) + origin.y, origin.z);
		Vector_t end(radius * cosf(angle + step) + origin.x, radius * sinf(angle + step) + origin.y, origin.z);

		ImVec2 start2d, end2d;
		if (D::WorldToScreen(start, &start2d) && D::WorldToScreen(end, &end2d))
		{
			ImGui::GetBackgroundDrawList()->AddLine(start2d, end2d, color);
		}
	}
}

bool CircleFilled3D(Vector_t& origin, float radius, ImColor color)
{
	std::vector<ImVec2> vert{};

	float step = (float)3.14f * 2.0f / 50.f;

	for (float a = 0; a < (3.14f * 2.0f); a += step)
	{
		Vector_t start(radius * cosf(a) + origin.x, radius * sinf(a) + origin.y, origin.z);
		Vector_t end(radius * cosf(a + step) + origin.x, radius * sinf(a + step) + origin.y, origin.z);

		ImVec2 start2d, end2d;
		if (!D::WorldToScreen(start, &start2d) || !D::WorldToScreen(end, &end2d))
			continue;

		vert.emplace_back(start2d);
		vert.emplace_back(end2d);
	}

	if (vert.size() > 3)
		ImGui::GetBackgroundDrawList()->AddConvexPolyFilled(vert.data(), vert.size(), color);

	return true;
}

void F::RAGEBOT::present()
{
	/*ImGuiIO& io = ImGui::GetIO();
	const ImVec2 vecScreenSize = io.DisplaySize;
	const ImVec2 vecScreenCenter = vecScreenSize / 2.f;*/


	static float angleOffset = 0.0f; // Initial angle
	static float rotationSpeed = 3.f; // Speed of rotation

	angleOffset += rotationSpeed * ImGui::GetIO().DeltaTime;
	if (angleOffset > (3.14f * 2.0f))
	{
		angleOffset -= (3.14f * 2.0f);
	}

	if (!retractPos.IsZero())
	{
		CircleFilled3D(retractPos, 20.f, get_rainbow(.1f, 3757.f).GetU32(1.f));
		SpinningCurve3D(retractPos, 20.f, get_rainbow(1.f, 3757.f).GetU32(1.f), angleOffset, 1.4f);
	}


	backtrack_render();
}



int get_hash_seed(C_CSPlayerPawn* player, QAngle_t angle, int attack_tick)
{
	static auto get_hash_seed = reinterpret_cast<int(__fastcall*)(C_CSPlayerPawn*, QAngle_t, int)>(MEM::FindPattern(CLIENT_DLL, "48 89 5C 24 ? 57 48 81 EC ? ? ? ? F3 0F 10 0A"));
	return get_hash_seed(player, angle, attack_tick);
}

Vector_t compensation_spread(CUserCmd* pCmd, C_CSWeaponBase* weapon)
{
	static bool penis = false;
	
	try
	{
		auto angle_ = pCmd->csgoUserCmd.inputHistoryField.pRep->tElements[pCmd->csgoUserCmd.inputHistoryField.nCurrentSize - 1]->pViewAngles->angValue;

		int hash_seed = get_hash_seed(m_data.m_local, angle_, pCmd->csgoUserCmd.inputHistoryField.pRep->tElements[pCmd->csgoUserCmd.inputHistoryField.nCurrentSize - 1]->nPlayerTickCount);
		auto spread_calc = weapon->calculate_spread(hash_seed);

		return spread_calc;
	}
	catch (...)
	{
		return {};
	}
}



void autoRevolver(CUserCmd* pCmd)
{
	//constexpr float revolverPrepareTime{ 0.234375f };
	static float revolverPrepareTime = 0.1f;

	static float readyTime;
	if (!SDK::LocalPawn)
		return;

	const auto activeWeapon = H::cheat->m_weapon;
	if (!activeWeapon)
		return;

	if (!I::NetworkClientService->GetNetworkGameClient())
		return;

	if (!I::NetworkClientService->GetNetworkGameClient()->get_net_channel2())
		return;

	if (!activeWeapon->GetAttributeManager())
		return;

	if (!activeWeapon->GetAttributeManager()->GetItem())
		return;

	if (activeWeapon->GetAttributeManager()->GetItem()->GetItemDefinitionIndex() == WEAPON_R8_REVOLVER)
	{

		if (readyTime == 0.0f)
			readyTime = I::GlobalVars->flCurrentTime + revolverPrepareTime;
		const auto ticksToReady = TIME_TO_TICKS(readyTime - I::GlobalVars->flCurrentTime - I::NetworkClientService->GetNetworkGameClient()->get_net_channel2()->get_latency(FLOW_OUTGOING));
		if (ticksToReady > 0 && ticksToReady <= TIME_TO_TICKS(revolverPrepareTime))
			pCmd->nButtons.nValue |= IN_ATTACK;
		else
			readyTime = 0.0f;
	}
}



inline float GetFov(const QAngle_t& viewAngle, const QAngle_t& aimAngle)
{
	QAngle_t delta = (aimAngle - viewAngle).Normalize();

	return sqrtf(powf(delta.x, 2.0f) + powf(delta.y, 2.0f));
}

const char* GetExtractedWeaponName(C_CSWeaponBase* weapon)
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

void SetAdaptiveWeapon(C_CSWeaponBase* weapon)
{
	const char* extractedWeaponName = GetExtractedWeaponName(weapon);

	bool has_awp = strcmp(extractedWeaponName, CS_XOR("awp")) == 0;
	bool has_heavy_pistols = strcmp(extractedWeaponName, CS_XOR("revolver")) == 0 || strcmp(extractedWeaponName, CS_XOR("deagle")) == 0;
	bool has_scout = strcmp(extractedWeaponName, CS_XOR("ssg08")) == 0;


	if (has_awp)
	{
		m_data.hitChance = C_GET_ARRAY(int, 7, Vars.iHitChance, 6);
		m_data.hitboxes = C_GET_ARRAY(unsigned int, 7, Vars.bRageHitboxes, 6);
		m_data.multipoint = C_GET_ARRAY(bool, 7, Vars.bMultiPoint, 6);
		m_data.penetrationCount = C_GET_ARRAY(int, 7, Vars.iPenetrationCount, 6);
		m_data.minDamage = C_GET_ARRAY(int, 7, Vars.iMinDamage, 6);
		m_data.overrideDamage = C_GET_ARRAY(int, 7, Vars.iOverrideDamage, 6);
	}

	else if (has_scout)
	{
		m_data.hitChance = C_GET_ARRAY(int, 7, Vars.iHitChance, 5);
		m_data.hitboxes = C_GET_ARRAY(unsigned int, 7, Vars.bRageHitboxes, 5);
		m_data.multipoint = C_GET_ARRAY(bool, 7, Vars.bMultiPoint, 5);
		m_data.penetrationCount = C_GET_ARRAY(int, 7, Vars.iPenetrationCount, 5);
		m_data.minDamage = C_GET_ARRAY(int, 7, Vars.iMinDamage, 5);
		m_data.overrideDamage = C_GET_ARRAY(int, 7, Vars.iOverrideDamage, 5);
	}

	else if (has_heavy_pistols)
	{
		m_data.hitChance = C_GET_ARRAY(int, 7, Vars.iHitChance, 2);
		m_data.hitboxes = C_GET_ARRAY(unsigned int, 7, Vars.bRageHitboxes, 2);
		m_data.multipoint = C_GET_ARRAY(bool, 7, Vars.bMultiPoint, 2);
		m_data.penetrationCount = C_GET_ARRAY(int, 7, Vars.iPenetrationCount, 2);
		m_data.minDamage = C_GET_ARRAY(int, 7, Vars.iMinDamage, 2);
		m_data.overrideDamage = C_GET_ARRAY(int, 7, Vars.iOverrideDamage, 2);
	}

	else if (m_data.m_wpn_data->GetWeaponType() == WEAPONTYPE_PISTOL && !has_heavy_pistols)
	{
		m_data.hitChance = C_GET_ARRAY(int, 7, Vars.iHitChance, 1);
		m_data.hitboxes = C_GET_ARRAY(unsigned int, 7, Vars.bRageHitboxes, 1);
		m_data.multipoint = C_GET_ARRAY(bool, 7, Vars.bMultiPoint, 1);
		m_data.penetrationCount = C_GET_ARRAY(int, 7, Vars.iPenetrationCount, 1);
		m_data.minDamage = C_GET_ARRAY(int, 7, Vars.iMinDamage, 1);
		m_data.overrideDamage = C_GET_ARRAY(int, 7, Vars.iOverrideDamage, 1);
	}

	else if (m_data.m_wpn_data->GetWeaponType() == WEAPONTYPE_MACHINEGUN)
	{
		m_data.hitChance = C_GET_ARRAY(int, 7, Vars.iHitChance, 3);
		m_data.hitboxes = C_GET_ARRAY(unsigned int, 7, Vars.bRageHitboxes, 3);
		m_data.multipoint = C_GET_ARRAY(bool, 7, Vars.bMultiPoint, 3);
		m_data.penetrationCount = C_GET_ARRAY(int, 7, Vars.iPenetrationCount, 3);
		m_data.minDamage = C_GET_ARRAY(int, 7, Vars.iMinDamage, 3);
		m_data.overrideDamage = C_GET_ARRAY(int, 7, Vars.iOverrideDamage, 3);
	}

	else if (m_data.m_wpn_data->GetWeaponType() == WEAPONTYPE_RIFLE)
	{
		m_data.hitChance = C_GET_ARRAY(int, 7, Vars.iHitChance, 3);
		m_data.hitboxes = C_GET_ARRAY(unsigned int, 7, Vars.bRageHitboxes, 3);
		m_data.multipoint = C_GET_ARRAY(bool, 7, Vars.bMultiPoint, 3);
		m_data.penetrationCount = C_GET_ARRAY(int, 7, Vars.iPenetrationCount, 3);
		m_data.minDamage = C_GET_ARRAY(int, 7, Vars.iMinDamage, 3);
		m_data.overrideDamage = C_GET_ARRAY(int, 7, Vars.iOverrideDamage, 3);
	}

	else if (m_data.m_wpn_data->GetWeaponType() == WEAPONTYPE_SNIPER_RIFLE)
	{
		m_data.hitChance = C_GET_ARRAY(int, 7, Vars.iHitChance, 4);
		m_data.hitboxes = C_GET_ARRAY(unsigned int, 7, Vars.bRageHitboxes, 4);
		m_data.multipoint = C_GET_ARRAY(bool, 7, Vars.bMultiPoint, 4);
		m_data.penetrationCount = C_GET_ARRAY(int, 7, Vars.iPenetrationCount, 4);
		m_data.minDamage = C_GET_ARRAY(int, 7, Vars.iMinDamage, 4);
		m_data.overrideDamage = C_GET_ARRAY(int, 7, Vars.iOverrideDamage, 4);
	}

	else
	{
		m_data.hitChance = C_GET_ARRAY(int, 7, Vars.iHitChance, 0);
		m_data.hitboxes = C_GET_ARRAY(unsigned int, 7, Vars.bRageHitboxes, 0);
		m_data.multipoint = C_GET_ARRAY(bool, 7, Vars.bMultiPoint, 0);
		m_data.penetrationCount = C_GET_ARRAY(int, 7, Vars.iPenetrationCount, 0);
		m_data.minDamage = C_GET_ARRAY(int, 7, Vars.iMinDamage, 0);
		m_data.overrideDamage = C_GET_ARRAY(int, 7, Vars.iOverrideDamage, 0);
	}
}


void FindBestExtrapolationTarget(CBaseUserCmdPB* pUserCmd)
{

}


#include <numbers>
void angle_vector(const QAngle_t& angles, Vector_t& forward) noexcept
{
	const float x = angles.x * std::numbers::pi_v<float> / 180.f;
	const float y = angles.y * std::numbers::pi_v<float> / 180.f;
	const float sp = std::sin(x);
	const float cp = std::cos(x);
	const float sy = std::sin(y);
	const float cy = std::cos(y);
	forward.x = cp * cy;
	forward.y = cp * sy;
	forward.z = -sp;
}



void F::RAGEBOT::AutoStop(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pLocalController)
{

	if (!C_GET(bool, Vars.bAutoStop) || I::Cvar->Find(FNV1A::HashConst("weapon_accuracy_nospread"))->value.i1)
		return;

	auto active_weapon = H::cheat->m_weapon;
	if (active_weapon == nullptr)
		return;

	if (pCmd->nButtons.nValue & IN_DUCK || !pLocalPawn || !active_weapon)
		return;

	if (!pLocalPawn->IsValidMoveType() || !(pLocalPawn->GetFlags() & FL_ONGROUND))
		return;


	pUserCmd->flSideMove = 0.0f;
	pUserCmd->flForwardMove = pLocalPawn->GetVecVelocity().Length2D() > 20.0f ? 1.0f : 0.0f;
	pUserCmd->SetBits(BASE_BITS_FORWARDMOVE);
	pUserCmd->SetBits(BASE_BITS_LEFTMOVE);

	QAngle_t angViewAngles = pUserCmd->pViewAngles->angValue;

	float flYaw = pLocalPawn->GetVecVelocity().ToAngles().y + 180.0f;
	float flRotation = M_DEG2RAD(angViewAngles.y - flYaw);

	float flCosRotation = std::cos(flRotation);
	float flSinRotation = std::sin(flRotation);

	float flNewForwardMove = flCosRotation * pUserCmd->flForwardMove - flSinRotation * pUserCmd->flSideMove;
	float flNewSideMove = flSinRotation * pUserCmd->flForwardMove + flCosRotation * pUserCmd->flSideMove;

	pUserCmd->flForwardMove = flNewForwardMove;
	pUserCmd->flSideMove = -flNewSideMove;
	pUserCmd->SetBits(BASE_BITS_FORWARDMOVE);
	pUserCmd->SetBits(BASE_BITS_LEFTMOVE);
}


void backtrack_render()
{
	const float las_valid_sim_time = get_las_valid_sim_time();
	for (const auto& [index, entity] : records)
	{
		const auto best_records = entity->find_best_records(las_valid_sim_time);
		if (!best_records.record1 || !best_records.record2)
			return;

		try
		{
			ImVec2 screenLastRecordPos;
			if (D::WorldToScreen(best_records.record1->vecOrigin, &screenLastRecordPos))
			{
				ImGui::GetBackgroundDrawList()->AddCircleFilled(screenLastRecordPos, 2.5f, Color_t(255, 255, 255).GetU32(), 10);
			}

			ImVec2 screenEnemyPos;
			if (D::WorldToScreen(best_records.record2->vecOrigin, &screenEnemyPos))
			{
				ImGui::GetBackgroundDrawList()->AddCircleFilled(screenEnemyPos, 2.5f, Color_t(255, 0, 0).GetU32(), 10);
			}
		}
		catch (...)
		{ }
	}
}



bool FireTarget(backtrack_entity* entity, record* rec, float& best_damage, Vector_t& best_point)
{
	if (!rec)
		return false;

	// Firstly, get the skeleton
	auto pSkeleton = rec->m_skeleton;
	if (pSkeleton == nullptr)
		return false;

	// Now the bones
	auto pBoneCache = pSkeleton->pBoneCache;
	if (pBoneCache == nullptr)
		return false;

	int min_damage = std::min(m_data.overrideDamage, entity->m_pawn->GetHealth());
	if (!C_GET(bool, Vars.bEnableOverrideDamage))
		min_damage = std::min(m_data.minDamage, entity->m_pawn->GetHealth() + 30);


	Vector_t start_pos = m_data.m_local->GetEyePosition();


	rec->apply(entity->m_pawn);


	/*CModelState pModel = rec->m_skeleton->GetModelState();
	CStrongHandle<CModel> pModelHandle = pModel.modelHandle;
	if (!pModelHandle)
		return false;*/

	for (int hitbox : m_hitboxes)
	{
		std::vector<Vector_t> points{};
		points.reserve(68);

		//CHitBox* cHitbox = pModelHandle->GetHitbox(hitbox);
		MultiPoints(pBoneCache, hitbox, points, nullptr);

		m_data.m_target = entity->m_pawn;

		for (auto& point : points)
		{
			if (!C_GET(bool, Vars.bAutoWall))
			{
				GameTrace_t trace = GameTrace_t();
				TraceFilter_t filter{ MASK_SHOT, m_data.m_local, NULL, 4 };
				Ray_t ray = Ray_t();

				I::GameTraceManager->TraceShape(&ray, start_pos, point, &filter, &trace);
				if (trace.m_pHitEntity != m_data.m_target)
					continue;
			}

			if (!F::RAGEBOT::FireBullet(m_data.m_target, start_pos, point))
				continue;


			if (m_data.m_dmg > best_damage && m_data.m_dmg > min_damage)
			{
				best_damage = m_data.m_dmg;
				best_point = point;
			}
		}
	}

	rec->reset(entity->m_pawn);


	if (min_damage > best_damage)
		return false;

	return true;
}


void F::RAGEBOT::RageAim(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pLocalController)
{
	if (!C_GET(bool, Vars.bRageBot))
	{
		quickpeek_retract = false;
		quickpeek_retract_shoot = false;
		return;
	}

	auto active_weapon = H::cheat->m_weapon;
	auto pWeaponVData = H::cheat->m_wpn_data;
	if (!active_weapon || !pWeaponVData)
		return;

	m_data.m_wpn_data = pWeaponVData;
	m_data.isValid = false;

	if (pWeaponVData->GetWeaponType() == WEAPONTYPE_KNIFE || pWeaponVData->GetWeaponType() == WEAPONTYPE_GRENADE)
		return;

	auto active_weapon_base_gun = I::GameResourceService->pGameEntitySystem->Get<C_CSWeaponBaseGun>(active_weapon->GetRefEHandle());
	if (active_weapon_base_gun == nullptr)
		return;


	SetAdaptiveWeapon(active_weapon);
	//autoRevolver(pCmd);


	// Setup Hitboxes
	m_hitboxes.clear();
	SetupHitboxes();
	if (m_hitboxes.empty())
		return;



	const auto max_ticks = TIME_TO_TICKS(I::Cvar->Find(FNV1A::Hash("sv_maxunlag"))->value.fl);
	const auto& entity_list = F::ANTIAIM::get("CCSPlayerController");
	if (entity_list.size() <= 0)
		return;


	const float las_valid_sim_time = get_las_valid_sim_time();

	int target_index = -1;
	float target_index_dist = FLT_MAX;


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
		if (pPlayer == nullptr)
			continue;

		// Check the entity is not us
		if (pPlayer == pLocalController)
		{
			auto player_iterator = records.find(pPlayer->GetPawnHandle().ToInt());
			if (player_iterator != records.end())
				records.erase(player_iterator);

			continue;
		}

		// Make sure they're alive
		if (!pPlayer->IsPawnAlive())
		{
			auto player_iterator = records.find(pPlayer->GetPawnHandle().ToInt());
			if (player_iterator != records.end())
				records.erase(player_iterator);

			continue;
		}

		// Get the player pawn
		C_CSPlayerPawn* pPawn = I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(pPlayer->GetPawnHandle());
		if (pPawn == nullptr)
		{
			auto player_iterator = records.find(pPlayer->GetPawnHandle().ToInt());
			if (player_iterator != records.end())
				records.erase(player_iterator);

			continue;
		}

		// Check if they're an enemy
		if (!pLocalPawn->IsOtherEnemy(pPawn))
		{
			auto player_iterator = records.find(pPlayer->GetPawnHandle().ToInt());
			if (player_iterator != records.end())
				records.erase(player_iterator);

			continue;
		}

		if (pPawn->GetGunGameImmunity())
		{
			auto player_iterator = records.find(pPlayer->GetPawnHandle().ToInt());
			if (player_iterator != records.end())
				records.erase(player_iterator);

			continue;
		}

		// Check if they're dormant
		CGameSceneNode* pCGameSceneNode = pPawn->GetGameSceneNode();
		if (pCGameSceneNode == nullptr || pCGameSceneNode->IsDormant())
		{
			auto player_iterator = records.find(pPlayer->GetPawnHandle().ToInt());
			if (player_iterator != records.end())
				records.erase(player_iterator);

			continue;
		}


		if (records.find(pPlayer->GetPawnHandle().ToInt()) == records.end())
		{
			records.insert_or_assign(pPlayer->GetPawnHandle().ToInt(), std::make_unique<backtrack_entity>(pPawn));
			continue;
		}


		try
		{
			auto entity_record = records[pPlayer->GetPawnHandle().ToInt()].get();
			entity_record->save_record();

			if (entity_record->records.size() > max_ticks)
				entity_record->records.resize(max_ticks);

			float distance = GetAngularDifferenceRageView(pUserCmd, entity_record->m_pawn->GetSceneOrigin(), pLocalPawn).Length2D();
			if (distance > C_GET(int, Vars.iRageAimRange))
				continue;
			if (target_index != -1 && distance > target_index_dist)
				continue;

			target_index = pPlayer->GetPawnHandle().ToInt();
			target_index_dist = distance;
		}
		catch (...)
		{ }
	}

	// no entities or to far away
	if (target_index == -1)
		return;



	const auto entity = records[target_index].get();
	if (!entity)
		return;
	if (!entity->m_pawn)
		return;

	const auto best_records = entity->find_best_records(las_valid_sim_time);
	if (best_records.record1 == nullptr || best_records.record2 == nullptr)
		return;

	float bestDamage = 0;
	Vector_t bestPoint = { 0, 0, 0 };
	record* bestRecord = nullptr;

	if (FireTarget(entity, best_records.record1, bestDamage, bestPoint))
	{
		bestRecord = best_records.record1;
	}
	else
	{
		if (FireTarget(entity, best_records.record2, bestDamage, bestPoint))
		{
			bestRecord = best_records.record2;
		}
		else
			return;
	}

	if (bestRecord == nullptr)
		return;


	if (pUserCmd->pViewAngles == nullptr)
		return;

	/*try
	{
		if (!CanShoot(pLocalController, active_weapon, pCmd->csgoUserCmd.inputHistoryField.pRep->tElements[pCmd->csgoUserCmd.inputHistoryField.nCurrentSize - 1]->nPlayerTickCount))
			return;
	}
	catch (...)
	{
		return;
	}*/


	if (!active_weapon_base_gun->CanPrimaryAttack(m_data.m_wpn_data->GetWeaponType(), TICKS_TO_TIME(pLocalController->GetTickBase())))
		return;



	const float best_time = bestRecord->m_simulation_time;
	const float best_tick_time = best_time / INTERVAL_PER_TICK;
	int best_tick = (int)std::floor(best_tick_time);
	const float best_tick_fraction = best_tick_time - best_tick;

	/*try
	{
		static auto debugOverlay = I::Client->GetSceneDebugOverlay();
		if (debugOverlay && C_GET(bool, Vars.bDebugShootHitbox))
			debugOverlay->add_box(bestPoint, Vector_t(-2, -2, -2), Vector_t(2, 2, 2), Vector_t(), Color_t(0, 255, 100, 255), 0.15f);
	}
	catch (...)
	{ }*/



	F::RAGEBOT::AutoStop(pCmd, pUserCmd, pLocalPawn, pLocalController);
	

	try
	{
		bool can_scope = active_weapon_base_gun->GetZoomLevel() == 0 && pWeaponVData->GetWeaponType() == WEAPONTYPE_SNIPER_RIFLE;
		if (can_scope && C_GET(bool, Vars.bAutoScope))
			pCmd->nButtons.nValue |= IN_SECOND_ATTACK;
	}
	catch (...)
	{ }

	


	bestRecord->apply(entity->m_pawn);

	QAngle_t spreadAngle = GetAimPunch(pLocalPawn);
	if (!NewHitchance(entity->m_pawn, active_weapon, bestPoint))
		return;


	if (C_GET(bool, Vars.bAntiaim))
		pUserCmd->pViewAngles->angValue = F::ANTIAIM::angStoredViewBackup;


	QAngle_t pViewAngles = pUserCmd->pViewAngles->angValue;
	QAngle_t vNewAngles = GetAngularDifferenceRageCurAngle(pUserCmd, bestPoint, pLocalPawn, pViewAngles) - spreadAngle;
	QAngle_t best_point_final = NormalizeAnglesRage(pViewAngles + vNewAngles);


	F::ANTIAIM::bestTargetSimTime = bestRecord->m_simulation_time;

	//pUserCmd->pViewAngles->angValue = best_point_final;
	//pCmd->SetSubTickAngle(best_point_final);
	ProcessShoot(pCmd, pUserCmd);


	for (int i = 0; i < pCmd->csgoUserCmd.inputHistoryField.nCurrentSize; i++)
	{
		CCSGOInputHistoryEntryPB* pInputEntry = pCmd->GetInputHistoryEntry(i);
		if (!pInputEntry)
			pInputEntry = pCmd->AddInputHistory();

		if (!pInputEntry)
			continue;

		/*pInputEntry->nRenderTickCount = best_tick + 1;
		pInputEntry->SetBits(EInputHistoryBits::INPUT_HISTORY_BITS_RENDERTICKCOUNT);*/

		if (pInputEntry->cl_interp != nullptr)
		{
			pInputEntry->cl_interp->flFraction = best_tick_fraction;
			pInputEntry->SetBits(EInputHistoryBits::INPUT_HISTORY_BITS_CL_INTERP);
		}

		if (pInputEntry->sv_interp0 != nullptr)
		{
			//pInputEntry->sv_interp0 = pInputEntry->create_new_interp();
			pInputEntry->sv_interp0->nSrcTick = best_tick;
			pInputEntry->sv_interp0->nDstTick = best_tick + 1;
			pInputEntry->sv_interp0->flFraction = 0.f;
			pInputEntry->SetBits(EInputHistoryBits::INPUT_HISTORY_BITS_SV_INTERP0);
		}

		if (pInputEntry->sv_interp1 != nullptr)
		{
			//pInputEntry->sv_interp1 = pInputEntry->create_new_interp();
			pInputEntry->sv_interp1->nSrcTick = best_tick + 1;
			pInputEntry->sv_interp1->nDstTick = best_tick + 2;
			pInputEntry->sv_interp1->flFraction = 0.f;
			pInputEntry->SetBits(EInputHistoryBits::INPUT_HISTORY_BITS_SV_INTERP1);
		}


		if (pInputEntry->pViewAngles != nullptr)
			pInputEntry->pViewAngles->angValue = best_point_final;


		pInputEntry->nPlayerTickCount = pLocalController->GetTickBase();
		pInputEntry->SetBits(EInputHistoryBits::INPUT_HISTORY_BITS_PLAYERTICKCOUNT);

		pInputEntry->nTargetEntIndex = entity->m_pawn->GetRefEHandle().GetEntryIndex();
		pInputEntry->SetBits(EInputHistoryBits::INPUT_HISTORY_BITS_TARGETENTINDEX);
	}


	bestRecord->reset(entity->m_pawn);

	if (C_GET(bool, Vars.bAutoPeek) && IPT::GetBindState(C_GET(KeyBind_t, Vars.nAutoPeekKey)))
		quickpeek_retract_shoot = true;
}

