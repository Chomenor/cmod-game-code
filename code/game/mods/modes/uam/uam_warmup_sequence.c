/*
* UAM Warmup Sequence
* 
* Show a series of messages explaining the game rules during warmup.
*/

#define MOD_NAME ModUAMWarmupSequence

#include "mods/modes/uam/uam_local.h"

static struct {
	trackedCvar_t g_mod_SimpleWarmup;	// always play short warmup
	qboolean shortWarmupReady;	// conditions met to play short warmups (no new players since full warmup completed)
	qboolean fullWarmupActive;	// playing full warmup this round
} *MOD_STATE;

/*
================
ModUAMWarmupSequence_Shared_DetpackEnabled
================
*/
qboolean ModUAMWarmupSequence_Shared_DetpackEnabled( void ) {
	return EF_WARN_ASSERT( MOD_STATE ) && MOD_STATE->fullWarmupActive;
}

/*
================
ModUAMWarmupSequence_WarmupSoundEvent
================
*/
static void ModUAMWarmupSequence_WarmupSoundEvent( const char *msg ) {
	G_GlobalSound( G_SoundIndex( msg ) );
}

/*
================
ModUAMWarmupSequence_WarmupRoundMessage
================
*/
static void ModUAMWarmupSequence_WarmupRoundMessage( const char *msg ) {
	if ( !ModElimMultiRound_Static_GetMultiRoundEnabled() ) {
		trap_SendServerCommand( -1, "cp \"\n\n\n\n^4and now:\n\n^7let the game begin...\"" );
	} else if ( ModElimMultiRound_Static_GetIsTiebreakerRound() ) {
		trap_SendServerCommand( -1, "cp \"\n\n\n\n^4scores are tied:\n\n^7repeat of final game begins...\"" );
	} else {
		trap_SendServerCommand( -1, va( "cp \"\n\n\n\n^4and now:\n\n^7let game %i of %i begin...\"",
				ModElimMultiRound_Static_GetCurrentRound(), ModElimMultiRound_Static_GetTotalRounds() ) );
	}
}

/*
================
(ModFN) GetWarmupSequence
================
*/
static qboolean MOD_PREFIX(GetWarmupSequence)( MODFN_CTV, modWarmupSequence_t *sequence ) {
	if ( MOD_STATE->fullWarmupActive ) {
		ModWarmupSequence_Static_AddEventToSequence( sequence, 4000, ModWarmupSequence_Static_ServerCommand, "cp \"\n\n^2welcome to the arena,\n^1Gladiator!\"" );
		ModWarmupSequence_Static_AddEventToSequence( sequence, 4000, ModUAMWarmupSequence_WarmupSoundEvent, "sound/movers/switches/boatswain.wav" );
		ModWarmupSequence_Static_AddEventToSequence( sequence, 8000, ModWarmupSequence_Static_ServerCommand, "cp \"\n\n^5these are the rules:\"" );
		ModWarmupSequence_Static_AddEventToSequence( sequence, 8000, ModUAMWarmupSequence_WarmupSoundEvent, "sound/interface/button3.wav" );
		ModWarmupSequence_Static_AddEventToSequence( sequence, 10000, ModWarmupSequence_Static_ServerCommand, "cp \"\n\n\n^71. ^1you have one life\"" );
		ModWarmupSequence_Static_AddEventToSequence( sequence, 10000, ModUAMWarmupSequence_WarmupSoundEvent, "sound/interface/button3.wav" );
		ModWarmupSequence_Static_AddEventToSequence( sequence, 12000, ModWarmupSequence_Static_ServerCommand, "cp \"\n\n\n\n^72. ^1you have one weapon\"" );
		ModWarmupSequence_Static_AddEventToSequence( sequence, 12000, ModUAMWarmupSequence_WarmupSoundEvent, "sound/interface/button3.wav" );
		ModWarmupSequence_Static_AddEventToSequence( sequence, 14000, ModWarmupSequence_Static_ServerCommand, "cp \"\n\n\n\n\n^73. ^1you have unlimited ammo\"" );
		ModWarmupSequence_Static_AddEventToSequence( sequence, 14000, ModUAMWarmupSequence_WarmupSoundEvent, "sound/interface/button3.wav" );
		ModWarmupSequence_Static_AddEventToSequence( sequence, 16000, ModWarmupSequence_Static_ServerCommand, "cp \"\n\n\n\n\n\n^74. ^1...and one goal:\n^2to survive!\"" );
		ModWarmupSequence_Static_AddEventToSequence( sequence, 16000, ModUAMWarmupSequence_WarmupSoundEvent, "sound/interface/button3.wav" );

		if ( ModElimMultiRound_Static_GetIsTiebreakerRound() ) {
			ModWarmupSequence_Static_AddEventToSequence( sequence, 18000, ModWarmupSequence_Static_ServerCommand, "cp \"\"" );
			ModWarmupSequence_Static_AddEventToSequence( sequence, 18000, ModUAMWarmupSequence_WarmupSoundEvent, "sound/voice/computer/misc/sudden_death.wav" );
		}

		ModWarmupSequence_Static_AddEventToSequence( sequence, 19000, ModUAMWarmupSequence_WarmupRoundMessage, NULL );
		sequence->duration = 22000;
	}

	else {
		if ( ModElimMultiRound_Static_GetIsTiebreakerRound() ) {
			ModWarmupSequence_Static_AddEventToSequence( sequence, 3000, ModWarmupSequence_Static_ServerCommand, "cp \"\"" );
			ModWarmupSequence_Static_AddEventToSequence( sequence, 3000, ModUAMWarmupSequence_WarmupSoundEvent, "sound/voice/computer/misc/sudden_death.wav" );
		}

		ModWarmupSequence_Static_AddEventToSequence( sequence, 4000, ModUAMWarmupSequence_WarmupRoundMessage, NULL );
		sequence->duration = 7000;
	}

	return qtrue;
}

/*
================
(ModFN) PostClientConnect
================
*/
static void MOD_PREFIX(PostClientConnect)( MODFN_CTV, int clientNum, qboolean firstTime, qboolean isBot ) {
	MODFN_NEXT( PostClientConnect, ( MODFN_NC, clientNum, firstTime, isBot ) );

	// Clear shortWarmupReady when new players join.
	// However, players joining during warmup are currently ignored, consistent with Gladiator mod.
	if ( firstTime && !isBot && level.matchState != MS_WARMUP ) {
		MOD_STATE->shortWarmupReady = qfalse;
	}

	// If the warmup hasn't started yet, recalculate whether to do full length warmup.
	if ( level.matchState < MS_WARMUP ) {
		MOD_STATE->fullWarmupActive = !( MOD_STATE->g_mod_SimpleWarmup.integer || MOD_STATE->shortWarmupReady );
	}
}

/*
================
(ModFN) GenerateGlobalSessionInfo
================
*/
static void MOD_PREFIX(GenerateGlobalSessionInfo)( MODFN_CTV, info_string_t *info ) {
	MODFN_NEXT( GenerateGlobalSessionInfo, ( MODFN_NC, info ) );

	// If we just completed a full warmup, initialize shortWarmupReady to true.
	if ( level.exiting && MOD_STATE->fullWarmupActive ) {
		MOD_STATE->shortWarmupReady = qtrue;
	}

	if ( MOD_STATE->shortWarmupReady ) {
		Info_SetValueForKey( info->s, "uam_shortWarmupReady", "1" );
	}
}

/*
================
ModUAMWarmupSequence_Init
================
*/
void ModUAMWarmupSequence_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		G_RegisterTrackedCvar( &MOD_STATE->g_mod_SimpleWarmup, "g_mod_SimpleWarmup", "0", 0, qfalse );

		// Warmup effects configuration
		ModUAMWarmupMode_Init();

		// Check if we are clear to use short warmup due to no map change or new players during last round
		if ( level.hasRestarted && G_RetrieveGlobalSessionValue( "uam_shortWarmupReady" ) ) {
			MOD_STATE->shortWarmupReady = qtrue;
		}

		// Warmup message configuration
		ModWarmupSequence_Init();

		MODFN_REGISTER( GetWarmupSequence, ++modePriorityLevel );
		MODFN_REGISTER( PostClientConnect, ++modePriorityLevel );
		MODFN_REGISTER( GenerateGlobalSessionInfo, ++modePriorityLevel );
	}
}
