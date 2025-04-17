#include "aim.h"

// used: sdk entity
#include "../../sdk/entity.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"
// used: cusercmd
#include "../../sdk/datatypes/usercmd.h"

// used: activation button
#include "../../utilities/inputsystem.h"

// used: cheat variables
#include "../../core/variables.h"

#include "../../sdk/interfaces/cgametracemanager.h"
#include "../antiaim/antiaim.hpp"

#include "../../core/sdk.h"






void F::LEGITBOT::AIM::OnMove(CUserCmd* pCmd, CBaseUserCmdPB* pBaseCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn)
{
	// Check if the legitbot is enabled
	if (!C_GET(bool, Vars.bLegitbot))
		return;



	AimAssist(pBaseCmd, pLocalPawn, pLocalController);
}

auto GetAimPunchLegit(C_CSPlayerPawn* pLocalPawn)
{
	using GetAimPunch_t = float(__fastcall*)(void*, Vector_t*, float, bool);
	static GetAimPunch_t GetAimPunch = reinterpret_cast<GetAimPunch_t>(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 48 8D 4E ? 48 8D 54 24 ? E8 ? ? ? ? F2 0F 10 44 24"), 0x1));

	Vector_t AimPunchAngle;
	GetAimPunch(pLocalPawn, &AimPunchAngle, 0.0f, true);

	return QAngle_t(AimPunchAngle.x, AimPunchAngle.y, AimPunchAngle.z);
}


QAngle_t GetRecoil(CBaseUserCmdPB* pCmd,C_CSPlayerPawn* pLocal)
{
	static QAngle_t OldPunch;//get last tick AimPunch angles
	if (pLocal->GetShotsFired() >= 1)//only update aimpunch while shooting
	{
		QAngle_t viewAngles = pCmd->pViewAngles->angValue;
		QAngle_t delta = viewAngles - (viewAngles + (OldPunch - (pLocal->GetAimPuchAngle() * 2.f)));//get current AimPunch angles delta

		return pLocal->GetAimPuchAngle() * 2.0f;//return correct aimpunch delta
	}
	else
	{
		return QAngle_t{ 0, 0 ,0};//return 0 if is not shooting
	}
}

QAngle_t GetAngularDifference(CBaseUserCmdPB* pCmd, Vector_t vecTarget, C_CSPlayerPawn* pLocal)
{
	if (pCmd == nullptr)
		return { 0, 0, 0 };

	if (pLocal == nullptr)
		return { 0, 0, 0 };

	// The current position
	Vector_t vecCurrent;
	vecCurrent = pLocal->GetEyePosition();

	// The new angle
	QAngle_t vNewAngle = (vecTarget - vecCurrent).ToAngles().Normalize();

	QAngle_t vCurAngle;
	if (pCmd->pViewAngles)
		vCurAngle = pCmd->pViewAngles->angValue;

	// Find the difference between the two angles (later useful when adding smoothing)
	vNewAngle -= vCurAngle;

	return vNewAngle;
}

float GetAngularDistance(CBaseUserCmdPB* pCmd, Vector_t vecTarget, C_CSPlayerPawn* pLocal)
{
	return GetAngularDifference(pCmd, vecTarget, pLocal).Length2D();
}


void F::LEGITBOT::AIM::AimAssist(CBaseUserCmdPB* pUserCmd, C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pLocalController)
{
	// Check if the activation key is down
	if (!IPT::IsKeyDown(C_GET(unsigned int, Vars.nLegitbotActivationKey)) && !C_GET(bool, Vars.bLegitbotAlwaysOn))
		return;

	// The current best distance
	float flDistance = INFINITY;
	// The target we have chosen
	CCSPlayerController* pTarget = nullptr;
	// Cache'd position
	Vector_t vecBestPosition = Vector_t();


	const auto& entity_list = F::ANTIAIM::get("CCSPlayerController");

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

		// Make sure they're a player controller
		if (uHashedName != FNV1A::HashConst("CCSPlayerController"))
			continue;

		// Cast to player controller
		CCSPlayerController* pPlayer = reinterpret_cast<CCSPlayerController*>(pEntity);
		if (pPlayer == nullptr)
			continue;

		// Check the entity is not us
		if (pPlayer == pLocalController)
			continue;

		// Make sure they're alive
		if (!pPlayer->IsPawnAlive())
			continue;

		// Get the player pawn
		C_CSPlayerPawn* pPawn = I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(pPlayer->GetPawnHandle());
		if (pPawn == nullptr)
			continue;

		
		// Check if they're an enemy
		if (!pLocalPawn->IsOtherEnemy(pPawn))
			continue;

		// Check if they're dormant
		CGameSceneNode* pCGameSceneNode = pPawn->GetGameSceneNode();
		if (pCGameSceneNode == nullptr || pCGameSceneNode->IsDormant())
			continue;

		// Firstly, get the skeleton
		CSkeletonInstance* pSkeleton = pCGameSceneNode->GetSkeletonInstance();
		if (pSkeleton == nullptr)
			continue;

		// Now the bones
		Matrix2x4_t* pBoneCache = pSkeleton->pBoneCache;
		if (pBoneCache == nullptr)
			continue;

		int iBone = 6; // You may wish to change this dynamically but for now let's target the head.

		// Get the bone's position
		Vector_t vecPos = pBoneCache->GetOrigin(iBone);

		// @note: this is a simple example of how to check if the player is visible

		Ray_t ray = Ray_t();
		GameTrace_t trace = GameTrace_t();
		TraceFilter_t filter(0x1C3003, pLocalPawn, NULL, 4);

		//I::GameTraceManager->TraceShape(&ray, pLocalPawn->GetEyePosition(), vecPos, &filter, &trace);
		//I::GameTraceManager->ClipRayToEntity(&ray, pLocalPawn->GetEyePosition(), vecPos, pPawn, &filter, &trace);
		//if (trace.m_pHitEntity != pPawn) //&& trace.m_pHitboxData->m_nHitboxId == m_best_target->best_hitbox)
		//	continue;

		// Get the distance/weight of the move
		float flCurrentDistance = GetAngularDistance(pUserCmd, vecPos, pLocalPawn);
		if (flCurrentDistance > C_GET(int, Vars.iAimRange)) // Skip if this move out of aim range
			continue;
		if (pTarget && flCurrentDistance > flDistance) // Override if this is the first move or if it is a better move
			continue;

		// Better move found, override.
		pTarget = pPlayer;
		flDistance = flCurrentDistance;
		vecBestPosition = vecPos;
	}


	// Check if a target was found
	if (pTarget == nullptr)
		return;


	// Find the change in angles
	QAngle_t vNewAngles = GetAngularDifference(pUserCmd, vecBestPosition, pLocalPawn);

	// Get the smoothing
	float flSmoothing = C_GET(float, Vars.flSmoothing);
	auto aimPunch = GetAimPunchLegit(pLocalPawn); //get AimPunch angles
	// Apply smoothing and set angles
	pUserCmd->pViewAngles->angValue.x += (vNewAngles.x - aimPunch.x) / flSmoothing; // minus AimPunch angle to counteract recoil
	pUserCmd->pViewAngles->angValue.y += (vNewAngles.y - aimPunch.y) / flSmoothing;
	pUserCmd->pViewAngles->angValue.Normalize();
}

