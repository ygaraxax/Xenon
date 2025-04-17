#pragma once

// used: callvfunc
#include "../../utilities/memory.h"

enum EClientFrameStage : int
{
	FRAME_UNDEFINED = -1,
	FRAME_START,
	// a network packet is being received
	FRAME_NET_UPDATE_START,
	// data has been received and we are going to start calling postdataupdate
	FRAME_NET_UPDATE_POSTDATAUPDATE_START,
	// data has been received and called postdataupdate on all data recipients
	FRAME_NET_UPDATE_POSTDATAUPDATE_END,
	// received all packets, we can now do interpolation, prediction, etc
	FRAME_NET_UPDATE_END,
	// start rendering the scene
	FRAME_RENDER_START,
	// finished rendering the scene
	FRAME_RENDER_END,
	FRAME_NET_FULL_FRAME_UPDATE_ON_REMOVE
};


#include "../datatypes/vector.h"
// used: color_t
#include "../datatypes/color.h"

enum flow : int
{
	FLOW_OUTGOING = 0,
	FLOW_INCOMING = 1,
};



class IDebugOverlayGameSystem
{
public:
	void add_box(const Vector_t& end, const Vector_t& mins, const Vector_t& maxs, const Vector_t& a3, Color_t clr, double time)
	{
		using function_t = void(__thiscall*)(IDebugOverlayGameSystem*, const Vector_t&, const Vector_t&, const Vector_t&, const Vector_t&, int, int, int, int, double);
		(*reinterpret_cast<function_t**>(std::uintptr_t(this)))[48](this, end, mins, maxs, a3, clr.r, clr.g, clr.b, clr.a, static_cast<double>(time));
	}
};

class ISource2Client
{
public:
	IDebugOverlayGameSystem* GetSceneDebugOverlay()
	{
		using function_t = IDebugOverlayGameSystem*(__thiscall*)(ISource2Client*);
		return (*reinterpret_cast<function_t**>(std::uintptr_t(this)))[163](this);
	}
};

class INetChannelInfo
{
public:
	float get_latency(flow flow)
	{
		return MEM::CallVFunc<float, 10U>(this, flow);  // 10
		//return MEM::CallVFunc<int, 10U>(this, flow);  // 10
	}
};


class CNetworkedClientInfo
{
	std::byte pad_001[0x4];

public:
	int m_render_tick;
	float m_render_tick_fraction;
	int m_player_tick_count;
	float m_player_tick_fraction;

private:
	std::byte pad_002[0x4];

public:
	struct
	{
	private:
		std::byte pad_022[0xC];

	public:
		Vector_t m_eye_pos;
	}* m_local_data;

private:
	std::byte pad_003[0x8];
};



class IEngineClient
{
public:
	int GetMaxClients()
	{
		return MEM::CallVFunc<int, 34U>(this);
	}

	bool IsInGame()
	{
		return MEM::CallVFunc<bool, 35U>(this);
	}

	bool IsConnected()
	{
		return MEM::CallVFunc<bool, 36U>(this);
	}

	INetChannelInfo* GetNetChannelInfo(int split_screen_slot)
	{
		return MEM::CallVFunc<INetChannelInfo*, 34U>(this, split_screen_slot);
	}


	CNetworkedClientInfo* get_networked_client_info()
	{

		CNetworkedClientInfo client_info;

		MEM::CallVFunc<void*, 176U>(this, &client_info);

		return &client_info;
	}


	// return CBaseHandle index
	int GetLocalPlayer()
	{
		int nIndex = -1;

		MEM::CallVFunc<void, 49U>(this, std::ref(nIndex), 0);

		return nIndex + 1;
	}

	[[nodiscard]] const char* GetLevelName()
	{
		return MEM::CallVFunc<const char*, 56U>(this);
	}

	[[nodiscard]] const char* GetLevelNameShort()
	{
		return MEM::CallVFunc<const char*, 57U>(this);
	}

	[[nodiscard]] const char* GetProductVersionString()
	{
		return MEM::CallVFunc<const char*, 84U>(this);
	}
};
