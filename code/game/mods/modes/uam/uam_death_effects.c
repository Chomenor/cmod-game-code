/*
* UAM Death Effects
* 
* Skip death effects that hide the body (such as explosions), and keep the body
* remaining indefinitely in Elimination mode.
*/

#define MOD_NAME ModUAMDeathEffects

#include "mods/modes/uam/uam_local.h"

static struct {
	int _unused;
} *MOD_STATE;

/*
==================
(ModFN) PlayerDeathEffect

Only do a basic effect in UAM regardless of the means of death.
==================
*/
static void MOD_PREFIX(PlayerDeathEffect)( MODFN_CTV, gentity_t *self, gentity_t *inflictor, gentity_t *attacker,
		int contents, int killer, int meansOfDeath ) {
	// Pass forcefield kills through so forcefield module can apply its own effects.
	if ( inflictor && inflictor->s.eType == ET_USEABLE && inflictor->s.modelindex == HI_SHIELD ) {
		MODFN_NEXT( PlayerDeathEffect, ( MODFN_NC, self, inflictor, attacker, contents, killer, meansOfDeath ) );
		return;
	}

	// Add sound effect
	if ( !( ( contents & CONTENTS_NODROP ) && meansOfDeath == MOD_TRIGGER_HURT && killer == ENTITYNUM_WORLD ) ) {
		G_AddEvent( self, irandom(EV_DEATH1, EV_DEATH3), killer );
	}

	// Can't destroy bodies in UAM
	self->takedamage = qfalse;

	// For certain weapons, health is set to 0 in player_die, which ensures the body is visible.
	// For other weapons, players can currently become invisible if they took enough damage at
	// once to put the health at or below -40 (GIB_HEALTH). In Gladiator, MOD_TRIGGER_HURT also
	// sets health to 0 and results in a visible body.
	if ( meansOfDeath == MOD_TRIGGER_HURT ) {
		self->health = 0;
	}
}

/*
=============
(ModFN) CopyToBodyQue

Make the body last indefinitely in Elimination mode.
=============
*/
static gentity_t *MOD_PREFIX(CopyToBodyQue)( MODFN_CTV, int clientNum ) {
	gentity_t *body = MODFN_NEXT( CopyToBodyQue, ( MODFN_NC, clientNum ) );

	if ( body ) {
		// Can't destroy bodies in UAM
		body->takedamage = qfalse;

		if ( modcfg.mods_enabled.elimination ) {
			// Never trigger the rez out, so body stays indefinitely
			body->nextthink = 0;
		}
	}

	return body;
}

/*
================
ModUAMDeathEffects_Init
================
*/
void ModUAMDeathEffects_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( PlayerDeathEffect, ++modePriorityLevel );
		MODFN_REGISTER( CopyToBodyQue, ++modePriorityLevel );
	}
}
