/*
* Warmup Sequence
* 
* This module provides a standard implementation for mods that want to play a sequence
* of messages while warmup is in progress.
* 
* To use it, mods register modWarmupSequence_shared->getSequence with a function that
* returns the list of events and times, and the length of warmup.
*/

#define MOD_NAME ModWarmupSequence

#include "mods/g_mod_local.h"

static struct {
	qboolean sequenceActive;
	modWarmupSequence_t sequence;
	int nextEvent;
} *MOD_STATE;

/*
================
ModWarmupSequence_Static_ServerCommand

Helper function which can be used for simple server command events.
================
*/
void ModWarmupSequence_Static_ServerCommand( const char *msg ) {
	trap_SendServerCommand( -1, msg );
}

/*
================
ModWarmupSequence_Static_AddEventToSequence

Adds event to warmup sequence.
================
*/
void ModWarmupSequence_Static_AddEventToSequence( modWarmupSequence_t *sequence, int time,
		void ( *operation )( const char *msg ), const char *msg ) {
	if ( EF_WARN_ASSERT( sequence->eventCount < MAX_INFO_SEQUENCE_EVENTS ) ) {
		modWarmupSequenceEvent_t event;
		event.time = time;
		event.operation = operation;
		event.msg = msg;
		sequence->events[sequence->eventCount++] = event;
	}
}

/*
================
(ModFN) GetWarmupSequence
================
*/
qboolean ModFNDefault_GetWarmupSequence( modWarmupSequence_t *sequence ) {
	return qfalse;
}

/*
================
ModWarmupSequence_GetLength

If warmup sequence is currently in progress, returns length of current sequence. Otherwise queries
modWarmupSequence_shared->getSequence to get expected sequence length. Returns 0 if sequence disabled.
================
*/
static int ModWarmupSequence_GetLength( void ) {
	if ( level.matchState == MS_WARMUP ) {
		// If warmup already running, return length of current sequence, if any.
		if ( MOD_STATE->sequenceActive ) {
			return MOD_STATE->sequence.duration;
		}

	} else if ( g_doWarmup.integer ) {
		// Get the sequence to determine length of future warmup.
		modWarmupSequence_t sequence;
		memset( &sequence, 0, sizeof( sequence ) );
		if ( modfn_lcl.GetWarmupSequence( &sequence ) )
			return sequence.duration;
	}

	return 0;
}

/*
================
ModWarmupSequence_SequenceInProgress

Returns whether warmup sequence is currently running.
================
*/
static qboolean ModWarmupSequence_SequenceInProgress( void ) {
	return MOD_STATE && level.matchState == MS_WARMUP && MOD_STATE->sequenceActive;
}

/*
================
(ModFN) SpawnCenterPrintMessage

Do a rough check to suppress center print messages if warmup sequence is currently running
or pending, to avoid too much message spam.
================
*/
static void MOD_PREFIX(SpawnCenterPrintMessage)( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ) {
	if ( ModWarmupSequence_SequenceInProgress() ||
			( level.matchState == MS_INIT && ModWarmupSequence_GetLength() ) ) {
		return;
	}

	MODFN_NEXT( SpawnCenterPrintMessage, ( MODFN_NC, clientNum, spawnType ) );
}

/*
==================
(ModFN) WarmupLength

Returns length in milliseconds to use for warmup if enabled, or <= 0 if warmup disabled.
==================
*/
static int MOD_PREFIX(WarmupLength)( MODFN_CTV ) {
	int sequenceLength = ModWarmupSequence_GetLength();
	if ( sequenceLength ) {
		return sequenceLength;
	} else {
		return MODFN_NEXT( WarmupLength, ( MODFN_NC ) );
	}
}

/*
================
(ModFN) PostRunFrame
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );

	if ( ModWarmupSequence_SequenceInProgress() ) {
		// Get time elapsed since warmup started.
		int time = level.time - ( level.warmupTime - MOD_STATE->sequence.duration );

		// Dispatch events.
		while ( MOD_STATE->nextEvent < MOD_STATE->sequence.eventCount ) {
			const modWarmupSequenceEvent_t *event = &MOD_STATE->sequence.events[MOD_STATE->nextEvent];
			if ( time < event->time ) {
				return;
			}

			if ( event->operation ) {
				event->operation( event->msg );
			}
			MOD_STATE->nextEvent++;
		}
	}
}

/*
================
(ModFN) MatchStateTransition
================
*/
static void MOD_PREFIX(MatchStateTransition)( MODFN_CTV, matchState_t oldState, matchState_t newState ) {
	MODFN_NEXT( MatchStateTransition, ( MODFN_NC, oldState, newState ) );

	// Clear existing sequence.
	memset( &MOD_STATE->sequence, 0, sizeof ( MOD_STATE->sequence ) );
	MOD_STATE->sequenceActive = qfalse;
	MOD_STATE->nextEvent = 0;

	if ( newState == MS_WARMUP ) {
		// Start the sequence.
		memset( &MOD_STATE->sequence, 0, sizeof( MOD_STATE->sequence ) );
		if ( modfn_lcl.GetWarmupSequence( &MOD_STATE->sequence ) ) {
			MOD_STATE->sequenceActive = qtrue;
		}
	}
}

/*
================
ModWarmupSequence_Init
================
*/
void ModWarmupSequence_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( SpawnCenterPrintMessage, MODPRIORITY_HIGH );
		MODFN_REGISTER( WarmupLength, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PostRunFrame, MODPRIORITY_GENERAL );
		MODFN_REGISTER( MatchStateTransition, MODPRIORITY_GENERAL );
	}
}
