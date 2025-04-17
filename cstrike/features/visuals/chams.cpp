#include "chams.h"

// used: game's interfaces
#include "../../core/interfaces.h"
#include "../../sdk/interfaces/imaterialsystem.h"
#include "../../sdk/interfaces/igameresourceservice.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iresourcesystem.h"
#include "../../core/sdk.h"
#include "../../sdk/entity.h"
	
// used: original call in hooked function
#include "../../core/hooks.h"

// used: cheat variables
#include "../../core/variables.h"

CStrongHandle<CMaterial2> F::VISUALS::CHAMS::CreateMaterial(const char* szMaterialName, const char szVmatBuffer[])
{
	CKeyValues3* pKeyValues3 = CKeyValues3::CreateMaterialResource();
	pKeyValues3->LoadFromBuffer(szVmatBuffer);

	CStrongHandle<CMaterial2> pCustomMaterial = {};
	MEM::fnCreateMaterial(nullptr, &pCustomMaterial, szMaterialName, pKeyValues3, 0, 1);

	return pCustomMaterial;
}

struct CustomMaterial_t
{
	CStrongHandle<CMaterial2> pMaterial;
	CStrongHandle<CMaterial2> pMaterialInvisible;
};

static CustomMaterial_t arrMaterials[VISUAL_MATERIAL_MAX];

bool F::VISUALS::CHAMS::Initialize()
{
	// check if we already initialized materials
	if (bInitialized)
		return bInitialized;


	arrMaterials[VISUAL_MATERIAL_PRIMARY_WHITE] = CustomMaterial_t{
		.pMaterial = CreateMaterial(CS_XOR("materials/dev/primary_white.vmat"), szVMatBufferWhiteVisible),
		.pMaterialInvisible = CreateMaterial(CS_XOR("materials/dev/primary_white.vmat"), szVMatBufferWhiteInvisible)
	};

	arrMaterials[VISUAL_MATERIAL_ILLUMINATE] = CustomMaterial_t{
		.pMaterial = CreateMaterial(CS_XOR("materials/dev/primary_white.vmat"), szVMatBufferIlluminateVisible),
		.pMaterialInvisible = CreateMaterial(CS_XOR("materials/dev/primary_white.vmat"), szVMatBufferIlluminateInvisible)
	};

	arrMaterials[VISUAL_MATERIAL_LATEX] = CustomMaterial_t{
		.pMaterial = CreateMaterial(CS_XOR("materials/dev/primary_white.vmat"), szVMatBufferLatexVisible),
		.pMaterialInvisible = CreateMaterial(CS_XOR("materials/dev/primary_white.vmat"), szVMatBufferLatexInvisible)
	};

	arrMaterials[VISUAL_MATERIAL_GLOW] = CustomMaterial_t{
		.pMaterial = CreateMaterial(CS_XOR("materials/dev/primary_white.vmat"), szVMatBufferGlowVisible),
		.pMaterialInvisible = CreateMaterial(CS_XOR("materials/dev/primary_white.vmat"), szVMatBufferGlowInisible)
	};

	arrMaterials[VISUAL_MATERIAL_GLASS] = CustomMaterial_t{
		.pMaterial = CreateMaterial(CS_XOR("materials/dev/primary_white.vmat"), szVMatBufferGlassVisible),
		.pMaterialInvisible = CreateMaterial(CS_XOR("materials/dev/primary_white.vmat"), szVMatBufferGlassInvisible)
	};

	bInitialized = true;
	for (auto& [pMaterial, pMaterialInvisible] : arrMaterials)
	{
		if (pMaterial == nullptr || pMaterialInvisible == nullptr)
			bInitialized = false;
	}

	return bInitialized;
}

void F::VISUALS::CHAMS::Destroy()
{
	for (auto& [pVisible, pInvisible] : arrMaterials)
	{
		if (pVisible)
			I::ResourceHandleUtils->DeleteResource(pVisible.pBinding);

		if (pInvisible)
			I::ResourceHandleUtils->DeleteResource(pInvisible.pBinding);
	}
}

bool F::VISUALS::CHAMS::OnDrawObject(void* pAnimatableSceneObjectDesc, void* pDx11, CMeshData* arrMeshDraw, int nDataCount, void* pSceneView, void* pSceneLayer, void* pUnk, void* pUnk2)
{
	if (!bInitialized)
		return false;


	if (!arrMeshDraw)
		return false;

	if (!SDK::LocalPawn)
		return false;

	if (!arrMeshDraw->pSceneAnimatableObject)
		return false;

	auto hOwner = arrMeshDraw->pSceneAnimatableObject->hOwner;
	if (!hOwner.IsValid())
		return false;

	auto pEntity = I::GameResourceService->pGameEntitySystem->Get<C_BaseEntity>(hOwner);
	if (!pEntity)
		return false;

	SchemaClassInfoData_t* pClassInfo;
	pEntity->GetSchemaClassInfo(&pClassInfo);
	if (!pClassInfo)
		return false;

	if (!pClassInfo->szName)
		return false;

	if (CRT::StringCompare(pClassInfo->szName, CS_XOR("C_C4")) == 0 && C_GET(bool, Vars.bC4Chams))
	{
		return OverrideC4Material(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2);
	}

	else if (CRT::StringCompare(pClassInfo->szName, CS_XOR("C_CSGOViewModel")) == 0)
	{
		if (C_GET(bool, Vars.bWeaponChams))
			return OverrideWeaponMaterial(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2);

		return false;
	}

	/*else if (pEntity->IsWeapon() && C_GET(bool, Vars.bWeaponChams))
	{
		auto pOwnerEntity = I::GameResourceService->pGameEntitySystem->Get<C_BaseEntity>(pEntity->GetOwnerHandle());

		if (pOwnerEntity && pOwnerEntity != SDK::LocalPawn)
			return OverrideWeaponMaterial(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2);

		return false;
	}*/

	if (CRT::StringCompare(pClassInfo->szName, CS_XOR("C_CSPlayerPawn")) != 0)
		return false;

	auto pPlayerPawn = I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(hOwner);
	if (pPlayerPawn == nullptr)
		return false;

	if (pPlayerPawn == SDK::LocalPawn && C_GET(bool, Vars.bRemoveLegs) && !C_GET(bool, Vars.bThirdPerson))
		return true;

	if (pPlayerPawn == SDK::LocalPawn)
		return false;

	if (!C_GET(bool, Vars.bPlayerChams))
		return false;

	if (pPlayerPawn->GetHealth() <= 0)
	{
		if (C_GET(bool, Vars.bRagdollChams))
			return OverrideRagdoll(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2, pPlayerPawn);
		else
			return false;
	}

	if (pPlayerPawn->IsOtherEnemy(SDK::LocalPawn))
		return OverridePlayerMaterial(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2, pPlayerPawn);
	else
	{
		if (C_GET(bool, Vars.bTeamChams))
			return OverridePlayerMaterial(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2, pPlayerPawn);
	}

	return false;
}

bool F::VISUALS::CHAMS::OverridePlayerMaterial(void* pAnimatableSceneObjectDesc, void* pDx11, CMeshData* arrMeshDraw, int nDataCount, void* pSceneView, void* pSceneLayer, void* pUnk, void* pUnk2, C_CSPlayerPawn* pPlayerPawn)
{
	const auto oDrawObject = H::hkDrawObject.GetOriginal();
	const CustomMaterial_t customVisibleMaterial = arrMaterials[C_GET(int, Vars.nVisualChamsVisibleMaterial)];
	const CustomMaterial_t custominvisibleMaterial = arrMaterials[C_GET(int, Vars.nVisualChamsInvisibleMaterial)];

	if (pPlayerPawn->IsOtherEnemy(SDK::LocalPawn))
	{
		if (C_GET(bool, Vars.bVisualChamsIgnoreZ))
		{
			arrMeshDraw->pMaterial = custominvisibleMaterial.pMaterialInvisible;
			arrMeshDraw->colValue = C_GET(Color_t, Vars.colVisualChamsEnemyIgnoreZ);
			oDrawObject(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2);
		}

		arrMeshDraw->pMaterial = customVisibleMaterial.pMaterial;
		arrMeshDraw->colValue = C_GET(Color_t, Vars.colVisualChamsEnemy);
		oDrawObject(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2);
	}

	else
	{
		if (C_GET(bool, Vars.bVisualChamsIgnoreZ))
		{
			arrMeshDraw->pMaterial = custominvisibleMaterial.pMaterialInvisible;
			arrMeshDraw->colValue = C_GET(Color_t, Vars.colVisualChamsTeamIgnoreZ);
			oDrawObject(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2);
		}

		arrMeshDraw->pMaterial = customVisibleMaterial.pMaterial;
		arrMeshDraw->colValue = C_GET(Color_t, Vars.colVisualChamsTeam);
		oDrawObject(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2);
	}

	return true;
}

bool F::VISUALS::CHAMS::OverrideRagdoll(void* pAnimatableSceneObjectDesc, void* pDx11, CMeshData* arrMeshDraw, int nDataCount, void* pSceneView, void* pSceneLayer, void* pUnk, void* pUnk2, C_CSPlayerPawn* pPlayerPawn)
{
	const auto oDrawObject = H::hkDrawObject.GetOriginal();
	const CustomMaterial_t customVisibleMaterial = arrMaterials[C_GET(int, Vars.nVisualChamsVisibleMaterial)];
	const CustomMaterial_t custominvisibleMaterial = arrMaterials[0];

	if (pPlayerPawn->IsOtherEnemy(SDK::LocalPawn))
	{
		if (C_GET(bool, Vars.bVisualChamsIgnoreZ))
		{
			arrMeshDraw->pMaterial = custominvisibleMaterial.pMaterialInvisible;
			arrMeshDraw->colValue = C_GET(Color_t, Vars.colVisualChamsEnemyIgnoreZ);
			oDrawObject(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2);
		}

		arrMeshDraw->pMaterial = customVisibleMaterial.pMaterial;
		arrMeshDraw->colValue = C_GET(Color_t, Vars.colVisualChamsEnemy);
		oDrawObject(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2);
	}

	else
	{
		if (C_GET(bool, Vars.bVisualChamsIgnoreZ))
		{
			arrMeshDraw->pMaterial = custominvisibleMaterial.pMaterialInvisible;
			arrMeshDraw->colValue = C_GET(Color_t, Vars.colVisualChamsTeamIgnoreZ);
			oDrawObject(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2);
		}

		arrMeshDraw->pMaterial = customVisibleMaterial.pMaterial;
		arrMeshDraw->colValue = C_GET(Color_t, Vars.colVisualChamsTeam);
		oDrawObject(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2);
	}

	return true;
}

bool F::VISUALS::CHAMS::OverrideC4Material(void* pAnimatableSceneObjectDesc, void* pDx11, CMeshData* arrMeshDraw, int nDataCount, void* pSceneView, void* pSceneLayer, void* pUnk, void* pUnk2)
{
	const auto oDrawObject = H::hkDrawObject.GetOriginal();
	const CustomMaterial_t customMaterial = arrMaterials[C_GET(int, Vars.nVisualChamsC4Material)];

	arrMeshDraw->pMaterial = customMaterial.pMaterialInvisible;
	arrMeshDraw->colValue = C_GET(Color_t, Vars.colVisualChamsC4);
	oDrawObject(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2);

	arrMeshDraw->pMaterial = customMaterial.pMaterial;
	arrMeshDraw->colValue = C_GET(Color_t, Vars.colVisualChamsC4);
	oDrawObject(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2);

	return true;
}

bool F::VISUALS::CHAMS::OverrideWeaponMaterial(void* pAnimatableSceneObjectDesc, void* pDx11, CMeshData* arrMeshDraw, int nDataCount, void* pSceneView, void* pSceneLayer, void* pUnk, void* pUnk2)
{
	if (arrMeshDraw->pSceneAnimatableObject == nullptr)
		return false;

	// Apply weapon chams here
	const auto oDrawObject = H::hkDrawObject.GetOriginal();
	const CustomMaterial_t customMaterial = arrMaterials[C_GET(int, Vars.nVisualChamsWeaponMaterial)];

	arrMeshDraw->pMaterial = customMaterial.pMaterial;
	arrMeshDraw->colValue = C_GET(Color_t, Vars.colVisualChamsWeapon);
	oDrawObject(pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk, pUnk2);

	return true;
}
