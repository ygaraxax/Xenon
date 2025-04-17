#pragma once

class CGameRules
{
public:
	char pad[0x30]; // 0x00
	bool bFreezePause; // 0x30
	char pad2[0xB]; // 0x31
	int iPause; // 0x3C
	char pad3[0x38]; // 0x40
	int iGamePhases; // 0x78
};
