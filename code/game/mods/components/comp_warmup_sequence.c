/*
* Warmup Sequence
* 
* This module provides a standard implementation for mods that want to play a sequence
* of messages while warmup is in progress.
* 
* To use it, mods register modWarmupSequence_shared->getSequence with a function that
* returns the list of events and times, and the length of warmup.
*/

#define MOD_PREFIX( x ) ModWarmupSequence_##x

#include "mods/g_mod_local.h"

static struct {
	qboolean sequenceActive;
	modWarmupSequence_t sequence;
	int nextEvent;

	ModFNType_WarmupLength Prev_WarmupLength;
	ModFNType_PostRunFrame Prev_PostRunFrame;
	ModFNType_MatchStateTransition Prev_MatchStateTransition;
} *MOD_STATE;

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
ModWarmupSequence_Static_SequenceInProgressOrPending

Returns whether warmup sequence is currently running, or pending until more players join.
================
*/
qboolean ModWarmupSequence_Static_SequenceInProgressOrPending( void ) {
	return MOD_STATE && ( ( level.matchState == MS_WARMUP && MOD_STATE->sequenceActive ) ||
			( level.matchState < MS_WARMUP && ModWarmupSequence_GetLength() ) );
}

/*
==================
(ModFN) WarmupLength

Returns length in milliseconds to use for warmup if enabled, or <= 0 if warmup disabled.
==================
*/
LOGFUNCTION_SRET( int, MOD_PREFIX(WarmupLength), ( void ), (), "G_MODFN_WARMUPLENGTH" ) {
	int sequenceLength = ModWarmupSequence_GetLength();
	if ( sequenceLength ) {
		return sequenceLength;
	} else {
		return MOD_STATE->Prev_WarmupLength();
	}
}

/*
================
(ModFN) PostRunFrame
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PostRunFrame), ( void ), (), "G_MODFN_POSTRUNFRAME" ) {
	MOD_STATE->Prev_PostRunFrame();

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
LOGFUNCTION_SVOID( MOD_PREFIX(MatchStateTransition), ( matchState_t oldState, matchState_t newState ),
		( oldState, newState ), "G_MODFN_MATCHSTATETRANSITION" ) {
	MOD_STATE->Prev_MatchStateTransition( oldState, newState );

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
LOGFUNCTION_VOID( ModWarmupSequence_Init, ( void ), (), "G_MOD_INIT G_MOD_WARMUPSEQUENCE" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		INIT_FN_STACKABLE( WarmupLength );
		INIT_FN_STACKABLE( PostRunFrame );
		INIT_FN_STACKABLE( MatchStateTransition );
	}
}
