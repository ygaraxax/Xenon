#pragma once

// used: mem_pad
#include "../../utilities/memory.h"

class IGlobalVars
{
public:
	float flRealTime; //0x0000
	int32_t nFrameCount; //0x0004
	float flFrameTime; //0x0008
	float flFrameTime2; //0x000C
	int32_t nMaxClients; //0x0010
	char pad_0014[28]; //0x0014
	float flIntervalPerTick; //0x0030
	float flCurrentTime; //0x0034
	float flCurrentTime2; //0x0038
	char pad_003C[20]; //0x003C
	int32_t nTickCount; //0x0048
	char pad_0054[292]; //0x0054
	uint64_t uCurrentMap; //0x0178
	uint64_t uCurrentMapName; //0x0180

	//float flRealTime; //0x0000
	//int32_t nFrameCount; //0x0004
	//float flFrameTime; //0x0008
	//float flFrameTime2; //0x000C
	//int32_t nMaxClients; //0x0010
	//MEM_PAD(0x1C);
	//float flFrameTime3; //0x0030
	//float flCurrentTime; //0x0034
	//float flCurrentTime2; //0x0038
	//MEM_PAD(0xC);
	//int32_t nTickCount; //0x0048
};



class CGlobalVarsBase
{
public:
	float m_flRealTime; //0x0000
	int32_t m_iFrameCount; //0x0004
	float m_flAbsoluteFrameTime; //0x0008
	float m_flAbsoluteFrameStartTimeStdDev; //0x000C
	int32_t m_nMaxClients; //0x0010
	char pad_0014[28]; //0x0014
	float m_flIntervalPerTick; //0x0030
	float m_flCurrentTime; //0x0034
	float m_flCurrentTime2; //0x0038
	char pad_003C[20]; //0x003C
	int32_t m_nTickCount; //0x0050
	char pad_0054[292]; //0x0054
	uint64_t m_uCurrentMap; //0x0178
	uint64_t m_uCurrentMapName; //0x0180
}; //Size: 0x0188
