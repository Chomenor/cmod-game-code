/*
* Fire Rate Configstring
* 
* Adds weapon fire rates to the modcfg configstring for client prediction purposes.
* 
* Fire rates are pulled from the ModifyFireRate mod function. This module should generally
* be initialized by any module that implements ModifyFireRate. ModModcfgCS_Static_Update
* should be called any time values might have changed, to ensure configstring is updated.
*/

#define MOD_NAME ModFireRateCS

#include "mods/g_mod_local.h"

static struct {
	int _unused;
} *MOD_STATE;

// Fire rate defaults copied from PM_Weapon function.
static const struct {
	int primaryDelay;
	int altDelay;
} fireRateDefaults[] = {
	{ 0, 0 },		// WP_NONE
	{ 100, 100 },	// WP_PHASER
	{ 250, 1200 },	// WP_COMPRESSION_RIFLE
	{ 350, 700 },	// WP_IMOD
	{ 100, 700 },	// WP_SCAVENGER_RIFLE
	{ 700, 700 },	// WP_STASIS
	{ 700, 700 },	// WP_GRENADE_LAUNCHER
	{ 100, 200 },	// WP_TETRION_DISRUPTOR
	{ 1200, 1600 },	// WP_QUANTUM_BURST
	{ 100, 500 },	// WP_DREADNOUGHT
	{ 100, 1000 },	// WP_VOYAGER_HYPO
	{ 100, 100 },	// WP_BORG_ASSIMILATOR
	{ 500, 300 },	// WP_BORG_WEAPON
};

/*
==============
ModFireRateCS_GetValueForWeapon

Returns fire interval value to set in configstring. Returns 0 for no value (client uses default).
==============
*/
static int ModFireRateCS_GetValueForWeapon( int weapon, qboolean alt ) {
	if ( weapon == WP_PHASER || ( weapon == WP_DREADNOUGHT && !alt ) ) {
		// Client doesn't need to predict rate for beam weapons
		return 0;
	}

	{
		int defaultValue = alt ? fireRateDefaults[weapon].altDelay : fireRateDefaults[weapon].primaryDelay;
		int modValue = modfn.ModifyFireRate( defaultValue, weapon, alt );

		if ( modValue == defaultValue ) {
			// Skip if default is correct
			return 0;
		}

		if ( modValue >= 350 && defaultValue >= 350 ) {
			// If both values are too long to matter for prediction, since predictions will be corrected
			// within client ping time, skip to save bytes in configstring.
			return 0;
		}

		return modValue;
	}
}

/*
==============
ModFireRateCS_GetWeaponChar

Returns character that identifies weapon in configstring.
==============
*/
static char ModFireRateCS_GetWeaponChar( int weapon ) {
	if ( weapon >= WP_PHASER && weapon <= WP_DREADNOUGHT ) {
		return '1' + ( weapon - WP_PHASER );
	} else if ( weapon == WP_VOYAGER_HYPO ) {
		return 'h';
	} else if ( weapon == WP_BORG_ASSIMILATOR ) {
		return 'a';
	} else if ( weapon == WP_BORG_WEAPON ) {
		return 'b';
	}
	
	// shouldn't happen
	G_Printf( "ModFireRateCS_GetWeaponChar: Invalid weapon\n" );
	return 'x';
}

/*
==============
ModFireRateCS_AddInfoForWeapon
==============
*/
static void ModFireRateCS_AddInfoForWeapon( int weapon, qboolean alt, char *buffer, int bufSize ) {
	int value = ModFireRateCS_GetValueForWeapon( weapon, alt );
	if ( value > 0 ) {
		char wepString[32];
		Com_sprintf( wepString, sizeof( wepString ), " %c%c:%i", ModFireRateCS_GetWeaponChar( weapon ), alt ? 'a' : 'p', value );
		Q_strcat( buffer, bufSize, wepString );
	}
}

/*
==============
ModFireRateCS_AddInfo
==============
*/
static void ModFireRateCS_AddInfo( char *info ) {
	int weapon;
	char buffer[512];
	buffer[0] = '\0';

	for ( weapon = WP_PHASER; weapon <= WP_BORG_WEAPON; ++weapon ) {
		ModFireRateCS_AddInfoForWeapon( weapon, qfalse, buffer, sizeof( buffer ) );
		ModFireRateCS_AddInfoForWeapon( weapon, qtrue, buffer, sizeof( buffer ) );
	}

	if ( *buffer ) {
		// skip leading space
		Info_SetValueForKey( info, "fireRate", &buffer[1] );
	}
}

/*
==============
(ModFN) AddModConfigInfo
==============
*/
static void MOD_PREFIX(AddModConfigInfo)( MODFN_CTV, char *info ) {
	MODFN_NEXT( AddModConfigInfo, ( MODFN_NC, info ) );
	ModFireRateCS_AddInfo( info );
}

/*
================
ModFireRateCS_Init
================
*/
void ModFireRateCS_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( AddModConfigInfo, MODPRIORITY_GENERAL );

		ModModcfgCS_Init();
	}
}
