#include "entity.h"

// used: convars
#include "../core/convars.h"
#include "interfaces/cgameentitysystem.h"
#include "interfaces/ienginecvar.h"
#include "interfaces/iengineclient.h"

// used: game's definitions, enums
#include "const.h"


// global empty vector for when we can't get the origin
static Vector_t vecEmpty = Vector_t(0, 0, 0);

CCSPlayerController* CCSPlayerController::GetLocalPlayerController()
{
	const int nIndex = I::Engine->GetLocalPlayer();
	return I::GameResourceService->pGameEntitySystem->Get<CCSPlayerController>(nIndex);
}

const Vector_t& CCSPlayerController::GetPawnOrigin()
{
	CBaseHandle hPawnHandle = this->GetPawnHandle();
	if (!hPawnHandle.IsValid())
		return vecEmpty;

	C_CSPlayerPawn* pPlayerPawn = I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(hPawnHandle);
	if (pPlayerPawn == nullptr)
		return vecEmpty;

	return pPlayerPawn->GetSceneOrigin();
}

void CCSPlayerController::PhysicsRunThink()
{
	static auto physics_run_think_ = reinterpret_cast<void*(__fastcall*)(void*)>(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 49 8B D6 48 8B CE E8 ? ? ? ? 48 8B 06"), 0x1));
	physics_run_think_(this);
}

C_CSPlayerPawn* CCSPlayerController::GetObserverPawn()
{
	if (!GetObserverPawnHandle().IsValid())
		return nullptr;

	int index = GetObserverPawnHandle().GetEntryIndex();
	return I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(index);
}

C_BaseEntity* C_BaseEntity::GetLocalPlayer()
{
	const int nIndex = I::Engine->GetLocalPlayer();
	return I::GameResourceService->pGameEntitySystem->Get(nIndex);
}

const Vector_t& C_BaseEntity::GetSceneOrigin()
{
	if (this->GetGameSceneNode())
		return GetGameSceneNode()->GetAbsOrigin();

	return vecEmpty;
}

bool C_CSPlayerPawn::IsOtherEnemy(C_CSPlayerPawn* pOther)
{
	// check are other player is invalid or we're comparing against ourselves
	if (pOther == nullptr || this == pOther)
		return false;

	if (CONVAR::game_type->value.i32 == GAMETYPE_FREEFORALL && CONVAR::game_mode->value.i32 == GAMEMODE_FREEFORALL_SURVIVAL)
		// check is not teammate
		return (this->GetSurvivalTeam() != pOther->GetSurvivalTeam());

	// @todo: check is deathmatch
	if (CONVAR::mp_teammates_are_enemies->value.i1)
		return true;

	return this->GetAssociatedTeam() != pOther->GetAssociatedTeam();
}

int C_CSPlayerPawn::GetAssociatedTeam()
{
	const int nTeam = this->GetTeam();

	// @todo: check is coaching, currently cs2 doesnt have sv_coaching_enabled, so just let it be for now...
	//if (CONVAR::sv_coaching_enabled->GetBool() && nTeam == TEAM_SPECTATOR)
	//	return this->GetCoachingTeam();

	return nTeam;
}

bool C_CSPlayerPawn::CanAttack(const float flServerTime)
{
	// check is player ready to attack
	if (CCSPlayer_WeaponServices* pWeaponServices = this->GetWeaponServices(); pWeaponServices != nullptr)
		if (this->IsWaitForNoAttack() || pWeaponServices->GetNextAttack() > flServerTime)
			return false;

	return true;
}


bool C_CSPlayerPawn::CanShoot(float svtime)
{
	CPlayer_WeaponServices* WeaponServices = this->GetWeaponServices();
	if (WeaponServices == nullptr)
		return false;

	auto ActiveWeapon = I::GameResourceService->pGameEntitySystem->Get<C_CSWeaponBase>(WeaponServices->GetActiveWeapon());
	if (ActiveWeapon == nullptr)
		return false;

	auto data = ActiveWeapon->GetWeaponVData();
	if (data == nullptr)
		return false;

	if (ActiveWeapon->GetClip1() <= 0)
		return false;

	if (data->GetWeaponType() == WEAPONTYPE_KNIFE || data->GetWeaponType() == WEAPONTYPE_FISTS)
		return true;
	/*
    auto next_attack  = (WeaponServices->m_flNextAttack());
    auto next_2 =  WeaponServices->m_flNextAttack() * I::GlobalVars->flIntervalPerTick;
    auto next_3 = static_cast<float>(WeaponServices->m_flNextAttack() * I::GlobalVars->flIntervalPerTick);
    */

	auto primary_tick = ActiveWeapon->GetNextPrimaryAttackTick();

	if (primary_tick > svtime)
	{
		return false;
	}

	return true;
}


std::uint32_t C_CSPlayerPawn::GetOwnerHandleIndex()
{
	std::uint32_t Result = -1;
	if (this && GetCollision() && !(GetCollision()->GetSolidFlags() & 4))
		Result = this->GetOwnerHandle().GetEntryIndex();

	return Result;
}

std::uint16_t C_CSPlayerPawn::GetCollisionMask()
{
	if (this && GetCollision())
		return GetCollision()->CollisionMask(); // Collision + 0x38

	return 0;
}

bool C_CSPlayerPawn::HasArmor(const int hitgroup)
{
	if (!this->GetItemServices())
		return false;

	switch (hitgroup)
	{
	case HITGROUP_HEAD:
		return this->GetItemServices()->HasHelmet();
	case HITGROUP_GENERIC:
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
	case HITGROUP_LEFTARM:
	case HITGROUP_RIGHTARM:
		return true;
	default:
		return false;
	}
}

bool C_CSPlayerPawn::is_throwing()
{
	CPlayer_WeaponServices* pWeaponServices = this->GetWeaponServices();
	if (pWeaponServices == nullptr)
		return false;

	C_CSWeaponBase* active_weapon = I::GameResourceService->pGameEntitySystem->Get<C_CSWeaponBase>(pWeaponServices->GetActiveWeapon());
	if (active_weapon == nullptr)
		return false;

	CCSWeaponBaseVData* pWeaponVData = active_weapon->GetWeaponVData();
	if (pWeaponVData == nullptr)
		return false;

	if (pWeaponVData->GetWeaponType() == WEAPONTYPE_GRENADE)
		return true;

	return false;
}

C_CSWeaponBase* C_CSPlayerPawn::GetActiveWeaponFromPlayer()
{
	CPlayer_WeaponServices* pWeaponServices = this->GetWeaponServices();
	if (pWeaponServices == nullptr)
		return nullptr;

	if (!pWeaponServices->GetActiveWeapon().IsValid())
		return nullptr;

	return I::GameResourceService->pGameEntitySystem->Get<C_CSWeaponBase>(pWeaponServices->GetActiveWeapon());
}

bool C_CSWeaponBaseGun::CanPrimaryAttack(const int nWeaponType, const float flServerTime)
{
	// check are weapon support burst mode and it's ready to attack
	if (this->IsBurstMode())
	{
		// check is it ready to attack
		if (this->GetBurstShotsRemaining() > 0 /*&& this->GetNextBurstShotTime() <= flServerTime*/)
			return true;
	}

		// check is weapon ready to attack
	if (this->GetNextPrimaryAttackTick() > TIME_TO_TICKS(flServerTime))
		return false;

	// we doesn't need additional checks for knives
	if (nWeaponType == WEAPONTYPE_KNIFE)
		return true;

	// check do weapon have ammo
	if (this->GetClip1() <= 0)
		return false;

	const ItemDefinitionIndex_t nDefinitionIndex = this->GetAttributeManager()->GetItem()->GetItemDefinitionIndex();

	// check for revolver cocking ready
	if (nDefinitionIndex == WEAPON_R8_REVOLVER && this->GetPostponeFireReadyFrac() > flServerTime)
		return false;

	return true;
}

bool C_CSWeaponBaseGun::CanSecondaryAttack(const int nWeaponType, const float flServerTime)
{
	// check is weapon ready to attack
	if (this->GetNextSecondaryAttackTick() > TIME_TO_TICKS(flServerTime))
		return false;

	// we doesn't need additional checks for knives
	if (nWeaponType == WEAPONTYPE_KNIFE)
		return true;

	// check do weapon have ammo
	if (this->GetClip1() <= 0)
		return false;

	// only revolver is allowed weapon for secondary attack
	if (this->GetAttributeManager()->GetItem()->GetItemDefinitionIndex() != WEAPON_R8_REVOLVER)
		return false;

	return true;
}


void CS_FASTCALL CSkeletonInstance::calc_world_space_bones(uint32_t parent, uint32_t mask)
{ // cHoca

	using fnNewCalcWSsBones = void(CS_FASTCALL)(void*, uint32_t);
	static auto bone_new = reinterpret_cast<fnNewCalcWSsBones*>(MEM::FindPattern(CLIENT_DLL, CS_XOR("40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC D0")));
	return bone_new(this, mask);
}

Vector_t C_CSWeaponBase::calculate_spread(unsigned int index)
{
	auto weapon_data = GetWeaponVData();
	if (!weapon_data)
		return {};

	auto econ_item_view = get_econ_view_item();
	if (!econ_item_view)
		return {};

	int recoil_index = GetRecoilIndex();
	int bullets = weapon_data->GetNumBullets();
	int item_index = econ_item_view->GetItemDefinitionIndex();
	float accuracy = get_inaccuracy();
	float spread = get_spread();

	I::RandomSeed(index + 1);

	float r1 = I::RandomFloat(0.0f, 1.0f);
	float r2 = I::RandomFloat(0.0f, 6.2831855f);

	float r3{};
	float r4{};

	r3 = r1;
	r4 = r2;

	if (item_index == WEAPON_R8_REVOLVER)
	{
		r1 = 1.f - (r1 * r1);
		r3 = 1.f - (r3 * r3);
	}
	else if (item_index == WEAPON_NEGEV && recoil_index < 3)
	{
		for (int i{ 3 }; i > recoil_index; --i)
		{
			r1 *= r1;
			r3 *= r3;
		}

		r1 = 1.f - r1;
		r3 = 1.f - r3;
	}

	// get needed sine / cosine values.
	float c1 = std::cosf(r2);
	float c2 = std::cosf(r4);
	float s1 = std::sinf(r2);
	float s2 = std::sinf(r4);

	float acc = r1 * accuracy;
	float sp = r3 * spread;

	return {
		(c1 * acc) + (c2 * sp),
		(s1 * acc) + (s2 * sp),
		0.f
	};
}
