/*
* Status Scores Override
* 
* This module is used to adjust the player scores sent in response to status queries to the
* server (from external server browser tools).
* 
* The adjusted score is determined by the EffectiveScore mod function with EST_STATUS_QUERY.
* 
* The current uses are:
* 1) to show informative score values for eliminated players in elimination mode, which would
*    otherwise be broken by the method used to display round numbers in the hud score area.
* 2) to show follow spectators as 0 score externally without breaking the hud score indicator.
* 
* This mod requires engine support via the trap_status_scores_override_set_array extension.
* If not available, this mod will not have any effect.
*/

#define MOD_NAME ModStatusScores

#include "mods/g_mod_local.h"

static struct {
	int sharedScores[MAX_CLIENTS];
} *MOD_STATE;

/*
================
(ModFN) PostRunFrame

Update scores array.
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PostRunFrame), ( MODFN_CTV ), ( MODFN_CTN ), "G_MODFN_POSTRUNFRAME" ) {
	int i;
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );

	for ( i = 0; i < level.maxclients; ++i ) {
		if ( level.clients[i].pers.connected >= CON_CONNECTING ) {
			MOD_STATE->sharedScores[i] = modfn.EffectiveScore( i, EST_STATUS_QUERY );
		} else {
			MOD_STATE->sharedScores[i] = 0;
		}
	}
}

/*
================
(ModFN) GeneralInit

Delay set array call until level.maxclients is initialized.
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(GeneralInit), ( MODFN_CTV ), ( MODFN_CTN ), "G_MODFN_GENERALINIT" ) {
	MODFN_NEXT( GeneralInit, ( MODFN_NC ) );
	VMExt_FN_StatusScoresOverride_SetArray( MOD_STATE->sharedScores, level.maxclients );
}

/*
================
ModStatusScores_Init
================
*/
LOGFUNCTION_VOID( ModStatusScores_Init, ( void ), (), "G_MOD_INIT" ) {
	if ( !MOD_STATE && VMExt_FNAvailable_StatusScoresOverride_SetArray() ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( PostRunFrame, MODPRIORITY_GENERAL );
		MODFN_REGISTER( GeneralInit, MODPRIORITY_GENERAL );
	}
}
