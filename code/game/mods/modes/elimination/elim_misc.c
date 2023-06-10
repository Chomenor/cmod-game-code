/*
* Elimination Misc
* 
* Extra elimination features that don't need to be part of the main module.
*/

#define MOD_NAME ModElimMisc

#include "mods/modes/elimination/elim_local.h"

// Show current round numbers in HUD score indicator in FFA games.
#define FEATURE_HUD_ROUND_INDICATOR_FFA

// Support cvar-enabled option to ignore eliminated players for adapt item respawn calculation.
#define FEATURE_ELIMINATION_ADAPT_RESPAWN

static struct {
	int _unused;
#ifdef FEATURE_ELIMINATION_ADAPT_RESPAWN
	trackedCvar_t g_adaptRespawnIgnoreEliminated;
#endif
} *MOD_STATE;

/*
================
(ModFN) AdaptRespawnNumPlayers

Support ignoring eliminated players in adapt respawn calculations.
================
*/
static int MOD_PREFIX(AdaptRespawnNumPlayers)( MODFN_CTV ) {
#ifdef FEATURE_ELIMINATION_ADAPT_RESPAWN
	if ( MOD_STATE->g_adaptRespawnIgnoreEliminated.integer ) {
		return ModElimination_Static_CountPlayersAlive();
	}
#endif

	return MODFN_NEXT( AdaptRespawnNumPlayers, ( MODFN_NC ) );
}

/*
============
(ModFN) SetScoresConfigStrings
============
*/
static void MOD_PREFIX(SetScoresConfigStrings)( MODFN_CTV ) {
#ifdef FEATURE_HUD_ROUND_INDICATOR_FFA
	if ( g_gametype.integer < GT_TEAM ) {
		// Place round numbers in score configstrings.
		trap_SetConfigstring( CS_SCORES1, va( "%i", ModElimMultiRound_Static_GetCurrentRound() ) );
		trap_SetConfigstring( CS_SCORES2, va( "%i", ModElimMultiRound_Static_GetTotalRounds() ) );
		return;
	}
#endif

	MODFN_NEXT( SetScoresConfigStrings, ( MODFN_NC ) );
}

/*
================
(ModFN) PostRunFrame

Assumed to be registered after main elimination version.
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );

#ifdef FEATURE_HUD_ROUND_INDICATOR_FFA
	if ( g_gametype.integer < GT_TEAM ) {
		// Set PERS_SCORE values to current round number, as a trick to get values to display correctly
		// in CG_DrawScores without the need for a client-side mod. This is valid because the elimination
		// module uses EffectiveScore in FFA and does not depend on PERS_SCORE for actual scorekeeping.
		int i;
		int roundNum = ModElimMultiRound_Static_GetCurrentRound();
		for ( i = 0; i < level.maxclients; ++i ) {
			level.clients[i].ps.persistant[PERS_SCORE] = roundNum;
		}
	}
#endif
}

/*
================
ModElimMisc_Init
================
*/
void ModElimMisc_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

#ifdef FEATURE_ELIMINATION_ADAPT_RESPAWN
		G_RegisterTrackedCvar( &MOD_STATE->g_adaptRespawnIgnoreEliminated, "g_adaptRespawnIgnoreEliminated", "0", 0, qfalse );
#endif

		MODFN_REGISTER( AdaptRespawnNumPlayers, ++modePriorityLevel );
		MODFN_REGISTER( SetScoresConfigStrings, ++modePriorityLevel );
		MODFN_REGISTER( PostRunFrame, ++modePriorityLevel );

		if ( modcfg.mods_enabled.uam || G_ModUtils_GetLatchedValue( "g_mod_elimTweaks", "0", 0 ) ) {
			ModElimTweaks_Init();
		} else {
			G_DedPrintf( "NOTE: Recommend setting 'g_mod_elimTweaks' to 1 to enable various "
					"Elimination enhancements.\n" );
		}

		if ( !modcfg.mods_enabled.tournament ) {
			ModElimMultiRound_Init();
		}

		if ( trap_Cvar_VariableIntegerValue( "fraglimit" ) != 0 ) {
			G_DedPrintf( "WARNING: Recommend setting 'fraglimit' to 0 for Elimination mode "
					"to remove unnecessary HUD indicator.\n" );
		}

	}
}
