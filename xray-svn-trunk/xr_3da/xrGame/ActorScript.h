#pragma once

#include "actor.h"

namespace ActorScriptSpace
{
	bool IsCrouched()			{ return Actor() ? !!(Actor()->MovingState()&mcCrouch) : false; };
	bool IsLowCrouched()		{ return Actor() ? (!!(Actor()->MovingState()&mcCrouch) && !!(Actor()->MovingState()&mcAccel)) : false; };
	bool IsWalking()			{ return Actor() ? !!(Actor()->MovingState()&mcAnyMove) : false; };
	bool IsSlowWalking()		{ return Actor() ? (!!(Actor()->MovingState()&mcAnyMove) && !!(Actor()->MovingState()&mcAccel)) : false; };
	bool IsSprinting()			{ return Actor() ? !!(Actor()->MovingState()&mcSprint) : false; };
	bool IsClimbing()			{ return Actor() ? !!(Actor()->MovingState()&mcClimb) : false; };
};