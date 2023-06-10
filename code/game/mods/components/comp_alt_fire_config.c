/*
* Alt Fire Config
* 
* Supports configuring alt fire mode for weapons. Configuration is determined by
* AltFireConfig mod function, which returns a control character for each weapon
* (see cg_weapons.c for a description of control values).
* 
* Forced modes ('f' for primary only, 'F' for alt only) are implemented on the server,
* and also added to configstring for client prediction. Other values are only added
* to the configstring to determine the alt fire swapping mode on the client.
*/

#define MOD_NAME ModAltFireConfig

#include "mods/g_mod_local.h"

static struct {
	char modeString[WP_NUM_WEAPONS];
} *MOD_STATE;

/*
================
(ModFN-Default) AltFireConfig
================
*/
char ModFNDefault_AltFireConfig( weapon_t weapon ) {
	return 'u';
}

/*
==============
(ModFN) AddModConfigInfo

Update mode string and add it to mod config info.
==============
*/
static void MOD_PREFIX(AddModConfigInfo)( MODFN_CTV, char *info ) {
	weapon_t weapon;

	// Generate string.
	for ( weapon = WP_PHASER; weapon < WP_NUM_WEAPONS; ++weapon ) {
		MOD_STATE->modeString[weapon - 1] = modfn_lcl.AltFireConfig( weapon );
	}
	MOD_STATE->modeString[WP_NUM_WEAPONS - 1] = '\0';

	// Remove trailing 'undefined' characters.
	for ( weapon = WP_NUM_WEAPONS - 1; weapon >= WP_PHASER; --weapon ) {
		if ( MOD_STATE->modeString[weapon - 1] != 'u' ) {
			break;
		}
		MOD_STATE->modeString[weapon - 1] = '\0';
	}

	// If the string wasn't emptied, add it to info string.
	if ( MOD_STATE->modeString[0] ) {
		Info_SetValueForKey( info, "altSwapPrefs", MOD_STATE->modeString );
	}

	MODFN_NEXT( AddModConfigInfo, ( MODFN_NC, info ) );
}

/*
================
(ModFN) PmoveInit

Apply forced modes.
================
*/
static void MOD_PREFIX(PmoveInit)( MODFN_CTV, int clientNum, pmove_t *pmove ) {
	playerState_t *ps = &level.clients[clientNum].ps;

	MODFN_NEXT( PmoveInit, ( MODFN_NC, clientNum, pmove ) );

	if ( MOD_STATE && ps->weapon >= 1 && ps->weapon < WP_NUM_WEAPONS ) {
		char mode = MOD_STATE->modeString[ps->weapon - 1];
		if ( mode == 'f' ) {
			pmove->altFireMode = ALTMODE_PRIMARY_ONLY;
		} else if ( mode == 'F' ) {
			pmove->altFireMode = ALTMODE_ALT_ONLY;
		}
	}
}

/*
================
ModAltFireConfig_Init
================
*/
void ModAltFireConfig_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		ModModcfgCS_Init();

		MODFN_REGISTER( AddModConfigInfo, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PmoveInit, MODPRIORITY_GENERAL + 1 );	// override alt fire swap module
	}
}
