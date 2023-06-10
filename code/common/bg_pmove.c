// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate

#include "q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

pmove_t		*pm;
pml_t		pml;

// movement parameters
const float	pm_stopspeed = 100;
const float	pm_duckScale = 0.50;
const float	pm_swimScale = 0.50;
const float	pm_ladderScale = 0.7f;

const float	pm_accelerate = 10;
const float	pm_airaccelerate = 1;
const float	pm_wateraccelerate = 4;
const float	pm_flyaccelerate = 8;

const float	pm_friction = 6;
const float	pm_waterfriction = 1;
const float	pm_flightfriction = 3;

int		c_pmove = 0;

#define	PHASER_RECHARGE_TIME	100

/*
===============
PM_AddEvent

===============
*/
void PM_AddEvent( int newEvent ) {
	BG_AddPredictableEventToPlayerstate( newEvent, 0, pm->ps );
}

/*
==============
PM_Use

Generates a use event
==============
*/
#define USE_DELAY 2000

void PM_Use( void )
{
	if ( pm->ps->useTime > 0 )
		pm->ps->useTime -= 100;//pm->cmd.msec;

	if ( pm->ps->useTime > 0 ) {
		return;
	}

	if ( ! (pm->cmd.buttons & BUTTON_USE ) )
	{
		pm->useEvent = 0;
		pm->ps->useTime = 0;
		return;
	}

	pm->useEvent = EV_USE;
	pm->ps->useTime = USE_DELAY;
}

/*
===============
PM_AddTouchEnt
===============
*/
void PM_AddTouchEnt( int entityNum ) {
	int		i;

	if ( entityNum == ENTITYNUM_WORLD ) {
		return;
	}
	if ( pm->numtouch == MAXTOUCH ) {
		return;
	}

	// see if it is already added
	for ( i = 0 ; i < pm->numtouch ; i++ ) {
		if ( pm->touchents[ i ] == entityNum ) {
			return;
		}
	}

	// add it
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}

/*
===================
PM_StartTorsoAnim
===================
*/
static void PM_StartTorsoAnim( int anim ) {
	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}
	pm->ps->torsoAnim = ( ( pm->ps->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
		| anim;
}
static void PM_StartLegsAnim( int anim ) {
	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}
	if ( pm->ps->legsTimer > 0 ) {
		return;		// a high priority animation is running
	}
	pm->ps->legsAnim = ( ( pm->ps->legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
		| anim;
}

static void PM_ContinueLegsAnim( int anim ) {
	if ( ( pm->ps->legsAnim & ~ANIM_TOGGLEBIT ) == anim ) {
		return;
	}
	if ( pm->ps->legsTimer > 0 ) {
		return;		// a high priority animation is running
	}
	PM_StartLegsAnim( anim );
}

static void PM_ContinueTorsoAnim( int anim ) {
	if ( ( pm->ps->torsoAnim & ~ANIM_TOGGLEBIT ) == anim ) {
		return;
	}
	if ( pm->ps->torsoTimer > 0 ) {
		return;		// a high priority animation is running
	}
	PM_StartTorsoAnim( anim );
}

static void PM_ForceLegsAnim( int anim ) {
	pm->ps->legsTimer = 0;
	PM_StartLegsAnim( anim );
}


/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
void PM_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, float overbounce ) {
	float	backoff;
	float	change;
	int		i;

	backoff = DotProduct (in, normal);

	if ( backoff < 0 ) {
		backoff *= overbounce;
	} else {
		backoff /= overbounce;
	}

	for ( i=0 ; i<3 ; i++ ) {
		change = normal[i]*backoff;
		out[i] = in[i] - change;
	}
}


/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction( void ) {
	vec3_t	vec;
	float	*vel;
	float	speed, newspeed, control;
	float	drop;

	vel = pm->ps->velocity;

	VectorCopy( vel, vec );
	if ( pml.walking ) {
		vec[2] = 0;	// ignore slope movement
	}

	speed = VectorLength(vec);
	if (speed < 1) {
		vel[0] = 0;
		vel[1] = 0;		// allow sinking underwater
		// FIXME: still have z friction underwater?
		if ( pm->noSpectatorDrift && (pm->ps->pm_type == PM_SPECTATOR || (pm->ps->eFlags&EF_ELIMINATED)) ) {
			vel[2] = 0;
		}
		return;
	}

	drop = 0;

	// apply ground friction
	if ( (pm->watertype & CONTENTS_LADDER) || pm->waterlevel <= 1 ) {
		if ( (pm->watertype & CONTENTS_LADDER) || (pml.walking && !(pml.groundTrace.surfaceFlags & SURF_SLICK)) ) {
			// if getting knocked back, no friction
			if ( ! (pm->ps->pm_flags & PMF_TIME_KNOCKBACK) ) {
				control = speed < pm_stopspeed ? pm_stopspeed : speed;
				drop += control*pm_friction*pml.frametime;
			}
		}
	}

	// apply water friction even if just wading
	if ( pm->waterlevel && !(pm->watertype & CONTENTS_LADDER) ) {
		drop += speed*pm_waterfriction*pm->waterlevel*pml.frametime;
	}

	// apply flying friction
	if ( pm->ps->powerups[PW_FLIGHT] || pm->ps->pm_type == PM_SPECTATOR || BG_BorgTransporting( pm->ps ) || (pm->ps->eFlags&EF_ELIMINATED) )
	{
		drop += speed*pm_flightfriction*pml.frametime;
	}

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0) {
		newspeed = 0;
	}
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}


/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate( vec3_t wishdir, float wishspeed, float accel ) {
#if 1
	// q2 style
	int			i;
	float		addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct (pm->ps->velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) {
		return;
	}
	accelspeed = accel*pml.frametime*wishspeed;
	if (accelspeed > addspeed) {
		accelspeed = addspeed;
	}

	for (i=0 ; i<3 ; i++) {
		pm->ps->velocity[i] += accelspeed*wishdir[i];
	}
#else
	// proper way (avoids strafe jump maxspeed bug), but feels bad
	vec3_t		wishVelocity;
	vec3_t		pushDir;
	float		pushLen;
	float		canPush;

	VectorScale( wishdir, wishspeed, wishVelocity );
	VectorSubtract( wishVelocity, pm->ps->velocity, pushDir );
	pushLen = VectorNormalize( pushDir );

	canPush = accel*pml.frametime*wishspeed;
	if (canPush > pushLen) {
		canPush = pushLen;
	}

	VectorMA( pm->ps->velocity, canPush, pushDir, pm->ps->velocity );
#endif
}



/*
============
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
static float PM_CmdScale( usercmd_t *cmd ) {
	int		max;
	float	total;
	float	scale;

	max = abs( cmd->forwardmove );
	if ( abs( cmd->rightmove ) > max ) {
		max = abs( cmd->rightmove );
	}
	if ( abs( cmd->upmove ) > max ) {
		max = abs( cmd->upmove );
	}
	if ( !max ) {
		return 0;
	}

	total = sqrt( cmd->forwardmove * cmd->forwardmove
		+ cmd->rightmove * cmd->rightmove + cmd->upmove * cmd->upmove );
	scale = (float)pm->ps->speed * max / ( 127.0 * total );

	return scale;
}


/*
================
PM_SetMovementDir

Determine the rotation of the legs reletive
to the facing dir
================
*/
static void PM_SetMovementDir( void ) {
	if ( pm->cmd.forwardmove || pm->cmd.rightmove ) {
		if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 0;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 1;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 2;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 3;
		} else if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 4;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 5;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 6;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 7;
		}
	} else {
		// if they aren't actively going directly sideways,
		// change the animation to the diagonal so they
		// don't stop too crooked
		if ( pm->ps->movementDir == 2 ) {
			pm->ps->movementDir = 1;
		} else if ( pm->ps->movementDir == 6 ) {
			pm->ps->movementDir = 7;
		}
	}
}


/*
=============
PM_CheckJump
=============
*/
static qboolean PM_CheckJump( void ) {
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return qfalse;		// don't allow jump until all buttons are up
	}

	if ( pm->cmd.upmove < 10 ) {
		// not holding jump
		return qfalse;
	}

	// must wait for jump to be released
	if ( pm->ps->pm_flags & PMF_JUMP_HELD ) {
		// clear upmove so cmdscale doesn't lower running speed
		pm->cmd.upmove = 0;
		return qfalse;
	}

	pml.groundPlane = qfalse;		// jumping away
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	if ( pm->ps->persistant[PERS_CLASS] == PC_INFILTRATOR )
	{//INFILTRATORs jump twice as high
		if ( pm->infilJumpFactor > 0.0f ) {
			pm->ps->velocity[2] = JUMP_VELOCITY * pm->infilJumpFactor;
		} else {
			pm->ps->velocity[2] = JUMP_VELOCITY*2;
		}
	}
	else
	{
		pm->ps->velocity[2] = JUMP_VELOCITY;
	}
	PM_AddEvent( EV_JUMP );

	if ( pm->cmd.forwardmove >= 0 ) {
		PM_ForceLegsAnim( LEGS_JUMP );
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	} else {
		PM_ForceLegsAnim( LEGS_JUMPB );
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}

	return qtrue;
}

/*
=============
PM_CheckWaterJump
=============
*/
static qboolean	PM_CheckWaterJump( void ) {
	vec3_t	spot;
	int		cont;
	vec3_t	flatforward;

	if (pm->ps->pm_time) {
		return qfalse;
	}

	// check for water jump
	if ( pm->waterlevel != 2 ) {
		return qfalse;
	}

	if ( pm->watertype & CONTENTS_LADDER ) {
		if (pm->ps->velocity[2] <= 0)
			return qfalse;
	}

	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize (flatforward);

	VectorMA (pm->ps->origin, 30, flatforward, spot);
	spot[2] += 4;
	cont = pm->pointcontents (spot, pm->ps->clientNum );
	if ( !(cont & CONTENTS_SOLID) ) {
		return qfalse;
	}

	spot[2] += 16;
	cont = pm->pointcontents (spot, pm->ps->clientNum );
	if ( cont ) {
		return qfalse;
	}

	// jump out of water
	VectorScale (pml.forward, 200, pm->ps->velocity);
	pm->ps->velocity[2] = 350;

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time = 2000;

	return qtrue;
}

//============================================================================


/*
===================
PM_WaterJumpMove

Flying out of the water
===================
*/
static void PM_WaterJumpMove( void ) {
	// waterjump has no control, but falls

	PM_StepSlideMove( qtrue );

	pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	if (pm->ps->velocity[2] < 0) {
		// cancel as soon as we are falling down again
		pm->ps->pm_flags &= ~PMF_ALL_TIMES;
		pm->ps->pm_time = 0;
	}
}

/*
===================
PM_WaterMove

===================
*/
static void PM_WaterMove( void ) {
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;
	float	scale;
	float	vel;

	if ( PM_CheckWaterJump() ) {
		PM_WaterJumpMove();
		return;
	}
#if 0
	// jump = head for surface
	if ( pm->cmd.upmove >= 10 ) {
		if (pm->ps->velocity[2] > -300) {
			if ( pm->watertype == CONTENTS_WATER ) {
				pm->ps->velocity[2] = 100;
			} else if (pm->watertype == CONTENTS_SLIME) {
				pm->ps->velocity[2] = 80;
			} else {
				pm->ps->velocity[2] = 50;
			}
		}
	}
#endif
	PM_Friction ();

	scale = PM_CmdScale( &pm->cmd );
	//
	// user intentions
	//
	if ( !scale ) {
		wishvel[0] = 0;
		wishvel[1] = 0;
		if ( pm->watertype & CONTENTS_LADDER ) {
			wishvel[2] = 0;
		} else {
			wishvel[2] = -60;		// sink towards bottom
		}
	} else {
		for (i=0 ; i<3 ; i++) {
			wishvel[i] = scale * pml.forward[i]*pm->cmd.forwardmove + scale * pml.right[i]*pm->cmd.rightmove;
		}
		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if ( pm->watertype & CONTENTS_LADDER )	//ladder
	{
		if ( wishspeed > pm->ps->speed * pm_ladderScale ) {
			wishspeed = pm->ps->speed * pm_ladderScale;
		}
		PM_Accelerate( wishdir, wishspeed, pm_flyaccelerate );
	} else {
		if ( wishspeed > pm->ps->speed * pm_swimScale ) {
			wishspeed = pm->ps->speed * pm_swimScale;
		}
		PM_Accelerate( wishdir, wishspeed, pm_wateraccelerate );
	}

	// make sure we can go up slopes easily under water
	if ( pml.groundPlane && DotProduct( pm->ps->velocity, pml.groundTrace.plane.normal ) < 0 ) {
		vel = VectorLength(pm->ps->velocity);
		// slide along the ground plane
		PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal,
			pm->ps->velocity, OVERCLIP );

		VectorNormalize(pm->ps->velocity);
		VectorScale(pm->ps->velocity, vel, pm->ps->velocity);
	}

	PM_SlideMove( qfalse );
}



/*
===================
PM_FlyMove

Only with the flight powerup
===================
*/
static void PM_FlyMove( void ) {
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;
	float	scale;

	// normal slowdown
	PM_Friction ();

	scale = PM_CmdScale( &pm->cmd );
	//
	// user intentions
	//
	if ( !scale ) {
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = 0;
	} else {
		for (i=0 ; i<3 ; i++) {
			wishvel[i] = scale * pml.forward[i]*pm->cmd.forwardmove + scale * pml.right[i]*pm->cmd.rightmove;
		}

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	PM_Accelerate (wishdir, wishspeed, pm_flyaccelerate);

	PM_StepSlideMove( qfalse );
}


/*
===================
PM_AirMove

===================
*/
static void PM_AirMove( void ) {
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;
	usercmd_t	cmd;

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	if ( pm->noJumpKeySlowdown ) {
		// clear upmove so cmdscale doesn't lower running speed
		cmd.upmove = 0;
	}
	scale = PM_CmdScale( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);

	for ( i = 0 ; i < 2 ; i++ ) {
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	}
	wishvel[2] = 0;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	// not on ground, so little effect on velocity
	if ( pm->ps->persistant[PERS_CLASS] == PC_INFILTRATOR )
	{//INFILTRATORs have more air control
		if ( pm->infilAirAccelFactor > 0.0f ) {
			PM_Accelerate (wishdir, wishspeed, pm_airaccelerate * pm->infilAirAccelFactor);
		} else {
			PM_Accelerate (wishdir, wishspeed, pm_airaccelerate*2);
		}
	}
	else
	{
		PM_Accelerate (wishdir, wishspeed, pm_airaccelerate);
	}

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if ( pml.groundPlane ) {
		PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal,
			pm->ps->velocity, OVERCLIP );
	}

	PM_StepSlideMove ( qtrue );
}

/*
===================
PM_WalkMove

===================
*/
static void PM_WalkMove( void ) {
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;
	usercmd_t	cmd;
	float		accelerate;
	float		vel;

	if ( pm->waterlevel > 2 && DotProduct( pml.forward, pml.groundTrace.plane.normal ) > 0 ) {
		// begin swimming
		PM_WaterMove();
		return;
	}


	if ( PM_CheckJump () ) {
		// jumped away
		if ( pm->waterlevel > 1 ) {
			PM_WaterMove();
		} else {
			PM_AirMove();
		}
		return;
	}

	PM_Friction ();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity (pml.forward, pml.groundTrace.plane.normal, pml.forward, OVERCLIP );
	PM_ClipVelocity (pml.right, pml.groundTrace.plane.normal, pml.right, OVERCLIP );
	//
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);

	for ( i = 0 ; i < 3 ; i++ ) {
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	}
	// when going up or down slopes the wish velocity should Not be zero
//	wishvel[2] = 0;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	// clamp the speed lower if ducking
	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		if ( wishspeed > pm->ps->speed * pm_duckScale ) {
			wishspeed = pm->ps->speed * pm_duckScale;
		}
	}

	// clamp the speed lower if wading or walking on the bottom
	if ( pm->waterlevel ) {
		float	waterScale;

		waterScale = pm->waterlevel / 3.0;
		waterScale = 1.0 - ( 1.0 - pm_swimScale ) * waterScale;
		if ( wishspeed > pm->ps->speed * waterScale ) {
			wishspeed = pm->ps->speed * waterScale;
		}
	}

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
		accelerate = pm_airaccelerate;
	} else {
		accelerate = pm_accelerate;
	}

	PM_Accelerate (wishdir, wishspeed, accelerate);

	//Com_Printf("velocity = %1.1f %1.1f %1.1f\n", pm->ps->velocity[0], pm->ps->velocity[1], pm->ps->velocity[2]);
	//Com_Printf("velocity1 = %1.1f\n", VectorLength(pm->ps->velocity));

	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
		pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	} else {
		// don't reset the z velocity for slopes
//		pm->ps->velocity[2] = 0;
	}

	vel = VectorLength(pm->ps->velocity);

	// slide along the ground plane
	PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal,
		pm->ps->velocity, OVERCLIP );

	if ( !( pm->bounceFix && vel > VectorLength(pm->ps->velocity) * 2 ) ) {
		// don't decrease velocity when going up or down a slope
		VectorNormalize(pm->ps->velocity);
		VectorScale(pm->ps->velocity, vel, pm->ps->velocity);
	}

	// don't do anything if standing still
	if (!pm->ps->velocity[0] && !pm->ps->velocity[1]) {
		return;
	}

	PM_StepSlideMove( qfalse );

	//Com_Printf("velocity2 = %1.1f\n", VectorLength(pm->ps->velocity));

}


/*
==============
PM_DeadMove
==============
*/
static void PM_DeadMove( void ) {
	float	forward;

	if ( !pml.walking ) {
		return;
	}

	// extra friction

	forward = VectorLength (pm->ps->velocity);
	forward -= 20;
	if ( forward <= 0 ) {
		VectorClear (pm->ps->velocity);
	} else {
		VectorNormalize (pm->ps->velocity);
		VectorScale (pm->ps->velocity, forward, pm->ps->velocity);
	}
}


/*
===============
PM_NoclipMove
===============
*/
static void PM_NoclipMove( void ) {
	float	speed, drop, friction, control, newspeed;
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	speed = VectorLength (pm->ps->velocity);
	if (speed < 1)
	{
		VectorCopy (vec3_origin, pm->ps->velocity);
	}
	else
	{
		drop = 0;

		friction = pm_friction*1.5;	// extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control*friction*pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale (pm->ps->velocity, newspeed, pm->ps->velocity);
	}

	// accelerate
	scale = PM_CmdScale( &pm->cmd );

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	for (i=0 ; i<3 ; i++)
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	wishvel[2] += pm->cmd.upmove;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	// move
	VectorMA (pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin);
}


/*
===================
PM_FreezeMove

===================
*/

static void PM_FreezeMove( void )
{
	trace_t		trace;
	short		temp, i;
	vec3_t		moveto;

	pm->mins[0] = -15;
	pm->mins[1] = -15;

	pm->maxs[0] = 15;
	pm->maxs[1] = 15;

	pm->mins[2] = MINS_Z;
	// stand up if possible
	if (pm->ps->pm_flags & PMF_DUCKED)
	{
		// try to stand up
		pm->maxs[2] = 32;
		pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask );
		if (!trace.allsolid)
			pm->ps->pm_flags &= ~PMF_DUCKED;
	}

	if (pm->ps->pm_flags & PMF_DUCKED)
	{
		pm->maxs[2] = 16;
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;
	}
	else
	{
		pm->maxs[2] = 32;
		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
	}

	// circularly clamp the angles with deltas
	for (i=0 ; i<3 ; i++) {
		temp = pm->cmd.angles[i] + pm->ps->delta_angles[i];
		if ( i == PITCH ) {
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 ) {
				pm->ps->delta_angles[i] = 16000 - pm->cmd.angles[i];
				temp = 16000;
			} else if ( temp < -16000 ) {
				pm->ps->delta_angles[i] = -16000 - pm->cmd.angles[i];
				temp = -16000;
			}
		}
//		ps->viewangles[i] = SHORT2ANGLE(temp);		// Clear the view angles, but don't set them.
	}

	VectorCopy (pm->ps->origin, moveto);
	moveto[2] -= 16;

	// test the player position if they were a stepheight higher
	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, moveto, pm->ps->clientNum, pm->tracemask);
	if ( trace.fraction < 1.0)
	{	// Something just below, snap to it, to prevent a little "hop" after the holodeck fades in.
		VectorCopy (trace.endpos, pm->ps->origin);
		pm->ps->groundEntityNum = trace.entityNum;

		// Touch it.
		PM_AddTouchEnt( trace.entityNum );

	}
}


//============================================================================

/*
================
PM_FootstepForSurface

Returns an event number apropriate for the groundsurface
================
*/
static int PM_FootstepForSurface( void ) {
	if ( pml.groundTrace.surfaceFlags & SURF_NOSTEPS ) {
		return 0;
	}
	if ( pml.groundTrace.surfaceFlags & SURF_METALSTEPS ) {
		return EV_FOOTSTEP_METAL;
	}
	return EV_FOOTSTEP;
}


/*
=================
PM_CrashLand

Check for hard landings that generate sound events
=================
*/
static void PM_CrashLand( void ) {
	float		delta;
	float		dist;
	float		vel, acc;
	float		t;
	float		a, b, c, den;

	// decide which landing animation to use
	if ( pm->ps->pm_flags & PMF_BACKWARDS_JUMP ) {
		PM_ForceLegsAnim( LEGS_LANDB );
	} else {
		PM_ForceLegsAnim( LEGS_LAND );
	}

	pm->ps->legsTimer = TIMER_LAND;

	// calculate the exact velocity on landing
	dist = pm->ps->origin[2] - pml.previous_origin[2];
	vel = pml.previous_velocity[2];
	acc = -pm->ps->gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den =  b * b - 4 * a * c;
	if ( den < 0 ) {
		return;
	}
	t = (-b - sqrt( den ) ) / ( 2 * a );

	delta = vel + t * acc;
	delta = delta*delta * 0.0001;

	// ducking while falling doubles damage
	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		delta *= 2;
	}

	// never take falling damage if completely underwater
	if ( pm->waterlevel == 3 ) {
		return;
	}

	// reduce falling damage if there is standing water
	if ( pm->waterlevel == 2 ) {
		delta *= 0.25;
	}
	if ( pm->waterlevel == 1 ) {
		delta *= 0.5;
	}

	if ( delta < 1 ) {
		return;
	}

	// create a local entity event to play the sound

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	// NOTE: INFILTRATORs land softly
	if ( !(pml.groundTrace.surfaceFlags & SURF_NODAMAGE) )  {
		if ( delta > 60 && pm->ps->persistant[PERS_CLASS] != PC_INFILTRATOR ) {
			PM_AddEvent( EV_FALL_FAR );
		} else if ( delta > 40 && pm->ps->persistant[PERS_CLASS] != PC_INFILTRATOR ) {
			// this is a pain grunt, so don't play it if dead
			if ( pm->ps->stats[STAT_HEALTH] > 0 ) {
				PM_AddEvent( EV_FALL_MEDIUM );
			}
		} else if ( delta > 7 ) {
			PM_AddEvent( EV_FALL_SHORT );
		} else {
			PM_AddEvent( PM_FootstepForSurface() );
		}
	}

	// start footstep cycle over
	pm->ps->bobCycle = 0;
}



/*
=============
PM_CorrectAllSolid
=============
*/
static void PM_CorrectAllSolid( void ) {
	if ( pm->debugLevel ) {
		Com_Printf("%i:allsolid\n", c_pmove);
	}

	// FIXME: jitter around

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
}


/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
static void PM_GroundTraceMissed( void ) {
	trace_t		trace;
	vec3_t		point;

	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {
		// we just transitioned into freefall
		if ( pm->debugLevel ) {
			Com_Printf("%i:lift\n", c_pmove);
		}

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy( pm->ps->origin, point );
		point[2] -= 64;

		pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
		if ( trace.fraction == 1.0 ) {
			if ( pm->cmd.forwardmove >= 0 ) {
				PM_ForceLegsAnim( LEGS_JUMP );
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			} else {
				PM_ForceLegsAnim( LEGS_JUMPB );
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
}


/*
=============
PM_GroundTrace
=============
*/
static void PM_GroundTrace( void ) {
	vec3_t		point;
	trace_t		trace;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] - 0.25;

	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if ( trace.allsolid ) {
		PM_CorrectAllSolid();
		PM_AddTouchEnt( trace.entityNum );	//well, we hit something, maybe it will be kind and move if we ask it nicely...
		return;
	}

	// if the trace didn't hit anything, we are in free fall
	if ( trace.fraction == 1.0 ) {
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// check if getting thrown off the ground
	if ( pm->ps->velocity[2] > 0 && DotProduct( pm->ps->velocity, trace.plane.normal ) > 10 ) {
		if ( pm->debugLevel ) {
			Com_Printf("%i:kickoff\n", c_pmove);
		}
		// go into jump animation
		if ( pm->cmd.forwardmove >= 0 ) {
			PM_ForceLegsAnim( LEGS_JUMP );
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		} else {
			PM_ForceLegsAnim( LEGS_JUMPB );
			pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		}

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// slopes that are too steep will not be considered onground
	if ( trace.plane.normal[2] < MIN_WALK_NORMAL ) {
		if ( pm->debugLevel ) {
			Com_Printf("%i:steep\n", c_pmove);
		}
		// FIXME: if they can't slide down the slope, let them
		// walk (sharp crevices)
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qtrue;
		pml.walking = qfalse;
		return;
	}

	pml.groundPlane = qtrue;
	pml.walking = qtrue;

	// hitting solid ground will end a waterjump
	if (pm->ps->pm_flags & PMF_TIME_WATERJUMP)
	{
		pm->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		pm->ps->pm_time = 0;
	}

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
		// just hit the ground
		if ( pm->debugLevel ) {
			Com_Printf("%i:Land\n", c_pmove);
		}

		PM_CrashLand();

		// don't do landing time if we were just going down a slope
		if ( pml.previous_velocity[2] < -200 ) {
			// don't allow another jump for a little while
			pm->ps->pm_flags |= PMF_TIME_LAND;
			pm->ps->pm_time = 250;
		}
	}

	pm->ps->groundEntityNum = trace.entityNum;

	// don't reset the z velocity for slopes
//	pm->ps->velocity[2] = 0;

	PM_AddTouchEnt( trace.entityNum );
}


/*
=============
PM_SetWaterLevel	FIXME: avoid this twice?  certainly if not moving
=============
*/
static void PM_SetWaterLevel( void ) {
	vec3_t		point;
	int			cont;
	int			sample1;
	int			sample2;

	//
	// get waterlevel, accounting for ducking
	//
	pm->waterlevel = 0;
	pm->watertype = 0;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + MINS_Z + 1;
	cont = pm->pointcontents( point, pm->ps->clientNum );

	if ( cont & (MASK_WATER|CONTENTS_LADDER) ) {
		sample2 = pm->ps->viewheight - MINS_Z;
		sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
		point[2] = pm->ps->origin[2] + MINS_Z + sample1;
		cont = pm->pointcontents (point, pm->ps->clientNum );
		if ( cont & (MASK_WATER|CONTENTS_LADDER) ) {
			pm->waterlevel = 2;
			point[2] = pm->ps->origin[2] + MINS_Z + sample2;
			cont = pm->pointcontents (point, pm->ps->clientNum );
			if ( cont & (MASK_WATER|CONTENTS_LADDER) ){
				pm->waterlevel = 3;
			}
		}
	}

}



/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->ps->viewheight
==============
*/
static void PM_CheckDuck (void)
{
	trace_t	trace;

	pm->mins[0] = -15;
	pm->mins[1] = -15;

	pm->maxs[0] = 15;
	pm->maxs[1] = 15;

	pm->mins[2] = MINS_Z;

	if (pm->ps->pm_type == PM_DEAD)
	{
		pm->maxs[2] = -8;
		pm->ps->viewheight = DEAD_VIEWHEIGHT;
		return;
	}

	if (pm->cmd.upmove < 0)
	{	// duck
		pm->ps->pm_flags |= PMF_DUCKED;
	}
	else
	{	// stand up if possible
		if (pm->ps->pm_flags & PMF_DUCKED)
		{
			// try to stand up
			pm->maxs[2] = 32;
			pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask );
			if (!trace.allsolid)
				pm->ps->pm_flags &= ~PMF_DUCKED;
		}
	}

	if (pm->ps->pm_flags & PMF_DUCKED)
	{
		pm->maxs[2] = 16;
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;
	}
	else
	{
		pm->maxs[2] = 32;
		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
	}
}



//===================================================================


/*
===============
PM_Footsteps
===============
*/
static void PM_Footsteps( void ) {
	float		bobmove;
	int			old;
	qboolean	footstep;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	pm->xyspeed = sqrt( pm->ps->velocity[0] * pm->ps->velocity[0]
		+  pm->ps->velocity[1] * pm->ps->velocity[1] );

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
		if (pm->watertype & CONTENTS_LADDER) {
		} else {
			// airborne leaves position in cycle intact, but doesn't advance
			if ( pm->waterlevel > 1 ) {
				PM_ContinueLegsAnim( LEGS_SWIM );
			} else if ( pm->ps->pm_flags & PMF_DUCKED ) {
				PM_ContinueLegsAnim( LEGS_IDLECR );
			}

			return;
		}
	}

	// if not trying to move
	if ( !pm->cmd.forwardmove && !pm->cmd.rightmove ) {
		if (  pm->xyspeed < 5 ) {
			pm->ps->bobCycle = 0;	// start at beginning of cycle again
			if ( pm->ps->pm_flags & PMF_DUCKED ) {
				PM_ContinueLegsAnim( LEGS_IDLECR );
			} else {
				PM_ContinueLegsAnim( LEGS_IDLE );
			}
		}
		return;
	}


	footstep = qfalse;

	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		bobmove = 0.5;	// ducked characters bob much faster
		PM_ContinueLegsAnim( LEGS_WALKCR );
		// ducked characters never play footsteps
	} else 	if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
		if ( !( pm->cmd.buttons & BUTTON_WALKING ) ) {
			bobmove = 0.4;	// faster speeds bob faster
			footstep = qtrue;
		} else {
			bobmove = 0.3;
		}
		PM_ContinueLegsAnim( LEGS_BACK );
	} else {
		if ( !( pm->cmd.buttons & BUTTON_WALKING ) ) {
			bobmove = 0.4;	// faster speeds bob faster
			PM_ContinueLegsAnim( LEGS_RUN );
			footstep = qtrue;
		} else {
			bobmove = 0.3;	// walking bobs slow
			PM_ContinueLegsAnim( LEGS_WALK );
		}
	}

	// check for footstep / splash sounds
	old = pm->ps->bobCycle;
	pm->ps->bobCycle = (int)( old + bobmove * pml.msec ) & 255;

	// if we just crossed a cycle boundary, play an apropriate footstep event
	if ( ( ( old + 64 ) ^ ( pm->ps->bobCycle + 64 ) ) & 128 ) {
		if ( pm->watertype & CONTENTS_LADDER ) {// on ladder
			if ( !pm->noFootsteps ) {
				PM_AddEvent( EV_FOOTSTEP_METAL );
			}
		} else if ( pm->waterlevel == 0 ) {
			// on ground will only play sounds if running
			if ( footstep && !pm->noFootsteps ) {
				PM_AddEvent( PM_FootstepForSurface() );
			}
		} else if ( pm->waterlevel == 1 ) {
			// splashing
			PM_AddEvent( EV_FOOTSPLASH );
		} else if ( pm->waterlevel == 2 ) {
			// wading / swimming at surface
			PM_AddEvent( EV_SWIM );
		} else if ( pm->waterlevel == 3 ) {
			// no sound when completely underwater

		}
	}
}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents( void ) {		// FIXME?
	if ( pm->watertype & CONTENTS_LADDER )	//fake water for ladder
	{
		return;
	}
	//
	// if just entered a water volume, play a sound
	//
	if (!pml.previous_waterlevel && pm->waterlevel) {
		PM_AddEvent( EV_WATER_TOUCH );
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if (pml.previous_waterlevel && !pm->waterlevel) {
		PM_AddEvent( EV_WATER_LEAVE );
	}

	//
	// check for head just going under water
	//
	if (pml.previous_waterlevel != 3 && pm->waterlevel == 3) {
		PM_AddEvent( EV_WATER_UNDER );
	}

	//
	// check for head just coming out of water
	//
	if (pml.previous_waterlevel == 3 && pm->waterlevel != 3) {
		PM_AddEvent( EV_WATER_CLEAR );
	}
}


/*
===============
PM_BeginWeaponChange
===============
*/
static void PM_BeginWeaponChange( int weapon ) {
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		return;
	}

	if ( !( pm->ps->stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		return;
	}

	PM_AddEvent( EV_CHANGE_WEAPON );
	pm->ps->weaponstate = WEAPON_DROPPING;
	pm->ps->weaponTime += 200;
	PM_StartTorsoAnim( TORSO_DROP );
}


/*
===============
PM_FinishWeaponChange
===============
*/
static void PM_FinishWeaponChange( void ) {
	int		weapon;

	weapon = pm->cmd.weapon;
	if ( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		weapon = WP_NONE;
	}

	if ( !( pm->ps->stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
		weapon = WP_NONE;
	}

	pm->ps->weapon = weapon;
	pm->ps->weaponstate = WEAPON_RAISING;
	pm->ps->weaponTime += 250;
	PM_StartTorsoAnim( TORSO_RAISE );
}


/*
==============
PM_TorsoAnimation

==============
*/
static void PM_TorsoAnimation( void )
{
	if ( pm->ps->weaponstate == WEAPON_READY )
	{
		if ( pm->ps->weapon == WP_PHASER )
		{
			PM_ContinueTorsoAnim( TORSO_STAND2 );
		}
		else
		{
			PM_ContinueTorsoAnim( TORSO_STAND );
		}
	}
	return;
}


#define PHASER_AMMO_PER_SHOT			1
#define PHASER_ALT_AMMO_PER_SHOT		2

// alt ammo usage
int		altAmmoUsage[WP_NUM_WEAPONS] =
{
	0,				//WP_NONE,
	1,				//WP_PHASER,
	8,				//WP_COMPRESSION_RIFLE,
	3,				//WP_IMOD,
	5,				//WP_SCAVENGER_RIFLE,
	1,				//WP_STASIS,
	1,				//WP_GRENADE_LAUNCHER,
	2,				//WP_TETRION_DISRUPTOR,
	2,				//WP_QUANTUM_BURST,
	5,				//WP_DREADNOUGHT,
	20,				//WP_VOYAGER_HYPO,
	1,				//WP_BORG_ASSIMILATOR,
	1				//WP_BORG_WEAPON

};

/*
==============
PM_Weapon

Generates weapon events and modifes the weapon counter
==============
*/
static void PM_Weapon( void ) {
	int			addTime;
	qboolean	altfired = qfalse;

	// don't allow attack until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return;
	}

	// ignore if spectator
	if ( pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR || (pm->ps->eFlags&EF_ELIMINATED) ) {
		return;
	}

	if ( pm->ps->persistant[PERS_CLASS] == PC_BORG )
	{//Borg weapons recharge
		if ( pm->ps->rechargeTime <= 0 && ( pm->ps->ammo[WP_BORG_ASSIMILATOR] < Max_Ammo[WP_BORG_ASSIMILATOR] || pm->ps->ammo[WP_BORG_WEAPON] < Max_Ammo[WP_BORG_WEAPON] ) )
		{
			pm->ps->rechargeTime = PHASER_RECHARGE_TIME*4;//recharges slower than phaser
			if ( pm->ps->ammo[WP_BORG_ASSIMILATOR] < Max_Ammo[WP_BORG_ASSIMILATOR] )
			{
				pm->ps->ammo[WP_BORG_ASSIMILATOR]++;
			}
			if ( pm->ps->ammo[WP_BORG_WEAPON] < Max_Ammo[WP_BORG_WEAPON] )
			{
				pm->ps->ammo[WP_BORG_WEAPON]++;
			}
		}
		else
		{
			pm->ps->rechargeTime -= pml.msec;
		}
	}
	else
	{//Check for phaser ammo recharge
		if ( pm->ps->rechargeTime <= 0 && pm->ps->ammo[WP_PHASER] < PHASER_AMMO_MAX )
		{//phaser recharges
			pm->ps->rechargeTime = PHASER_RECHARGE_TIME;
			pm->ps->ammo[WP_PHASER]++;
			//FIXME: recharge all ammo in specialty mode?
			//do it slower for more powerful weapons
		}
		else
		{
			pm->ps->rechargeTime -= pml.msec;
		}
	}

	// check for dead player
	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->ps->weapon = WP_NONE;
		return;
	}

	// check for item using
	if ( pm->cmd.buttons & BUTTON_USE_HOLDABLE )
	{
		if ( ! ( pm->ps->pm_flags & PMF_USE_ITEM_HELD ) )
		{
			int tag = bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag;

			/*
			//commented out, too non-intuitive
			if ( (HI_DETPACK == tag) && ( pm->ps->introTime > pm->cmd.serverTime || pm->ps->powerups[PW_GHOST] >= pm->cmd.serverTime ) )
			{//can't use detpack while in intro or ghosted
				pm->cmd.buttons &= ~BUTTON_USE_HOLDABLE;
				pm->ps->pm_flags &= ~PMF_USE_ITEM_HELD;
			}
			else
			*/
			{
				pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
				PM_AddEvent( EV_USE_ITEM0 + tag );
				// if we're placing the detpack, don't remove it from our "inventory"
				// if we're a Borg, the transporter has a two-step item usage as well
				if ( ((HI_DETPACK == tag) || (HI_TRANSPORTER == tag && pm->ps->persistant[PERS_CLASS] == PC_BORG ) )  &&
					(IT_HOLDABLE == bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giType) )
				{
					// are we placing it?
					if ( pm->ps->stats[STAT_USEABLE_PLACED] == 2 )
					{
						// we've placed the first stage of a 2-stage transporter or are counting down to respawning an item
					}
					else if ( pm->ps->stats[STAT_USEABLE_PLACED] )
					{
						// we already placed it, we're activating it.
						pm->ps->stats[STAT_HOLDABLE_ITEM] = 0;
					}
				}
				else
				{
					pm->ps->stats[STAT_HOLDABLE_ITEM] = 0;
				}
				return;
			}
		}
	}
	else
	{
		pm->ps->pm_flags &= ~PMF_USE_ITEM_HELD;
	}

	if ( BG_BorgTransporting( pm->ps ) )
	{//in borg teleport dest picking-mode
		return;
	}

	// make weapon function
	if ( pm->ps->weaponTime > 0 ) {
		pm->ps->weaponTime -= pml.msec;
	}

	// check for weapon change
	// can't change if weapon is firing, but can change
	// again if lowering or raising
	if ( pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING ) {
		if ( pm->ps->weapon != pm->cmd.weapon ) {
			PM_BeginWeaponChange( pm->cmd.weapon );
		}
	}

	if ( pm->ps->weaponTime > 0 ) {
		return;
	}

	// change weapon if time
	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		PM_FinishWeaponChange();
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_RAISING )
	{
		pm->ps->weaponstate = WEAPON_READY;
		if ( pm->ps->weapon == WP_PHASER )
		{
			PM_StartTorsoAnim( TORSO_STAND2 );
		}
		else
		{
			PM_StartTorsoAnim( TORSO_STAND );
		}
		return;
	}

	// check for fire
	if ( !(pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK) ) )
	{
		pm->ps->weaponTime = 0;
		pm->ps->weaponstate = WEAPON_READY;

		return;
	}

	//hypo ammo uses same as phaser, for now
	pm->ps->ammo[WP_VOYAGER_HYPO] = pm->ps->ammo[WP_PHASER];

	// take an ammo away if not infinite
	if ( pm->ps->ammo[ pm->ps->weapon ] != -1 )
	{
		if (pm->cmd.buttons & BUTTON_ALT_ATTACK)
		{
			// alt fire
			// check for out of ammo
			int ammoUsage = altAmmoUsage[pm->ps->weapon];
#ifdef MODULE_GAME
			if ( pm->modifyAmmoUsage ) {
				ammoUsage = pm->modifyAmmoUsage( ammoUsage, pm->ps->weapon, qtrue );
			}
#endif
			if ( pm->ps->ammo[pm->ps->weapon] < ammoUsage )
			{
				//FIXME: flash a message and sound that indicates not enough ammo
//				PM_AddEvent( EV_NOAMMO_ALT );
//				pm->ps->weaponTime += 500;
//				pm->ps->eFlags &= ~EF_ALT_FIRING;
//				pm->ps->eFlags &= ~EF_FIRING;
///				pm->ps->weaponstate = WEAPON_READY;

				if ( pm->ps->weapon == WP_PHASER ) // phaser out of ammo is special case
				{
					pm->ps->ammo[pm->ps->weapon] = 0;
				}
				else
				{
					PM_AddEvent( EV_NOAMMO_ALT );		// Just try to switch weapons like any other.

	// check out the EV_NOAMMO_ALT event

					pm->ps->weaponTime += 500;
					return;
				}
			}
			else
			{
				pm->ps->ammo[pm->ps->weapon] -= ammoUsage;
				altfired = qtrue;
			}
		}
		else
		{
			// check for out of ammo
			int ammoUsage = 1;
#ifdef MODULE_GAME
			if ( pm->modifyAmmoUsage ) {
				ammoUsage = pm->modifyAmmoUsage( ammoUsage, pm->ps->weapon, qfalse );
			}
#endif
			if ( pm->ps->ammo[pm->ps->weapon] < ammoUsage )
			{
				if ( pm->ps->weapon == WP_PHASER ) // phaser out of ammo is special case
				{
					pm->ps->ammo[pm->ps->weapon] = 0;
				}
				else
				{
					PM_AddEvent( EV_NOAMMO );
					pm->ps->weaponTime += 500;
					return;
				}
			}
			else
			{
				pm->ps->ammo[pm->ps->weapon] -= ammoUsage;
			}
		}
	}

	// *don't* start the animation if out of ammo
	if ( pm->ps->weapon == WP_PHASER )
	{
		PM_StartTorsoAnim( TORSO_ATTACK2 );
	}
	else
	{
		PM_StartTorsoAnim( TORSO_ATTACK );
	}

	pm->ps->weaponstate = WEAPON_FIRING;

	// fire weapon
	if ( pm->cmd.buttons & BUTTON_ALT_ATTACK )
	{
		if (altfired) // it's either a legally altfirable non-phaser, or it's a phaser with ammo left
		{
			PM_AddEvent( EV_ALT_FIRE ); // fixme, because I'm deducting ammo earlier, the last alt-fire shot turns into a main fire
		}
		else
		{
			PM_AddEvent( EV_FIRE_EMPTY_PHASER );
		}
		switch( pm->ps->weapon ) {
		default:
		case WP_PHASER:
			addTime = 100;
			//If the phaser has been fired, delay the next recharge time
			pm->ps->rechargeTime = PHASER_RECHARGE_TIME;
			break;
		case WP_DREADNOUGHT:
			addTime = 500;
			break;
		case WP_GRENADE_LAUNCHER:
			addTime = 700;
			break;
		case WP_STASIS:
			addTime = 700;
			break;
		case WP_SCAVENGER_RIFLE:
			addTime = 700;
			break;
		case WP_QUANTUM_BURST:
			addTime = 1600;
			break;
		case WP_IMOD:
			addTime = 700;
			break;
		case WP_COMPRESSION_RIFLE:
			addTime = 1200;
			break;
		case WP_TETRION_DISRUPTOR:
			addTime = 200;
			break;
		case WP_VOYAGER_HYPO:
			addTime = 1000;
			break;
		case WP_BORG_ASSIMILATOR:
			addTime = 100;
			//If the assimilator has been used, delay the next recharge time
			pm->ps->rechargeTime = PHASER_RECHARGE_TIME*4;
			break;
		case WP_BORG_WEAPON:
			addTime = 300;
			//If the assimilator has been used, delay the next recharge time
			pm->ps->rechargeTime = PHASER_RECHARGE_TIME*4;
			break;
		}
	}
	else
	{
		if (pm->ps->ammo[pm->ps->weapon])
		{
			PM_AddEvent( EV_FIRE_WEAPON );
		}
		else
		{
			PM_AddEvent( EV_FIRE_EMPTY_PHASER );
		}
		switch( pm->ps->weapon ) {
		default:
		case WP_PHASER:
			addTime = 100;
			//If the phaser has been fired, delay the next recharge time
			pm->ps->rechargeTime = 2*PHASER_RECHARGE_TIME;
			break;
		case WP_DREADNOUGHT:
			addTime = 100;
			break;
		case WP_GRENADE_LAUNCHER:
			addTime = 700;
			break;
		case WP_STASIS:
			addTime = 700;
			break;
		case WP_SCAVENGER_RIFLE:
			addTime = 100;
			break;
		case WP_QUANTUM_BURST:
			addTime = 1200;
			break;
		case WP_IMOD:
			addTime = 350;
			break;
		case WP_COMPRESSION_RIFLE:
			addTime = 250;
			break;
		case WP_TETRION_DISRUPTOR:
			addTime = 100;
			break;
		case WP_VOYAGER_HYPO:
			addTime = 100;
			break;
		case WP_BORG_ASSIMILATOR:
			addTime = 100;
			//If the assimilator has been used, delay the next recharge time
			pm->ps->rechargeTime = PHASER_RECHARGE_TIME*4;
			break;
		case WP_BORG_WEAPON:
			addTime = 500;
			//If the assimilator has been used, delay the next recharge time
			pm->ps->rechargeTime = PHASER_RECHARGE_TIME*4;
			break;
		}
	}

	if ( pm->modifyFireRate ) {
		qboolean altAttack = ( pm->cmd.buttons & BUTTON_ALT_ATTACK ) ? qtrue : qfalse;
		addTime = pm->modifyFireRate( addTime, pm->ps->weapon, altAttack );
	}

	if ( pm->ps->powerups[PW_HASTE] ) {
		addTime /= 1.3;
	}

	pm->ps->weaponTime += addTime;
}

/*
================
PM_Animate
================
*/
static void PM_Animate( void ) {
	if ( pm->cmd.buttons & BUTTON_GESTURE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GESTURE );
			pm->ps->torsoTimer = TIMER_GESTURE;
			PM_AddEvent( EV_TAUNT );
		}
	}
}


/*
================
PM_DropTimers
================
*/
static void PM_DropTimers( void ) {
	// drop misc timing counter
	if ( pm->ps->pm_time ) {
		if ( pml.msec >= pm->ps->pm_time ) {
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		} else {
			pm->ps->pm_time -= pml.msec;
		}
	}

	// drop animation counter
	if ( pm->ps->legsTimer > 0 ) {
		pm->ps->legsTimer -= pml.msec;
		if ( pm->ps->legsTimer < 0 ) {
			pm->ps->legsTimer = 0;
		}
	}

	if ( pm->ps->torsoTimer > 0 ) {
		pm->ps->torsoTimer -= pml.msec;
		if ( pm->ps->torsoTimer < 0 ) {
			pm->ps->torsoTimer = 0;
		}
	}
}

/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated isntead of a full move
================
*/
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd ) {
	short		temp;
	int		i;

	if ( ps->pm_type == PM_INTERMISSION ) {
		return;		// no view changes at all
	}

	if ( ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0 ) {
		return;		// no view changes at all
	}

	// circularly clamp the angles with deltas
	for (i=0 ; i<3 ; i++) {
		temp = cmd->angles[i] + ps->delta_angles[i];
		if ( i == PITCH ) {
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 ) {
				ps->delta_angles[i] = 16000 - cmd->angles[i];
				temp = 16000;
			} else if ( temp < -16000 ) {
				ps->delta_angles[i] = -16000 - cmd->angles[i];
				temp = -16000;
			}
		}
		ps->viewangles[i] = SHORT2ANGLE(temp);
	}

}


/*
================
PmoveSingle

================
*/
void PmoveSingle (pmove_t *pmove) {
	pm = pmove;

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint fot the previous frame
	c_pmove++;

	// clear results
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;

	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->tracemask &= ~CONTENTS_BODY;	// corpses can fly through bodies
	}

	// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
	if ( abs( pm->cmd.forwardmove ) > 64 || abs( pm->cmd.rightmove ) > 64 ) {
		pm->cmd.buttons &= ~BUTTON_WALKING;
	}

	// set the talk balloon flag
	if ( pm->cmd.buttons & BUTTON_TALK ) {
		pm->ps->eFlags |= EF_TALK;

		// dissallow all other input to prevent possible fake talk balloons
		pmove->cmd.buttons = 0;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		if (pmove->cmd.upmove)
		{
			if (pmove->cmd.upmove > 0)
			{
				pmove->cmd.upmove = 1;
			}
			else
			{
				pmove->cmd.upmove = -1;//allow a tiny bit to keep the duck anim
			}
		}
	} else {
		pm->ps->eFlags &= ~EF_TALK;
	}

	// clear the respawned flag if attack and use are cleared
	if ( pm->ps->stats[STAT_HEALTH] > 0 &&
		!( pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE) ) ) {
		pm->ps->pm_flags &= ~PMF_RESPAWNED;
	}

	// perform alt attack modification
	if ( pm->altFireMode == ALTMODE_SWAPPED ) {
		if ( pm->cmd.buttons & BUTTON_ALT_ATTACK ) {
			pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
			pm->cmd.buttons |= BUTTON_ATTACK;
		} else if ( pm->cmd.buttons & BUTTON_ATTACK ) {
			pm->cmd.buttons |= ( BUTTON_ATTACK | BUTTON_ALT_ATTACK );
		}
	} else if ( pm->altFireMode == ALTMODE_PRIMARY_ONLY ) {
		if ( pm->cmd.buttons & ( BUTTON_ATTACK | BUTTON_ALT_ATTACK ) ) {
			pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
			pm->cmd.buttons |= BUTTON_ATTACK;
		}
	} else if ( pm->altFireMode == ALTMODE_ALT_ONLY ) {
		if ( pm->cmd.buttons & ( BUTTON_ATTACK | BUTTON_ALT_ATTACK ) ) {
			pm->cmd.buttons |= ( BUTTON_ATTACK | BUTTON_ALT_ATTACK );
		}
	}

	// set the firing flag for continuous beam weapons
	if (	!(pm->ps->pm_flags & PMF_RESPAWNED) &&
			pm->ps->pm_type != PM_INTERMISSION &&
			!(pm->ps->introTime > pm->cmd.serverTime) &&
			( (pm->cmd.buttons & BUTTON_ATTACK) || (pm->cmd.buttons & BUTTON_ALT_ATTACK) ) &&
			(pm->ps->ammo[ pm->ps->weapon ] || pm->ps->weapon == WP_PHASER))
	{
		if ( pm->pModDisintegration )
		{//in disintegration mode, all attacks are alt attacks
			pm->cmd.buttons &=~BUTTON_ATTACK;
			pm->cmd.buttons |= BUTTON_ALT_ATTACK;
		}
		if ( pm->ps->persistant[PERS_CLASS] == PC_TECH && pm->ps->weapon == WP_DREADNOUGHT )
		{//tech can't use alt attack
			pm->cmd.buttons &=~BUTTON_ALT_ATTACK;
			pm->cmd.buttons |= BUTTON_ATTACK;
		}

		if (((pm->ps->weapon == WP_PHASER) && (!pm->ps->ammo[ pm->ps->weapon ])) || (!(pm->cmd.buttons & BUTTON_ALT_ATTACK)))
		{
			pm->ps->eFlags &= ~EF_ALT_FIRING;
		}
		else // if ( pm->cmd.buttons & BUTTON_ALT_ATTACK ) <-- implied
		{
			pm->ps->eFlags |= EF_ALT_FIRING;
		}
		// This flag should always get set, even when alt-firing
		pm->ps->eFlags |= EF_FIRING;
	}
	else
	{
		pm->ps->eFlags &= ~EF_FIRING;
		pm->ps->eFlags &= ~EF_ALT_FIRING;
	}

	// clear all pmove local vars
	memset (&pml, 0, sizeof(pml));

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if ( pml.msec < 1 ) {
		pml.msec = 1;
	} else if ( pml.msec > 200 ) {
		pml.msec = 200;
	}
	pm->ps->commandTime = pmove->cmd.serverTime;

	// save old org in case we get stuck
	VectorCopy (pm->ps->origin, pml.previous_origin);

	// save old velocity for crashlanding
	VectorCopy (pm->ps->velocity, pml.previous_velocity);

	pml.frametime = pml.msec * 0.001;

	if (pm->ps->pm_type == PM_FREEZE || pm->ps->introTime > pm->cmd.serverTime)
	{
		PM_FreezeMove();
		return;		// no movement at all
	}

	// update the viewangles
	PM_UpdateViewAngles( pm->ps, &pm->cmd );

	AngleVectors (pm->ps->viewangles, pml.forward, pml.right, pml.up);

	if ( pm->cmd.upmove < 10 ) {
		// not holding jump
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;
	}

	// decide if backpedaling animations should be used
	if ( pm->cmd.forwardmove < 0 ) {
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	} else if ( pm->cmd.forwardmove > 0 || ( pm->cmd.forwardmove == 0 && pm->cmd.rightmove ) ) {
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	}

	if ( pm->ps->pm_type >= PM_DEAD ) {
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR || (pm->ps->eFlags&EF_ELIMINATED) ) {
		//spectator or an eliminated player
		PM_CheckDuck ();
		PM_FlyMove ();
		PM_DropTimers ();
		return;
	}

	if ( BG_BorgTransporting( pm->ps ) ) {
		//in Borg pick teleport dest mode
		PM_CheckDuck ();
		PM_FlyMove ();
		PM_DropTimers ();
		PM_Weapon();
		return;
	}

	if ( pm->ps->pm_type == PM_NOCLIP ) {
		PM_NoclipMove ();
		PM_DropTimers ();
		return;
	}

	if ( pm->ps->pm_type == PM_INTERMISSION ) {
		return;		// no movement at all
	}

	// set watertype, and waterlevel
	PM_SetWaterLevel();
	if ( !(pm->watertype & CONTENTS_LADDER) )
	{//Don't want to remember this for ladders, is only for waterlevel change events (sounds)
		pml.previous_waterlevel = pmove->waterlevel;
	}

	// set mins, maxs, and viewheight
	PM_CheckDuck ();

	// set groundentity
	PM_GroundTrace();

	if ( pm->ps->pm_type == PM_DEAD ) {
		PM_DeadMove ();
	}

	PM_DropTimers();

	if ( pm->ps->powerups[PW_FLIGHT] ) {
		// flight powerup doesn't allow jump and has different friction
		PM_FlyMove();
	} else if (pm->ps->pm_flags & PMF_TIME_WATERJUMP) {
		PM_WaterJumpMove();
	} else if ( pm->waterlevel > 1 ) {
		// swimming
		PM_WaterMove();
	} else if ( pml.walking ) {
		// walking on ground
		PM_WalkMove();
	} else {
		// airborne
		PM_AirMove();
	}

	PM_Animate();

	// set groundentity, watertype, and waterlevel
	PM_GroundTrace();
	PM_SetWaterLevel();

	// weapons
	PM_Weapon();

	PM_Use();

	// torso animation
	PM_TorsoAnimation();

	// footstep events / legs animations
	PM_Footsteps();

	// entering / leaving water splashes
	PM_WaterEvents();

	if ( !( pm->noFlyingDrift && pm->ps->powerups[PW_FLIGHT] ) ) {
		// snap velocity for traditional physics behavior
		if ( pm->ps->gravity < pm->snapVectorGravLimit ) {
			float oldVelocity = pm->ps->velocity[2];
			SnapVector( pm->ps->velocity );
			pm->ps->velocity[2] = oldVelocity;
		} else {
			SnapVector( pm->ps->velocity );
		}
	}
}

/*
================
PM_NextFrameBreak

Given a current command time, returns next valid frame end time according to
fixed length settings. Returned value will always be higher than input.
================
*/
static int PM_NextFrameBreak( int commandTime, int fixedLength ) {
	return ( commandTime + fixedLength ) - ( commandTime % fixedLength );
}

/*
================
PM_NextMoveTime

Given a current and target time, returns next time to move player.
Returns currentTime if no move is needed, otherwise returns a value greater than currentTime.
================
*/
int PM_NextMoveTime( int currentTime, int targetTime, int pMoveFixed ) {
	if ( currentTime > targetTime ) {
		return currentTime;
	}

	if ( pMoveFixed > 0 && pMoveFixed < 35 ) {
		int nextTime = PM_NextFrameBreak( currentTime, pMoveFixed );
		if ( nextTime > targetTime ) {
			return currentTime;
		}
		return nextTime;
	}

	if ( targetTime > currentTime + 66 ) {
		targetTime = currentTime + 66;
	}

	return targetTime;
}

/*
================
PM_IsMoveNeeded

Given a current and target time, returns whether any move is needed given the current fps settings.
================
*/
qboolean PM_IsMoveNeeded( int currentTime, int targetTime, int pMoveFixed ) {
	return PM_NextMoveTime( currentTime, targetTime, pMoveFixed ) > currentTime;
}

/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove( pmove_t *pmove, int pMoveFixed,
		void ( *postMove )( pmove_t *pmove, qboolean finalFragment, void *context ), void *postMoveContext ) {
	usercmd_t inputCmd = pmove->cmd;

	if ( inputCmd.serverTime > pmove->ps->commandTime + 1000 ) {
		pmove->ps->commandTime = inputCmd.serverTime - 1000;
	}

	while( 1 ) {
		int nextTime = PM_NextMoveTime( pmove->ps->commandTime, inputCmd.serverTime, pMoveFixed );
		if ( nextTime <= pmove->ps->commandTime ) {
			break;
		}

		pmove->cmd.serverTime = nextTime;
		PmoveSingle( pmove );

		// reset any changes to command during move
		pmove->cmd = inputCmd;

		if ( postMove ) {
			qboolean finalFragment = !PM_IsMoveNeeded( pmove->ps->commandTime,inputCmd.serverTime, pMoveFixed );
			postMove( pmove, finalFragment, postMoveContext );
		}
	}
}
