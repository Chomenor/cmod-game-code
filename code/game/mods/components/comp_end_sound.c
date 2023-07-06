/*
* End Sound
* 
* Play an extra sound effect when entering intermission consistent with Gladiator and Pinball mods.
*/

#define MOD_NAME ModEndSound

#include "mods/g_mod_local.h"

static struct {
	int _unused;
} *MOD_STATE;

/*
================
(ModFN) MatchStateTransition
================
*/
static void MOD_PREFIX(MatchStateTransition)( MODFN_CTV, matchState_t oldState, matchState_t newState ) {
	if ( oldState < MS_INTERMISSION_ACTIVE && newState >= MS_INTERMISSION_ACTIVE ) {
		// Play sound when entering intermission
		G_GlobalSound( G_SoundIndex( rand() % 2 ? "sound/enemies/borg/borgrecycle.wav" : "sound/movers/telsiagone.wav" ) );
	}

	MODFN_NEXT( MatchStateTransition, ( MODFN_NC, oldState, newState ) );
}

/*
================
ModEndSound_Init
================
*/
void ModEndSound_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( MatchStateTransition, MODPRIORITY_GENERAL );
	}
}
