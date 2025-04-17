#include "antiaim.hpp"


// used: sdk entity
#include "../../sdk/entity.h"
// used: cusercmd
#include "../../sdk/datatypes/usercmd.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"


// used: convars
#include "../../core/convars.h"
#include "../../sdk/interfaces/ienginecvar.h"

// used: cheat variables
#include "../../core/variables.h"
#include "../../core/sdk.h"
#include "../../core/hooks.h"


#include <algorithm>
#include "../../utilities/inputsystem.h"
#include "../../sdk/interfaces/cgametracemanager.h"


struct data_t
{
	float m_dmg;
	bool isValid;
	C_CSPlayerPawn* m_local;
	C_CSPlayerPawn* m_target;
	CCSWeaponBaseVData* m_wpn_data;
};


QAngle_t NormalizeAnglesAA(QAngle_t angles)
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


QAngle_t GetAngularDifferenceAA(CBaseUserCmdPB* pCmd, Vector_t vecTarget, C_CSPlayerPawn* pLocal)
{
	// The current position
	Vector_t vecCurrent;
	vecCurrent = pLocal->GetEyePosition();

	// The new angle
	QAngle_t vNewAngle = (vecTarget - vecCurrent).ToAngles();
	vNewAngle.Normalize(); // Normalise it so we don't jitter about

	// Store our current angles
	//QAngle_t vCurAngle = pCmd->pViewAngles->angValue;

	QAngle_t vCurAngle;
	if (pCmd->pViewAngles)
		vCurAngle = pCmd->pViewAngles->angValue;

	vCurAngle.Normalize();

	/*if (C_GET(bool, Vars.bAntiaim) && C_GET(bool, Vars.bAntiaimYaw))
		vCurAngle.y -= 180.f;*/

	// Find the difference between the two angles (later useful when adding smoothing)
	vNewAngle -= vCurAngle;

	return vNewAngle;
}


float GetAngularDistanceAA(CBaseUserCmdPB* pCmd, Vector_t vecTarget, C_CSPlayerPawn* pLocal)
{
	return GetAngularDifferenceAA(pCmd, vecTarget, pLocal).Length2D();
}


void AngleQanglesAA(const QAngle_t& angles, QAngle_t* forward, QAngle_t* right, QAngle_t* up)
{
	float angle;
	float sr, sp, sy, cr, cp, cy;

	// Convert angles from degrees to radians
	angle = angles.y * (MATH::_PI / 180.0);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles.x * (MATH::_PI / 180.0);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles.z * (MATH::_PI / 180.0);
	sr = sin(angle);
	cr = cos(angle);

	if (forward)
	{
		forward->x = cp * cy;
		forward->y = cp * sy;
		forward->z = -sp;
	}

	if (right)
	{
		right->x = (-1 * sr * sp * cy + -1 * cr * -sy);
		right->y = (-1 * sr * sp * sy + -1 * cr * cy);
		right->z = -1 * sr * cp;
	}

	if (up)
	{
		up->x = (cr * sp * cy + -sr * -sy);
		up->y = (cr * sp * sy + -sr * cy);
		up->z = cr * cp;
	}
}



void MovementCorrectionAA(CBaseUserCmdPB* pUserCmd, const QAngle_t& angDesiredViewPoint)
{
	if (pUserCmd == nullptr)
		return;

	if (!(pUserCmd->pViewAngles))
		return;

	QAngle_t wish_angle;
	wish_angle = pUserCmd->pViewAngles->angValue;
	//wish_angle = angDesiredViewPoint;
	int revers = wish_angle.x > 89.f ? -1 : 1;
	wish_angle.Clamp();

	QAngle_t view_fwd, view_right, view_up, cmd_fwd, cmd_right, cmd_up;
	auto viewangles = angDesiredViewPoint;
	//auto viewangles = pUserCmd->pViewAngles->angValue;

	AngleQanglesAA(wish_angle, &view_fwd, &view_right, &view_up);
	AngleQanglesAA(viewangles, &cmd_fwd, &cmd_right, &cmd_up);

	const float v8 = sqrtf((view_fwd.x * view_fwd.x) + (view_fwd.y * view_fwd.y));
	const float v10 = sqrtf((view_right.x * view_right.x) + (view_right.y * view_right.y));
	const float v12 = sqrtf(view_up.z * view_up.z);

	const Vector_t norm_view_fwd((1.f / v8) * view_fwd.x, (1.f / v8) * view_fwd.y, 0.f);
	const Vector_t norm_view_right((1.f / v10) * view_right.x, (1.f / v10) * view_right.y, 0.f);
	const Vector_t norm_view_up(0.f, 0.f, (1.f / v12) * view_up.z);

	const float v14 = sqrtf((cmd_fwd.x * cmd_fwd.x) + (cmd_fwd.y * cmd_fwd.y));
	const float v16 = sqrtf((cmd_right.x * cmd_right.x) + (cmd_right.y * cmd_right.y));
	const float v18 = sqrtf(cmd_up.z * cmd_up.z);

	const Vector_t norm_cmd_fwd((1.f / v14) * cmd_fwd.x, (1.f / v14) * cmd_fwd.y, 0.f);
	const Vector_t norm_cmd_right((1.f / v16) * cmd_right.x, (1.f / v16) * cmd_right.y, 0.f);
	const Vector_t norm_cmd_up(0.f, 0.f, (1.f / v18) * cmd_up.z);

	const float v22 = norm_view_fwd.x * pUserCmd->flForwardMove;
	const float v26 = norm_view_fwd.y * pUserCmd->flForwardMove;
	const float v28 = norm_view_fwd.z * pUserCmd->flForwardMove;
	const float v24 = norm_view_right.x * pUserCmd->flSideMove;
	const float v23 = norm_view_right.y * pUserCmd->flSideMove;
	const float v25 = norm_view_right.z * pUserCmd->flSideMove;
	const float v30 = norm_view_up.x * pUserCmd->flUpMove;
	const float v27 = norm_view_up.z * pUserCmd->flUpMove;
	const float v29 = norm_view_up.y * pUserCmd->flUpMove;

	pUserCmd->flForwardMove = ((((norm_cmd_fwd.x * v24) + (norm_cmd_fwd.y * v23)) + (norm_cmd_fwd.z * v25)) + (((norm_cmd_fwd.x * v22) + (norm_cmd_fwd.y * v26)) + (norm_cmd_fwd.z * v28))) + (((norm_cmd_fwd.y * v30) + (norm_cmd_fwd.x * v29)) + (norm_cmd_fwd.z * v27));
	pUserCmd->flSideMove = ((((norm_cmd_right.x * v24) + (norm_cmd_right.y * v23)) + (norm_cmd_right.z * v25)) + (((norm_cmd_right.x * v22) + (norm_cmd_right.y * v26)) + (norm_cmd_right.z * v28))) + (((norm_cmd_right.x * v29) + (norm_cmd_right.y * v30)) + (norm_cmd_right.z * v27));
	pUserCmd->flUpMove = ((((norm_cmd_up.x * v23) + (norm_cmd_up.y * v24)) + (norm_cmd_up.z * v25)) + (((norm_cmd_up.x * v26) + (norm_cmd_up.y * v22)) + (norm_cmd_up.z * v28))) + (((norm_cmd_up.x * v30) + (norm_cmd_up.y * v29)) + (norm_cmd_up.z * v27));
	pUserCmd->flForwardMove = revers * ((((norm_cmd_fwd.x * v24) + (norm_cmd_fwd.y * v23)) + (norm_cmd_fwd.z * v25)) + (((norm_cmd_fwd.x * v22) + (norm_cmd_fwd.y * v26)) + (norm_cmd_fwd.z * v28)));
	pUserCmd->flSideMove = ((((norm_cmd_right.x * v24) + (norm_cmd_right.y * v23)) + (norm_cmd_right.z * v25)) + (((norm_cmd_right.x * v22) + (norm_cmd_right.y * v26)) + (norm_cmd_right.z * v28)));
	pUserCmd->flForwardMove = std::clamp(pUserCmd->flForwardMove, -1.f, 1.f);
	pUserCmd->flSideMove = std::clamp(pUserCmd->flSideMove, -1.f, 1.f);
}



inline void AngleVectors(const QAngle_t& angles, Vector_t* forward, Vector_t* right, Vector_t* up)
{
	float cp = std::cos(M_DEG2RAD(angles.x)), sp = std::sin(M_DEG2RAD(angles.x));
	float cy = std::cos(M_DEG2RAD(angles.y)), sy = std::sin(M_DEG2RAD(angles.y));
	float cr = std::cos(M_DEG2RAD(angles.z)), sr = std::sin(M_DEG2RAD(angles.z));

	if (forward)
	{
		forward->x = cp * cy;
		forward->y = cp * sy;
		forward->z = -sp;
	}

	if (right)
	{
		right->x = -1.f * sr * sp * cy + -1.f * cr * -sy;
		right->y = -1.f * sr * sp * sy + -1.f * cr * cy;
		right->z = -1.f * sr * cp;
	}

	if (up)
	{
		up->x = cr * sp * cy + -sr * -sy;
		up->y = cr * sp * sy + -sr * cy;
		up->z = cr * cp;
	}
}



void F::ANTIAIM::level_init()
{
	entities.clear();
}

void F::ANTIAIM::level_shutdown()
{
	entities.clear();
}

std::vector<CEntityInstance*> F::ANTIAIM::get(const char* name)
{
	return entities[FNV1A::Hash(name)];
}

void F::ANTIAIM::add_entity(CEntityInstance* pEntity, int index)
{

	// Get the class info
	SchemaClassInfoData_t* pClassInfo = nullptr;
	pEntity->GetSchemaClassInfo(&pClassInfo);
	if (pClassInfo == nullptr)
		return;

	// Get the hashed name
	const FNV1A_t className = FNV1A::Hash(pClassInfo->szName);
	// Make sure they're a player controller
	if (className != FNV1A::HashConst("CCSPlayerController") && className != FNV1A::HashConst("C_EnvSky"))
		return;

	entities[className].emplace_back(pEntity);
}

void F::ANTIAIM::remove_entity(CEntityInstance* pEntity, int index)
{
	// Get the class info
	SchemaClassInfoData_t* pClassInfo = nullptr;
	pEntity->GetSchemaClassInfo(&pClassInfo);
	if (pClassInfo == nullptr)
		return;

	// Get the hashed name
	const FNV1A_t className = FNV1A::Hash(pClassInfo->szName);
	// Make sure they're a player controller
	if (className != FNV1A::HashConst("CCSPlayerController") && className != FNV1A::HashConst("C_EnvSky"))
		return;

	try
	{
		auto it = entities.find(className);
		if (it == entities.end())
			return;

		std::vector<CEntityInstance*>& entities = it->second;

		for (auto iter = entities.begin(); iter != entities.end(); ++iter)
		{
			if ((*iter) == pEntity)
			{
				entities.erase(iter);

				break;
			}
		}
	}
	catch (...)
	{ }
}


void F::ANTIAIM::RunAA(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, C_CSPlayerPawn* pLocalPawn)
{
	if (!C_GET(bool, Vars.bAntiaim))
		return;

	if (pCmd == nullptr)
		return;

	if (pUserCmd == nullptr)
		return;

	if (pUserCmd->pViewAngles == nullptr)
		return;

	if (!pLocalPawn->IsValidMoveType())
		return;

	if (pCmd->nButtons.nValue & IN_USE || pCmd->nButtons.nValue & IN_ATTACK)
		return;


	auto active_weapon = H::cheat->m_weapon;
	if (active_weapon == nullptr)
		return;

	auto pWeaponVData = H::cheat->m_wpn_data;
	if (pWeaponVData == nullptr)
		return;

	if (pWeaponVData->GetWeaponType() == WEAPONTYPE_GRENADE)
	{
		auto Grenade = I::GameResourceService->pGameEntitySystem->Get<C_BaseCSGrenade>(active_weapon->GetRefEHandle());
		if (Grenade)
		{
			if (Grenade->GetThrowTime() > 0)
				return;
		}
	}


	float flPitch = 0;
	float flYaw = pCmd->csgoUserCmd.pBaseCmd->pViewAngles->angValue.y;



	if (C_GET(int, Vars.iAntiaimPitch) == AA_UP)
		flPitch = -89.f;

	if (C_GET(int, Vars.iAntiaimPitch) == AA_DOWN)
		flPitch = 89.f;

	if (C_GET(int, Vars.iAntiaimPitch) == AA_CENTER)
		flPitch = 0;

	if (C_GET(bool, Vars.bAntiaimYaw))
		flYaw -= 180.0f;


	// Manuals
	static bool Left, Right, Back;

	if (IPT::GetBindState(C_GET(KeyBind_t, Vars.nLeft)))
	{
		Left = true;
		Right = false;
		Back = false;
		C_GET(KeyBind_t, Vars.nRight).bEnable = false;
		C_GET(KeyBind_t, Vars.nBackward).bEnable = false;
	}
	else
		Left = false;

	if (IPT::GetBindState(C_GET(KeyBind_t, Vars.nRight)))
	{
		Left = false;
		Right = true;
		Back = false;
		C_GET(KeyBind_t, Vars.nLeft).bEnable = false;
		C_GET(KeyBind_t, Vars.nBackward).bEnable = false;
	}
	else
		Right = false;

	if (IPT::GetBindState(C_GET(KeyBind_t, Vars.nBackward)))
	{
		Left = false;
		Right = false;
		Back = true;
		C_GET(KeyBind_t, Vars.nRight).bEnable = false;
		C_GET(KeyBind_t, Vars.nLeft).bEnable = false;

		flYaw = 189.0f;
	}


	if (Left)
		flYaw -= 90.f;

	if (Right)
		flYaw += 90.f;


	static float spinbot_angle = 0;


	static int tick_3_way = 0;
	static int jitter_side = -1;


	int YawSpeed = 45 + 1;
	int YawBase = YawSpeed / 2;
	int YawTickCount = I::GlobalVars->nTickCount % YawSpeed;


	if (C_GET(int, Vars.iJitterType) != AntiAimType::JITTER_NONE)
	{
		if (C_GET(int, Vars.iJitterType) == AntiAimType::JITTER_CENTER)
		{
			//flYaw += 179;
			flYaw += C_GET(int, Vars.iJitterAngle) * jitter_side;
		}

		if (C_GET(int, Vars.iJitterType) == AntiAimType::JITTER_SPINBOT)
		{
			spinbot_angle += C_GET(int, Vars.iJitterAngle);
			flYaw = spinbot_angle;
		}

		if (C_GET(int, Vars.iJitterType) == AntiAimType::JITTER_3_WAY)
		{
			switch (tick_3_way)
			{
			case 0:
				flYaw -= C_GET(int, Vars.iJitterAngle);
				break;
			case 1:
				flYaw += C_GET(int, Vars.iJitterAngle);
				break;
			case 2:
				//float randSwitch = I::RandomFloat(-1.f, 1.f) < 0 ? 1 : -1;
				flYaw += C_GET(int, Vars.iJitterAngle) * jitter_side;
				break;
			}
			++tick_3_way;
			tick_3_way %= 3;
		}
	}

	jitter_side = -jitter_side;

	try
	{
		if (C_GET(bool, Vars.bFreestading))
		{
			float Right, Left;
			Vector_t src3D, dst3D, forward, right, up;
			GameTrace_t tr_right, tr_left;
			Ray_t ray_right, ray_left;
			TraceFilter_t filter;

			QAngle_t engineViewAngles = pCmd->csgoUserCmd.pBaseCmd->pViewAngles->angValue;
			engineViewAngles.x = 0.0f;

			AngleVectors(engineViewAngles, &forward, &right, &up);

			src3D = pLocalPawn->GetEyePosition();
			dst3D = src3D + forward * 100.0f;

			I::GameTraceManager->Init(filter, pLocalPawn, MASK_SOLID & ~CONTENTS_MONSTER, 3, 7);
			I::GameTraceManager->TraceShape(&ray_right, src3D + right * 35.0f, dst3D + right * 35.0f, &filter, &tr_right);

			Right = (tr_right.m_vecEndPos - tr_right.m_vecStartPos).Length();

			I::GameTraceManager->Init(filter, pLocalPawn, MASK_SOLID & ~CONTENTS_MONSTER, 3, 7);
			I::GameTraceManager->TraceShape(&ray_left, src3D - right * 35.0f, dst3D - right * 35.0f, &filter, &tr_left);

			Left = (tr_left.m_vecEndPos - tr_left.m_vecStartPos).Length();

			static auto left_ticks = 0;
			static auto right_ticks = 0;
			static auto back_ticks = 0;

			if (Right - Left > 20.0f)
				left_ticks++;
			else
				left_ticks = 0;

			if (Left - Right > 20.0f)
				right_ticks++;
			else
				right_ticks = 0;

			if (fabs(Right - Left) <= 20.0f)
				back_ticks++;
			else
				back_ticks = 0;

			if (right_ticks > 10)
			{
				if (C_GET(bool, Vars.bAntiaimYaw) || Back)
					flYaw += 90.f;
				else
					flYaw -= 90.f;
			}
			else if (left_ticks > 10)
			{
				if (C_GET(bool, Vars.bAntiaimYaw) || Back)
					flYaw -= 90.f;
				else
					flYaw += 90.f;
			}
			//else if (back_ticks > 10)
			//  final_manual_side = SIDE_BACK;
		}
	}
	catch (...)
	{ }

	pUserCmd->pViewAngles->angValue = NormalizeAnglesAA(QAngle_t{ flPitch, flYaw, 0 });
	pUserCmd->SetBits(EBaseCmdBits::BASE_BITS_VIEWANGLES);
}
