// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_weapons_predict.c -- handles client prediction of weapons fire

#include "cg_local.h"

#define WP_DEBUG cg_debugPredictWeapons.integer
#define WP_DEBUG_VERBOSE ( cg_debugPredictWeapons.integer >= 2 )

/* ----------------------------------------------------------------------- */

// Server configuration

/* ----------------------------------------------------------------------- */

static struct {
	qboolean supported;		// whether server support for weapon prediction is available
	qboolean projectiles;	// whether projectile compensation is enabled
	qboolean tripMines;		// grenade tripmines enabled
	qboolean imodRifle;		// gladiator-style instagib rifle enabled
	qboolean seekerAccel;	// pinball-style weapon acceleration with seeker powerup
	int projectileSpeed4p;
	int projectileSpeed4a;
	int projectileSpeed5p;
	int projectileSpeed6p;
	int projectileSpeed6a;
	int projectileSpeed7a;
	int projectileSpeed8p;
	int projectileSpeed8a;
	int projectileSpeed9a;
	int projectileSpeedbp;
} wepPredictCfg;

/*
================
CG_WeaponPredict_IsActive

Weapon effect prediction is considered active if the client cg_predictWeapons cvar is enabled,
server support is indicated (via modcfg configstrings), playerstate prediction is active,
and game is in a normal playing configuration (not spectator or intermission).
================
*/
qboolean CG_WeaponPredict_IsActive( void ) {
	return cg_predictWeapons.integer && wepPredictCfg.supported && EF_WARN_ASSERT( cg.snap ) &&
			!cg.demoPlayback && !cg_nopredict.integer && !cg_synchronousClients.integer &&
			!( cg.snap->ps.pm_flags & PMF_FOLLOW ) && cg.snap->ps.pm_type != PM_SPECTATOR &&
			cg.snap->ps.pm_type != PM_INTERMISSION;
}

/*
================
CG_WeaponPredict_ResetConfig

Reset configuration to clean, disabled state ahead of modconfig update.
================
*/
void CG_WeaponPredict_ResetConfig( void ) {
	memset( &wepPredictCfg, 0, sizeof( wepPredictCfg ) );
}

/*
==================
CG_WeaponPredict_LoadInteger
==================
*/
static void CG_WeaponPredict_LoadInteger( const char *key, const char *value, const char *targetKey, int *target ) {
	if ( !Q_stricmp( key, targetKey ) ) {
		*target = atoi( value );
	}
}

/*
==================
CG_WeaponPredict_LoadBoolean
==================
*/
static void CG_WeaponPredict_LoadBoolean( const char *key, const char *value, const char *targetKey, qboolean *target ) {
	if ( !Q_stricmp( key, targetKey ) ) {
		*target = atoi( value ) ? qtrue : qfalse;
	}
}

/*
==================
CG_WeaponPredict_LoadValue
==================
*/
static void CG_WeaponPredict_LoadValue( const char *key, const char *value ) {
	if ( !Q_stricmp( key, "ver" ) && (
			// Support server using either old or new version string.
			!Q_stricmp( value, "uxb09a9y" ) || !Q_stricmp( value, "uxb09a9y-2" ) ) ) {
		wepPredictCfg.supported = qtrue;
	}
	CG_WeaponPredict_LoadBoolean( key, value, "proj", &wepPredictCfg.projectiles );
	CG_WeaponPredict_LoadBoolean( key, value, "tm", &wepPredictCfg.tripMines );
	CG_WeaponPredict_LoadBoolean( key, value, "imodRifle", &wepPredictCfg.imodRifle );
	CG_WeaponPredict_LoadBoolean( key, value, "seekerAccel", &wepPredictCfg.seekerAccel );
	CG_WeaponPredict_LoadInteger( key, value, "ps4p", &wepPredictCfg.projectileSpeed4p );
	CG_WeaponPredict_LoadInteger( key, value, "ps4a", &wepPredictCfg.projectileSpeed4a );
	CG_WeaponPredict_LoadInteger( key, value, "ps5p", &wepPredictCfg.projectileSpeed5p );
	CG_WeaponPredict_LoadInteger( key, value, "ps6p", &wepPredictCfg.projectileSpeed8p );
	CG_WeaponPredict_LoadInteger( key, value, "ps6a", &wepPredictCfg.projectileSpeed8a );
	CG_WeaponPredict_LoadInteger( key, value, "ps7a", &wepPredictCfg.projectileSpeed7a );
	CG_WeaponPredict_LoadInteger( key, value, "ps8p", &wepPredictCfg.projectileSpeed8p );
	CG_WeaponPredict_LoadInteger( key, value, "ps8a", &wepPredictCfg.projectileSpeed8a );
	CG_WeaponPredict_LoadInteger( key, value, "ps9a", &wepPredictCfg.projectileSpeed9a );
	CG_WeaponPredict_LoadInteger( key, value, "psbp", &wepPredictCfg.projectileSpeedbp );
}

/*
================
CG_WeaponPredict_LoadConfig

Load configuration from "weaponPredict" modconfig string.
================
*/
void CG_WeaponPredict_LoadConfig( const char *configStr ) {
	char srcBuffer[256];
	char *current = srcBuffer;
	Q_strncpyz( srcBuffer, configStr, sizeof( srcBuffer ) );

	while ( current ) {
		char *next;
		char *value;

		next = strchr( current, ' ' );
		if ( next ) {
			*next = '\0';
			++next;
		}

		value = strchr( current, ':' );
		if ( value ) {
			*value = '\0';
			++value;
		} else {
			value = "";
		}

		CG_WeaponPredict_LoadValue( current, value );
		current = next;
	}
}

/* ----------------------------------------------------------------------- */

// Definitions from g_weapon.c

/* ----------------------------------------------------------------------- */

// Muzzle point table...
static const vec3_t CG_WP_MuzzlePoint[WP_NUM_WEAPONS] =
{//	Fwd,	right,	up.
	{0,		0,		0	},	// WP_NONE,
	{29,	2,		-4	},	// WP_PHASER,
	{25,	7,		-10	},	// WP_COMPRESSION_RIFLE,
	{25,	4,		-5	},	// WP_IMOD,
	{10,	14,		-8	},	// WP_SCAVENGER_RIFLE,
	{25,	5,		-8	},	// WP_STASIS,
	{25,	5,		-10	},	// WP_GRENADE_LAUNCHER,
	{22,	4.5,	-8	},	// WP_TETRION_DISRUPTOR,
	{5,		6,		-6	},	// WP_QUANTUM_BURST,
	{27,	8,		-10	},	// WP_DREADNOUGHT,
	{29,	2,		-4	},	// WP_VOYAGER_HYPO,
	{29,	2,		-4	},	// WP_BORG_ASSIMILATOR
	{27,	8,		-10	},	// WP_BORG_WEAPON
};

#define PHASER_ALT_RADIUS			12
#define COMPRESSION_SPREAD	100
#define MAXRANGE_CRIFLE		8192
#define MAXRANGE_IMOD			4096
#define	MAX_RAIL_HITS	4
#define SCAV_ALT_SIZE		6
#define SCAV_ALT_VELOCITY	( wepPredictCfg.projectileSpeed4a > 0 ? wepPredictCfg.projectileSpeed4a : 1000 )
#define SCAV_ALT_UP_VELOCITY	100
#define SCAV_ALT_SIZE		6
#define SCAV_SPREAD			0.5
#define SCAV_VELOCITY		( wepPredictCfg.projectileSpeed4p > 0 ? wepPredictCfg.projectileSpeed4p : 1500 )
#define SCAV_SIZE			3
#define STASIS_VELOCITY		( wepPredictCfg.projectileSpeed5p > 0 ? wepPredictCfg.projectileSpeed5p : 1100 )
#define STASIS_SPREAD		0.085f
#define STASIS_MAIN_MISSILE_BIG		4
#define STASIS_MAIN_MISSILE_SMALL	2
#define MAXRANGE_ALT_STASIS		4096
#define GRENADE_VELOCITY		( wepPredictCfg.projectileSpeed6p > 0 ? wepPredictCfg.projectileSpeed6p : 1000 )
#define GRENADE_VELOCITY_ALT		( wepPredictCfg.projectileSpeed6a > 0 ? wepPredictCfg.projectileSpeed6a : 1000 )
#define GRENADE_SIZE			4
#define TETRION_ALT_VELOCITY	( wepPredictCfg.projectileSpeed7a > 0 ? wepPredictCfg.projectileSpeed7a : 1500 )
#define TETRION_ALT_SIZE		6
#define QUANTUM_VELOCITY	( wepPredictCfg.projectileSpeed8p > 0 ? wepPredictCfg.projectileSpeed8p : 1100 )
#define QUANTUM_SIZE		3
#define QUANTUM_ALT_VELOCITY	( wepPredictCfg.projectileSpeed8a > 0 ? wepPredictCfg.projectileSpeed8a : 550 )
#define DN_SEARCH_DIST	( wepPredictCfg.projectileSpeed9a > 0 ? wepPredictCfg.projectileSpeed9a : 256 )
#define DN_ALT_SIZE		12
#define BORG_PROJECTILE_SIZE	8
#define BORG_PROJ_VELOCITY		( wepPredictCfg.projectileSpeedbp > 0 ? wepPredictCfg.projectileSpeedbp : 1000 )
#define MAX_FORWARD_TRACE	8192

#define SEEKER_ACCEL_ACTIVE ( wepPredictCfg.seekerAccel && cg.predictedPlayerState.powerups[PW_SEEKER] )

static float CG_WeaponPredict_ShotSize( weapon_t weapon ) {
	switch ( weapon ) {
	case WP_SCAVENGER_RIFLE:
		return SCAV_SIZE;
	case WP_STASIS:
		return STASIS_MAIN_MISSILE_BIG*3;
	case WP_GRENADE_LAUNCHER:
		return GRENADE_SIZE;
	case WP_QUANTUM_BURST:
		return QUANTUM_SIZE;
	case WP_BORG_WEAPON:
		return BORG_PROJECTILE_SIZE;
	}

	return 0;
}

static float CG_WeaponPredict_ShotAltSize( weapon_t weapon ) {
	switch ( weapon ) {
		case WP_PHASER:
			return PHASER_ALT_RADIUS;
		case WP_SCAVENGER_RIFLE:
			return SCAV_ALT_SIZE;
		case WP_GRENADE_LAUNCHER:
			return GRENADE_SIZE;
		case WP_TETRION_DISRUPTOR:
			return TETRION_ALT_SIZE;
		case WP_QUANTUM_BURST:
			return QUANTUM_SIZE;
		case WP_DREADNOUGHT:
			return DN_ALT_SIZE;
	}

	return 0;
}

/* ----------------------------------------------------------------------- */

// Direction Calculation

/* ----------------------------------------------------------------------- */

typedef struct {
	vec3_t forward, right, up;
	vec3_t muzzle;
} weaponPredictDirs_t;

/*
================
CG_WeaponPredict_CorrectForwardVector
================
*/
void CG_WeaponPredict_CorrectForwardVector(const playerState_t *ps, vec3_t forward, vec3_t muzzlePoint, float projsize)
{
	trace_t		tr;
	vec3_t		end;
	vec3_t		eyepoint;
	vec3_t		mins, maxs;

	// Find the eyepoint.
	VectorCopy(ps->origin, eyepoint);
	eyepoint[2] += ps->viewheight;

	// First we must trace from the eyepoint to the muzzle point, to make sure that we have a legal muzzle point.
	if (projsize>0)
	{
		VectorSet(mins, -projsize, -projsize, -projsize);
		VectorSet(maxs, projsize, projsize, projsize);
		CG_Trace(&tr, eyepoint, mins, maxs, muzzlePoint, -1, MASK_SHOT);
	}
	else
	{
		CG_Trace(&tr, eyepoint, NULL, NULL, muzzlePoint, -1, MASK_SHOT);
	}

	if (tr.fraction < 1.0)
	{	// We hit something here...  Stomp the muzzlePoint back to the eye...
		VectorCopy(eyepoint, muzzlePoint);
		// Keep the forward vector where it is, 'cause straight forward from the eyeball is right where we want to be.
	}
	else
	{
		// figure out what our crosshairs are on...
		VectorMA(eyepoint, MAX_FORWARD_TRACE, forward, end);
		CG_Trace (&tr, eyepoint, NULL, NULL, end, -1, MASK_SHOT );

		// ...and have our new forward vector point at it
		VectorSubtract(tr.endpos, muzzlePoint, forward);
		VectorNormalize(forward);
	}
}

/*
================
CG_WeaponPredict_CalcMuzzlePoint
================
*/
void CG_WeaponPredict_CalcMuzzlePoint ( const playerState_t *ps, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint, float projsize )
{
	int weapontype;

	weapontype = ps->weapon;
	VectorCopy( ps->origin, muzzlePoint );
	SnapVector( muzzlePoint );

	if (weapontype > WP_NONE && weapontype < WP_NUM_WEAPONS)
	{	// Use the table to generate the muzzlepoint;
		{	// Crouching.  Use the add-to-Z method to adjust vertically.
			VectorMA(muzzlePoint, CG_WP_MuzzlePoint[weapontype][0], forward, muzzlePoint);
			VectorMA(muzzlePoint, CG_WP_MuzzlePoint[weapontype][1], right, muzzlePoint);
			muzzlePoint[2] += ps->viewheight + CG_WP_MuzzlePoint[weapontype][2];
			// VectorMA(muzzlePoint, ent->client->ps.viewheight + WP_MuzzlePoint[weapontype][2], up, muzzlePoint);
		}
	}

	CG_WeaponPredict_CorrectForwardVector(ps, forward, muzzlePoint, projsize);
	SnapVector( muzzlePoint );
}

/*
================
CG_WeaponPredict_GetDirs
================
*/
weaponPredictDirs_t CG_WeaponPredict_GetDirs( const playerState_t *ps, qboolean altFire ) {
	weaponPredictDirs_t dirs;
	int projsize;
	memset( &dirs, 0, sizeof( dirs ) );
	
	// set aiming directions
	AngleVectors (ps->viewangles, dirs.forward, dirs.right, dirs.up);

	if (altFire)
	{
		projsize = CG_WeaponPredict_ShotAltSize(ps->weapon);
	}
	else
	{
		projsize = CG_WeaponPredict_ShotSize(ps->weapon);
	}
	CG_WeaponPredict_CalcMuzzlePoint ( ps, dirs.forward, dirs.right, dirs.up, dirs.muzzle, projsize);
	return dirs;
}

/* ----------------------------------------------------------------------- */

// Misc

/* ----------------------------------------------------------------------- */

static predictableRNG_t predictRNG;

#define PREDICTABLE_RAND() BG_PredictableRNG_Rand( &predictRNG, cg.predictedPlayerState.commandTime )
#define PREDICTABLE_RANDOM() BG_PREDICTABLE_RANDOM( PREDICTABLE_RAND() )
#define PREDICTABLE_CRANDOM() BG_PREDICTABLE_CRANDOM( PREDICTABLE_RAND() )
#define PREDICTABLE_IRANDOM( min, max ) BG_PREDICTABLE_IRANDOM( PREDICTABLE_RAND(), min, max )

/*
================
CG_WeaponPredict_NormalizedDotProduct
================
*/
static float CG_WeaponPredict_NormalizedDotProduct( const vec3_t v1, const vec3_t v2 ) {
	vec3_t n1;
	vec3_t n2;
	VectorNormalize2( v1, n1 );
	VectorNormalize2( v2, n2 );
	return DotProduct( n1, n2 );
}

/*
================
CG_WeaponPredict_CanTakeDamage

Predict whether entity can take damage.
================
*/
static qboolean CG_WeaponPredict_CanTakeDamage( int entityNum ) {
	if ( entityNum < 0 || entityNum >= ENTITYNUM_MAX_NORMAL ||
			( cg_entities[entityNum].currentState.eFlags & EF_PINGCOMP_NO_DAMAGE ) ) {
		return qfalse;
	}

	return qtrue;
}

/*
================
CG_WeaponPredict_CanStickGrenade

Predict whether grenade can stick to this entity, based on checks in g_missile.c
================
*/
static qboolean CG_WeaponPredict_CanStickGrenade( int entityNum ) {
	if ( entityNum >= 0 && entityNum < ENTITYNUM_MAX_NORMAL ) {
		const centity_t *cent = &cg_entities[entityNum];
		if ( cent->currentState.eType == ET_USEABLE && cent->currentState.modelindex == HI_SHIELD ) {
			return qfalse;
		}
		if ( !VectorCompare( vec3_origin, cent->currentState.pos.trDelta ) && cent->currentState.pos.trType != TR_STATIONARY ) {
			return qfalse;
		}
		if ( !VectorCompare( vec3_origin, cent->currentState.apos.trDelta ) && cent->currentState.apos.trType != TR_STATIONARY ) {
			return qfalse;
		}
	}

	return qtrue;
}

/*
================
CG_WeaponPredict_EntitySearchRange

Increase allowed match range for predicted entities and events when moving fast or standing on a
fast mover. This range is just used as a sanity check so it's fine if the values are very rough.
================
*/
static float CG_WeaponPredict_EntitySearchRange( void ) {
	int groundEntityNum = cg.predictedPlayerState.groundEntityNum;
	float range = 250.0f;

	float speed = VectorLength( cg.predictedPlayerState.velocity ) / 8.0f;
	if ( speed > range ) {
		range = speed;
	}

	if ( groundEntityNum > 0 && groundEntityNum <= ENTITYNUM_MAX_NORMAL ) {
		centity_t *cent = &cg_entities[groundEntityNum];
		if ( cent->currentState.eType == ET_MOVER ) {
			float speed = VectorLength( cent->currentState.pos.trDelta );
			if ( speed > range ) {
				range = speed;
			}
		}
	}

	return range;
}

/*
================
CG_WeaponPredict_FindMatchingEntity

Searches current snapshot for the best matching entity according to eval function.
Returns NULL if no match found.

Eval function should return negative value for non-matching entities and positive value
for matching entities (the lower the value, the closer the match). 0 = exact match.
================
*/
static centity_t *CG_WeaponPredict_FindMatchingEntity(
		float ( *eval )( const centity_t *cent, void *context ), void *evalContext ) {
	int i;
	centity_t *cent;
	centity_t *best = NULL;
	float score;
	float bestScore = 0.0f;

	for ( i = 0; i < cg.snap->numEntities; i++ ) {
		cent = &cg_entities[cg.snap->entities[i].number];
		score = eval( cent, evalContext );
		if ( score >= 0.0f && ( !best || score < bestScore ) ) {
			best = cent;
			bestScore = score;
		}
	}

	return best;
}

/* ----------------------------------------------------------------------- */

// Adjusted Trace

/* ----------------------------------------------------------------------- */

typedef struct {
	vec3_t origin[MAX_GENTITIES];
} oldLerpPositions_t;

/*
================
CG_WeaponPredict_CalcCommandLerpOrigins

Adjust lerp origins to predicted command time instead of cg.time for accurate hit calculation
for instant-hit weapons.

Based on CG_CalcEntityLerpPositions
================
*/
static void CG_WeaponPredict_CalcCommandLerpOrigins( void ) {
	int num;
	centity_t *cent;

	for ( num = 0; num < cg.snap->numEntities; num++ ) {
		cent = &cg_entities[cg.snap->entities[num].number];

		if ( cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE ) {
			const float *current = cent->currentState.pos.trBase;
			const float *next = cent->nextState.pos.trBase;
			int delta = cg.nextSnap->serverTime - cg.snap->serverTime;
			float f = delta ? (float)( cg.predictedPlayerState.commandTime - cg.snap->serverTime ) / delta : 0.0f;

			cent->lerpOrigin[0] = current[0] + f * ( next[0] - current[0] );
			cent->lerpOrigin[1] = current[1] + f * ( next[1] - current[1] );
			cent->lerpOrigin[2] = current[2] + f * ( next[2] - current[2] );
		} else {
			BG_EvaluateTrajectory( &cent->currentState.pos, cg.predictedPlayerState.commandTime, cent->lerpOrigin );
		}
	}
}

/*
================
CG_WeaponPredict_CalcSnapshotLerpOrigins

Adjust lerp origins to current snapshot time instead of cg.time for accurate hit calculation
for projectiles.

Based on CG_CalcEntityLerpPositions
================
*/
static void CG_WeaponPredict_CalcSnapshotLerpOrigins( void ) {
	int num;
	centity_t *cent;

	for ( num = 0; num < cg.snap->numEntities; num++ ) {
		cent = &cg_entities[cg.snap->entities[num].number];

		if ( cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE ) {
			VectorCopy( cent->currentState.pos.trBase, cent->lerpOrigin );
		} else {
			BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin );
		}
	}
}

/*
================
CG_WeaponPredict_StoreLerpOrigins

Store current lerp origins ahead of temporary modifications.
================
*/
static void CG_WeaponPredict_StoreLerpOrigins( oldLerpPositions_t *oldPositions ) {
	int num;
	centity_t *cent;

	for ( num = 0; num < cg.snap->numEntities; num++ ) {
		cent = &cg_entities[cg.snap->entities[num].number];
		VectorCopy( cent->lerpOrigin, oldPositions->origin[num] );
	}
}

/*
================
CG_WeaponPredict_RestoreLerpOrigins

Restore lerp origins to previously saved values.
================
*/
static void CG_WeaponPredict_RestoreLerpOrigins( oldLerpPositions_t *oldPositions ) {
	int num;
	centity_t *cent;

	for ( num = 0; num < cg.snap->numEntities; num++ ) {
		cent = &cg_entities[cg.snap->entities[num].number];
		VectorCopy( oldPositions->origin[num], cent->lerpOrigin );
	}
}

/* ----------------------------------------------------------------------- */

// Event Handling

/* ----------------------------------------------------------------------- */

typedef struct {
	int time;	// 0 if inactive
	int event;
	vec3_t origin;
	vec3_t origin2;
} predictedEvent_t;

#define MAX_PREDICTED_EVENTS 16
static predictedEvent_t predictedEvents[MAX_PREDICTED_EVENTS];
static int predictedEventCycle;

// Use origin representing weapon start point for comparing entities
#define ORIGIN2_BASED_EVENT( event ) ( event == EV_IMOD || event == EV_IMOD_ALTFIRE || \
	event == EV_STASIS || event == EV_BORG_ALT_WEAPON )

/*
================
CG_WeaponPredict_EventToString

For debug prints.
================
*/
const char *CG_WeaponPredict_EventToString( int event ) {
	switch( event ) {
	case EV_GRENADE_BOUNCE:
		return "EV_GRENADE_BOUNCE";
	case EV_MISSILE_HIT:
		return "EV_MISSILE_HIT";
	case EV_COMPRESSION_RIFLE:
		return "EV_COMPRESSION_RIFLE";
	case EV_COMPRESSION_RIFLE_ALT:
		return "EV_COMPRESSION_RIFLE_ALT";
	case EV_IMOD:
		return "EV_IMOD";
	case EV_IMOD_HIT:
		return "EV_IMOD_HIT";
	case EV_IMOD_ALTFIRE:
		return "EV_IMOD_ALTFIRE";
	case EV_IMOD_ALTFIRE_HIT:
		return "EV_IMOD_ALTFIRE_HIT";
	case EV_STASIS:
		return "EV_STASIS";
	case EV_GRENADE_SHRAPNEL_EXPLODE:
		return "EV_GRENADE_SHRAPNEL_EXPLODE";
	case EV_DREADNOUGHT_MISS:
		return "EV_DREADNOUGHT_MISS";
	case EV_TETRION:
		return "EV_TETRION";
	case EV_BORG_ALT_WEAPON:
		return "EV_BORG_ALT_WEAPON";
	}

	return "<UNKNOWN>";
}

/*
================
CG_WeaponPredict_ComparePredictedEvent

Selects the closest match to a predicted event from the server snapshot.
================
*/
static float CG_WeaponPredict_ComparePredictedEvent( const centity_t *cent, void *context ) {
	predictedEvent_t *predicted = (predictedEvent_t *)context;
	const entityState_t *es = &cent->currentState;
	int event = es->eType > ET_EVENTS ? es->eType - ET_EVENTS : ( es->event & ~EV_EVENT_BITS );

	if ( event == EV_MISSILE_MISS ) {
		event = EV_MISSILE_HIT;
	}

	if ( cent->interpolate || cent->previousEvent ) {
		// Only search for new events
		return -1.0f;
	}

	if ( event == predicted->event ) {
		float origin_delta = ORIGIN2_BASED_EVENT( event ) ? Distance( es->origin2, predicted->origin2 ) :
			Distance( es->pos.trBase, predicted->origin );

		if ( origin_delta < CG_WeaponPredict_EntitySearchRange() ) {
			return origin_delta;
		}
	}

	return -1.0f;
}

/*
================
CG_WeaponPredict_CheckPredictedEvents

Check for new events in the server snapshot, and disable any that have already been predicted
to avoid playing the same event twice.
================
*/
static void CG_WeaponPredict_CheckPredictedEvents( void ) {
	int i;

	for ( i = 0; i < MAX_PREDICTED_EVENTS; ++i ) {
		predictedEvent_t *pe = &predictedEvents[i];
		if ( pe->time && cg.snap->ps.commandTime >= pe->time ) {
			if ( cg.snap->ps.commandTime >= pe->time + 200 ) {
				pe->time = 0;
				if ( WP_DEBUG ) {
					CG_Printf( "dropped predicted event: name(%s) num(%i)\n", CG_WeaponPredict_EventToString( pe->event ), i );
				}
			}

			else {
				centity_t *match = CG_WeaponPredict_FindMatchingEntity( CG_WeaponPredict_ComparePredictedEvent, (void *)pe );
				if ( match ) {
					const entityState_t *es = &match->currentState;
					const float *matchOrigin = ORIGIN2_BASED_EVENT( pe->event ) ? es->origin2 : es->pos.trBase;
					pe->time = 0;
					// disable the event from firing
					match->previousEvent = match->currentState.eType > ET_EVENTS ? 1 : match->currentState.event;

					if ( WP_DEBUG ) {
						CG_Printf( "matched predicted event: name(%s) num(%i)\n   matchEnt(%i) cgTime(%i) originCompare(%f %f)\n",
								CG_WeaponPredict_EventToString( pe->event ), i, match - cg_entities,
								cg.time, Distance( es->pos.trBase, pe->origin ), Distance( es->origin2, pe->origin2 )  );
					}
				}
			}
		}
	}
}

/*
================
CG_WeaponPredict_LogPredictedEvent
================
*/
static void CG_WeaponPredict_LogPredictedEvent( int time, int event, const vec3_t origin, const vec3_t origin2 ) {
	int index = predictedEventCycle;
	predictedEvent_t *pe = &predictedEvents[index];

	memset( pe, 0, sizeof( *pe ) );
	pe->time = time;
	pe->event = event;
	VectorCopy( origin, pe->origin );
	VectorCopy( origin2, pe->origin2 );

	if ( WP_DEBUG ) {
		CG_Printf( "running predicted event: name(%s) num(%i)\n   commandTime(%i) origin(%f %f %f)\n",
				CG_WeaponPredict_EventToString( event ), index, time, EXPAND_VECTOR( origin ) );
	}

	predictedEventCycle = ( predictedEventCycle + 1 ) % MAX_PREDICTED_EVENTS;
}

/*
================
CG_WeaponPredict_RunPredictedEvent
================
*/
static void CG_WeaponPredict_RunPredictedEvent( centity_t *cent, int time ) {
	entityState_t *es = &cent->currentState;
	int event = es->eType > ET_EVENTS ? es->eType - ET_EVENTS : ( es->event & ~EV_EVENT_BITS );
	CG_WeaponPredict_LogPredictedEvent( time, event, cent->lerpOrigin, es->origin2 );
	CG_EntityEvent( cent, cent->lerpOrigin );
}

/* ----------------------------------------------------------------------- */

// Projectile Handling

/* ----------------------------------------------------------------------- */

typedef struct {
	centity_t cent;
	qboolean active;
	qboolean frozen;
	int startTime;
	vec3_t currentOrigin;
	int lastThink;
	vec3_t mins;
	vec3_t maxs;

	// for welder only
	vec3_t movedir;
	int nextThink;
	qboolean hasMoved;

	// for debug print purposes
	int skippedFrames;
} predictedProjectile_t;

#define MAX_PREDICTED_PROJECTILES 16
static predictedProjectile_t predictedProjectiles[MAX_PREDICTED_PROJECTILES];
static int predictedProjectileCycle;

/*
================
CG_WeaponPredict_LogPredictedProjectile
================
*/
static void CG_WeaponPredict_LogPredictedProjectile( const predictedProjectile_t *proj ) {
	if ( WP_DEBUG ) {
		CG_Printf( "launched predicted projectile: wep(%i%s) num(%i) commandTime(%i)\n",
				proj->cent.currentState.weapon - WP_PHASER + 1, proj->cent.currentState.eType == ET_ALT_MISSILE ? "a" : "p",
				proj - predictedProjectiles, proj->startTime );
	}
}

/*
================
CG_WeaponPredict_SpawnPredictedProjectile

Spawns a predicted projectile and initializes some common fields.
================
*/
static predictedProjectile_t *CG_WeaponPredict_SpawnPredictedProjectile( int eType, int weapon, trType_t trType,
		float speed, float size, const weaponPredictDirs_t *dirs ) {
	int index = predictedProjectileCycle;
	predictedProjectile_t *proj = &predictedProjectiles[index];
	predictedProjectileCycle = ( predictedProjectileCycle + 1 ) % MAX_PREDICTED_PROJECTILES;

	memset( proj, 0, sizeof( *proj ) );
	proj->active = qtrue;
	proj->cent.currentState.eType = eType;
	proj->cent.currentState.weapon = weapon;
	proj->cent.currentState.pos.trType = trType;
	proj->startTime = cg.predictedPlayerState.commandTime;
	proj->cent.currentState.pos.trTime = cg.predictedPlayerState.commandTime;
	proj->lastThink = cg.predictedPlayerState.commandTime;
	VectorCopy( dirs->muzzle, proj->currentOrigin );
	VectorCopy( dirs->muzzle, proj->cent.currentState.pos.trBase );
	VectorScale( dirs->forward, speed, proj->cent.currentState.pos.trDelta );
	VectorSet( proj->mins, -size, -size, -size );
	VectorSet( proj->maxs, size, size, size );
	return proj;
}

/*
================
CG_WeaponPredict_DreadnoughtBurstThink
================
*/
static void CG_WeaponPredict_DreadnoughtBurstThink( predictedProjectile_t *proj, int time ) {
	if ( time >= proj->nextThink ) {
		vec3_t startpos, endpos;
		trace_t tr;
		vec3_t dest, mins={0,0,-1}, maxs={0,0,1};
		static qboolean recursion=qfalse;

		proj->nextThink += 100;

		if ( WP_DEBUG ) {
			CG_Printf( "welder think: num(%i) time(%i) currentOrigin(%f %f %f) sOrigin(%f %f %f) sOrigin2(%f %f %f) trBase(%f %f %f)\n",
					proj - predictedProjectiles, time, EXPAND_VECTOR( proj->currentOrigin ), EXPAND_VECTOR( proj->cent.currentState.origin ),
					EXPAND_VECTOR( proj->cent.currentState.origin2 ), EXPAND_VECTOR( proj->cent.currentState.pos.trBase ) );
		}

		mins[0] = mins[1] = -DN_ALT_SIZE;
		maxs[0] = maxs[1] = DN_ALT_SIZE;

		VectorCopy(proj->cent.currentState.origin, startpos);
		VectorMA(startpos, DN_SEARCH_DIST, proj->movedir, endpos);
		CG_Trace (&tr, startpos, mins, maxs, endpos, -1, MASK_SHOT );

		if ( CG_WeaponPredict_CanTakeDamage( tr.entityNum ) ) {
			VectorCopy(proj->cent.currentState.origin, proj->cent.currentState.origin2);
			SnapVector(proj->cent.currentState.origin);
			SnapVector(proj->cent.currentState.origin2);
			VectorCopy(cg_entities[tr.entityNum].lerpOrigin, proj->cent.currentState.origin);
			return;
		}

		else if ( tr.fraction < 0.02 ) {
			float dot = DotProduct(proj->movedir, tr.plane.normal);
			if (dot < -0.6 || dot == 0.0) {
				centity_t tent;
				memset( &tent, 0, sizeof( tent ) );
				tent.currentState.event = EV_DREADNOUGHT_MISS;
				VectorCopy( tr.endpos, tent.lerpOrigin );
				SnapVector( tent.lerpOrigin );
				VectorCopy( startpos, tent.currentState.origin2 );
				SnapVector( tent.currentState.origin2 );
				tent.currentState.eventParm = DirToByte( tr.plane.normal );
				CG_WeaponPredict_RunPredictedEvent( &tent, proj->startTime );
				proj->active = qfalse;
				return;
			} else {
				if ( WP_DEBUG ) {
					CG_Printf( "welder wall bounce: num(%i) time(%i)\n", proj - predictedProjectiles, time );
				}

				// Bounce off the surface just a little
				VectorMA(proj->movedir, -1.25*dot, tr.plane.normal, proj->movedir);
				VectorNormalize(proj->movedir);

				// Repeat the trace (substitute for recursive call in server implementation)
				VectorCopy(tr.endpos, startpos);
				VectorMA(startpos, DN_SEARCH_DIST, proj->movedir, endpos);
				CG_Trace (&tr, startpos, mins, maxs, endpos, -1, MASK_SHOT );
			}
		}

		VectorCopy(tr.endpos, dest);

		VectorCopy(proj->cent.currentState.origin, proj->cent.currentState.origin2);
		VectorCopy(dest, proj->cent.currentState.origin);
		VectorCopy(dest, proj->cent.currentState.pos.trBase);
		SnapVector(proj->cent.currentState.origin2);

	}
}

/*
================
CG_WeaponPredict_BounceMissile
================
*/
static qboolean CG_WeaponPredict_BounceMissile( predictedProjectile_t *proj, const trace_t *trace, int time, int previousTime ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = previousTime + ( time - previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &proj->cent.currentState.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, proj->cent.currentState.pos.trDelta );

	if ( proj->cent.currentState.eFlags & EF_BOUNCE_HALF ) {
		VectorScale( proj->cent.currentState.pos.trDelta, 0.65,proj->cent.currentState.pos.trDelta );
		proj->cent.currentState.pos.trDelta[2]*=0.5f;
		// check for stop
		if ( trace->plane.normal[2] > 0.2 && VectorLength( proj->cent.currentState.pos.trDelta ) < 40 ) {
			VectorCopy( trace->endpos, proj->cent.currentState.pos.trBase );
			proj->cent.currentState.pos.trType = TR_STATIONARY;
			return qfalse;
		}
	}

	VectorCopy( trace->endpos, proj->cent.currentState.pos.trBase );

	// this is weird, but keeping for consistency with server
	VectorAdd( proj->cent.currentState.pos.trBase, trace->plane.normal, proj->cent.currentState.pos.trBase);
	SnapVector( proj->cent.currentState.pos.trBase );
	proj->cent.currentState.pos.trTime = time;

	if ( WP_DEBUG ) {
		CG_Printf( "projectile bounce: num(%i) time(%i : %i) origin(%f %f %f)\n",
				proj - predictedProjectiles, previousTime, time, EXPAND_VECTOR( proj->cent.currentState.pos.trBase ) );
	}

	return qtrue;
}

/*
================
CG_WeaponPredict_MissileThink
================
*/
static void CG_WeaponPredict_MissileThink( predictedProjectile_t *proj, int time ) {
	if ( proj->active && !proj->frozen && time > proj->lastThink ) {
		vec3_t newOrigin;
		trace_t tr;
		int lastThink = proj->lastThink;

		proj->lastThink = time;
		proj->hasMoved = qtrue;

		BG_EvaluateTrajectory( &proj->cent.currentState.pos, time, newOrigin );

		if ( WP_DEBUG_VERBOSE ) {
			CG_Printf( "predicted projectile move: num(%i) time(%i => %i) start(%f %f %f) end(%f %f %f)\n",
					proj - predictedProjectiles, lastThink, time, EXPAND_VECTOR( proj->currentOrigin ), EXPAND_VECTOR( newOrigin ) );
		}

		CG_Trace( &tr, proj->currentOrigin, proj->mins, proj->maxs, newOrigin, -1, MASK_SHOT );

		VectorCopy( tr.endpos, proj->currentOrigin );

		if ( tr.startsolid ) {
			tr.fraction = 0;
		}

		if ( tr.fraction != 1 && !tr.allsolid ) {
			// never explode or bounce on sky
			if ( tr.surfaceFlags & SURF_NOIMPACT ) {
				if ( WP_DEBUG ) {
					CG_Printf( "predicted projectile hit noimpact surface: num(%i) time(%i) pos(%f %f %f)\n",
							proj - predictedProjectiles, time, EXPAND_VECTOR( tr.endpos ) );
				}
				proj->active = qfalse;
				return;
			}

			if ( !CG_WeaponPredict_CanTakeDamage( tr.entityNum ) ) {
				// solid map impact
				if ( ( proj->cent.currentState.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF ) ) &&
						CG_WeaponPredict_BounceMissile( proj, &tr, time, lastThink ) ) {
					// generate bounce event
					centity_t tent;
					memset( &tent, 0, sizeof( tent ) );
					VectorCopy( tr.endpos, tent.lerpOrigin );
					tent.currentState.event = EV_GRENADE_BOUNCE;
					tent.currentState.weapon = proj->cent.currentState.weapon;
					VectorCopy(tr.plane.normal, tent.currentState.angles2);
					VectorShort(tent.currentState.angles2);
					CG_WeaponPredict_RunPredictedEvent( &tent, proj->startTime );
					return;
				}

				if ( ( proj->cent.currentState.eFlags & EF_MISSILE_STICK ) && CG_WeaponPredict_CanStickGrenade( tr.entityNum ) ) {
					// predict position adjustment due to grenade sticking
					vec3_t dir;
					VectorMA( tr.endpos, -2.5, tr.plane.normal, proj->cent.currentState.pos.trBase );
					proj->cent.currentState.pos.trType = TR_STATIONARY;
					VectorScale ( tr.plane.normal, -1, dir );
					vectoangles( dir, proj->cent.currentState.angles );
					SnapVector( proj->cent.currentState.angles );
					proj->frozen = qtrue;
					if ( WP_DEBUG ) {
						CG_Printf( "predicted projectile stick: num(%i) time(%i) pos(%f %f %f)\n",
								proj - predictedProjectiles, time, EXPAND_VECTOR( proj->cent.currentState.pos.trBase ) );
					}
					return;
				}
			}

			if ( WP_DEBUG ) {
				CG_Printf( "predicted projectile impact: num(%i) time(%i) pos(%f %f %f) hitEntity(%i) hitEntityPos(%f %f %f)\n",
						proj - predictedProjectiles, time, EXPAND_VECTOR( tr.endpos ), tr.entityNum,
						EXPAND_VECTOR( cg_entities[tr.entityNum].currentState.pos.trBase ) );
			}

			// predict missile explosion
			if ( proj->cent.currentState.weapon == WP_GRENADE_LAUNCHER && proj->cent.currentState.eType == ET_ALT_MISSILE ) {
				centity_t tent;
				memset( &tent, 0, sizeof( tent ) );
				VectorCopy( tr.endpos, tent.lerpOrigin );
				SnapVector( tent.lerpOrigin );
				tent.currentState.event = EV_GRENADE_SHRAPNEL_EXPLODE;
				CG_WeaponPredict_RunPredictedEvent( &tent, proj->startTime );
			} else {
				centity_t tent = proj->cent;
				VectorCopy( tr.endpos, tent.lerpOrigin );
				//SnapVectorTowards( tent.lerpOrigin, tent.currentState.pos.trBase );
				tent.currentState.event = EV_MISSILE_HIT;
				tent.currentState.eventParm = DirToByte( tr.plane.normal );
				CG_WeaponPredict_RunPredictedEvent( &tent, proj->startTime );
			}
			proj->active = qfalse;
			return;
		}

		if ( proj->cent.currentState.weapon == WP_DREADNOUGHT ) {
			CG_WeaponPredict_DreadnoughtBurstThink( proj, time );
		}
	}
}

/*
================
CG_WeaponPredict_RunMissiles

Advance predicted missiles through the world. The time values and player/entity coordinates for
collision detection should match those used by the server as closely as possible.
================
*/
static void CG_WeaponPredict_RunMissiles( void ) {
	if ( CG_WeaponPredict_IsActive() ) {
		int i;
		oldLerpPositions_t oldPositions;

		CG_WeaponPredict_StoreLerpOrigins( &oldPositions );
		CG_WeaponPredict_CalcSnapshotLerpOrigins();

		for ( i = 0; i < MAX_PREDICTED_PROJECTILES; ++i ) {
			if ( predictedProjectiles[i].active ) {
				predictedProjectile_t *proj = &predictedProjectiles[i];
				CG_WeaponPredict_MissileThink( proj, cg.snap->serverTime );
			}
		}

		CG_WeaponPredict_RestoreLerpOrigins( &oldPositions );
	}
}

/*
================
CG_WeaponPredict_DrawPredictedProjectiles
================
*/
void CG_WeaponPredict_DrawPredictedProjectiles( void ) {
	if ( CG_WeaponPredict_IsActive() ) {
		int i;
		for ( i = 0; i < MAX_PREDICTED_PROJECTILES; ++i ) {
			if ( predictedProjectiles[i].active && cg.snap->ps.commandTime < predictedProjectiles[i].startTime ) {
				predictedProjectile_t *proj = &predictedProjectiles[i];

				if ( cg.snap->ps.commandTime >= proj->startTime ) {
					// the actual server entity should be available
					continue;
				}

				if ( proj->cent.currentState.weapon == WP_DREADNOUGHT && !proj->hasMoved ) {
					// wait until the first server frame to avoid fx timing quirks
					continue;
				}

				BG_EvaluateTrajectory( &proj->cent.currentState.pos, cg.time, proj->cent.lerpOrigin );
				BG_EvaluateTrajectory( &proj->cent.currentState.apos, cg.time, proj->cent.lerpAngles );
				CG_Missile( &proj->cent, proj->cent.currentState.eType == ET_ALT_MISSILE ? qtrue : qfalse );
			}
		}
	}
}

/*
================
CG_WeaponPredict_ComparePredictedMissile

Select the closest match to a predicted missile from the server snapshot.
================
*/
static float CG_WeaponPredict_ComparePredictedMissile( const centity_t *cent, void *context ) {
	predictedProjectile_t *predictedMissile = (predictedProjectile_t *)context;
	const entityState_t *es = &cent->currentState;

	if ( cent->interpolate ) {
		// Only search for new events
		return -1.0f;
	}

	if ( predictedMissile->cent.currentState.eType == es->eType &&
			predictedMissile->cent.currentState.weapon == es->weapon &&
			( es->weapon != WP_STASIS || es->time2 == predictedMissile->cent.currentState.time2 ) ) {
		float deltaOrigin = Distance( es->pos.trBase, predictedMissile->cent.currentState.pos.trBase );
		float dot = CG_WeaponPredict_NormalizedDotProduct( es->pos.trDelta, predictedMissile->cent.currentState.pos.trDelta );

		// only factor in reasonable dot product values, since invalid values can occur with certain weapons,
		// (e.g. sticky grenades) even though the projectile is still valid
		if ( dot < 0.5f ) {
			dot = 0.5f;
		}

		if ( deltaOrigin < CG_WeaponPredict_EntitySearchRange() ) {
			// take dot product into account even if deltaOrigin is 0
			return deltaOrigin / dot + ( 10.0f - dot );
		}
	}

	return -1.0f;
}

/*
================
CG_WeaponPredict_CheckPredictedMissiles

Check new snapshot for entities matching predicted projectiles. Only the scavenger alt projectile
and stasis primary main projectile are actually modified, the others are only checked for debug
print purposes.
================
*/
static void CG_WeaponPredict_CheckPredictedMissiles( void ) {
	int i;

	for ( i = 0; i < MAX_PREDICTED_PROJECTILES; ++i ) {
		predictedProjectile_t *proj = &predictedProjectiles[i];
		if ( proj->active ) {
			qboolean isStasis = proj->cent.currentState.eType == ET_MISSILE &&
					proj->cent.currentState.weapon == WP_STASIS && proj->cent.currentState.time2 >= 3 ? qtrue : qfalse;
			qboolean isScav = proj->cent.currentState.eType == ET_ALT_MISSILE &&
					proj->cent.currentState.weapon == WP_SCAVENGER_RIFLE ? qtrue : qfalse;

			if ( cg.snap->ps.commandTime >= proj->startTime + 200 ) {
				// no server projectile match, even though we are well past the command time, so assume projectile is lost
				// this happens normally if projectile exploded during ping compensation window before the entity was sent
				proj->active = qfalse;
				if ( WP_DEBUG ) {
					CG_Printf( "dropped predicted projectile: wep(%i%s) num(%i)\n",
							proj->cent.currentState.weapon - WP_PHASER + 1, proj->cent.currentState.eType == ET_ALT_MISSILE ? "a" : "p", i );
				}
			}

			else if ( cg.snap->ps.commandTime >= proj->startTime && ( isStasis || isScav || WP_DEBUG ) ) {
				// server has acknowledged command time when projectile was predicted, so look for it in the server snapshot
				centity_t *match = CG_WeaponPredict_FindMatchingEntity( CG_WeaponPredict_ComparePredictedMissile, (void *)proj );
				if ( match ) {
					proj->active = qfalse;
					if ( WP_DEBUG ) {
						CG_Printf( "matched predicted projectile: wep(%i%s) num(%i) matchEnt(%i)"
								"\n   trajectoryCompare(time:%i pos:%f dir:%f)\n",
								proj->cent.currentState.weapon - WP_PHASER + 1, proj->cent.currentState.eType == ET_ALT_MISSILE ? "a" : "p",
								i, match - cg_entities, match->currentState.pos.trTime - proj->cent.currentState.pos.trTime,
								Distance( match->currentState.pos.trBase, proj->cent.currentState.pos.trBase ),
								CG_WeaponPredict_NormalizedDotProduct( match->currentState.pos.trDelta, proj->cent.currentState.pos.trDelta ) );
						if ( proj->skippedFrames ) {
							CG_Printf( "   WARNING: Projectile skipped %i frames\n", proj->skippedFrames );
						}
					}

					if ( isStasis ) {
						VectorCopy( proj->cent.rawOrigin, match->rawOrigin );
						VectorCopy( proj->cent.rawAngles, match->rawAngles );
						match->thinkFlag = 1;
					}
					if ( isScav ) {
						VectorCopy( proj->cent.beamEnd, match->beamEnd );
						match->thinkFlag = 1;
					}
				} else {
					++proj->skippedFrames;
				}
			}
		}
	}
}

/* ----------------------------------------------------------------------- */

// Main Event Handling

/* ----------------------------------------------------------------------- */

/*
================
CG_WeaponPredict_SpawnStasisProjectile
================
*/
static void CG_WeaponPredict_SpawnStasisProjectile( const weaponPredictDirs_t *dirs, int size, float spread ) {
	predictedProjectile_t *proj = CG_WeaponPredict_SpawnPredictedProjectile(
					ET_MISSILE, WP_STASIS, TR_LINEAR, STASIS_VELOCITY + ( 50 * size ), 3 * size, dirs );

	if ( spread ) {
		VectorMA( dirs->forward, spread, dirs->right, proj->cent.currentState.pos.trDelta );
		VectorNormalize( proj->cent.currentState.pos.trDelta );
		VectorScale( proj->cent.currentState.pos.trDelta, STASIS_VELOCITY + ( 50 * size ), proj->cent.currentState.pos.trDelta );
	}

	proj->cent.currentState.time2 = size;
	VectorCopy(proj->cent.currentState.pos.trBase, proj->cent.currentState.angles2);

	CG_WeaponPredict_LogPredictedProjectile( proj );
}

/*
================
CG_WeaponPredict_FirePredictedRifle
================
*/
static void CG_WeaponPredict_FirePredictedRifle( const weaponPredictDirs_t *dirs, qboolean alt_fire, qboolean imod_fx ) {
	trace_t tr;
	vec3_t end;
	centity_t tent;

	VectorMA (dirs->muzzle, MAXRANGE_CRIFLE, dirs->forward, end);

	if ( !alt_fire || cg.predictedPlayerState.velocity[0] || cg.predictedPlayerState.velocity[1] || cg.predictedPlayerState.velocity[2] ) {
		float r = PREDICTABLE_CRANDOM()*COMPRESSION_SPREAD;
		float u = PREDICTABLE_CRANDOM()*COMPRESSION_SPREAD;
		VectorMA (end, r, dirs->right, end);
		VectorMA (end, u, dirs->up, end);
	}

	CG_Trace (&tr, dirs->muzzle, NULL, NULL, end, -1, MASK_SHOT );

	if ( WP_DEBUG ) {
		CG_Printf( "rifle trace: time(%i) hitPos(%f %f %f)\n   hitEntity(%i) hitEntityPos(%f %f %f)\n",
				cg.predictedPlayerState.commandTime, EXPAND_VECTOR( tr.endpos ), tr.entityNum,
				EXPAND_VECTOR( cg_entities[tr.entityNum].currentState.pos.trBase ) );
	}

	memset( &tent, 0, sizeof( tent ) );
	if ( imod_fx ) {
		tent.currentState.event = EV_IMOD;
		VectorCopy( tr.endpos, tent.lerpOrigin );
		VectorCopy( dirs->muzzle, tent.currentState.origin2 );
	} else {
		if ( alt_fire ) {
			tent.currentState.event = EV_COMPRESSION_RIFLE_ALT;
			tent.currentState.eFlags |= EF_ALT_FIRING;
		} else {
			tent.currentState.event = EV_COMPRESSION_RIFLE;
		}
		VectorCopy( dirs->muzzle, tent.lerpOrigin );
		VectorSubtract(end, dirs->muzzle, tent.currentState.origin2);
		VectorShort(tent.currentState.origin2);
	}

	CG_WeaponPredict_RunPredictedEvent( &tent, cg.predictedPlayerState.commandTime );
}

#define RENDER_IMPACT( tr ) ( !( ( tr ).surfaceFlags & SURF_NOIMPACT ) )
#define SKIP_ENTITY_ID -1

/*
================
CG_WeaponPredict_FirePredictedImod
================
*/
static void CG_WeaponPredict_FirePredictedImod( const weaponPredictDirs_t *dirs, qboolean alt_fire ) {
	int i;
	trace_t tr;
	vec3_t end;
	centity_t tent;
	int unlinked = 0;
	int unlinkedEntities[MAX_RAIL_HITS];

	VectorMA (dirs->muzzle, MAXRANGE_IMOD, dirs->forward, end);

	do {
		CG_Trace( &tr, dirs->muzzle, NULL, NULL, end, SKIP_ENTITY_ID, MASK_SHOT );

		if ( WP_DEBUG ) {
			CG_Printf( "imod trace: time(%i) pass(%i) takeDmg(%s) render(%s)\n"
					"   solid(%s) hitPos(%f %f %f)\n   hitEntity(%i) hitEntityPos(%f %f %f)\n",
					cg.predictedPlayerState.commandTime, unlinked, CG_WeaponPredict_CanTakeDamage( tr.entityNum ) ? "yes" : "no",
					RENDER_IMPACT( tr ) ? "yes" : "no", ( tr.contents & CONTENTS_SOLID ) ? "yes" : "no",
					EXPAND_VECTOR( tr.endpos ), tr.entityNum, EXPAND_VECTOR( cg_entities[tr.entityNum].currentState.pos.trBase ) );
		}

		if ( CG_WeaponPredict_CanTakeDamage( tr.entityNum ) && RENDER_IMPACT( tr ) ) {
			memset( &tent, 0, sizeof( tent ) );
			tent.currentState.event = alt_fire ? EV_IMOD_ALTFIRE_HIT : EV_IMOD_HIT;
			VectorCopy( tr.endpos, tent.lerpOrigin );
			SnapVector( tent.lerpOrigin );
			CG_WeaponPredict_RunPredictedEvent( &tent, cg.predictedPlayerState.commandTime );
		}

		if ( tr.contents & CONTENTS_SOLID || tr.entityNum == ENTITYNUM_NONE || tr.entityNum == ENTITYNUM_WORLD ) {
			break;
		}

		// Make sure the entity numbers are valid
		if ( !EF_WARN_ASSERT( tr.entityNum >= 0 && tr.entityNum < MAX_GENTITIES ) ||
				!EF_WARN_ASSERT( tr.entityNum == cg_entities[tr.entityNum].currentState.number ) ) {
			break;
		}

		// Simulate unlinking the entity
		// CG_ClipMoveToEntities should skip the entity as long as the trace is called with the same skipNumber
		cg_entities[tr.entityNum].currentState.number = SKIP_ENTITY_ID;
		unlinkedEntities[unlinked] = tr.entityNum;
		unlinked++;
	} while ( unlinked < MAX_RAIL_HITS );

	// 'link' back in any entities we unlinked
	for ( i = 0; i < unlinked; i++ ) {
		cg_entities[unlinkedEntities[i]].currentState.number = unlinkedEntities[i];
	}

	memset( &tent, 0, sizeof( tent ) );
	tent.currentState.event = alt_fire ? EV_IMOD_ALTFIRE : EV_IMOD;
	VectorCopy( tr.endpos, tent.lerpOrigin );
	SnapVector( tent.lerpOrigin );

	VectorMA( dirs->muzzle, alt_fire ? 10 : 2, dirs->forward, tent.currentState.origin2 );
	SnapVector( tent.currentState.origin2 );

	CG_WeaponPredict_RunPredictedEvent( &tent, cg.predictedPlayerState.commandTime );
}

/*
================
CG_WeaponPredict_PrimaryAttack

Generate effects for a predicted primary attack event.
================
*/
static void CG_WeaponPredict_PrimaryAttack( int event ) {
	oldLerpPositions_t oldPositions;
	weaponPredictDirs_t dirs;

	CG_WeaponPredict_StoreLerpOrigins( &oldPositions );
	CG_WeaponPredict_CalcCommandLerpOrigins();
	dirs = CG_WeaponPredict_GetDirs( &cg.predictedPlayerState, qfalse );

	if ( cg.predictedPlayerState.weapon == WP_COMPRESSION_RIFLE ) {
		CG_WeaponPredict_FirePredictedRifle( &dirs, qfalse, qfalse );
	}

	if ( cg.predictedPlayerState.weapon == WP_IMOD ) {
		if ( wepPredictCfg.imodRifle ) {
			CG_WeaponPredict_FirePredictedRifle( &dirs, qtrue, qtrue );
		} else {
			CG_WeaponPredict_FirePredictedImod( &dirs, qfalse );
		}
	}

	if ( cg.predictedPlayerState.weapon == WP_SCAVENGER_RIFLE && wepPredictCfg.projectiles ) {
		vec3_t angles;
		vec3_t temp_ang;
		vec3_t dir;
		float offset;
		vec3_t temp_org;

		predictedProjectile_t *proj = CG_WeaponPredict_SpawnPredictedProjectile(
				ET_MISSILE, WP_SCAVENGER_RIFLE, TR_LINEAR, SCAV_VELOCITY, SCAV_SIZE, &dirs );

		vectoangles( dirs.forward, angles );
		VectorSet( temp_ang, angles[0] + (PREDICTABLE_CRANDOM() * SCAV_SPREAD),
				angles[1] + (PREDICTABLE_CRANDOM() * SCAV_SPREAD), angles[2] );
		AngleVectors( temp_ang, dir, NULL, NULL );

		offset = PREDICTABLE_IRANDOM(0, 1) * 2 + 1;
		VectorMA( dirs.muzzle, offset, dirs.right, temp_org );
		VectorMA( temp_org, offset, dirs.up, proj->cent.currentState.pos.trBase );
		SnapVector( proj->cent.currentState.pos.trBase );

		VectorScale( dir, SCAV_VELOCITY, proj->cent.currentState.pos.trDelta );
		SnapVector( proj->cent.currentState.pos.trDelta );

		proj->cent.currentState.pos.trTime -= 10;

		CG_WeaponPredict_LogPredictedProjectile( proj );
	}

	if ( cg.predictedPlayerState.weapon == WP_STASIS && wepPredictCfg.projectiles ) {
		CG_WeaponPredict_SpawnStasisProjectile( &dirs, STASIS_MAIN_MISSILE_BIG, 0.0f );
		CG_WeaponPredict_SpawnStasisProjectile( &dirs, STASIS_MAIN_MISSILE_SMALL, -STASIS_SPREAD );
		CG_WeaponPredict_SpawnStasisProjectile( &dirs, STASIS_MAIN_MISSILE_SMALL, STASIS_SPREAD );
	}

	if ( cg.predictedPlayerState.weapon == WP_GRENADE_LAUNCHER && wepPredictCfg.projectiles ) {
		predictedProjectile_t *proj = CG_WeaponPredict_SpawnPredictedProjectile(
				ET_MISSILE, WP_GRENADE_LAUNCHER, TR_GRAVITY, GRENADE_VELOCITY, GRENADE_SIZE, &dirs );
		SnapVector( proj->cent.currentState.pos.trDelta );
		proj->cent.currentState.eFlags |= EF_BOUNCE_HALF;

		if ( SEEKER_ACCEL_ACTIVE ) {
			proj->cent.currentState.eType = ET_ALT_MISSILE;
			proj->cent.currentState.weapon = WP_QUANTUM_BURST;
			VectorScale( proj->cent.currentState.pos.trDelta, 2.0f, proj->cent.currentState.pos.trDelta );
		}

		CG_WeaponPredict_LogPredictedProjectile( proj );
	}

	if ( cg.predictedPlayerState.weapon == WP_TETRION_DISRUPTOR ) {
		centity_t tent;

		memset( &tent, 0, sizeof( tent ) );
		tent.currentState.event = EV_TETRION;
		VectorCopy( dirs.muzzle, tent.lerpOrigin );

		VectorCopy( dirs.forward, tent.currentState.angles2 );
		VectorShort(tent.currentState.angles2);

		CG_WeaponPredict_RunPredictedEvent( &tent, cg.predictedPlayerState.commandTime );
	}

	if ( cg.predictedPlayerState.weapon == WP_QUANTUM_BURST && wepPredictCfg.projectiles ) {
		predictedProjectile_t *proj = CG_WeaponPredict_SpawnPredictedProjectile(
				ET_MISSILE, WP_QUANTUM_BURST, TR_LINEAR, QUANTUM_VELOCITY, QUANTUM_SIZE, &dirs );
		SnapVector( proj->cent.currentState.pos.trDelta );

		if ( SEEKER_ACCEL_ACTIVE ) {
			proj->cent.currentState.eType = ET_ALT_MISSILE;
			VectorScale( proj->cent.currentState.pos.trDelta, 2.5f, proj->cent.currentState.pos.trDelta );
		}

		CG_WeaponPredict_LogPredictedProjectile( proj );
	}

	if ( cg.predictedPlayerState.weapon == WP_BORG_WEAPON && wepPredictCfg.projectiles ) {
		predictedProjectile_t *proj = CG_WeaponPredict_SpawnPredictedProjectile(
				ET_MISSILE, WP_BORG_WEAPON, TR_LINEAR, BORG_PROJ_VELOCITY, BORG_PROJECTILE_SIZE, &dirs );

		CG_WeaponPredict_LogPredictedProjectile( proj );
	}

	CG_WeaponPredict_RestoreLerpOrigins( &oldPositions );
}

/*
================
CG_WeaponPredict_AltAttack

Generate effects for a predicted alt attack event.
================
*/
static void CG_WeaponPredict_AltAttack( int event ) {
	oldLerpPositions_t oldPositions;
	weaponPredictDirs_t dirs;

	CG_WeaponPredict_StoreLerpOrigins( &oldPositions );
	CG_WeaponPredict_CalcCommandLerpOrigins();
	dirs = CG_WeaponPredict_GetDirs( &cg.predictedPlayerState, qtrue );

	if ( cg.predictedPlayerState.weapon == WP_COMPRESSION_RIFLE ) {
		CG_WeaponPredict_FirePredictedRifle( &dirs, qtrue, qfalse );
	}

	if ( cg.predictedPlayerState.weapon == WP_IMOD ) {
		CG_WeaponPredict_FirePredictedImod( &dirs, qtrue );
	}

	if ( cg.predictedPlayerState.weapon == WP_SCAVENGER_RIFLE && wepPredictCfg.projectiles ) {
		predictedProjectile_t *proj = CG_WeaponPredict_SpawnPredictedProjectile(
				ET_ALT_MISSILE, WP_SCAVENGER_RIFLE, TR_GRAVITY, SCAV_ALT_VELOCITY, SCAV_ALT_SIZE, &dirs );
		proj->cent.currentState.eFlags |= EF_ALT_FIRING;
		VectorScale( dirs.forward, PREDICTABLE_RANDOM() * 100 + SCAV_ALT_VELOCITY, proj->cent.currentState.pos.trDelta );
		proj->cent.currentState.pos.trDelta[2] += SCAV_ALT_UP_VELOCITY;
		VectorCopy (proj->cent.currentState.pos.trBase, proj->cent.currentState.origin2);

		CG_WeaponPredict_LogPredictedProjectile( proj );
	}

	if ( cg.predictedPlayerState.weapon == WP_STASIS ) {
		trace_t tr;
		vec3_t end;
		centity_t tent;

		VectorMA (dirs.muzzle, MAXRANGE_ALT_STASIS, dirs.forward, end);
		CG_Trace (&tr, dirs.muzzle, NULL, NULL, end, -1, MASK_SHOT );

		memset( &tent, 0, sizeof( tent ) );
		tent.currentState.event = EV_STASIS;
		VectorCopy( tr.endpos, tent.lerpOrigin );

		VectorCopy( dirs.muzzle, tent.currentState.origin2 );

		tent.currentState.angles[YAW] = (int)(cg.predictedPlayerState.viewangles[YAW]);

		CG_WeaponPredict_RunPredictedEvent( &tent, cg.predictedPlayerState.commandTime );
	}

	if ( cg.predictedPlayerState.weapon == WP_GRENADE_LAUNCHER && wepPredictCfg.projectiles ) {
		trType_t moveType = wepPredictCfg.tripMines ? TR_LINEAR : TR_GRAVITY;
		predictedProjectile_t *proj = CG_WeaponPredict_SpawnPredictedProjectile(
				ET_ALT_MISSILE, WP_GRENADE_LAUNCHER, moveType, GRENADE_VELOCITY_ALT, GRENADE_SIZE, &dirs );
		SnapVector( proj->cent.currentState.pos.trDelta );
		proj->cent.currentState.eFlags |= EF_MISSILE_STICK;

		if ( SEEKER_ACCEL_ACTIVE ) {
			VectorScale( proj->cent.currentState.pos.trDelta, 2.0f, proj->cent.currentState.pos.trDelta );
		}

		CG_WeaponPredict_LogPredictedProjectile( proj );
	}

	if ( cg.predictedPlayerState.weapon == WP_TETRION_DISRUPTOR && wepPredictCfg.projectiles ) {
		predictedProjectile_t *proj = CG_WeaponPredict_SpawnPredictedProjectile(
				ET_ALT_MISSILE, WP_TETRION_DISRUPTOR, TR_LINEAR, TETRION_ALT_VELOCITY, TETRION_ALT_SIZE, &dirs );
		proj->cent.currentState.eFlags |= EF_BOUNCE;

		CG_WeaponPredict_LogPredictedProjectile( proj );
	}

	if ( cg.predictedPlayerState.weapon == WP_QUANTUM_BURST && wepPredictCfg.projectiles ) {
		predictedProjectile_t *proj = CG_WeaponPredict_SpawnPredictedProjectile(
				ET_ALT_MISSILE, WP_QUANTUM_BURST, TR_LINEAR, QUANTUM_ALT_VELOCITY, QUANTUM_SIZE, &dirs );
		proj->cent.currentState.eFlags |= EF_ALT_FIRING;

		CG_WeaponPredict_LogPredictedProjectile( proj );
	}

	if ( cg.predictedPlayerState.weapon == WP_DREADNOUGHT && wepPredictCfg.projectiles ) {
		predictedProjectile_t *proj = CG_WeaponPredict_SpawnPredictedProjectile(
				ET_ALT_MISSILE, WP_DREADNOUGHT, TR_LINEAR, 0, 0, &dirs );
		VectorCopy (proj->cent.currentState.pos.trBase, proj->cent.currentState.origin);
		VectorCopy( dirs.forward, proj->movedir );
		proj->nextThink = cg.predictedPlayerState.commandTime;
		CG_WeaponPredict_DreadnoughtBurstThink( proj, cg.predictedPlayerState.commandTime );

		CG_WeaponPredict_LogPredictedProjectile( proj );
	}

	if ( cg.predictedPlayerState.weapon == WP_BORG_WEAPON ) {
		trace_t tr;
		vec3_t end;
		centity_t tent;

		VectorMA (dirs.muzzle, MAXRANGE_IMOD, dirs.forward, end);
		CG_Trace (&tr, dirs.muzzle, NULL, NULL, end, -1, MASK_SHOT );

		memset( &tent, 0, sizeof( tent ) );
		tent.currentState.event = EV_BORG_ALT_WEAPON;
		VectorCopy( tr.endpos, tent.lerpOrigin );

		VectorMA (dirs.muzzle, 2, dirs.forward, tent.currentState.origin2);
		SnapVector( tent.currentState.origin2 );

		CG_WeaponPredict_RunPredictedEvent( &tent, cg.predictedPlayerState.commandTime );
	}

	CG_WeaponPredict_RestoreLerpOrigins( &oldPositions );
}

/*
================
CG_WeaponPredict_TransitionSnapshot

Called when transitioning a new server snapshot.
================
*/
void CG_WeaponPredict_TransitionSnapshot( void ) {
	if ( CG_WeaponPredict_IsActive() ) {
		centity_t *cent;
		int i;

		// Advance predicted missiles through the world.
		CG_WeaponPredict_RunMissiles();

		// Check for new missiles in the server snapshot matching predicted ones.
		CG_WeaponPredict_CheckPredictedMissiles();

		// Check for new events in the server snapshot, and disable any that have already been predicted.
		CG_WeaponPredict_CheckPredictedEvents();

		// Run regular entity events. This is normally called from CG_TransitionEntity, but moved here when
		// weapon prediction is active, to allow the preceding checks to run first.
		for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
			cent = &cg_entities[ cg.snap->entities[ i ].number ];
			CG_CheckEvents( cent );
		}
	}
}

/*
================
CG_WeaponPredict_HandleEvent

When a weapon fire event is generated by playerstate prediction, generate additional weapon effects
and/or simulated projectiles consistent with the ping compensation settings on the server.
================
*/
void CG_WeaponPredict_HandleEvent( centity_t *cent ) {
	if ( CG_WeaponPredict_IsActive() ) {
		// only handle predicted fire events here, not server-issued ones
		if ( cent == &cg.predictedPlayerEntity ) {
			int event = cent->currentState.event & ~EV_EVENT_BITS;

			// In some cases EV_FIRE_EMPTY_PHASER events can be issued in place of EV_FIRE_WEAPON even if
			// the phaser isn't the active weapon. This is due to quirks in the PM_Weapon function
			// (which can't be easily changed without compatibility considerations).
			if ( event == EV_FIRE_WEAPON || event == EV_FIRE_EMPTY_PHASER ) {
				CG_WeaponPredict_PrimaryAttack( event );
			}

			if ( event == EV_ALT_FIRE ) {
				CG_WeaponPredict_AltAttack( event );
			}
		}
	}
}
