/*
* UAM End Sound
* 
* Play an extra sound effect when the round is over.
*/

#define MOD_NAME ModUAMEndSound

#include "mods/modes/uam/uam_local.h"

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
ModUAMEndSound_Init
================
*/
void ModUAMEndSound_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( MatchStateTransition, ++modePriorityLevel );
	}
}
