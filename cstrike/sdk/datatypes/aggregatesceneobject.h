#pragma once


class CMaterial2;



class CAggregateSceneObjectData
{
private:
	char pad_0000[0x38]; // 0x0
public:
	std::uint8_t r; // 0x38
	std::uint8_t g; // 0x39
	std::uint8_t b; // 0x3A
private:
	char pad_0038[0x9];
};

class CAggregateSceneObject
{
private:
	char pad_0000[0x120];

public:
	int count; // 0x120
private:
	char pad_0120[0x4];

public:
	CAggregateSceneObjectData* array; // 0x128
};


class CAggregateSceneObjectLighting
{
public:
	char pad_0000[0xE4]; // 0x0
	float red;
	float green;
	float blue;
};




class CSceneObject
{
public:
	char pad_0000[184]; //0x0000
	uint8_t r; //0x00B8
	uint8_t g; //0x00B9
	uint8_t b; //0x00BA
	uint8_t a; //0x00BB
	char pad_00BC[196]; //0x00BC
}; //Size: 0x0180

class CBaseSceneData
{
public:
	char pad_0000[24]; //0x0000
	CSceneObject* sceneObject; //0x0018
	CMaterial2* material; //0x0020
	CMaterial2* material2; //0x0028
	char pad_0030[16]; //0x0030 Size Changed
	uint8_t r; //0x0050
	uint8_t g; //0x0051
	uint8_t b; //0x0052
	uint8_t a; //0x0053
	char pad_0044[36]; //0x0054
}; //Size: 0x0078
