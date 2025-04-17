#pragma once

// used: callvfunc
#include "../../utilities/memory.h"

struct ResourceBinding_t;

class ResourceArray_t;

struct ResourceBinding_t;

class IResourceSystem
{
public:
	void* QueryInterface(const char* szInterfaceName)
	{
		return MEM::CallVFunc<void*, 2U>(this, szInterfaceName);
	}

	void EnumMaterials(uint64_t iTypeHash, ResourceArray_t* pResult, uint8_t Flag)
	{
		MEM::CallVFunc<void, 38>(this, iTypeHash, pResult, Flag);
	}
};

class CResourceHandleUtils
{
public:
	void DeleteResource(const ResourceBinding_t* pBinding)
	{
		MEM::CallVFunc<void, 2U>(this, pBinding);
	}
};
