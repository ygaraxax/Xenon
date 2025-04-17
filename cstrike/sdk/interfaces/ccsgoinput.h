#pragma once

// used: mem_pad
#include "../../utilities/memory.h"

// used: cusercmd
#include "../datatypes/usercmd.h"

#define MULTIPLAYER_BACKUP 150

class CTinyMoveStepData
{
public:
	float flWhen; //0x0000
	MEM_PAD(0x4); //0x0004
	std::uint64_t nButton; //0x0008
	bool bPressed; //0x0010
	MEM_PAD(0x7); //0x0011
}; //Size: 0x0018

class CMoveStepButtons
{
public:
	std::uint64_t nKeyboardPressed; //0x0000
	std::uint64_t nMouseWheelheelPressed; //0x0008
	std::uint64_t nUnPressed; //0x0010
	std::uint64_t nKeyboardCopy; //0x0018
}; //Size: 0x0020

// @credits: www.unknowncheats.me/forum/members/2943409.html
class CExtendedMoveData : public CMoveStepButtons
{
public:
	float flForwardMove; //0x0020
	float flSideMove; //0x0024
	float flUpMove; //0x0028
	std::int32_t nMouseDeltaX; //0x002C
	std::int32_t nMouseDeltaY; //0x0030
	std::int32_t nAdditionalStepMovesCount; //0x0034
	CTinyMoveStepData tinyMoveStepData[12]; //0x0038
	Vector_t vecViewAngle; //0x0158
	std::int32_t nTargetHandle; //0x0164
}; //Size:0x0168







class C_CSInputMessage
{
public:
	int32_t RenderTickCount; //0x0000
	float RenderFraction; //0x0004
	int32_t PlayerTickCount; //0x0008
	float PlayerFraction; //0x000C
	Vector_t Angle; //0x0010
	Vector_t Position; //0x001C
	char pad_0028[40]; //0x0028
	int32_t N000005D8; //0x0050
	char pad_0054[4]; //0x0054
	int32_t N000005D9; //0x0058
	int32_t N000005E5; //0x005C
}; //Size: 0x0060


class CSubtickInput
{
public:
	uint64_t nButton; // 0x18
	bool bPressed; // 0x20
	float flWhen; // 0x24
	float flAnalogForwardDelta; // 0x28
	float flAnalogLeftDelta; // 0x2C
};


class CCSGOInput
{
public:
	char pad_0000[592]; //0x0000
	bool bBlockShot; //0x0250
	bool bInThirdPerson; //0x0251
	char pad_0252[6]; //0x0252
	QAngle_t angThirdPersonAngles; //0x0258
	char pad_0264[20]; //0x0264
	uint64_t nKeyboardPressed; //0x0278
	uint64_t nMouseWheelheelPressed; //0x0280
	uint64_t nUnPressed; //0x0288
	uint64_t nKeyboardCopy; //0x0290
	float flForwardMove; //0x0298
	float flSideMove; //0x029C
	float flUpMove; //0x02A0
	Vector2D_t nMousePos; //0x02A4
	int32_t SubticksCount; //0x02AC
	//CExtendedMoveData CurrentMoveData; //0x02B0
	CSubtickInput Subticks[12]; //0x02B0
	Vector_t vecViewAngle; //0x03D0
	int32_t nTargetHandle; //0x03DC
	char pad_03E0[560]; //0x03E0
	int32_t nAttackStartHistoryIndex1; //0x0610
	int32_t nAttackStartHistoryIndex2; //0x0614
	int32_t nAttackStartHistoryIndex3; //0x0618
	char pad_061C[4]; //0x061C
	int32_t MessageSize; //0x0620
	char pad_0624[4]; //0x0624
	C_CSInputMessage* Message; //0x0628



	void SetAttackHistory(int index)
	{
		this->nAttackStartHistoryIndex1 = index;
		this->nAttackStartHistoryIndex2 = index;
		this->nAttackStartHistoryIndex3 = index;
	}


	void SetViewAngle(QAngle_t& angView)
	{
		// @ida: this got called before GetMatricesForView
		using fnSetViewAngle = std::int64_t(CS_FASTCALL*)(void*, std::int32_t, QAngle_t&);
		static auto oSetViewAngle = reinterpret_cast<fnSetViewAngle>(MEM::FindPattern(CLIENT_DLL, CS_XOR("85 D2 75 ? 48 63 81")));

		#ifdef CS_PARANOID
		//CS_ASSERT(oSetViewAngle != nullptr);
		#endif

		oSetViewAngle(this, 0, std::ref(angView));
	}

	QAngle_t GetViewAngles()
	{
		using fnGetViewAngles = std::int64_t(CS_FASTCALL*)(CCSGOInput*, std::int32_t);
		static auto oGetViewAngles = reinterpret_cast<fnGetViewAngles>(MEM::FindPattern(CLIENT_DLL, CS_XOR("4C 8B C1 85 D2 74 08 48 8D 05 ? ? ? ? C3")));

		#ifdef CS_PARANOID
		//CS_ASSERT(oGetViewAngles != nullptr);
		#endif

		return *reinterpret_cast<QAngle_t*>(oGetViewAngles(this, 0));
	}
};
