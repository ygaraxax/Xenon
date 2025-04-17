#include "nightmode.hpp"

#include "../../core/sdk.h"
#include "../../sdk/interfaces/iengineclient.h"

#include "../visuals/chams.h"
#include "../../sdk/datatypes/vector.h"
#include "../../sdk/interfaces/imaterialsystem.h"
#include "../../sdk/interfaces/iresourcesystem.h"

#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../antiaim/antiaim.hpp"
#include "../../sdk/entity.h"
#include "../../core/variables.h"


struct material_paramater
{
	Vector4D_t vec_value; // 0x0
	void* texture_value; // 0x10
	MEM_PAD(0x10); // 0x18
	const char* parameter_name; // 0x28
	const char* text_value; // 0x30
	int64_t int_value; // 0x38
};

CMaterial2* reconstruct_sky_material(CMaterial2* mat)
{
	std::string _template = R"#(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d}
			format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
			{
                shader = "sky.vfx"
                F_TEXTURE_FORMAT2 = 6
                g_flBrightnessExposureBias = -0.5
                g_flRenderOnlyExposureBias = 0
                SkyTexture = resource:"materials/skybox/tests/src/lightingtest_sky_night.png"
                g_tSkyTexture = resource:"materials/skybox/tests/src/lightingtest_sky_night_exr_2c5e8c62.vtex"
			} )#";

	/*std::string _template = R"#(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d}
   format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
   {
                shader = "generic.vfx"
                g_tColor = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
                g_tNormal = resource:"materials/default/default_normal_tga_7652cb.vtex"
                g_tRoughness = resource:"materials/default/default_normal_tga_b3f4ec4c.vtex"
                g_tMetalness = resource:"materials/default/default_normal_tga_b3f4ec4c.vtex"
                g_tAmbientOcclusion = resource:"materials/default/default_normal_tga_b3f4ec4c.vtex"
                F_IGNOREZ = 0
                F_DISABLE_Z_WRITE = 0
                F_DISABLE_Z_BUFFERING = 0
                F_RENDER_BACKFACES = 1
                g_vColorTint = [1.0, 1.0, 1.0, 1.0]
   } )#";*/

	CMaterial2* material = F::VISUALS::CHAMS::CreateMaterial(CS_XOR("new_sky"), _template.c_str());
	if (!material)
	{
		return nullptr;
	}

	if (material->FindParameter("F_TEXTURE_FORMAT2") && mat->FindParameter("F_TEXTURE_FORMAT2"))
		material->FindParameter("F_TEXTURE_FORMAT2")->texture_value = mat->FindParameter("F_TEXTURE_FORMAT2")->texture_value;
	if (material->FindParameter("g_flBrightnessExposureBias") && mat->FindParameter("g_flBrightnessExposureBias"))
		material->FindParameter("g_flBrightnessExposureBias")->texture_value = mat->FindParameter("g_flBrightnessExposureBias")->texture_value;
	if (material->FindParameter("g_flRenderOnlyExposureBias") && mat->FindParameter("g_flRenderOnlyExposureBias"))
		material->FindParameter("g_flRenderOnlyExposureBias")->texture_value = mat->FindParameter("g_flRenderOnlyExposureBias")->texture_value;
	if (material->FindParameter("SkyTexture") && mat->FindParameter("SkyTexture"))
		material->FindParameter("SkyTexture")->texture_value = mat->FindParameter("SkyTexture")->texture_value;
	if (material->FindParameter("g_tSkyTexture") && mat->FindParameter("g_tSkyTexture"))
		material->FindParameter("g_tSkyTexture")->texture_value = mat->FindParameter("g_tSkyTexture")->texture_value;

	/*if (material->FindParameter("g_vColorTint"))
	{
		Color_t colLight = C_GET(Color_t, Vars.colLight);
		material->FindParameter("g_vColorTint")->vec_value = Vector4D_t{ colLight.r / 255.f, colLight.g / 255.f, colLight.b / 255.f, colLight.a / 255.f };
	}*/

	return material;
}

void WorldColorChanger(Color_t targetColor)
{
	if (!C_GET(bool, Vars.bNightMode))
		targetColor = { 255, 255, 255 };

	static Color_t currentColor{ 69, 69, 69 };
	static uint64_t oldArrCount = 0;

	ResourceArray_t arr{};

	arr.m_nCount = 0;
	arr.m_aResources = nullptr;
	I::ResourceSystem->EnumMaterials(0x74616D76, &arr, 2);

	if (targetColor == currentColor && oldArrCount == arr.m_nCount)
		return;

	if (!arr.m_nCount)
		return;

	oldArrCount = arr.m_nCount;
	currentColor = targetColor;

	for (int i = 0; i < arr.m_nCount; ++i)
	{
		auto pmat = arr.m_aResources[i];
		if (!pmat)
			continue;

		auto mat = *pmat;
		if (!mat)
			continue;

		std::string name = mat->GetName();

		if (name.find("char") != std::string::npos)
			continue;

		if (name.find("weapon") != std::string::npos)
			continue;

		/*if (name.find("sky") != std::string::npos)
		{
			CMaterial2* material = reconstruct_sky_material(mat);
			*pmat = material;
			mat = material;
			mat->UpdateParameter();
			continue;
		}*/

		auto param = mat->FindParameter("g_vColorTint");
		if (!param)
			continue;

		param->vec_value = Vector4D_t{ targetColor.r / 255.f, targetColor.g / 255.f, targetColor.b / 255.f, targetColor.a / 255.f };
		mat->UpdateParameter();
	}
}

void SkyBoxChanger(Color_t targetColor)
{
	if (!C_GET(bool, Vars.bNightMode))
		targetColor = { 255, 255, 255 };
	else
		targetColor = targetColor;

	const auto& entity_list = F::ANTIAIM::get("C_EnvSky");

	for (auto& entity : entity_list)
	{
		auto sky = I::GameResourceService->pGameEntitySystem->Get<C_EnvSky>(entity->GetRefEHandle());
		if (!sky)
			continue;

		Color_t curCol = sky->m_vTintColor();
		if (curCol == targetColor)
			return;

		sky->m_vTintColor() = targetColor;

		MEM::fnUpdateSky(sky);
	}
}

void BrightnessChanger()
{
	if (!SDK::LocalPawn || !I::Engine->IsInGame() || !I::Engine->IsConnected())
		return;

	auto cameraServices = SDK::LocalPawn->GetCameraServices();
	if (!cameraServices)
		return;

	auto volumeHandle = cameraServices->GetActivePostProcessingVolume();
	if (!volumeHandle.IsValid())
		return;

	auto volume = I::GameResourceService->pGameEntitySystem->Get<C_PostProcessingVolume>(volumeHandle);
	if (!volume)
		return;

	volume->m_flMinExposure() = C_GET(int, Vars.nBrightness) / 100.f;
	volume->m_flMaxExposure() = C_GET(int, Vars.nBrightness) / 100.f;
	volume->m_flFadeDuration() = 0.f;
	volume->m_flExposureFadeSpeedDown() = 0.f;
	volume->m_flExposureFadeSpeedUp() = 0.f;
}






void F::VISUALS::NIGHTMODE::Draw()
{
	if (C_GET(bool, Vars.bNightMode))
	{
		WorldColorChanger(C_GET(Color_t, Vars.colWorld));
		SkyBoxChanger(C_GET(Color_t, Vars.colSkybox));
	}
	else
	{
		/*WorldColorChanger(Color_t(255, 255, 255));
		SkyBoxChanger(Color_t(255, 255, 255));*/
	}

	BrightnessChanger();
}
