#pragma once

// @test: using interfaces in the header | not critical but could blow up someday with thousands of errors or affect to compilation time etc
// used: cgameentitysystem, ischemasystem
#include "../core/interfaces.h"
#include "interfaces/igameresourceservice.h"
#include "interfaces/ischemasystem.h"
#include "datatypes/stronghandle.h"

// used: schema field
#include "../core/schema.h"

// used: l_print
#include "../utilities/log.h"
// used: vector_t
#include "datatypes/vector.h"
// used: qangle_t
#include "datatypes/qangle.h"
// used: ctransform
#include "datatypes/transform.h"

// used: cbasehandle
#include "entity_handle.h"
// used: game's definitions
#include "const.h"
// used: entity vdata
#include "vdata.h"

using GameTime_t = std::float_t;
using GameTick_t = std::int32_t;


enum bone_flags : uint32_t
{
	FLAG_NO_BONE_FLAGS = 0x0,
	FLAG_BONEFLEXDRIVER = 0x4,
	FLAG_CLOTH = 0x8,
	FLAG_PHYSICS = 0x10,
	FLAG_ATTACHMENT = 0x20,
	FLAG_ANIMATION = 0x40,
	FLAG_MESH = 0x80,
	FLAG_HITBOX = 0x100,
	FLAG_BONE_USED_BY_VERTEX_LOD0 = 0x400,
	FLAG_BONE_USED_BY_VERTEX_LOD1 = 0x800,
	FLAG_BONE_USED_BY_VERTEX_LOD2 = 0x1000,
	FLAG_BONE_USED_BY_VERTEX_LOD3 = 0x2000,
	FLAG_BONE_USED_BY_VERTEX_LOD4 = 0x4000,
	FLAG_BONE_USED_BY_VERTEX_LOD5 = 0x8000,
	FLAG_BONE_USED_BY_VERTEX_LOD6 = 0x10000,
	FLAG_BONE_USED_BY_VERTEX_LOD7 = 0x20000,
	FLAG_BONE_MERGE_READ = 0x40000,
	FLAG_BONE_MERGE_WRITE = 0x80000,
	FLAG_ALL_BONE_FLAGS = 0xfffff,
	BLEND_PREALIGNED = 0x100000,
	FLAG_RIGIDLENGTH = 0x200000,
	FLAG_PROCEDURAL = 0x400000,
};

enum HITBOXES : uint32_t
{
	HEAD = 6,
	NECK = 5,
	CHEST = 4,
	RIGHT_CHEST = 8,
	LEFT_CHEST = 13,
	STOMACH = 3,
	PELVIS = 2,
	CENTER = 1,
	L_LEG = 23,
	L_FEET = 24,
	R_LEG = 26,
	R_FEET = 27
};


enum HitGroup_t : std::uint32_t
{
	HITGROUP_INVALID = -1,
	HITGROUP_GENERIC = 0,
	HITGROUP_HEAD = 1,
	HITGROUP_CHEST = 2,
	HITGROUP_STOMACH = 3,
	HITGROUP_LEFTARM = 4,
	HITGROUP_RIGHTARM = 5,
	HITGROUP_LEFTLEG = 6,
	HITGROUP_RIGHTLEG = 7,
	HITGROUP_NECK = 8,
	HITGROUP_UNUSED = 9,
	HITGROUP_GEAR = 10,
	HITGROUP_SPECIAL = 11,
	HITGROUP_COUNT = 12,
};


enum ObserverMode_t : int
{
	OBS_MODE_NONE = 0U,
	OBS_MODE_FIXED = 1U,
	OBS_MODE_IN_EYE = 2U,
	OBS_MODE_CHASE = 3U,
	OBS_MODE_ROAMING = 4U,
	OBS_MODE_DIRECTED = 5U,
	NUM_OBSERVER_MODES = 6U,
};





class C_PostProcessingVolume
{
public:
	CS_CLASS_NO_INITIALIZER(C_PostProcessingVolume);

	SCHEMA_ADD_FIELD(float, m_flFadeDuration, "C_PostProcessingVolume->m_flFadeDuration");
	SCHEMA_ADD_FIELD(float, m_flMinLogExposure, "C_PostProcessingVolume->m_flMinLogExposure");
	SCHEMA_ADD_FIELD(float, m_flMaxLogExposure, "C_PostProcessingVolume->m_flMaxLogExposure");
	SCHEMA_ADD_FIELD(float, m_flMinExposure, "C_PostProcessingVolume->m_flMinExposure");
	SCHEMA_ADD_FIELD(float, m_flMaxExposure, "C_PostProcessingVolume->m_flMaxExposure");
	SCHEMA_ADD_FIELD(float, m_flExposureCompensation, "C_PostProcessingVolume->m_flExposureCompensation");
	SCHEMA_ADD_FIELD(float, m_flExposureFadeSpeedUp, "C_PostProcessingVolume->m_flExposureFadeSpeedUp");
	SCHEMA_ADD_FIELD(float, m_flExposureFadeSpeedDown, "C_PostProcessingVolume->m_flExposureFadeSpeedDown");
	SCHEMA_ADD_FIELD(float, m_flTonemapEVSmoothingRange, "C_PostProcessingVolume->m_flTonemapEVSmoothingRange");
	SCHEMA_ADD_FIELD(bool, m_bMaster, "C_PostProcessingVolume->m_bMaster");
	SCHEMA_ADD_FIELD(bool, m_bExposureControl, "C_PostProcessingVolume->m_bExposureControl");
	SCHEMA_ADD_FIELD(float, m_flRate, "C_PostProcessingVolume->m_flRate");
	SCHEMA_ADD_FIELD(float, m_flTonemapPercentTarget, "C_PostProcessingVolume->m_flTonemapPercentTarget");
	SCHEMA_ADD_FIELD(float, m_flTonemapPercentBrightPixels, "C_PostProcessingVolume->m_flTonemapPercentBrightPixels");
	SCHEMA_ADD_FIELD(float, m_flTonemapMinAvgLum, "C_PostProcessingVolume->m_flTonemapMinAvgLum");
};



class CEntityInstance;

class CEntityIdentity
{
public:
	CS_CLASS_NO_INITIALIZER(CEntityIdentity);

	// @note: handle index is not entity index
	SCHEMA_ADD_OFFSET(std::uint32_t, GetIndex, 0x10);
	SCHEMA_ADD_FIELD(const char*, GetDesignerName, "CEntityIdentity->m_designerName");
	SCHEMA_ADD_FIELD(std::uint32_t, GetFlags, "CEntityIdentity->m_flags");

	[[nodiscard]] bool IsValid()
	{
		return GetIndex() != INVALID_EHANDLE_INDEX;
	}

	[[nodiscard]] int GetEntryIndex()
	{
		if (!IsValid())
			return ENT_ENTRY_MASK;
		
		return GetIndex() & ENT_ENTRY_MASK;
	}

	[[nodiscard]] int GetSerialNumber()
	{
		return GetIndex() >> NUM_SERIAL_NUM_SHIFT_BITS;
	}

	CEntityInstance* pInstance; // 0x00
};

class CEntityInstance
{
public:
	CS_CLASS_NO_INITIALIZER(CEntityInstance);

	void GetSchemaClassInfo(SchemaClassInfoData_t** pReturn)
	{
		return MEM::CallVFunc<void, 38U>(this, pReturn);
	}

	[[nodiscard]] CBaseHandle GetRefEHandle()
	{
		CEntityIdentity* pIdentity = GetIdentity();
		if (pIdentity == nullptr)
			return CBaseHandle();

		return CBaseHandle(pIdentity->GetEntryIndex(), pIdentity->GetSerialNumber() - (pIdentity->GetFlags() & 1));
	}

	SCHEMA_ADD_FIELD(CEntityIdentity*, GetIdentity, "CEntityInstance->m_pEntity");
};

class CCollisionProperty
{
public:
	std::uint16_t CollisionMask()
	{
		return *reinterpret_cast<std::uint16_t*>(reinterpret_cast<std::uintptr_t>(this) + 0x38);
	}

	CS_CLASS_NO_INITIALIZER(CCollisionProperty);

	SCHEMA_ADD_FIELD(Vector_t, GetMins, "CCollisionProperty->m_vecMins");
	SCHEMA_ADD_FIELD(Vector_t, GetMaxs, "CCollisionProperty->m_vecMaxs");

	SCHEMA_ADD_FIELD(std::uint8_t, GetSolidFlags, "CCollisionProperty->m_usSolidFlags");
	SCHEMA_ADD_FIELD(std::uint8_t, GetCollisionGroup, "CCollisionProperty->m_CollisionGroup");
};



struct BoneData_t
{
	Vector_t Location;
	float Scale;
	Quaternion_t Rotation;
};

class CHitBox
{
public:
	const char* Name()
	{
		return *(const char**)(this + 0x0);
	}

	Vector_t& VectorMin()
	{
		return *(Vector_t*)(this + 0x0020);
	}

	Vector_t& VectorMax()
	{
		return *(Vector_t*)(this + 0x002C);
	}

	float ShapeRadius()
	{
		return *(float*)(this + 0x30);
	}

	uint8_t ShapeType()
	{
		return *(uint8_t*)(this + 0x3C);
	}

	uint16_t HitBoxIndex()
	{
		return *(uint16_t*)(this + 0x48);
	}
};

class c_drawcalls
{
public:
	__int32 m_nPrimitiveType; // 0x0000
	__int32 m_nBaseVertex; // 0x0004
	__int32 m_nVertexCount; // 0x0008
	__int32 m_nStartIndex; // 0x000C
	__int32 m_nIndexCount; // 0x0010
	float m_flUvDensity; // 0x0014
	float m_vTintColor[3]; // 0x0018
	float m_flAlpha; // 0x0024
	char pad_0x0028[0xC0]; // 0x0028

}; // Size=0x00E8

class c_attachments
{
public:
	char pad_0x0000[0x10]; // 0x0000
	const char* m_key; // 0x0010
	char pad_0x0018[0x8]; // 0x0018
	const char* m_name; // 0x0020
	const char* m_influenceNames[3]; // 0x0028
	char pad_0x0030[0x10]; // 0x0030
	Vector4D_t m_vInfluenceRotations[3]; // 0x0040
	Vector_t m_vInfluenceOffsets[3]; // 0x0070
	float m_influenceWeights[3]; // 0x0094
	char pad_0x00A0[0x10]; // 0x00A0
};

class c_bones
{
public:
	const char* m_boneName; // 0x0000
	const char* m_parentName; // 0x0008
	float m_invBindPose[12]; // 0x0010
	Vector_t m_vecCenter; // 0x0040
	Vector_t m_vecSize; // 0x004C
	float m_flSphereradius; // 0x0058
	char pad_0x005C[0x4]; // 0x005C
};

class c_hitboxsets
{
public:
	char pad_0x0000[0x20]; // 0x0000
	uint32_t m_nNameHash; // 0x0020
	char pad_0x0024[0x4]; // 0x0024
	__int32 m_nHitboxCount; // 0x0028
	char pad_0x002C[0x4]; // 0x002C
	CHitBox* m_hitbox; // 0x0030
	char pad_0x0038[0x18]; // 0x0038
};

class c_rendermesh
{
public:
	char pad_0x0000[0x28]; // 0x0000
	Vector_t m_vMinBounds; // 0x0028
	Vector_t m_vMaxBounds; // 0x0034
	__int32 m_drawCallsCount; // 0x0040
	char pad_0x0044[0x4]; // 0x0044
	c_drawcalls* m_drawCalls; // 0x0048
	char pad_0x0050[0x68]; // 0x0050
	__int32 m_skeletoncount; // 0x00B8
	char pad_0x00BC[0x4]; // 0x00BC
	BoneData_t* m_skeleton; // 0x00C0
	char pad_0x00C8[0x8]; // 0x00C8
	__int32 m_hashescount; // 0x00D0
	char pad_0x00D4[0x4]; // 0x00D4
	uint32_t* m_hashes; // 0x00D8
	char pad_0x00E0[0x20]; // 0x00E0
	__int32 m_nBoneWeightCount; // 0x0100
	char pad_0x0104[0xC]; // 0x0104
	c_attachments* m_attachments; // 0x0110
	__int32 m_attachments_count; // 0x0118
	char pad_0x011C[0x1C]; // 0x011C
	c_hitboxsets* m_hitboxsets; // 0x0138
	__int32 m_nHitboxSets; // 0x0140
};

class c_rendermeshes
{
public:
	c_rendermesh* m_meshes;
};

class c_physmodel_bonenames
{
public:
	uint32_t m_boneNames_count;
	char padd[0x4];
	const char* m_boneNames[];
};

class c_physmodel
{
public:
	char padd[0x10];
	c_physmodel_bonenames* m_boneName;
	uint32_t* m_boneHash;
	uint32_t m_indexNames_count;
	char padd2[0x4];
	uint16_t* m_indexNames;
	uint32_t m_indexHash_count;
	char padd3[0x4];
	uint16_t* m_indexHash;
};


class CModel
{
public:
	CHitBox* GetHitbox(int index)
	{
		if (m_rendermesh_count <= 0)
			return nullptr;

		if (!m_rendermesh)
			return nullptr;

		c_rendermesh* meshes = m_rendermesh->m_meshes;
		if (!meshes)
			return nullptr;

		c_rendermesh mesh = meshes[0];

		c_hitboxsets* hithoxsets = mesh.m_hitboxsets;
		if (!hithoxsets)
			return nullptr;

		c_hitboxsets hithoxset = hithoxsets[0];
		if (hithoxset.m_nHitboxCount <= 0 || index > hithoxset.m_nHitboxCount)
			return nullptr;

		CHitBox* hitbox = hithoxset.m_hitbox;
		if (!hitbox)
			return nullptr;

		CHitBox* hbx0 = &hitbox[0];

		return hbx0 + 0x70 * index;
	}

	int HitboxCount()
	{
		if (m_rendermesh_count <= 0)
			return 0;

		if (!m_rendermesh)
			return 0;

		c_rendermesh* meshes = m_rendermesh->m_meshes;
		if (!meshes)
			return 0;

		c_rendermesh mesh = meshes[0];

		c_hitboxsets* hithoxsets = mesh.m_hitboxsets;
		if (!hithoxsets)
			return 0;

		c_hitboxsets hithoxset = hithoxsets[0];
		if (hithoxset.m_nHitboxCount <= 0)
			return 0;

		return hithoxset.m_nHitboxCount;
	}

private:
	char pad_0x0000[0x8]; // 0x0000
	const char* m_name; // 0x0008
	char pad_0x0010[0x4]; // 0x0010
	__int32 m_nFlags; // 0x0014
	Vector_t m_vHullMin; // 0x0018
	Vector_t m_vHullMax; // 0x0024
	Vector_t m_vViewMin; // 0x0030
	Vector_t m_vViewMax; // 0x003C
	char pad_0x0048[0x28]; // 0x0048
	__int32 m_rendermesh_count; // 0x0070
	char pad_0x0074[0x4]; // 0x0074
	c_rendermeshes* m_rendermesh; // 0x0078
	char pad_0x0080[0x50]; // 0x0080
	c_physmodel* m_physmodel; // 0x00D0
	__int32 m_animationgroupresource_count; // 0x00D8
	char pad_0x00DC[0x4]; // 0x00DC
	void* m_animationgroupresource; // 0x00E0
	char pad_0x00E8[0x8]; // 0x00E8
	__int32 m_meshes_count; // 0x00F0
	char pad_0x00F4[0x4]; // 0x00F4
	void* m_meshes; // 0x00F8
	char pad_0x0100[0x28]; // 0x0100
	void* N00000135; // 0x0128
	char pad_0x0130[0x28]; // 0x0130
	__int32 m_names_count; // 0x0158
	char pad_0x015C[0x4]; // 0x015C
	const char** m_names; // 0x0160
	char pad_0x0168[0x8]; // 0x0168
	__int32 m_elementindexcount; // 0x0170
	char pad_0x0174[0x4]; // 0x0174
	uint16_t* m_elementindex; // 0x0178
	char pad_0x0180[0x8]; // 0x0180
	__int32 N00000141; // 0x0188
	char pad_0x018C[0x4]; // 0x018C
	void* N00000142; // 0x0190
	char pad_0x0198[0x8]; // 0x0198
	__int32 N00000144; // 0x01A0
	char pad_0x01A4[0x4]; // 0x01A4
	void* N00000145; // 0x01A8
	char pad_0x01B0[0x8]; // 0x01B0
	__int32 m_pos_count; // 0x01B8
	char pad_0x01BC[0x4]; // 0x01BC
	Vector_t* m_pos; // 0x01C0
	char pad_0x01C8[0x8]; // 0x01C8
	__int32 m_quat_count; // 0x01D0
	char pad_0x01D4[0x4]; // 0x01D4
	Vector4D_t* m_quat; // 0x01D8
	char pad_0x01E0[0x8]; // 0x01E0
	__int32 m_scale_count; // 0x01E8
	char pad_0x01EC[0x4]; // 0x01EC
	float* m_scale; // 0x01F0
	char pad_0x01F8[0x8]; // 0x01F8
	__int32 N00000150; // 0x0200
	char pad_0x0204[0x4]; // 0x0204
	void* N00000151; // 0x0208
	char pad_0x0210[0x8]; // 0x0210
	__int32 m_somecount2; // 0x0218
	char pad_0x021C[0x4]; // 0x021C
	uint16_t* m_somearray2; // 0x0220
	char pad_0x0228[0x8]; // 0x0228
	__int32 m_somecount1; // 0x0230
	char pad_0x0234[0x4]; // 0x0234
	uint16_t* m_somearray1; // 0x0238
	char pad_0x0240[0x20]; // 0x0240
	__int32 N0000015C; // 0x0260
	char pad_0x0264[0x4]; // 0x0264
	void* N0000015D; // 0x0268
	char pad_0x0270[0x8]; // 0x0270
	__int32 N0000015F; // 0x0278
	char pad_0x027C[0x4]; // 0x027C
	void* N00000160; // 0x0280
	char pad_0x0288[0x40]; // 0x0288
	__int32 N00000169; // 0x02C8
	char pad_0x02CC[0x4]; // 0x02CC
	void* N0000016A; // 0x02D0
	char pad_0x02D8[0x8]; // 0x02D8
	__int32 m_somecount3; // 0x02E0
	char pad_0x02E4[0x4]; // 0x02E4
	uint16_t* m_somearray3; // 0x02E8
	char pad_0x02F0[0x58]; // 0x02F0
	void* N00000179; // 0x0348
	char pad_0x0350[0x8]; // 0x0350
	__int32 N0000017B; // 0x0358
	char pad_0x035C[0x4]; // 0x035C
	void* N0000017C; // 0x0360
	char pad_0x0368[0x70]; // 0x0368
	__int32 N0000018B; // 0x03D8
	char pad_0x03DC[0x4]; // 0x03DC
	void* N0000018C; // 0x03E0
	char pad_0x03E8[0x30]; // 0x03E8
	__int32 N00000193; // 0x0418
	char pad_0x041C[0x4]; // 0x041C
	void* N00000194; // 0x0420
	char pad_0x0428[0x5D8]; // 0x0428
};


class CModelState
{
public:
	std::uint8_t padding_0[0x80];
	BoneData_t* bones;
	std::uint8_t padding_1[0x18];
	CStrongHandle<CModel> modelHandle;
};

class CSkeletonInstance;

class CGameSceneNode
{
public:
	CS_CLASS_NO_INITIALIZER(CGameSceneNode);

	SCHEMA_ADD_FIELD(CTransform, GetNodeToWorld, "CGameSceneNode->m_nodeToWorld");
	SCHEMA_ADD_FIELD(CEntityInstance*, GetOwner, "CGameSceneNode->m_pOwner");

	SCHEMA_ADD_FIELD(Vector_t, GetOrigin, "CGameSceneNode->m_vecOrigin");
	SCHEMA_ADD_FIELD(Vector_t, GetAbsOrigin, "CGameSceneNode->m_vecAbsOrigin");
	SCHEMA_ADD_FIELD(Vector_t, GetRenderOrigin, "CGameSceneNode->m_vRenderOrigin");

	SCHEMA_ADD_FIELD(QAngle_t, GetAngleRotation, "CGameSceneNode->m_angRotation");
	SCHEMA_ADD_FIELD(QAngle_t, GetAbsAngleRotation, "CGameSceneNode->m_angAbsRotation");

	SCHEMA_ADD_FIELD(bool, IsDormant, "CGameSceneNode->m_bDormant");

	CSkeletonInstance* GetSkeletonInstance()
	{
		return MEM::CallVFunc<CSkeletonInstance*, 8U>(this);
	}
};

class C_BaseEntity : public CEntityInstance
{
public:
	CS_CLASS_NO_INITIALIZER(C_BaseEntity);

	[[nodiscard]] bool IsBasePlayerController()
	{
		SchemaClassInfoData_t* pClassInfo;
		GetSchemaClassInfo(&pClassInfo);
		if (pClassInfo == nullptr)
			return false;

		return FNV1A::Hash(pClassInfo->szName) == FNV1A::HashConst("C_CSPlayerController");
	}

	[[nodiscard]] bool IsWeapon()
	{
		static SchemaClassInfoData_t* pWeaponBaseClass = nullptr;
		if (pWeaponBaseClass == nullptr)
		I::SchemaSystem->FindTypeScopeForModule(CS_XOR("client.dll"))->FindDeclaredClass(&pWeaponBaseClass, CS_XOR("C_CSWeaponBase"));


		SchemaClassInfoData_t* pClassInfo;
		GetSchemaClassInfo(&pClassInfo);
		if (pClassInfo == nullptr)
			return false;

		return (pClassInfo->InheritsFrom(pWeaponBaseClass));
	}

	[[nodiscard]] bool IsValidMoveType() noexcept
	{
		if (!this)
			return false;

		const auto move_type = this->GetMoveType();
		return move_type != MOVETYPE_NOCLIP && move_type != MOVETYPE_LADDER;
	}

	static C_BaseEntity* GetLocalPlayer();

	// get entity origin on scene
	[[nodiscard]] const Vector_t& GetSceneOrigin();

	SCHEMA_ADD_FIELD(CGameSceneNode*, GetGameSceneNode, "C_BaseEntity->m_pGameSceneNode");
	SCHEMA_ADD_FIELD(CCollisionProperty*, GetCollision, "C_BaseEntity->m_pCollision");
	SCHEMA_ADD_FIELD(std::uint8_t, GetTeam, "C_BaseEntity->m_iTeamNum");
	SCHEMA_ADD_FIELD(CBaseHandle, GetOwnerHandle, "C_BaseEntity->m_hOwnerEntity");
	SCHEMA_ADD_FIELD(Vector_t, GetBaseVelocity, "C_BaseEntity->m_vecBaseVelocity");
	SCHEMA_ADD_FIELD(Vector_t, GetVecVelocity, "C_BaseEntity->m_vecVelocity");
	SCHEMA_ADD_FIELD(Vector_t, GetAbsVelocity, "C_BaseEntity->m_vecAbsVelocity");
	SCHEMA_ADD_FIELD(float, GetSimulationTime, "C_BaseEntity->m_flSimulationTime");
	SCHEMA_ADD_FIELD(bool, IsTakingDamage, "C_BaseEntity->m_bTakesDamage");
	SCHEMA_ADD_FIELD(std::uint32_t, GetFlags, "C_BaseEntity->m_fFlags");
	SCHEMA_ADD_FIELD(std::int32_t, GetEflags, "C_BaseEntity->m_iEFlags");
	SCHEMA_ADD_FIELD(std::uint8_t, GetMoveType, "C_BaseEntity->m_nActualMoveType"); // m_nActualMoveType returns CSGO style movetype, m_nMoveType returns bitwise shifted move type
	SCHEMA_ADD_FIELD(std::uint8_t, GetLifeState, "C_BaseEntity->m_lifeState");
	SCHEMA_ADD_FIELD(std::int32_t, GetHealth, "C_BaseEntity->m_iHealth");
	SCHEMA_ADD_FIELD(std::int32_t, GetMaxHealth, "C_BaseEntity->m_iMaxHealth");
	SCHEMA_ADD_FIELD(float, GetWaterLevel, "C_BaseEntity->m_flWaterLevel");
	SCHEMA_ADD_FIELD_OFFSET(void*, GetVData, "C_BaseEntity->m_nSubclassID", 0x8);

};

class CGlowProperty;

class C_BaseModelEntity : public C_BaseEntity
{
public:
	CS_CLASS_NO_INITIALIZER(C_BaseModelEntity);

	SCHEMA_ADD_FIELD(CCollisionProperty, GetCollisionInstance, "C_BaseModelEntity->m_Collision");
	SCHEMA_ADD_FIELD(CGlowProperty, GetGlowProperty, "C_BaseModelEntity->m_Glow");
	SCHEMA_ADD_FIELD(Vector_t, GetViewOffset, "C_BaseModelEntity->m_vecViewOffset");
	SCHEMA_ADD_FIELD(GameTime_t, GetCreationTime, "C_BaseModelEntity->m_flCreateTime");
	SCHEMA_ADD_FIELD(GameTick_t, GetCreationTick, "C_BaseModelEntity->m_nCreationTick");
	SCHEMA_ADD_FIELD(CBaseHandle, GetMoveParent, "C_BaseModelEntity->m_hOldMoveParent");
	SCHEMA_ADD_FIELD(std::float_t, GetAnimTime, "C_BaseModelEntity->m_flAnimTime");
};

class CPlayer_ItemServices
{
public:
	CS_CLASS_NO_INITIALIZER(CPlayer_ItemServices);

	SCHEMA_ADD_FIELD(bool, HasDefuser, "CCSPlayer_ItemServices->m_bHasDefuser");
	SCHEMA_ADD_FIELD(bool, HasHelmet, "CCSPlayer_ItemServices->m_bHasHelmet");
	SCHEMA_ADD_FIELD(bool, HasHeavyArmor, "CCSPlayer_ItemServices->m_bHasHeavyArmor");
};

class CPlayer_CameraServices
{
public:
	SCHEMA_ADD_FIELD(Vector_t, GetViewPunchAngle, "CPlayer_CameraServices->m_vecCsViewPunchAngle");
	SCHEMA_ADD_FIELD(CBaseHandle, GetActivePostProcessingVolume, "CPlayer_CameraServices->m_hActivePostProcessingVolume");
};

class CPlayer_WeaponServices
{
public:
	SCHEMA_ADD_FIELD(CBaseHandle, GetActiveWeapon, "CPlayer_WeaponServices->m_hActiveWeapon");
};

class CCSPlayer_WeaponServices : public CPlayer_WeaponServices
{
public:
	SCHEMA_ADD_FIELD(GameTime_t, GetNextAttack, "CCSPlayer_WeaponServices->m_flNextAttack");
};
 
class CPlayer_ObserverServices
{
public:
	CS_CLASS_NO_INITIALIZER(CPlayer_ObserverServices);

	SCHEMA_ADD_FIELD(CBaseHandle, GetObserverTarget, "CPlayer_ObserverServices->m_hObserverTarget");
	SCHEMA_ADD_FIELD(ObserverMode_t, GetObserverMode, "CPlayer_ObserverServices->m_iObserverMode");
	SCHEMA_ADD_FIELD(float, GetObserverChaseDistance, "CPlayer_ObserverServices->m_flObserverChaseDistance");
	SCHEMA_ADD_FIELD(GameTime_t, GetObserverChaseDistanceCalcTime, "CPlayer_ObserverServices->m_flObserverChaseDistanceCalcTime");
};

class CUserCmd;

class CPlayer_MovementServices
{
public:
	CS_CLASS_NO_INITIALIZER(CPlayer_MovementServices);

	SCHEMA_ADD_FIELD(float, GetMaxspeed, "CPlayer_MovementServices->m_flMaxspeed");
	SCHEMA_ADD_FIELD(float, GetForwardMove, "CPlayer_MovementServices->m_flForwardMove");
	SCHEMA_ADD_FIELD(float, GetLeftMove, "CPlayer_MovementServices->m_flLeftMove");
	SCHEMA_ADD_FIELD(float, GetSurfaceFriction, "CPlayer_MovementServices_Humanoid->m_flSurfaceFriction");
	//SCHEMA_ADD_OFFSET(float, GetSurfaceFriction, 0x1FC);

	void run_command(CUserCmd* pCmd)
	{
		MEM::CallVFunc<void, 25U>(this, pCmd);
	}

	void set_prediction_command(CUserCmd* pCmd)
	{
		// 48 89 5C 24 ? 57 48 83 EC ? 48 8B DA E8 ? ? ? ? 48 8B F8 48 85 C0 74

		MEM::CallVFunc<void, 36U>(this, pCmd);
	}

	void reset_prediction_command()
	{
		MEM::CallVFunc<void, 37U>(this);
	}
};

class C_BasePlayerPawn : public C_BaseModelEntity
{
public:
	CS_CLASS_NO_INITIALIZER(C_BasePlayerPawn);

	SCHEMA_ADD_FIELD(CBaseHandle, GetControllerHandle, "C_BasePlayerPawn->m_hController");
	SCHEMA_ADD_FIELD(CCSPlayer_WeaponServices*, GetWeaponServices, "C_BasePlayerPawn->m_pWeaponServices");
	SCHEMA_ADD_FIELD(CPlayer_ItemServices*, GetItemServices, "C_BasePlayerPawn->m_pItemServices");
	SCHEMA_ADD_FIELD(CPlayer_CameraServices*, GetCameraServices, "C_BasePlayerPawn->m_pCameraServices");
	SCHEMA_ADD_FIELD(CPlayer_ObserverServices*, GetObserverServices, "C_BasePlayerPawn->m_pObserverServices");
	SCHEMA_ADD_FIELD(CPlayer_MovementServices*, GetMovementServices, "C_BasePlayerPawn->m_pMovementServices");



	[[nodiscard]] Vector_t GetEyePosition()
	{
		/*Vector_t vecEyePosition = Vector_t(0.0f, 0.0f, 0.0f);
		MEM::CallVFunc<void, 169U>(this, &vecEyePosition);
		return vecEyePosition;*/

		if (!this->GetGameSceneNode())
			return Vector_t(0, 0, 0);

		return this->GetGameSceneNode()->GetAbsOrigin() + this->GetViewOffset();
	}
};

class CCSPlayer_ViewModelServices;

class C_CSPlayerPawnBase : public C_BasePlayerPawn
{
public:
	CS_CLASS_NO_INITIALIZER(C_CSPlayerPawnBase);

	SCHEMA_ADD_FIELD(CCSPlayer_ViewModelServices*, GetViewModelServices, "C_CSPlayerPawnBase->m_pViewModelServices");
	SCHEMA_ADD_FIELD(float, GetLowerBodyYawTarget, "C_CSPlayerPawnBase->m_flLowerBodyYawTarget");
	SCHEMA_ADD_FIELD(float, GetFlashMaxAlpha, "C_CSPlayerPawnBase->m_flFlashMaxAlpha");
	SCHEMA_ADD_FIELD(float, GetFlashDuration, "C_CSPlayerPawnBase->m_flFlashDuration");
	SCHEMA_ADD_FIELD(bool, GetGunGameImmunity, "C_CSPlayerPawnBase->m_bGunGameImmunity");
	SCHEMA_ADD_FIELD(Vector_t, GetLastSmokeOverlayColor, "C_CSPlayerPawnBase->m_vLastSmokeOverlayColor");
	SCHEMA_ADD_FIELD(int, GetSurvivalTeam, "C_CSPlayerPawnBase->m_nSurvivalTeam"); // danger zone

	SCHEMA_ADD_FIELD(QAngle_t, GetAngEyeAngles, "C_CSPlayerPawnBase->m_angEyeAngles");
};

class C_CSWeaponBase;

class C_CSPlayerPawn : public C_CSPlayerPawnBase
{
public:
	CS_CLASS_NO_INITIALIZER(C_CSPlayerPawn);

	[[nodiscard]] bool IsOtherEnemy(C_CSPlayerPawn* pOther);
	[[nodiscard]] int GetAssociatedTeam();
	[[nodiscard]] bool CanAttack(const float flServerTime);
	[[nodiscard]] bool CanShoot(float svtime);
	[[nodiscard]] std::uint32_t GetOwnerHandleIndex();
	[[nodiscard]] std::uint16_t GetCollisionMask();
	[[nodiscard]] bool HasArmor(const int hitgroup);
	[[nodiscard]] bool is_throwing();
	[[nodiscard]] C_CSWeaponBase* GetActiveWeaponFromPlayer();

	SCHEMA_ADD_FIELD(bool, IsScoped, "C_CSPlayerPawn->m_bIsScoped");
	SCHEMA_ADD_FIELD(bool, IsDefusing, "C_CSPlayerPawn->m_bIsDefusing");
	SCHEMA_ADD_FIELD(bool, IsGrabbingHostage, "C_CSPlayerPawn->m_bIsGrabbingHostage");
	SCHEMA_ADD_FIELD(bool, IsWaitForNoAttack, "C_CSPlayerPawn->m_bWaitForNoAttack");
	SCHEMA_ADD_FIELD(int, GetShotsFired, "C_CSPlayerPawn->m_iShotsFired");
	SCHEMA_ADD_FIELD(std::int32_t, GetArmorValue, "C_CSPlayerPawn->m_ArmorValue");
	SCHEMA_ADD_FIELD(QAngle_t, GetAimPuchAngle, "C_CSPlayerPawn->m_aimPunchAngle");
	SCHEMA_ADD_FIELD(CUtlVectorCS2<QAngle_t>, GetAimPunchCache, "C_CSPlayerPawn->m_aimPunchCache");
};


class C_CSObserverPawn : public C_CSPlayerPawnBase
{
public:
	CS_CLASS_NO_INITIALIZER(C_CSObserverPawn);
};


class CBasePlayerController : public C_BaseModelEntity
{
public:
	CS_CLASS_NO_INITIALIZER(CBasePlayerController);

	SCHEMA_ADD_FIELD(std::uint64_t, GetSteamId, "CBasePlayerController->m_steamID");
	SCHEMA_ADD_FIELD(std::uint32_t, GetTickBase, "CBasePlayerController->m_nTickBase");
	SCHEMA_ADD_FIELD(CBaseHandle, GetPawnHandle, "CBasePlayerController->m_hPawn");
	SCHEMA_ADD_FIELD(CBaseHandle, GetPredictedPawnHandle, "CBasePlayerController->m_hPredictedPawn");
	SCHEMA_ADD_FIELD(bool, IsLocalPlayerController, "CBasePlayerController->m_bIsLocalPlayerController");
};

// forward decleration
class C_CSWeaponBaseGun;
class C_BasePlayerWeapon;
class CCSPlayerController : public CBasePlayerController
{
public:
	CS_CLASS_NO_INITIALIZER(CCSPlayerController);

	[[nodiscard]] static CCSPlayerController* GetLocalPlayerController();

	// @note: always get origin from pawn not controller
	[[nodiscard]] const Vector_t& GetPawnOrigin();
	[[nodiscard]] void PhysicsRunThink();
	[[nodiscard]] C_CSPlayerPawn* GetObserverPawn();

	SCHEMA_ADD_FIELD(std::uint32_t, GetPing, "CCSPlayerController->m_iPing");
	SCHEMA_ADD_FIELD(const char*, GetPlayerName, "CCSPlayerController->m_sSanitizedPlayerName");
	SCHEMA_ADD_FIELD(std::int32_t, GetPawnHealth, "CCSPlayerController->m_iPawnHealth");
	SCHEMA_ADD_FIELD(std::int32_t, GetPawnArmor, "CCSPlayerController->m_iPawnArmor");
	SCHEMA_ADD_FIELD(bool, IsPawnHasDefuser, "CCSPlayerController->m_bPawnHasDefuser");
	SCHEMA_ADD_FIELD(bool, IsPawnHasHelmet, "CCSPlayerController->m_bPawnHasHelmet");
	SCHEMA_ADD_FIELD(bool, IsPawnAlive, "CCSPlayerController->m_bPawnIsAlive");
	SCHEMA_ADD_FIELD(CBaseHandle, GetPlayerPawnHandle, "CCSPlayerController->m_hPlayerPawn");
	SCHEMA_ADD_FIELD(CBaseHandle, GetObserverPawnHandle, "CCSPlayerController->m_hObserverPawn");


	__forceinline CUserCmd*& m_current_command()
	{
		return *reinterpret_cast<CUserCmd**>(reinterpret_cast<std::uint64_t>(this) + this->GetSteamId() - 0x8);
	};
};

class CBaseAnimGraph : public C_BaseModelEntity
{
public:
	CS_CLASS_NO_INITIALIZER(CBaseAnimGraph);

	SCHEMA_ADD_FIELD(bool, IsClientRagdoll, "CBaseAnimGraph->m_bClientRagdoll");
};

class C_BaseFlex : public CBaseAnimGraph
{
public:
	CS_CLASS_NO_INITIALIZER(C_BaseFlex);
	/* not implemented */
};

class C_EconItemView
{
public:
	CS_CLASS_NO_INITIALIZER(C_EconItemView);

	SCHEMA_ADD_FIELD(std::uint16_t, GetItemDefinitionIndex, "C_EconItemView->m_iItemDefinitionIndex");
	SCHEMA_ADD_FIELD(std::uint64_t, GetItemID, "C_EconItemView->m_iItemID");
	SCHEMA_ADD_FIELD(std::uint32_t, GetItemIDHigh, "C_EconItemView->m_iItemIDHigh");
	SCHEMA_ADD_FIELD(std::uint32_t, GetItemIDLow, "C_EconItemView->m_iItemIDLow");
	SCHEMA_ADD_FIELD(std::uint32_t, GetAccountID, "C_EconItemView->m_iAccountID");
	SCHEMA_ADD_FIELD(char[161], GetCustomName, "C_EconItemView->m_szCustomName");
	SCHEMA_ADD_FIELD(char[161], GetCustomNameOverride, "C_EconItemView->m_szCustomNameOverride");
};

class CAttributeManager
{
public:
	CS_CLASS_NO_INITIALIZER(CAttributeManager);
	virtual ~CAttributeManager() = 0;
};
static_assert(sizeof(CAttributeManager) == 0x8);

class C_AttributeContainer : public CAttributeManager
{
public:
	CS_CLASS_NO_INITIALIZER(C_AttributeContainer);

	SCHEMA_ADD_PFIELD(C_EconItemView, GetItem, "C_AttributeContainer->m_Item");
};

class C_EconEntity : public C_BaseFlex
{
public:
	CS_CLASS_NO_INITIALIZER(C_EconEntity);

	SCHEMA_ADD_PFIELD(C_AttributeContainer, GetAttributeManager, "C_EconEntity->m_AttributeManager");
	SCHEMA_ADD_FIELD(std::uint32_t, GetOriginalOwnerXuidLow, "C_EconEntity->m_OriginalOwnerXuidLow");
	SCHEMA_ADD_FIELD(std::uint32_t, GetOriginalOwnerXuidHigh, "C_EconEntity->m_OriginalOwnerXuidHigh");
	SCHEMA_ADD_FIELD(std::int32_t, GetFallbackPaintKit, "C_EconEntity->m_nFallbackPaintKit");
	SCHEMA_ADD_FIELD(std::int32_t, GetFallbackSeed, "C_EconEntity->m_nFallbackSeed");
	SCHEMA_ADD_FIELD(std::int32_t, GetFallbackWear, "C_EconEntity->m_flFallbackWear");
	SCHEMA_ADD_FIELD(std::int32_t, GetFallbackStatTrak, "C_EconEntity->m_nFallbackStatTrak");
	SCHEMA_ADD_FIELD(CBaseHandle, GetViewModelAttachmentHandle, "C_EconEntity->m_hViewmodelAttachment");
};

class C_EconWearable : public C_EconEntity
{
public:
	CS_CLASS_NO_INITIALIZER(C_EconWearable);

	SCHEMA_ADD_FIELD(std::int32_t, GetForceSkin, "C_EconWearable->m_nForceSkin");
	SCHEMA_ADD_FIELD(bool, IsAlwaysAllow, "C_EconWearable->m_bAlwaysAllow");
};


class C_BasePlayerWeapon : public C_EconEntity
{
public:
	CS_CLASS_NO_INITIALIZER(C_BasePlayerWeapon);

	SCHEMA_ADD_FIELD(GameTick_t, GetNextPrimaryAttackTick, "C_BasePlayerWeapon->m_nNextPrimaryAttackTick");
	SCHEMA_ADD_FIELD(float, GetNextPrimaryAttackTickRatio, "C_BasePlayerWeapon->m_flNextPrimaryAttackTickRatio");
	SCHEMA_ADD_FIELD(GameTick_t, GetNextSecondaryAttackTick, "C_BasePlayerWeapon->m_nNextSecondaryAttackTick");
	SCHEMA_ADD_FIELD(float, GetNextSecondaryAttackTickRatio, "C_BasePlayerWeapon->m_flNextSecondaryAttackTickRatio");
	SCHEMA_ADD_FIELD(std::int32_t, GetClip1, "C_BasePlayerWeapon->m_iClip1");
	SCHEMA_ADD_FIELD(std::int32_t, GetClip2, "C_BasePlayerWeapon->m_iClip2");
	SCHEMA_ADD_FIELD(std::int32_t[2], GetReserveAmmo, "C_BasePlayerWeapon->m_pReserveAmmo");


	C_EconItemView* get_econ_view_item()
	{
		return reinterpret_cast<C_EconItemView*>(std::uintptr_t(this) + 0x10E8);
	}

	[[nodiscard]] void update_accuracy_penality()
	{
		// 48 89 5C 24 ? 57 48 83 EC ? 48 8B F9 E8 ? ? ? ? 48 8B D8 48 85 C0 0F 84 ? ? ? ? 44 0F 29 44 24

		using function_t = void(__fastcall*)(void*);
		static function_t fn = reinterpret_cast<function_t>(MEM::FindPattern(CLIENT_DLL, CS_XOR("48 89 5C 24 ? 57 48 83 EC ? 48 8B F9 E8 ? ? ? ? 48 8B D8 48 85 C0 0F 84 ? ? ? ? 44 0F 29 44 24")));
		return fn(this);

		//return MEM::CallVFunc<void, 411>(this);
	}

	[[nodiscard]] float get_spread()
	{
		// 48 83 EC ? 48 63 91

		using function_t = float(__fastcall*)(void*);
		static function_t fn = reinterpret_cast<function_t>(MEM::FindPattern(CLIENT_DLL, CS_XOR("48 83 EC ? 48 63 91")));
		return fn(this);

		//return MEM::CallVFunc<float, 364>(this);
	}

	[[nodiscard]] float get_inaccuracy()
	{
		// 48 89 5C 24 ? 55 56 57 48 81 EC ? ? ? ? 44 0F 29 84 24

		float x = 0.0f, y = 0.0f;

		using function_t = float(__fastcall*)(void*, float*, float*);
		static function_t fn = reinterpret_cast<function_t>(MEM::FindPattern(CLIENT_DLL, CS_XOR("48 89 5C 24 ? 55 56 57 48 81 EC ? ? ? ? 44 0F 29 84 24")));
		return fn(this, &x, &y);

		//return MEM::CallVFunc<float, 410>(this, &x, &y);
	}
};

class C_CSWeaponBase : public C_BasePlayerWeapon
{
public:
	CS_CLASS_NO_INITIALIZER(C_CSWeaponBase);

	SCHEMA_ADD_FIELD(bool, IsInReload, "C_CSWeaponBase->m_bInReload");
	SCHEMA_ADD_FIELD(float, GetRecoilIndex, "C_CSWeaponBase->m_flRecoilIndex");
	SCHEMA_ADD_FIELD(float, GetPostponeFireReadyTicks, "C_CSWeaponBase->m_nPostponeFireReadyTicks");


	__forceinline float get_max_speed()
	{
		using original_fn = float(__fastcall*)(void*);
		return (*(original_fn**)this)[340](this);
	}

	Vector_t calculate_spread(unsigned int index);


	CCSWeaponBaseVData* GetWeaponVData()
	{
		return static_cast<CCSWeaponBaseVData*>(GetVData());
	}
};

class C_CSWeaponBaseGun : public C_CSWeaponBase
{
public:
	CS_CLASS_NO_INITIALIZER(C_CSWeaponBaseGun);

	SCHEMA_ADD_FIELD(std::int32_t, GetZoomLevel, "C_CSWeaponBaseGun->m_zoomLevel");
	SCHEMA_ADD_FIELD(std::int32_t, GetBurstShotsRemaining, "C_CSWeaponBaseGun->m_iBurstShotsRemaining");
	SCHEMA_ADD_FIELD(bool, IsBurstMode, "C_CSWeaponBase->m_bBurstMode");
	SCHEMA_ADD_FIELD(float, GetPostponeFireReadyFrac, "C_CSWeaponBase->m_flPostponeFireReadyFrac");

	[[nodiscard]] bool CanPrimaryAttack(const int nWeaponType, const float flServerTime);
	[[nodiscard]] bool CanSecondaryAttack(const int nWeaponType, const float flServerTime);
};

class C_BaseCSGrenade : public C_CSWeaponBase
{
public:
	SCHEMA_ADD_FIELD(bool, IsHeldByPlayer, "C_BaseCSGrenade->m_bIsHeldByPlayer");
	SCHEMA_ADD_FIELD(bool, IsPinPulled, "C_BaseCSGrenade->m_bPinPulled");
	SCHEMA_ADD_FIELD(GameTime_t, GetThrowTime, "C_BaseCSGrenade->m_fThrowTime");
	SCHEMA_ADD_FIELD(float, GetThrowStrength, "C_BaseCSGrenade->m_flThrowStrength");
};

class C_BaseGrenade : public C_BaseFlex
{
public:
	CS_CLASS_NO_INITIALIZER(C_BaseGrenade);
};


class CSkeletonInstance : public CGameSceneNode
{
public:
	MEM_PAD(0x1CC); //0x0000
	int nBoneCount; //0x01CC
	MEM_PAD(0x18); //0x01D0
	int nMask; //0x01E8
	MEM_PAD(0x4); //0x01EC
	Matrix2x4_t* pBoneCache; //0x01F0
	//CBoneData* pBoneCache; //0x01F0

	//CSkeletonInstance() = delete;
	//CSkeletonInstance(CSkeletonInstance&&) = delete;
	//CSkeletonInstance(const CSkeletonInstance&) = delete;

	SCHEMA_ADD_FIELD(CModelState, GetModelState, "CSkeletonInstance->m_modelState");
	SCHEMA_ADD_FIELD(std::uint8_t, m_nHitboxSet, "CSkeletonInstance->m_nHitboxSet");

	//void get_bone_data(bone_data& data, int index);
	//fnCalculateWorldSpaceBones : 40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC D0
	void CS_FASTCALL calc_world_space_bones(uint32_t parent, uint32_t mask);
	void CS_FASTCALL spoofed_calc_world_space_bones(uint32_t mask);

	//CBoneData GetBone(int index)
	//{
	//	CModelState ms = GetModelState();
	//	if (!ms.modelHandle || !ms.bones || ms.NumBones() < index)
	//		return CBoneData{};
	//	return ms.bones[index];
	//}
};

class C_EnvSky
{
public:
	CS_CLASS_NO_INITIALIZER(C_EnvSky);

	SCHEMA_ADD_FIELD(bool, m_bStartDisabled, "C_EnvSky->m_bStartDisabled");
	SCHEMA_ADD_FIELD(Color_t, m_vTintColor, "C_EnvSky->m_vTintColor");
	SCHEMA_ADD_FIELD(ImColor, m_vTintColorIm, "C_EnvSky->m_vTintColor");
	SCHEMA_ADD_FIELD(Color_t, m_vTintColorLightingOnly, "C_EnvSky->m_vTintColorLightingOnly");
	SCHEMA_ADD_FIELD(float, m_flBrightnessScale, "C_EnvSky->m_flBrightnessScale");
	SCHEMA_ADD_FIELD(int, m_nFogType, "C_EnvSky->m_nFogType");
	SCHEMA_ADD_FIELD(float, m_flFogMinStart, "C_EnvSky->m_flFogMinStart");
	SCHEMA_ADD_FIELD(float, m_flFogMinEnd, "C_EnvSky->m_flFogMinEnd");
	SCHEMA_ADD_FIELD(float, m_flFogMaxStart, "C_EnvSky->m_flFogMaxStart");
	SCHEMA_ADD_FIELD(float, m_flFogMaxEnd, "C_EnvSky->m_flFogMaxEnd");
	SCHEMA_ADD_FIELD(bool, m_bEnabled, "C_EnvSky->m_bEnabled");
};

class CGlowProperty
{
public:
	CS_CLASS_NO_INITIALIZER(CGlowProperty);

	SCHEMA_ADD_FIELD(Vector_t, m_fGlowColor, "CGlowProperty->m_fGlowColor");
	SCHEMA_ADD_FIELD(int, m_iGlowType, "CGlowProperty->m_iGlowType");
	SCHEMA_ADD_FIELD(int, m_iGlowTeam, "CGlowProperty->m_iGlowTeam");
	SCHEMA_ADD_FIELD(int, m_nGlowRange, "CGlowProperty->m_nGlowRange");
	SCHEMA_ADD_FIELD(int, m_nGlowRangeMin, "CGlowProperty->m_nGlowRangeMin");
	SCHEMA_ADD_FIELD(Color_t, m_glowColorOverride, "CGlowProperty->m_glowColorOverride");
	SCHEMA_ADD_FIELD(bool, m_bFlashing, "CGlowProperty->m_bFlashing");
	SCHEMA_ADD_FIELD(float, m_flGlowTime, "CGlowProperty->m_flGlowTime");
	SCHEMA_ADD_FIELD(float, m_flGlowStartTime, "CGlowProperty->m_flGlowStartTime");
	SCHEMA_ADD_FIELD(bool, m_bEligibleForScreenHighlight, "CGlowProperty->m_bEligibleForScreenHighlight");
	SCHEMA_ADD_FIELD(bool, m_bGlowing, "CGlowProperty->m_bGlowing");

	C_BaseEntity* Owner()
	{
		return *(C_BaseEntity**)(this + 0x18);
	}
};
