/*
* Razor Damage Handling
* 
* Trigger damage and effects when player hits the edge of the map.
*/

#define MOD_NAME ModRazorDamage

#include "mods/modes/razor/razor_local.h"

// Need to differentiate hit registered vs actual G_Damage call for gold armor teleport check.
typedef enum {
	WALLHIT_INACTIVE,
	WALLHIT_REGISTERED,	// wallhit registered; waiting to run G_Damage
	WALLHIT_DAMAGE,		// currently calling G_Damage
} wallHitState_t;

static struct {
	void ( *Prev_Trace )( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
			const vec3_t end, int passEntityNum, int contentMask );

	// Temporary flag that indicates currently moving player hit map edge
	wallHitState_t wallHit;
} *MOD_STATE;

#define IS_FORCEFIELD( ent ) ( ( ent ) && ( ent )->inuse && ( ent )->s.eType == ET_USEABLE && \
		( ent )->s.modelindex == HI_SHIELD )

/*
================
ModRazorDamage_Trace
================
*/
static void ModRazorDamage_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
		const vec3_t end, int passEntityNum, int contentMask ) {
	MOD_STATE->Prev_Trace( results, start, mins, maxs, end, passEntityNum, contentMask );

	if ( MOD_STATE->wallHit == WALLHIT_INACTIVE &&
			ModRazorBounds_Static_CheckTraceHit( results, start, mins, maxs, end, passEntityNum, contentMask ) ) {
		MOD_STATE->wallHit = WALLHIT_REGISTERED;
	}
}

/*
================
(ModFN) PmoveInit
================
*/
static void MOD_PREFIX(PmoveInit)( MODFN_CTV, int clientNum, pmove_t *pmove ) {
	MODFN_NEXT( PmoveInit, ( MODFN_NC, clientNum, pmove ) );

	MOD_STATE->Prev_Trace = pmove->trace;
	pmove->trace = ModRazorDamage_Trace;
}

/*
==============
(ModFN) PostPmoveActions
==============
*/
static void MOD_PREFIX(PostPmoveActions)( MODFN_CTV, pmove_t *pmove, int clientNum, int oldEventSequence ) {
	gentity_t *ent = &g_entities[clientNum];

	MODFN_NEXT( PostPmoveActions, ( MODFN_NC, pmove, clientNum, oldEventSequence ) );

	// Trigger damage if player hit map edge during move.
	if ( MOD_STATE->wallHit == WALLHIT_REGISTERED ) {
		MOD_STATE->wallHit = WALLHIT_DAMAGE;

		// Gold armor case - triggers effects but doesn't kill.
		// Alive player case - kills them.
		// Dead body drifting into wall - blows it up.
		G_Damage( ent, NULL, NULL, NULL, NULL, 1000, 0, MOD_TRIGGER_HURT );

		MOD_STATE->wallHit = WALLHIT_INACTIVE;
	}
}

/*
==================
(ModFN) PlayerDeathEffect
==================
*/
static void MOD_PREFIX(PlayerDeathEffect)( MODFN_CTV, gentity_t *self, gentity_t *inflictor, gentity_t *attacker,
		int contents, int killer, int meansOfDeath ) {
	// Never hide body due to GIB_HEALTH.
	self->health = 0;

	if ( MOD_STATE->wallHit == WALLHIT_DAMAGE ) {
		// Play disintegration effect.
		G_AddEvent( self, EV_EXPLODESHELL, 0 );
		self->client->ps.powerups[PW_DISINTEGRATE] = level.time + 100000;
		VectorClear( self->client->ps.velocity );
		self->takedamage = qfalse;
		self->r.contents = 0;
	} else {
		// Play sound regardless of "bottomless pit" case.
		contents &= ~CONTENTS_NODROP;
		MODFN_NEXT( PlayerDeathEffect, ( MODFN_NC, self, inflictor, attacker, contents, killer, meansOfDeath ) );
	}
}

/*
================
(ModFN) Damage
================
*/
static void MOD_PREFIX(Damage)( MODFN_CTV, gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
			vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	// Teleport player out if they fell into a damage zone while holding gold armor.
	if ( targ->client && targ->client->ps.powerups[PW_BATTLESUIT] && mod == MOD_TRIGGER_HURT &&
			!IS_FORCEFIELD( inflictor ) && MOD_STATE->wallHit != WALLHIT_DAMAGE ) {
		vec3_t origin, angles;
		SelectSpawnPoint( targ->client->ps.origin, origin, angles, qtrue, qtrue );
		TeleportPlayer( targ, origin, angles, TP_NORMAL );
	}

	MODFN_NEXT( Damage, ( MODFN_NC, targ, inflictor, attacker, dir, point, damage, dflags, mod ) );
}

/*
================
(ModFN) AdjustModConstant
================
*/
static int MOD_PREFIX(AdjustModConstant)( MODFN_CTV, modConstant_t mcType, int defaultValue ) {
	switch ( mcType ) {
		case MC_PINGCOMP_NO_TH_DEAD_MOVE:
			// Use simple timeshifted playerstate position, rather than split movement, for
			// smoothing corpses because it works better for wall hit kills.
			return 1;
	}

	return MODFN_NEXT( AdjustModConstant, ( MODFN_NC, mcType, defaultValue ) );
}

/*
================
ModRazorDamage_Init
================
*/
void ModRazorDamage_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		ModRazorBounds_Init();

		MODFN_REGISTER( PmoveInit, ++modePriorityLevel );
		MODFN_REGISTER( PostPmoveActions, ++modePriorityLevel );
		MODFN_REGISTER( PlayerDeathEffect, ++modePriorityLevel );
		MODFN_REGISTER( Damage, ++modePriorityLevel );
		MODFN_REGISTER( AdjustModConstant, ++modePriorityLevel );
	}
}
