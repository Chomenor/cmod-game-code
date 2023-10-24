/*
* Razor Scoring
* 
* Award points to a player if they push another player into a wall or off the map.
* 
* Also drop powerups at the location the player was pushed, rather than the final
* location they died, for easier pickup.
*/

#define MOD_NAME ModRazorScoring

#include "mods/modes/razor/razor_local.h"

typedef struct {
	int lastPushExpireTime;
	int lastPushAttacker;
	vec3_t lastPushPosition;
} razorScoringClient_t;

static struct {
	razorScoringClient_t clients[MAX_CLIENTS];
} *MOD_STATE;

#define LAST_PUSH_VALID( modclient ) ( ( modclient )->lastPushExpireTime && ( modclient )->lastPushExpireTime > level.time )

/*
============
ModRazorScoring_Static_LastPusher

Returns client number of recent player who pushed this player, or -1 if none.
============
*/
int ModRazorScoring_Static_LastPusher( int clientNum ) {
	if ( MOD_STATE && G_AssertConnectedClient( clientNum ) ) {
		razorScoringClient_t *modclient = &MOD_STATE->clients[clientNum];
		if ( LAST_PUSH_VALID( modclient ) ) {
			return modclient->lastPushAttacker;
		}
	}

	return -1;
}

/*
==============
(ModFN) RunPlayerMove
==============
*/
static void MOD_PREFIX(RunPlayerMove)( MODFN_CTV, int clientNum, qboolean spectator ) {
	razorScoringClient_t *modclient = &MOD_STATE->clients[clientNum];

	MODFN_NEXT( RunPlayerMove, ( MODFN_NC, clientNum, spectator ) );

	if ( LAST_PUSH_VALID( modclient ) && VectorCompare( level.clients[clientNum].ps.velocity, vec3_origin ) ) {
		// Reset recorded hit due to no movement.
		modclient->lastPushExpireTime = 0;
	}
}

/*
================
(ModFN) Damage
================
*/
static void MOD_PREFIX(Damage)( MODFN_CTV, gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
			vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	int clientNum = targ - g_entities;
	razorScoringClient_t *modclient = &MOD_STATE->clients[clientNum];

	// Record knockback-causing hit from another player.
	if ( dir && targ->client && attacker && attacker->client ) {
		int clientNum = targ - g_entities;
		razorScoringClient_t *modclient = &MOD_STATE->clients[clientNum];
		if ( targ == attacker ) {
			// Reset due to self hit
			modclient->lastPushExpireTime = 0;
		} else if ( g_friendlyFire.integer || !OnSameTeam( targ, attacker ) ) {
			// Register enemy hit
			modclient->lastPushExpireTime = level.time + 8000;
			modclient->lastPushAttacker = attacker - g_entities;
			VectorCopy( targ->client->ps.origin, modclient->lastPushPosition );
		}
	}

	// Check for replacing attacker with player responsible for pushing.
	if ( !dir ) {
		if ( inflictor && inflictor->s.eType == ET_USEABLE && inflictor->s.modelindex == HI_SHIELD ) {
			forcefieldRelation_t relation = G_GetForcefieldRelation( clientNum, inflictor );
			if ( relation == FFR_ENEMY ) {
				// Treat enemy forcefield owner as killer.
			} else if ( LAST_PUSH_VALID( modclient ) ) {
				// If pushed into friendly (our own or teamates) forcefield, treat pusher as killer.
				attacker = &g_entities[modclient->lastPushAttacker];
			} else {
				// If killed by friendly forcefield without being pushed, treat as a world kill.
				attacker = NULL;
			}
		}

		else if ( LAST_PUSH_VALID( modclient ) ) {
			// If pushed and hit by anything other than a forcefield, always award the kill to pusher.
			attacker = &g_entities[modclient->lastPushAttacker];
		}
	}

	MODFN_NEXT( Damage, ( MODFN_NC, targ, inflictor, attacker, dir, point, damage, dflags, mod ) );
}

/*
==============
(ModFN) ItemTossOrigin
==============
*/
static void MOD_PREFIX(ItemTossOrigin)( MODFN_CTV, int clientNum, vec3_t originOut ) {
	razorScoringClient_t *modclient = &MOD_STATE->clients[clientNum];

	if ( LAST_PUSH_VALID( modclient ) ) {
		VectorCopy( modclient->lastPushPosition, originOut );
	} else {
		MODFN_NEXT( ItemTossOrigin, ( MODFN_NC, clientNum, originOut ) );
	}
}

/*
================
ModRazorScoring_Init
================
*/
void ModRazorScoring_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( RunPlayerMove, ++modePriorityLevel );
		MODFN_REGISTER( Damage, ++modePriorityLevel );
		MODFN_REGISTER( ItemTossOrigin, ++modePriorityLevel );
	}
}
