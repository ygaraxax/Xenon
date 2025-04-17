#pragma once
#include "../../utilities/memory.h"

enum flow;

class c_net_chan
{
public:
	void send_net_msg(void* message_handle, void* message_data, int unk)
	{
		// xref: SetSignonState
		MEM::CallVFunc<void, 60U>(this, message_handle, message_data, unk);
	}

	float get_latency(flow flow)
	{
		return MEM::CallVFunc<float, 10U>(this, flow);
	}

	float get_network_latency()
	{
		// xref: StartLagcompensation
		return MEM::CallVFunc<float, 10U>(this);
	}

	float get_engine_latency()
	{
		// xref: StartLagcompensation
		return MEM::CallVFunc<float, 11U>(this);
	}

	float get_avg_latency(int type)
	{
		// xref: %4.0f ms : %s\n
		return MEM::CallVFunc<float, 11U>(this, type);
	}
};

class CNetworkGameClient
{
public:
	bool IsConnected()
	{
		return MEM::CallVFunc<bool, 12U>(this);
	}

	// force game to clear cache and reset delta tick
	void FullUpdate()
	{
		// @ida: #STR: "Requesting full game update (%s)...\n"
		MEM::CallVFunc<void, 28U>(this, CS_XOR("unk"));
	}

	int GetDeltaTick()
	{
		// @ida: offset in FullUpdate();
		// (nDeltaTick = -1) == FullUpdate() called
		return *reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(this) + 0x25C);
	}

	bool get_prediction()
	{
		return *(bool*)(std::uintptr_t(this) + 0xD4);
	}

	void set_prediction(bool value)
	{
		*(bool*)(std::uintptr_t(this) + 0xD4) = value;
	}

	c_net_chan* get_net_channel2()
	{
		return *(c_net_chan**)(std::uintptr_t(this) + 0xE8);
	}

	[[nodiscard]] float GetClientInterpAmount()
	{
		return MEM::CallVFunc<float, 61U>(this);
	}
};

class INetworkClientService
{
public:
	[[nodiscard]] CNetworkGameClient* GetNetworkGameClient()
	{
		return MEM::CallVFunc<CNetworkGameClient*, 23U>(this);
	}
};
