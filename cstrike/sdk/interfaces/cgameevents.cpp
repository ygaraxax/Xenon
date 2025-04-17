#include "cgameevents.h"
#include <thread>


void DrawDamageText(int damage, C_CSPlayerPawn* pVictimPawn);



CCSPlayerController* CGameEventHelper::GetPlayerController()
{
    if (!Event)
        return nullptr;

    int controller_id{};

    CBuffer buffer;
    buffer.name = "userid";

    Event->GetControllerId(controller_id, &buffer);

    if (controller_id == -1)
        return nullptr;

    return (CCSPlayerController*)I::GameResourceService->pGameEntitySystem->Get(controller_id + 1);
}

CCSPlayerController* CGameEventHelper::GetAttackerController()
{
    if (!Event)
        return nullptr;

    int controller_id{};

    CBuffer buffer;
    buffer.name = "attacker";

    Event->GetControllerId(controller_id, &buffer);

    if (controller_id == -1)
        return nullptr;

    return (CCSPlayerController*)I::GameResourceService->pGameEntitySystem->Get(controller_id + 1);
}

int CGameEventHelper::GetDamage()
{
    if (!Event)
        return -1;

    return Event->GetInt2("dmg_health", false);
}

int CGameEventHelper::GetHealth()
{
    if (!Event)
        return -1;

    CBuffer buffer;
    buffer.name = "health";

    return Event->GetInt(&buffer);
}

Vector_t CGameEventHelper::GetBulletPos()
{
    if (!Event)
        return Vector_t();

    std::string_view token_name_x = CS_XOR("x");
    cUltStringToken tokenx(token_name_x.data());

    std::string_view token_name_y = CS_XOR("y");
    cUltStringToken tokeny(token_name_y.data());

    std::string_view token_name_z = CS_XOR("z");
    cUltStringToken tokenz(token_name_z.data());

    return Vector_t(Event->GetFloatNew(tokenx), Event->GetFloatNew(tokeny), Event->GetFloatNew(tokenz));
}

bool CEvents::Intilization()
{
    I::GameEventManager->AddListener(this, "player_hurt", false);
    if (!I::GameEventManager->FindListener(this, "player_hurt"))
        return false;

    I::GameEventManager->AddListener(this, "player_death", false);
    if (!I::GameEventManager->FindListener(this, "player_death"))
        return false;

    I::GameEventManager->AddListener(this, "bullet_impact", false);
    if (!I::GameEventManager->FindListener(this, "bullet_impact"))
        return false;

    I::GameEventManager->AddListener(this, "round_start", false);
    if (!I::GameEventManager->FindListener(this, "round_start"))
        return false;

    return true;
}

void CEvents::FireGameEvent(CGameEvent* event)
{
    std::string name = event->GetName();

    if (name.find("player_hurt") != std::string::npos)
        OnPlayerHurt(event);

    if (name.find("player_death") != std::string::npos)
        OnPlayerDeath(event);

    if (name.find("bullet_impact") != std::string::npos)
        OnBulletImpact(event);

    if (name.find("round_start") != std::string::npos)
        OnRoundStart(event);
}


void CEvents::OnPlayerHurt(CGameEvent* event)
{
	CGameEventHelper eventHelper = event->GetEventHelper();

	auto pVictimController = eventHelper.GetPlayerController();
	auto pAttackerController = eventHelper.GetAttackerController();
	if (!pVictimController || !pAttackerController || !SDK::LocalController)
		return;

	if (pAttackerController == SDK::LocalController && pVictimController != SDK::LocalController)
	{
		C_CSPlayerPawn* pVictimPawn = I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(pVictimController->GetPawnHandle());
		if (!pVictimPawn)
			return;

        int damage = eventHelper.GetDamage();

		this->damage = damage;
		pos = pVictimPawn->GetSceneOrigin();
        //table.insert(shot_particle, damage)

		//PlaySoundA("coin.mp3", NULL, SND_ASYNC);

		if (!I::GlobalVars)
			return;

		flHurtTime = I::GlobalVars->flCurrentTime;
	}
}


void CEvents::OnPaint()
{
	if (!C_GET(bool, Vars.bHitmarker))
		return;

	if (!I::GlobalVars)
		return;

	auto curtime = I::GlobalVars->flCurrentTime;
	auto lineSize = 9.f;

	ImGuiIO& io = ImGui::GetIO();
	const ImVec2 vecScreenSize = io.DisplaySize;
	const ImVec2 vecScreenCenter = vecScreenSize / 2.f;


	if (flHurtTime + 1.2f >= curtime)
	{
		ImVec2 vecScreen;
		if (D::WorldToScreen(pos, &vecScreen))
			D::pDrawListActive->AddText(FONT::pCheatLogo, 25, vecScreen, Color_t(230, 230, 230).GetU32(), std::to_string(this->damage).c_str());

		D::pDrawListActive->AddLine(ImVec2(vecScreenCenter.x - lineSize, vecScreenCenter.y - lineSize), ImVec2(vecScreenCenter.x - (lineSize / 4), vecScreenCenter.y - (lineSize / 4)), Color_t(200, 200, 200).GetU32());
		D::pDrawListActive->AddLine(ImVec2(vecScreenCenter.x - lineSize, vecScreenCenter.y + lineSize), ImVec2(vecScreenCenter.x - (lineSize / 4), vecScreenCenter.y + (lineSize / 4)), Color_t(200, 200, 200).GetU32());
		D::pDrawListActive->AddLine(ImVec2(vecScreenCenter.x + lineSize, vecScreenCenter.y + lineSize), ImVec2(vecScreenCenter.x + (lineSize / 4), vecScreenCenter.y + (lineSize / 4)), Color_t(200, 200, 200).GetU32());
		D::pDrawListActive->AddLine(ImVec2(vecScreenCenter.x + lineSize, vecScreenCenter.y + -lineSize), ImVec2(vecScreenCenter.x + (lineSize / 4), vecScreenCenter.y - (lineSize / 4)), Color_t(200, 200, 200).GetU32());
	}
	else
		flHurtTime = 0.f;
}



void CEvents::OnPlayerDeath(CGameEvent* event)
{ }

void CEvents::OnBulletImpact(CGameEvent* event)
{
	if (!C_GET(bool, Vars.bDebugShootHitbox) || true)
		return;

    C_CSPlayerPawn* pLocalPawn = SDK::LocalPawn;
    if (!pLocalPawn)
        return;

    std::string_view token_name_x = CS_XOR("x");
    cUltStringToken tokenx(token_name_x.data());

    std::string_view token_name_y = CS_XOR("y");
    cUltStringToken tokeny(token_name_y.data());

    std::string_view token_name_z = CS_XOR("z");
    cUltStringToken tokenz(token_name_z.data());

    auto player = event->GetEventHelper().GetPlayerController();
	if (!player)
		return;
    auto position = Vector_t(event->GetFloatNew(tokenx), event->GetFloatNew(tokeny), event->GetFloatNew(tokenz));


	auto debugOverlay = I::Client->GetSceneDebugOverlay();
	if (debugOverlay)
		debugOverlay->add_box(position, Vector_t(-2, -2, -2), Vector_t(2, 2, 2), Vector_t(), Color_t(0, 0, 255, 127), .3f);


    /*if (C_GET(unsigned int, Vars.bBulletImpacts) & SERVER)
    {
        auto debugOverlay = I::Client->GetSceneDebugOverlay();
        debugOverlay->addBox(position, Vector_t(-2, -2, -2), Vector_t(2, 2, 2), Vector_t(), Color_t(0, 0, 255, 127));
    }

    if (C_GET(unsigned int, Vars.bBulletImpacts) & LOCAL)
        F::VISUALS::WORLD::BulletImpact(pLocalPawn);*/
}

void CEvents::OnRoundStart(CGameEvent* event)
{ }
