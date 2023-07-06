/*
* Razor Difficulty
* 
* Support the "difficulty" cvar to allow selecting the amount of powerups available
* in the game.
*/

#define MOD_NAME ModRazorDifficulty

#include "mods/modes/razor/razor_local.h"

typedef enum {
	RAZOR_EASY,
	RAZOR_CLASSIC,
	RAZOR_MEDIUM,
	RAZOR_HARD,
} razorDifficulty_t;

static struct {
	razorDifficulty_t difficulty;
} *MOD_STATE;

/*
================
ModRazorDifficulty_Static_DifficultyString
================
*/
const char *ModRazorDifficulty_Static_DifficultyString( void ) {
	switch ( MOD_STATE ? MOD_STATE->difficulty : RAZOR_EASY ) {
		default:
			return "easy";
		case RAZOR_CLASSIC:
			return "classic";
		case RAZOR_MEDIUM:
			return "medium";
		case RAZOR_HARD:
			return "hard";
	}
}

/*
================
(ModFN) CheckItemSpawnDisabled
================
*/
static qboolean MOD_PREFIX(CheckItemSpawnDisabled)( MODFN_CTV, gitem_t *item ) {
	if ( MOD_STATE->difficulty >= RAZOR_CLASSIC && item->giType == IT_POWERUP &&
			( item->giTag == PW_FLIGHT || item->giTag == PW_SEEKER ) ) {
		return qtrue;
	}
	if ( MOD_STATE->difficulty >= RAZOR_MEDIUM && item->giType == IT_POWERUP ) {
		return qtrue;
	}
	if ( MOD_STATE->difficulty >= RAZOR_HARD && item->giType == IT_HOLDABLE ) {
		return qtrue;
	}

	return MODFN_NEXT( CheckItemSpawnDisabled, ( MODFN_NC, item ) );
}

/*
================
ModRazorDifficulty_Init
================
*/
void ModRazorDifficulty_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		// Determine difficulty
		{
			vmCvar_t cvar;
			trap_Cvar_Register( &cvar, "difficulty", "0", CVAR_LATCH | CVAR_SERVERINFO );
			MOD_STATE->difficulty = cvar.integer;
			if ( MOD_STATE->difficulty < RAZOR_EASY ) {
				MOD_STATE->difficulty = RAZOR_EASY;
			}
			if ( MOD_STATE->difficulty > RAZOR_HARD ) {
				MOD_STATE->difficulty = RAZOR_HARD;
			}
			trap_Cvar_Set( "difficulty", va( "%i", MOD_STATE->difficulty ) );
		}

		MODFN_REGISTER( CheckItemSpawnDisabled, ++modePriorityLevel );
	}
}
