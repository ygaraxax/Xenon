#pragma once

// used: QAngle_t
#include "qangle.h"
// used: MEM_PAD
#include "../../utilities/memory.h"
// used: memalloc
#include "../../core/interfaces.h"
#include "../interfaces/imemalloc.h"

// @source: server.dll
enum ECommandButtons : std::uint64_t
{
	IN_ATTACK = 1 << 0,
	IN_JUMP = 1 << 1,
	IN_DUCK = 1 << 2,
	IN_FORWARD = 1 << 3,
	IN_BACK = 1 << 4,
	IN_USE = 1 << 5,
	IN_LEFT = 1 << 7,
	IN_RIGHT = 1 << 8,
	IN_MOVELEFT = 1 << 9,
	IN_MOVERIGHT = 1 << 10,
	IN_SECOND_ATTACK = 1 << 11,
	IN_RELOAD = 1 << 13,
	IN_SPRINT = 1 << 16,
	IN_JOYAUTOSPRINT = 1 << 17,
	IN_SHOWSCORES = 1ULL << 33,
	IN_ZOOM = 1ULL << 34,
	IN_LOOKATWEAPON = 1ULL << 35
};

// compiled protobuf messages and looked at what bits are used in them
enum ESubtickMoveStepBits : std::uint32_t
{
	MOVESTEP_BITS_BUTTON = 0x1U,
	MOVESTEP_BITS_PRESSED = 0x2U,
	MOVESTEP_BITS_WHEN = 0x4U,
	MOVESTEP_BITS_ANALOG_FORWARD_DELTA = 0x8U,
	MOVESTEP_BITS_ANALOG_LEFT_DELTA = 0x10U
};

enum EInputHistoryBits : std::uint32_t
{
	INPUT_HISTORY_BITS_VIEWANGLES = 0x1U,
	INPUT_HISTORY_BITS_SHOOTPOSITION = 0x2U,
	INPUT_HISTORY_BITS_TARGETHEADPOSITIONCHECK = 0x4U,
	INPUT_HISTORY_BITS_TARGETABSPOSITIONCHECK = 0x8U,
	INPUT_HISTORY_BITS_TARGETANGCHECK = 0x10U,
	INPUT_HISTORY_BITS_CL_INTERP = 0x20U,
	INPUT_HISTORY_BITS_SV_INTERP0 = 0x40U,
	INPUT_HISTORY_BITS_SV_INTERP1 = 0x80U,
	INPUT_HISTORY_BITS_PLAYER_INTERP = 0x100U,
	INPUT_HISTORY_BITS_RENDERTICKCOUNT = 0x200U,
	INPUT_HISTORY_BITS_RENDERTICKFRACTION = 0x400U,
	INPUT_HISTORY_BITS_PLAYERTICKCOUNT = 0x800U,
	INPUT_HISTORY_BITS_PLAYERTICKFRACTION = 0x1000U,
	INPUT_HISTORY_BITS_FRAMENUMBER = 0x2000U,
	INPUT_HISTORY_BITS_TARGETENTINDEX = 0x4000U
};

enum EButtonStatePBBits : uint32_t
{
	BUTTON_STATE_PB_BITS_BUTTONSTATE1 = 0x1U,
	BUTTON_STATE_PB_BITS_BUTTONSTATE2 = 0x2U,
	BUTTON_STATE_PB_BITS_BUTTONSTATE3 = 0x4U
};

enum EBaseCmdBits : std::uint32_t
{
	BASE_BITS_MOVE_CRC = 0x1U,
	BASE_BITS_BUTTONPB = 0x2U,
	BASE_BITS_VIEWANGLES = 0x4U,
	BASE_BITS_COMMAND_NUMBER = 0x8U,
	BASE_BITS_CLIENT_TICK = 0x10U,
	BASE_BITS_FORWARDMOVE = 0x20U,
	BASE_BITS_LEFTMOVE = 0x40U,
	BASE_BITS_UPMOVE = 0x80U,
	BASE_BITS_IMPULSE = 0x100U,
	BASE_BITS_WEAPON_SELECT = 0x200U,
	BASE_BITS_RANDOM_SEED = 0x400U,
	BASE_BITS_MOUSEDX = 0x800U,
	BASE_BITS_MOUSEDY = 0x1000U,
	BASE_BITS_CONSUMED_SERVER_ANGLE = 0x2000U,
	BASE_BITS_CMD_FLAGS = 0x4000U,
	BASE_BITS_ENTITY_HANDLE = 0x8000U
};

enum ECSGOUserCmdBits : std::uint32_t
{
	CSGOUSERCMD_BITS_BASECMD = 0x1U,
    CSGOUSERCMD_BITS_LEFTHAND = 0x2U,
    CSGOUSERCMD_BITS_PREDICTING_BODY_SHOT = 0x4U,
    CSGOUSERCMD_BITS_PREDICTING_HEAD_SHOT = 0x8U,
    CSGOUSERCMD_BITS_PREDICTING_KILL_RAGDOLLS = 0x10U,
    CSGOUSERCMD_BITS_ATTACK3START = 0x20U,
    CSGOUSERCMD_BITS_ATTACK1START = 0x40U,
    CSGOUSERCMD_BITS_ATTACK2START = 0x80U
};

template <typename T>
struct RepeatedPtrField_t
{
	struct Rep_t
	{
		int nAllocatedSize;
		T* tElements[(std::numeric_limits<int>::max() - 2 * sizeof(int)) / sizeof(void*)];
	};

	void* pArena;
	int nCurrentSize;
	int nTotalSize;
	Rep_t* pRep;

	template <typename T>
	T* add(T* element)
	{
		// Define the function pointer correctly
		static auto add_to_rep_addr = reinterpret_cast<T*(__fastcall*)(RepeatedPtrField_t*, T*)>(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 4C 8B E0 48 8B 44 24 ? 4C 8B CF"), 0x1));

		// Use the function pointer to call the function
		return add_to_rep_addr(this, element);
	}
};

class CBasePB
{
public:
	MEM_PAD(0x8) // 0x0 VTABLE
	std::uint32_t nHasBits; // 0x8
	std::uint64_t nCachedBits; // 0xC

	void SetBits(std::uint64_t nBits)
	{
		// @note: you don't need to check if the bits are already set as bitwise OR will not change the value if the bit is already set
		nCachedBits |= nBits;
	}
};

static_assert(sizeof(CBasePB) == 0x18);

class CMsgQAngle : public CBasePB
{
public:
	QAngle_t angValue; // 0x18
};

static_assert(sizeof(CMsgQAngle) == 0x28);

class CMsgVector : public CBasePB
{
public:
	Vector4D_t vecValue; // 0x18
};

static_assert(sizeof(CMsgVector) == 0x28);



class CCSGOInterpolationInfoPB : public CBasePB
{
public:
	float flFraction; // 0x18
	int nSrcTick; // 0x1C
	int nDstTick; // 0x20
};

static_assert(sizeof(CCSGOInterpolationInfoPB) == 0x28);

class CCSGOInterpolationInfoPB_CL : public CBasePB
{
public:
	float flFraction; // 0x18
};

static_assert(sizeof(CCSGOInterpolationInfoPB_CL) == 0x20);



class CCSGOInputHistoryEntryPB : public CBasePB
{
public:
	CMsgQAngle* pViewAngles; // 0x18
	CMsgVector* pShootPosition; // 0x20
	CMsgVector* pTargetHeadPositionCheck; // 0x28
	CMsgVector* pTargetAbsPositionCheck; // 0x30
	CMsgQAngle* pTargetAngPositionCheck; // 0x38
	CCSGOInterpolationInfoPB_CL* cl_interp; // 0x40
	CCSGOInterpolationInfoPB* sv_interp0; // 0x48
	CCSGOInterpolationInfoPB* sv_interp1; // 0x50
	CCSGOInterpolationInfoPB* player_interp; // 0x58
	int nRenderTickCount; // 0x60
	float flRenderTickFraction; // 0x64
	int nPlayerTickCount; // 0x68
	float flPlayerTickFraction; // 0x6C
	int nFrameNumber; // 0x70
	int nTargetEntIndex; // 0x74

	/*CMsgQAngle* CreateMsgQAngle()
	{
		this->nCachedBits |= 1;

		auto a1 = (DWORD64)(nHasBits & 0xFFFFFFFFFFFFFFFC);

		if ((nHasBits & 1) != 0)
			a1 = (DWORD64)a1;
		using fn = CMsgQAngle*(__fastcall)(DWORD64);

		static auto fncreate = reinterpret_cast<fn*>(GetSigPtr(FNV1A::HashConst("CREATE_MSG_ANGLE")));
		return fncreate(a1);
	}*/

	CCSGOInterpolationInfoPB_CL* create_new_interp_cl()
	{
		this->nCachedBits |= 1;

		auto v14 = nCachedBits & 0xFFFFFFFFFFFFFFFC;

		if ((nCachedBits & 1) != 0)
			v14 = (unsigned __int64)(nCachedBits & 0xFFFFFFFFFFFFFFFC);

		//  client.dll  E8 ? ? ? ? 48 89 43 ? 8B 45
		using fn = CCSGOInterpolationInfoPB_CL*(__fastcall)(__int64);
		static auto fncreate = reinterpret_cast<fn*>(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 48 89 47 ? 48 8B 57 ? 4C 8B C3 48 8B CD E8 ? ? ? ? 48 8B D8 48 85 C0 0F 85 ? ? ? ? 49 8B DC E9 ? ? ? ? 40 84 F6"), 1, 0));
		return fncreate(v14);
	}


	CCSGOInterpolationInfoPB* create_new_interp()
	{
		this->nHasBits |= 1;

		auto v14 = nHasBits & 0xFFFFFFFFFFFFFFFC;

		 if ((nHasBits & 1) != 0)
			v14 = (unsigned __int64)(nHasBits & 0xFFFFFFFFFFFFFFFC);

		//  client.dll  E8 ? ? ? ? 48 89 43 ? 8B 45
		using fn = CCSGOInterpolationInfoPB*(__fastcall)(__int64);
		static auto fncreate = reinterpret_cast<fn*>(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 48 89 47 ? 48 8B 57 ? 4C 8B C3 48 8B CD E8 ? ? ? ? 48 8B D8 48 85 C0 0F 85 ? ? ? ? 49 8B DC E9 ? ? ? ? 40 84 F6"), 1, 0));
		return fncreate(v14);
	}
};

static_assert(sizeof(CCSGOInputHistoryEntryPB) == 0x78);




struct CInButtonStatePB : CBasePB
{
	std::uint64_t nValue;
	std::uint64_t nValueChanged;
	std::uint64_t nValueScroll;
};

static_assert(sizeof(CInButtonStatePB) == 0x30);

struct CSubtickMoveStep : CBasePB
{
public:
	std::uint64_t nButton;
	bool bPressed;
	float flWhen;
	float flAnalogForwardDelta;
	float flAnalogLeftDelta;

	//
	std::int16_t unk;
	std::int8_t unk2;
};

//static_assert(sizeof(CSubtickMoveStep) == 0x30);

class CBaseUserCmdPB : public CBasePB
{
public:
	RepeatedPtrField_t<CSubtickMoveStep> subtickMovesField;
	std::string* strMoveCrc;
	CInButtonStatePB* pInButtonState;
	CMsgQAngle* pViewAngles;
	std::int32_t nLegacyCommandNumber;
	std::int32_t nClientTick;
	float flForwardMove;
	float flSideMove;
	float flUpMove;
	std::int32_t nImpulse;
	std::int32_t nWeaponSelect;
	std::int32_t nRandomSeed;
	std::int32_t nMousedX;
	std::int32_t nMousedY;
	std::uint32_t nConsumedServerAngleChanges;
	std::int32_t nCmdFlags;
	std::uint32_t nPawnEntityHandle;

	CSubtickMoveStep* add_subtick_move()
	{
		using fn_add_subtick_move_step = CSubtickMoveStep*(__fastcall*)(void*);
		static fn_add_subtick_move_step fn_create_new_subtick_move_step = reinterpret_cast<fn_add_subtick_move_step>(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 48 8B D0 48 8D 4F 18 E8 ? ? ? ? 48 8B D0"), 0x1));

		if (subtickMovesField.pRep && subtickMovesField.nCurrentSize < subtickMovesField.pRep->nAllocatedSize)
			return subtickMovesField.pRep->tElements[subtickMovesField.nCurrentSize++];

		CSubtickMoveStep* subtick = fn_create_new_subtick_move_step(nullptr);
		subtickMovesField.add(subtick);

		return subtick;
	}

	int CalculateCmdCRCSize()
	{
		return MEM::CallVFunc<int, 7U>(this);
	}
};

static_assert(sizeof(CBaseUserCmdPB) == 0x80);

class CCSGOUserCmdPB
{
public:
	uint32_t nHasBits;
	uint64_t nCachedSize;
	RepeatedPtrField_t<CCSGOInputHistoryEntryPB> inputHistoryField;
	CBaseUserCmdPB* pBaseCmd;
	bool bLeftHandDesired;
	bool m_bIsPredictingBodyShotFX;
	bool m_bIsPredictingHeadShotFX;
	bool m_bIsPredictingKillRagdolls;
	int nAttack3StartHistoryIndex;
	int nAttack1StartHistoryIndex;
	int nAttack2StartHistoryIndex;

	// @note: this function is used to check if the bits are set and set them if they are not
	void CheckAndSetBits(std::uint32_t nBits)
	{
		/*if (!(nHasBits & nBits))
			nHasBits |= nBits;*/

		if (!(nCachedSize & nBits))
			nCachedSize |= nBits;
	}
};
static_assert(sizeof(CCSGOUserCmdPB) == 0x40);

struct CInButtonState
{
public:
	MEM_PAD(0x8) // 0x0 VTABLE
	std::uint64_t nValue; // 0x8
	std::uint64_t nValueChanged; // 0x10
	std::uint64_t nValueScroll; // 0x18
};
static_assert(sizeof(CInButtonState) == 0x20);

class CUserCmd
{
public:
	MEM_PAD(0x8); // 0x0 VTABLE
	MEM_PAD(0x10); // TODO: find out what this is, added 14.08.2024
	CCSGOUserCmdPB csgoUserCmd; // 0x18
	CInButtonState nButtons; // 0x58
	MEM_PAD(0x10);
	bool bHasBeenPredicted;
	MEM_PAD(0xF);


	CCSGOInputHistoryEntryPB* CreateNewInputHistory(RepeatedPtrField_t<CCSGOInputHistoryEntryPB> rept_ptr, void* arena)
	{
		using fnCreateNewInputHistory = CCSGOInputHistoryEntryPB*(CS_FASTCALL*)(void*);
		static auto CreateNewInputHistory = reinterpret_cast<fnCreateNewInputHistory>(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 48 8B D0 48 8D 4E ? E8 ? ? ? ? 4C 8B F8"), 0x1)); // E8 ? ? ? ? 48 8B D0 49 8D 4E ? E8 ? ? ? ? 4C 8B E0

		auto InputHistory = CreateNewInputHistory(arena);

		using fnAddElementToRepFieldContainer = CCSGOInputHistoryEntryPB*(CS_FASTCALL*)(RepeatedPtrField_t<CCSGOInputHistoryEntryPB>&, void*);
		static auto AddElementToRepFieldContainer = reinterpret_cast<fnAddElementToRepFieldContainer>(MEM::GetAbsoluteAddress(MEM::FindPattern(CLIENT_DLL, "E8 ? ? ? ? 4C 8B F8 48 8B 44 24 ? 4C 8B CF 48 89 44 24 ? 45 0F B6 C4 49 8B D7"), 0x1)); // E8 ? ? ? ? 4C 8B E0 48 8B 44 24 ? 4C 8B CF

		return AddElementToRepFieldContainer(rept_ptr, InputHistory);
	}

	CCSGOInputHistoryEntryPB* AddInputHistory()
	{
		auto ReptFieldInputHistory = this->csgoUserCmd.inputHistoryField;
		CCSGOInputHistoryEntryPB* InputHistoryEntry = nullptr;

		if (ReptFieldInputHistory.pRep && ReptFieldInputHistory.nCurrentSize < ReptFieldInputHistory.pRep->nAllocatedSize)
			InputHistoryEntry = ReptFieldInputHistory.pRep->tElements[ReptFieldInputHistory.nCurrentSize++];
		else
			InputHistoryEntry = CreateNewInputHistory(ReptFieldInputHistory, ReptFieldInputHistory.pArena);

		return InputHistoryEntry;
	}


	CCSGOInputHistoryEntryPB* GetInputHistoryEntry(int nIndex)
	{
		if (!csgoUserCmd.inputHistoryField.pRep)
			return nullptr;

		if (nIndex >= csgoUserCmd.inputHistoryField.pRep->nAllocatedSize || nIndex >= csgoUserCmd.inputHistoryField.nCurrentSize)
			return nullptr;

		return csgoUserCmd.inputHistoryField.pRep->tElements[nIndex];
	}

	void SetSubTickAngle(const QAngle_t& angView)
	{
		if (!csgoUserCmd.inputHistoryField.pRep)
			return;

		for (int i = 0; i < this->csgoUserCmd.inputHistoryField.pRep->nAllocatedSize; i++)
		{
			CCSGOInputHistoryEntryPB* pInputEntry = this->GetInputHistoryEntry(i);
			if (!pInputEntry)
				pInputEntry = this->AddInputHistory();

			if (!pInputEntry)
				continue;

			if (!pInputEntry->pViewAngles)
				continue;

			pInputEntry->pViewAngles->angValue = angView;
			pInputEntry->SetBits(EInputHistoryBits::INPUT_HISTORY_BITS_VIEWANGLES);
		}
	}

	void SetAttackHistory(int index)
	{
		this->csgoUserCmd.nAttack3StartHistoryIndex = index;
		this->csgoUserCmd.CheckAndSetBits(ECSGOUserCmdBits::CSGOUSERCMD_BITS_ATTACK3START);

		this->csgoUserCmd.nAttack1StartHistoryIndex = index;
		this->csgoUserCmd.CheckAndSetBits(ECSGOUserCmdBits::CSGOUSERCMD_BITS_ATTACK1START);

		this->csgoUserCmd.nAttack2StartHistoryIndex = index;
		this->csgoUserCmd.CheckAndSetBits(ECSGOUserCmdBits::CSGOUSERCMD_BITS_ATTACK2START);
	}

	CSubtickMoveStep* CreateSubtickStep(void* arena)
	{
		using fnCreateSubtickStep = CSubtickMoveStep*(CS_FASTCALL*)(void*);
		//static auto fn = reinterpret_cast<fnCreateSubtickStep>(MEM::FindPattern(CLIENT_DLL, CS_XOR("48 89 5C 24 10 57 48 83 EC 20 33 FF 48 8B D9 48 85 C9 75 21 8D 4B 30 E8 ?? ?? ?? ?? 48 85 C0 74 06 48 89 78 08 EB 23 48 8B C7 48 8B 5C 24 38 48 83 C4 20 5F C3 4C 8D 05 ?? ?? ?? ?? BA 30 00 00 00 E8 ?? ?? ?? ?? 48 89 58 08 48 8B 5C 24 38 48 8D 0D ?? ?? ?? ?? 48 89 08 33 C9 48 89 48 10 89 78 2C")));
		static auto fn = reinterpret_cast<fnCreateSubtickStep>(MEM::FindPattern(CLIENT_DLL, CS_XOR("40 53 48 83 EC ? 48 8B D9 48 85 C9 75 ? 8D 4B ? E8 ? ? ? ? 48 85 C0 74 ? 33 D2 45 33 C0 48 8B C8 E8 ? ? ? ? 48 83 C4 ? 5B C3 48 83 C4 ? 5B C3 4C 8D 05 ? ? ? ? BA ? ? ? ? E8 ? ? ? ? 48 8B D3 45 33 C0 48 8B C8 E8 ? ? ? ? 48 83 C4 ? 5B C3 CC CC CC CC CC CC CC 48 89 5C 24 ? 57 48 83 EC ? 33 FF 48 8B D9 48 85 C9 75 ? 8D 4B ? E8 ? ? ? ? 48 85 C0 74 ? 48 89 78 ? EB ? 48 8B C7 48 8B 5C 24 ? 48 83 C4 ? 5F C3 4C 8D 05 ? ? ? ? BA ? ? ? ? E8 ? ? ? ? 48 89 58 ? 48 8B 5C 24 ? 48 8D 0D ? ? ? ? 48 89 08 33 C9 48 89 48 ? 48 89 78 ? 48 83 C4 ? 5F C3 CC CC CC CC CC CC CC 48 89 5C 24 ? 56")));

		return fn(arena);
	}

	CSubtickMoveStep* CreateSubtick()
	{
		if (!csgoUserCmd.pBaseCmd)
			return nullptr;

		if (csgoUserCmd.pBaseCmd->subtickMovesField.pRep && (csgoUserCmd.pBaseCmd->subtickMovesField.nCurrentSize < csgoUserCmd.pBaseCmd->subtickMovesField.pRep->nAllocatedSize))
			return csgoUserCmd.pBaseCmd->subtickMovesField.pRep->tElements[csgoUserCmd.pBaseCmd->subtickMovesField.nCurrentSize++];

		auto new_subtick = CreateSubtickStep(csgoUserCmd.pBaseCmd->subtickMovesField.pArena);

		using fnCreateSubtickStep = CSubtickMoveStep*(CS_FASTCALL*)(RepeatedPtrField_t<CSubtickMoveStep>&, CSubtickMoveStep*);
		static auto fn = reinterpret_cast<fnCreateSubtickStep>(MEM::FindPattern(CLIENT_DLL, CS_XOR("48 89 5C 24 08 57 48 83 EC 20 48 8B D9 48 8B FA 48 8B 49 10"))); // E8 ? ? ? ? 48 8B D0 48 8D 4B ? E8 ? ? ? ? 48 8B D0

		return fn(csgoUserCmd.pBaseCmd->subtickMovesField, new_subtick);
	}

	void AddSubtick(bool pressed, int button, float when=0.999f)
	{
		auto subtick = CreateSubtick();

		if (!subtick)
			return;

		subtick->nCachedBits = 7;
		subtick->nButton = button;
		subtick->bPressed = pressed;
		subtick->unk = 4;
		subtick->flWhen = when;
	}

	void AdjustAttackStartIndex(int nTick)
	{
		this->csgoUserCmd.nAttack1StartHistoryIndex = nTick;
		this->csgoUserCmd.nAttack2StartHistoryIndex = nTick;
		this->csgoUserCmd.nAttack3StartHistoryIndex = nTick;
	}
};
static_assert(sizeof(CUserCmd) == 0x98);



class CCSInputMessage
{
public:
	int32_t m_frame_tick_count; //0x0000
	float m_frame_tick_fraction; //0x0004
	int32_t m_player_tick_count; //0x0008
	float m_player_tick_fraction; //0x000C
	Vector_t m_view_angles; //0x0010
	Vector_t m_shoot_position; //0x001C
	int32_t m_target_index; //0x0028
	Vector_t m_target_head_position; //0x002C
	Vector_t m_target_abs_origin; //0x0038
	Vector_t m_target_angle; //0x0044
	int32_t m_sv_show_hit_registration; //0x0050
	int32_t m_entry_index_max; //0x0054
	int32_t m_index_idk; //0x0058
}; //Size: 0x005C
