#pragma once

// used: call virtual function
#include "../../utilities/memory.h"

// used: color_t
#include "../datatypes/color.h"
// used: stronghandle
#include "../datatypes/stronghandle.h"
// used: keyvalue3
#include "../datatypes/keyvalue3.h"
// used vector4d_t
#include "../datatypes/vector.h"

// used: cbasehandle
#include "../entity_handle.h"

#define MATERIAL_FIND_PARAMETER_SIG CS_XOR("E8 ? ? ? ? 4C 8B E8 48 85 C0 0F 84 ? ? ? ?")
#define MATERIAL_UPDATE_PARAMETER_SIG CS_XOR("48 89 7C 24 ? 41 56 48 83 EC ? 8B 81") // CS_XOR("40 56 41 54 48 83 EC")

class material_paramater;

class CMaterial2
{
public:
	virtual const char* GetName() = 0;
	virtual const char* GetShareName() = 0;

	material_paramater* FindParameter(const char* name)
	{
		using fn_t = material_paramater*(__fastcall*)(CMaterial2 * _this, const char* name);
		static fn_t fn = (fn_t)MEM::GetAbsoluteAddress(MEM::FindPattern(MATERIAL_SYSTEM2_DLL, MATERIAL_FIND_PARAMETER_SIG), 1, 0);
		return fn(this, name);
	}

	void UpdateParameter()
	{
		using fn_t = void(__fastcall*)(CMaterial2* _this);
		static fn_t fn = (fn_t)MEM::FindPattern(MATERIAL_SYSTEM2_DLL, MATERIAL_UPDATE_PARAMETER_SIG);
		return fn(this);
	}
};

// idk
struct MaterialKeyVar_t
{
	std::uint64_t uKey;
	const char* szName;

	MaterialKeyVar_t(std::uint64_t uKey, const char* szName) :
		uKey(uKey), szName(szName) { }

	MaterialKeyVar_t(const char* szName, bool bShouldFindKey = false) :
		szName(szName)
	{
		uKey = bShouldFindKey ? FindKey(szName) : 0x0;
	}

	std::uint64_t FindKey(const char* szName)
	{
		using fnFindKeyVar = std::uint64_t(CS_FASTCALL*)(const char*, unsigned int, int);
		static auto oFindKeyVar = reinterpret_cast<fnFindKeyVar>(MEM::FindPattern(PARTICLES_DLL, CS_XOR("48 89 5C 24 ? 57 48 81 EC ? ? ? ? 33 C0 8B DA")));


		// idk those enum flags, just saw it called like that soooo yea
		return oFindKeyVar(szName, 0x12, 0x31415926);
	}
};



struct ResourceArray_t
{
	uint64_t m_nCount;
	CMaterial2*** m_aResources;
	uint64_t pad_0010[3];
};



class CObjectInfo
{
private:
	MEM_PAD(0xB0);

public:
	int nId;
};

class CSceneAnimatableObject
{
private:
	MEM_PAD(0xB8);

public:
	CBaseHandle hOwner;
};

// the naming is incorrect but i dont care atm
class CMeshData
{
public:
	void SetShaderType(const char* szShaderName)
	{
		// @ida: #STR: shader, spritecard.vfx
		using fnSetMaterialShaderType = void(CS_FASTCALL*)(void*, MaterialKeyVar_t, const char*, int);
		static auto oSetMaterialShaderType = reinterpret_cast<fnSetMaterialShaderType>(MEM::FindPattern(PARTICLES_DLL, CS_XOR("48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC ? 0F B6 01 45 0F B6 F9 8B 2A 4D 8B E0 4C 8B 72 ? 48 8B F9 C0 E8 ? 24 ? 3C ? 74 ? 41 B0 ? B2 ? E8 ? ? ? ? 0F B6 07 33 DB C0 E8 ? 24 ? 3C ? 75 ? 48 8B 77 ? EB ? 48 8B F3 4C 8D 44 24 ? C7 44 24 ? ? ? ? ? 48 8D 54 24 ? 89 6C 24 ? 48 8B CE 4C 89 74 24 ? E8 ? ? ? ? 8B D0 83 F8 ? 75 ? 45 33 C9 89 6C 24 ? 4C 8D 44 24 ? 4C 89 74 24 ? 48 8B D7 48 8B CE E8 ? ? ? ? 8B D0 0F B6 0F C0 E9 ? 80 E1 ? 80 F9 ? 75 ? 48 8B 4F ? EB ? 48 8B CB 8B 41 ? 85 C0 74 ? 48 8D 59 ? 83 F8 ? 76 ? 48 8B 1B 48 63 C2 4D 85 E4")));

		MaterialKeyVar_t shaderVar(0x162C1777, CS_XOR("shader"));
		oSetMaterialShaderType(this, shaderVar, szShaderName, 0x18);
	}

	void SetMaterialFunction(const char* szFunctionName, int nValue)
	{
		using fnSetMaterialFunction = void(__fastcall*)(void*, MaterialKeyVar_t, int, int);
		static auto oSetMaterialFunction = reinterpret_cast<fnSetMaterialFunction>(MEM::FindPattern(PARTICLES_DLL, CS_XOR("48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC ? 0F B6 01 45 0F B6 F9 8B 2A 48 8B F9")));


		MaterialKeyVar_t functionVar(szFunctionName, true);
		oSetMaterialFunction(this, functionVar, nValue, 0x18);
	}

private:
	MEM_PAD(0x18); // 0x0
public:
	CSceneAnimatableObject* pSceneAnimatableObject; // 0x18
	CMaterial2* pMaterial; // 0x20
	CMaterial2* pMaterialCopy; // 0x20
private:
	MEM_PAD(0x10); // 0x28
public:
	CObjectInfo* pObjectInfo; // 0x48
private:
	MEM_PAD(0x8); // 0x28
public:
	Color_t colValue; // 0x50
};


enum ObjectInfoIdList : int32_t
{
	PLAYER_T = 104,
	PLAYER_CT = 113,
};



class IMaterialSystem2
{
public:
	CMaterial2*** FindOrCreateFromResource(CMaterial2*** pOutMaterial, const char* szMaterialName)
	{
		return MEM::CallVFunc<CMaterial2***, 14U>(this, pOutMaterial, szMaterialName);
	}

	CMaterial2** CreateMaterial(CMaterial2*** pOutMaterial, const char* szMaterialName, CMeshData* pData)
	{
		return MEM::CallVFunc<CMaterial2**, 29U>(this, pOutMaterial, szMaterialName, pData, 0, 0, 0, 0, 0, 1);
	}

	void SetCreateDataByMaterial(const void* pData, CMaterial2*** const pInMaterial)
	{
		return MEM::CallVFunc<void, 37U>(this, pInMaterial, pData);
	}

	void SetColor(void* data, Color_t color)
	{
		*(std::uint8_t*)((std::uintptr_t)data + 0x40) = color.r;
		*(std::uint8_t*)((std::uintptr_t)data + 0x41) = color.g;
		*(std::uint8_t*)((std::uintptr_t)data + 0x42) = color.b;
		*(std::uint8_t*)((std::uintptr_t)data + 0x43) = color.a;
	}
};
