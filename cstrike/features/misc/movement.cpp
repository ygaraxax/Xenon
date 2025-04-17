#include "movement.h"

// used: sdk entity
#include "../../sdk/entity.h"
// used: cusercmd
#include "../../sdk/datatypes/usercmd.h"

// used: convars
#include "../../core/convars.h"
#include "../../sdk/interfaces/ienginecvar.h"

// used: cheat variables
#include "../../core/variables.h"
#include "../../core/sdk.h"
#include "../../core/hooks.h"
#include <algorithm>
#include "../antiaim/antiaim.hpp"

#include "../../sdk/interfaces/ccsgoinput.h"


void vector_angles(const Vector_t& vecForward, Vector_t& vecAngles)
{
	Vector_t vecView;
	if (vecForward.y == 0 && vecForward.x == 0)
	{
		vecView.x = 0.f;
		vecView.y = 0.f;
	}
	else
	{
		vecView.y = atan2(vecForward.y, vecForward.x) * 180.f / 3.14f;

		if (vecView.y < 0.f)
			vecView.y += 360.f;

		auto tmp = vecForward.Length2D();

		vecView.x = atan2(-vecForward.z, tmp) * 180.f / 3.14f;

		if (vecView.x < 0.f)
			vecView.x += 360.f;
	}

	vecAngles.x = vecView.x;
	vecAngles.y = vecView.y;
	vecAngles.z = 0.f;
}

void sin_cos(float rad, float* sine, float* cosine)
{
	*sine = std::sinf(rad);
	*cosine = std::cosf(rad);
}

void angle_vectors(const Vector_t& angles, Vector_t& forward)
{
	float sp, sy, cp, cy;

	sin_cos(M_DEG2RAD(angles[1]), &sy, &cy);
	sin_cos(M_DEG2RAD(angles[0]), &sp, &cp);

	forward.x = cp * cy;
	forward.y = cp * sy;
	forward.z = -sp;
}

// movement correction angles
static QAngle_t angCorrectionView = {};

void F::MISC::MOVEMENT::OnMove(CUserCmd* pCmd, CBaseUserCmdPB* pBaseCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn)
{
	if (pCmd == nullptr || pBaseCmd == nullptr)
		return;

	if (pLocalPawn == nullptr || pLocalController == nullptr)
		return;

	if (!pLocalController->IsPawnAlive())
		return;

	if (const int32_t nMoveType = pLocalPawn->GetMoveType(); nMoveType == MOVETYPE_NOCLIP || nMoveType == MOVETYPE_LADDER || pLocalPawn->GetWaterLevel() >= WL_WAIST)
		return;


	AutoStrafe(pCmd, pBaseCmd, pLocalPawn);
	BunnyHop(pCmd, pBaseCmd, pLocalPawn);
	//MovementCorrection(pBaseCmd, F::ANTIAIM::angStoredViewBackup);

	if (!pCmd->csgoUserCmd.inputHistoryField.pRep)
		return;

	// loop through all tick commands
	for (int nSubTick = 0; nSubTick < pCmd->csgoUserCmd.inputHistoryField.pRep->nAllocatedSize; nSubTick++)
	{
		CCSGOInputHistoryEntryPB* pInputEntry = pCmd->GetInputHistoryEntry(nSubTick);
		if (pInputEntry == nullptr)
			continue;

		if (pInputEntry->pViewAngles == nullptr)
			continue;

		
		// save view angles for movement correction
		angCorrectionView = pInputEntry->pViewAngles->angValue;

		// movement correction & anti-untrusted
		ValidateUserCommand(pCmd, pBaseCmd, pInputEntry);
	}
}


void F::MISC::MOVEMENT::BunnyHop(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, C_CSPlayerPawn* pLocalPawn)
{
	if (!C_GET(bool, Vars.bAutoBHop) || CONVAR::sv_autobunnyhopping->value.i1)
		return;

	// update random seed
	//MATH::fnRandomSeed(pUserCmd->nRandomSeed);

	//// bypass of possible SMAC/VAC server anticheat detection
	//if (static bool bShouldFakeJump = false; bShouldFakeJump)
	//{
	//	pCmd->nButtons.nValue |= IN_JUMP;
	//	bShouldFakeJump = false;
	//}
	//// check is player want to jump
	//else if (pCmd->nButtons.nValue & IN_JUMP)
	//{
	//	// check is player on the ground
	//	if (pLocalPawn->GetFlags() & FL_ONGROUND)
	//		// note to fake jump at the next tick
	//		bShouldFakeJump = true;
	//	// check did random jump chance passed
	//	else if (MATH::fnRandomInt(0, 100) <= C_GET(int, Vars.nAutoBHopChance))
	//		pCmd->nButtons.nValue &= ~IN_JUMP;
	//}

	// im lazy so yea :D
	if (pLocalPawn->GetFlags() & FL_ONGROUND)
	{
		pCmd->nButtons.nValue &= ~IN_JUMP;
	}

	/*const bool is_jumping = pCmd->nButtons.nValue & IN_JUMP;
	if (is_jumping)
		return;

	pCmd->nButtons.nValue &= ~IN_JUMP;
	pCmd->nButtons.nValueChanged &= ~IN_JUMP;
	pCmd->nButtons.nValueScroll &= ~IN_JUMP;*/

	/*if (pLocalPawn->GetFlags() & FL_ONGROUND)
	{
		pCmd->nButtons.nValue &= ~IN_JUMP;
		pCmd->nButtons.nValueScroll &= ~IN_JUMP;

		auto poopl = pCmd->CreateSubtick();
		poopl->nButton = IN_JUMP;
		poopl->bPressed = true;
		poopl->flWhen = 0.999f;

		auto poopl1 = pCmd->CreateSubtick();
		poopl1->nButton = IN_JUMP;
		poopl1->bPressed = false;
		poopl1->flWhen = 0.999f;
	}*/
}

void F::MISC::MOVEMENT::AutoStrafe(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, C_CSPlayerPawn* pLocalPawn)
{
	static bool m_switch_value = false;
	static float m_old_yaw;
	static uint64_t last_pressed = 0;
	static uint64_t last_buttons = 0;

	if (!C_GET(bool, Vars.bAutoStrafe))
		return;

	if (pUserCmd->pViewAngles == nullptr)
		return;


	const auto current_buttons = pCmd->nButtons.nValue;
	auto yaw = MATH::Normalize(pUserCmd->pViewAngles->angValue.y);

	const auto check_button = [&](const uint64_t button)
	{
		if (current_buttons & button && (!(last_buttons & button) || button & IN_MOVELEFT && !(last_pressed & IN_MOVERIGHT) || button & IN_MOVERIGHT && !(last_pressed & IN_MOVELEFT) || button & IN_FORWARD && !(last_pressed & IN_BACK) || button & IN_BACK && !(last_pressed & IN_FORWARD)))
		{
			if (button & IN_MOVELEFT)
				last_pressed &= ~IN_MOVERIGHT;
			else if (button & IN_MOVERIGHT)
				last_pressed &= ~IN_MOVELEFT;
			else if (button & IN_FORWARD)
				last_pressed &= ~IN_BACK;
			else if (button & IN_BACK)
				last_pressed &= ~IN_FORWARD;

			last_pressed |= button;
		}
		else if (!(current_buttons & button))
			last_pressed &= ~button;
	};

	check_button(IN_MOVELEFT);
	check_button(IN_MOVERIGHT);
	check_button(IN_FORWARD);
	check_button(IN_BACK);

	last_buttons = current_buttons;

	const auto velocity = pLocalPawn->GetAbsVelocity();

	if (pLocalPawn->GetFlags() & FL_ONGROUND)
		return;

	auto offset = 0.f;
	if (last_pressed & IN_MOVELEFT)
		offset += 90.f;
	if (last_pressed & IN_MOVERIGHT)
		offset -= 90.f;
	if (last_pressed & IN_FORWARD)
		offset *= 0.5f;
	else if (last_pressed & IN_BACK)
		offset = -offset * 0.5f + 180.f;

	yaw += offset;

	auto velocity_angle = M_RAD2DEG(std::atan2f(velocity.y, velocity.x));
	if (velocity_angle < 0.0f)
		velocity_angle += 360.0f;

	if (velocity_angle < 0.0f)
		velocity_angle += 360.0f;

	velocity_angle -= floorf(velocity_angle / 360.0f + 0.5f) * 360.0f;

	const auto speed = velocity.Length2D();
	auto ideal = 0.f;

	if (speed > 0.f)
		ideal = MATH::Clamp(M_RAD2DEG(std::atan2(15.f, speed)), 0.0f, 90.0f);

	auto correct = ideal;

	pUserCmd->flForwardMove = 0.f;
	pUserCmd->SetBits(EBaseCmdBits::BASE_BITS_FORWARDMOVE);
	const auto velocity_delta = MATH::Normalize(yaw - velocity_angle);

	// get our viewangle change.
	auto delta = MATH::Normalize(yaw - m_old_yaw);

	// convert to absolute change.
	auto abs_delta = std::abs(delta);

	// save old yaw for next call.
	m_old_yaw = yaw;

	auto rotate_movement = [](CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, float target_yaw)
	{
		const float rot = M_DEG2RAD(pUserCmd->pViewAngles->angValue.y - target_yaw);

		const float new_forward = std::cos(rot) * pUserCmd->flForwardMove - std::sin(rot) * pUserCmd->flSideMove;
		const float new_side = std::sin(rot) * pUserCmd->flForwardMove + std::cos(rot) * pUserCmd->flSideMove;

		pCmd->nButtons.nValue &= ~(IN_BACK | IN_FORWARD | IN_MOVELEFT | IN_MOVERIGHT);
		pCmd->nButtons.nValueChanged &= ~(IN_BACK | IN_FORWARD | IN_MOVELEFT | IN_MOVERIGHT);

		pUserCmd->flForwardMove = std::round(new_forward);
		pUserCmd->SetBits(EBaseCmdBits::BASE_BITS_FORWARDMOVE);
		pUserCmd->flSideMove = std::round(new_side * -1.f);
		pUserCmd->SetBits(EBaseCmdBits::BASE_BITS_LEFTMOVE);

		if (pUserCmd->flForwardMove > 0.0f)
		{
			//Interfaces::csgo_input->add_button(IN_FORWARD);
			pUserCmd->pInButtonState->nValue |= IN_FORWARD;
			pUserCmd->pInButtonState->SetBits(IN_FORWARD);
		}
		else if (pUserCmd->flForwardMove <= 0.0f)
		{
			//Interfaces::csgo_input->add_button(IN_BACK);
			pUserCmd->pInButtonState->nValue |= IN_BACK;
			pUserCmd->pInButtonState->SetBits(IN_BACK);
		}

		if (pUserCmd->flSideMove > 0.0f)
		{
			//Interfaces::csgo_input->add_button(IN_MOVELEFT);
			pUserCmd->pInButtonState->nValue |= IN_MOVELEFT;
			pUserCmd->pInButtonState->SetBits(IN_MOVELEFT);
		}
		else if (pUserCmd->flSideMove <= 0.0f)
		{
			//Interfaces::csgo_input->add_button(IN_MOVERIGHT);
			pUserCmd->pInButtonState->nValue |= IN_MOVERIGHT;
			pUserCmd->pInButtonState->SetBits(IN_MOVERIGHT);
		}
	};

	if (fabsf(velocity_delta) > 170.0f && speed > 80.0f || velocity_delta > correct && speed > 80.0f)
	{
		yaw = correct + velocity_angle;
		pUserCmd->flSideMove = -1.0f;
		pUserCmd->SetBits(EBaseCmdBits::BASE_BITS_LEFTMOVE);
		rotate_movement(pCmd, pUserCmd, MATH::Normalize(yaw));
		return;
	}

	m_switch_value = !m_switch_value;

	if (-correct <= velocity_delta || speed <= 80.f)
	{
		if (m_switch_value)
		{
			yaw = yaw - ideal;
			pUserCmd->flSideMove = -1.0f;
			pUserCmd->SetBits(EBaseCmdBits::BASE_BITS_LEFTMOVE);
		}
		else
		{
			yaw = ideal + yaw;
			pUserCmd->flSideMove = 1.0f;
			pUserCmd->SetBits(EBaseCmdBits::BASE_BITS_LEFTMOVE);
		}
	}
	else
	{
		yaw = velocity_angle - correct;
		pUserCmd->flSideMove = 1.0f;
		pUserCmd->SetBits(EBaseCmdBits::BASE_BITS_LEFTMOVE);
	}

	rotate_movement(pCmd, pUserCmd, MATH::Normalize(yaw));




	if (pUserCmd->subtickMovesField.pRep == nullptr)
		return;


	/*for (auto i = 0; i < 12; i++)
		auto subtick = pCmd->CreateSubtick();


	for (auto i = 0; i < pUserCmd->subtickMovesField.pRep->nAllocatedSize; i++)
	{
		auto subtick = pUserCmd->subtickMovesField.pRep->tElements[i];
		if (!subtick)
			continue;

		if (i == 0)
		{
			subtick->flWhen = FLT_TRUE_MIN;
			continue;
		}
		subtick->flAnalogForwardDelta = pUserCmd->flForwardMove;
		subtick->flAnalogLeftDelta = pUserCmd->flSideMove;

		subtick->nButton = pCmd->nButtons.nValue;
		subtick->SetBits(BUTTON_STATE_PB_BITS_BUTTONSTATE1);

		subtick->flWhen = ((1.f / 12.f) * (i));
	}*/

	/*I::Input->flForwardMove = pUserCmd->flForwardMove;
	I::Input->flSideMove = pUserCmd->flSideMove;*/

	/*for (auto i = 0; i < pUserCmd->subtickMovesField.pRep->nAllocatedSize; i++)
	{
		auto Subtick = pUserCmd->subtickMovesField.pRep->tElements[i];
		if (!Subtick)
			continue;

		Subtick->flAnalogForwardDelta = pUserCmd->flForwardMove;
		Subtick->flAnalogLeftDelta = pUserCmd->flSideMove;

		Subtick->nButton |= IN_SPRINT;
		Subtick->SetBits(IN_SPRINT);

		Subtick->flWhen = (1.f / 12.f) * (i);
	}*/


	I::Input->flForwardMove = std::round(pUserCmd->flForwardMove);
	I::Input->flSideMove = std::round(pUserCmd->flSideMove);

	CSubtickMoveStep* Subtick = pCmd->CreateSubtick();
	if (Subtick == nullptr)
		return;

	Subtick->flAnalogForwardDelta = std::round(pUserCmd->flForwardMove);
	Subtick->flAnalogLeftDelta = std::round(pUserCmd->flSideMove);

	Subtick->nButton = pCmd->nButtons.nValue;
	Subtick->SetBits(BUTTON_STATE_PB_BITS_BUTTONSTATE1);
	Subtick->flWhen = (1.f / 12.f) * pUserCmd->subtickMovesField.pRep->nAllocatedSize;
}

void F::MISC::MOVEMENT::ValidateUserCommand(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, CCSGOInputHistoryEntryPB* pInputEntry)
{
	if (pUserCmd == nullptr)
		return;
	

	// clamp angle to avoid untrusted angle
	if (C_GET(bool, Vars.bAntiUntrusted))
	{
		pInputEntry->SetBits(EInputHistoryBits::INPUT_HISTORY_BITS_VIEWANGLES);
		if (pInputEntry->pViewAngles->angValue.IsValid())
		{
			pInputEntry->pViewAngles->angValue.Clamp();
			pInputEntry->pViewAngles->angValue.z = 0.f;
		}
		else
		{
			pInputEntry->pViewAngles->angValue = {};
		}
	}


	// correct movement buttons while player move have different to buttons values
	// clear all of the move buttons states
	if (false)
	{
		pCmd->nButtons.nValue &= (~IN_FORWARD | ~IN_BACK | ~IN_LEFT | ~IN_RIGHT);

		// re-store buttons by active forward/side moves
		if (pUserCmd->flForwardMove > 0.0f)
			pCmd->nButtons.nValue |= IN_FORWARD;
		else if (pUserCmd->flForwardMove < 0.0f)
			pCmd->nButtons.nValue |= IN_BACK;

		if (pUserCmd->flSideMove > 0.0f)
			pCmd->nButtons.nValue |= IN_RIGHT;
		else if (pUserCmd->flSideMove < 0.0f)
			pCmd->nButtons.nValue |= IN_LEFT;
	}

	if (!pInputEntry->pViewAngles->angValue.IsZero())
	{
		const float flDeltaX = std::remainderf(pInputEntry->pViewAngles->angValue.x - angCorrectionView.x, 360.f);
		const float flDeltaY = std::remainderf(pInputEntry->pViewAngles->angValue.y - angCorrectionView.y, 360.f);

		float flPitch = CONVAR::m_pitch->value.fl;
		float flYaw = CONVAR::m_yaw->value.fl;

		float flSensitivity = CONVAR::sensitivity->value.fl;
		if (flSensitivity == 0.0f)
			flSensitivity = 1.0f;

		pUserCmd->SetBits(EBaseCmdBits::BASE_BITS_MOUSEDX);
		pUserCmd->nMousedX = static_cast<short>(flDeltaX / (flSensitivity * flPitch));

		pUserCmd->SetBits(EBaseCmdBits::BASE_BITS_MOUSEDY);
		pUserCmd->nMousedY = static_cast<short>(-flDeltaY / (flSensitivity * flYaw));
	}
}

void AngleQangles(const QAngle_t& angles, QAngle_t* forward, QAngle_t* right, QAngle_t* up)
{
	float angle;
	float sr, sp, sy, cr, cp, cy;

	// Convert angles from degrees to radians
	angle = angles.y * (MATH::_PI / 180.0);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles.x * (MATH::_PI / 180.0);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles.z * (MATH::_PI / 180.0);
	sr = sin(angle);
	cr = cos(angle);

	if (forward)
	{
		forward->x = cp * cy;
		forward->y = cp * sy;
		forward->z = -sp;
	}

	if (right)
	{
		right->x = (-1 * sr * sp * cy + -1 * cr * -sy);
		right->y = (-1 * sr * sp * sy + -1 * cr * cy);
		right->z = -1 * sr * cp;
	}

	if (up)
	{
		up->x = (cr * sp * cy + -sr * -sy);
		up->y = (cr * sp * sy + -sr * cy);
		up->z = cr * cp;
	}
}

void F::MISC::MOVEMENT::MovementCorrection(CBaseUserCmdPB* pUserCmd, const QAngle_t& angDesiredViewPoint)
{
	if (!pUserCmd)
		return;

	if (!(pUserCmd->pViewAngles))
		return;

	QAngle_t wish_angle;
	wish_angle = pUserCmd->pViewAngles->angValue;
	//wish_angle = angDesiredViewPoint;
	int revers = wish_angle.x > 89.f ? -1 : 1;
	wish_angle.Clamp();

	QAngle_t view_fwd, view_right, view_up, cmd_fwd, cmd_right, cmd_up;
	auto viewangles = angDesiredViewPoint;
	//auto viewangles = pUserCmd->pViewAngles->angValue;

	AngleQangles(wish_angle, &view_fwd, &view_right, &view_up);
	AngleQangles(viewangles, &cmd_fwd, &cmd_right, &cmd_up);

	const float v8 = sqrtf((view_fwd.x * view_fwd.x) + (view_fwd.y * view_fwd.y));
	const float v10 = sqrtf((view_right.x * view_right.x) + (view_right.y * view_right.y));
	const float v12 = sqrtf(view_up.z * view_up.z);

	const Vector_t norm_view_fwd((1.f / v8) * view_fwd.x, (1.f / v8) * view_fwd.y, 0.f);
	const Vector_t norm_view_right((1.f / v10) * view_right.x, (1.f / v10) * view_right.y, 0.f);
	const Vector_t norm_view_up(0.f, 0.f, (1.f / v12) * view_up.z);

	const float v14 = sqrtf((cmd_fwd.x * cmd_fwd.x) + (cmd_fwd.y * cmd_fwd.y));
	const float v16 = sqrtf((cmd_right.x * cmd_right.x) + (cmd_right.y * cmd_right.y));
	const float v18 = sqrtf(cmd_up.z * cmd_up.z);

	const Vector_t norm_cmd_fwd((1.f / v14) * cmd_fwd.x, (1.f / v14) * cmd_fwd.y, 0.f);
	const Vector_t norm_cmd_right((1.f / v16) * cmd_right.x, (1.f / v16) * cmd_right.y, 0.f);
	const Vector_t norm_cmd_up(0.f, 0.f, (1.f / v18) * cmd_up.z);

	const float v22 = norm_view_fwd.x * pUserCmd->flForwardMove;
	const float v26 = norm_view_fwd.y * pUserCmd->flForwardMove;
	const float v28 = norm_view_fwd.z * pUserCmd->flForwardMove;
	const float v24 = norm_view_right.x * pUserCmd->flSideMove;
	const float v23 = norm_view_right.y * pUserCmd->flSideMove;
	const float v25 = norm_view_right.z * pUserCmd->flSideMove;
	const float v30 = norm_view_up.x * pUserCmd->flUpMove;
	const float v27 = norm_view_up.z * pUserCmd->flUpMove;
	const float v29 = norm_view_up.y * pUserCmd->flUpMove;

	pUserCmd->flForwardMove = ((((norm_cmd_fwd.x * v24) + (norm_cmd_fwd.y * v23)) + (norm_cmd_fwd.z * v25)) + (((norm_cmd_fwd.x * v22) + (norm_cmd_fwd.y * v26)) + (norm_cmd_fwd.z * v28))) + (((norm_cmd_fwd.y * v30) + (norm_cmd_fwd.x * v29)) + (norm_cmd_fwd.z * v27));
	pUserCmd->flSideMove = ((((norm_cmd_right.x * v24) + (norm_cmd_right.y * v23)) + (norm_cmd_right.z * v25)) + (((norm_cmd_right.x * v22) + (norm_cmd_right.y * v26)) + (norm_cmd_right.z * v28))) + (((norm_cmd_right.x * v29) + (norm_cmd_right.y * v30)) + (norm_cmd_right.z * v27));
	pUserCmd->flUpMove = ((((norm_cmd_up.x * v23) + (norm_cmd_up.y * v24)) + (norm_cmd_up.z * v25)) + (((norm_cmd_up.x * v26) + (norm_cmd_up.y * v22)) + (norm_cmd_up.z * v28))) + (((norm_cmd_up.x * v30) + (norm_cmd_up.y * v29)) + (norm_cmd_up.z * v27));
	pUserCmd->flForwardMove = revers * ((((norm_cmd_fwd.x * v24) + (norm_cmd_fwd.y * v23)) + (norm_cmd_fwd.z * v25)) + (((norm_cmd_fwd.x * v22) + (norm_cmd_fwd.y * v26)) + (norm_cmd_fwd.z * v28)));
	pUserCmd->flSideMove = ((((norm_cmd_right.x * v24) + (norm_cmd_right.y * v23)) + (norm_cmd_right.z * v25)) + (((norm_cmd_right.x * v22) + (norm_cmd_right.y * v26)) + (norm_cmd_right.z * v28)));
	/*pUserCmd->flForwardMove = std::clamp(pUserCmd->flForwardMove, -1.f, 1.f);
	pUserCmd->flSideMove = std::clamp(pUserCmd->flSideMove, -1.f, 1.f);*/
	pUserCmd->flForwardMove = std::round(pUserCmd->flForwardMove);
	pUserCmd->flSideMove = std::round(pUserCmd->flSideMove);
	pUserCmd->SetBits(BASE_BITS_FORWARDMOVE);
	pUserCmd->SetBits(BASE_BITS_LEFTMOVE);


	I::Input->flForwardMove = pUserCmd->flForwardMove;
	I::Input->flSideMove = pUserCmd->flSideMove;

}
