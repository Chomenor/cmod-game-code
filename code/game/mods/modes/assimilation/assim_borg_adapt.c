/*
* Assimilation Borg Adaptation
* 
* After a certain amount of hits from the same weapon, borg "adapt" to the
* weapon and develop shielding to it.
*/

#define MOD_NAME ModAssimBorgAdapt

#include "mods/modes/assimilation/assim_local.h"

#define BORG_ADAPT_NUM_HITS 10

static struct {
	// For global shield adaptation
	int hits[WP_NUM_WEAPONS];
} *MOD_STATE;

/*
================
ModAssimBorgAdapt_AdaptWeaponType

Convert damage MoD value to weapon type for borg adaptation. Returns WP_NONE for non-adaptable damage.
================
*/
static weapon_t ModAssimBorgAdapt_AdaptWeaponType( meansOfDeath_t mod ) {
	switch ( mod ) {
		case MOD_PHASER:
		case MOD_PHASER_ALT:
			return WP_PHASER;
		case MOD_CRIFLE:
		case MOD_CRIFLE_SPLASH:
		case MOD_CRIFLE_ALT:
		case MOD_CRIFLE_ALT_SPLASH:
			return WP_COMPRESSION_RIFLE;
		case MOD_SCAVENGER:
		case MOD_SCAVENGER_ALT:
		case MOD_SCAVENGER_ALT_SPLASH:
		case MOD_SEEKER:
			return WP_SCAVENGER_RIFLE;
		case MOD_STASIS:
		case MOD_STASIS_ALT:
			return WP_STASIS;
		case MOD_GRENADE:
		case MOD_GRENADE_ALT:
		case MOD_GRENADE_SPLASH:
		case MOD_GRENADE_ALT_SPLASH:
			return WP_GRENADE_LAUNCHER;
		case MOD_TETRION:
		case MOD_TETRION_ALT:
			return WP_TETRION_DISRUPTOR;
		case MOD_DREADNOUGHT:
		case MOD_DREADNOUGHT_ALT:
			return WP_DREADNOUGHT;
		case MOD_QUANTUM:
		case MOD_QUANTUM_SPLASH:
		case MOD_QUANTUM_ALT:
		case MOD_QUANTUM_ALT_SPLASH:
			return WP_QUANTUM_BURST;
		default:
			break;
	}

	return WP_NONE;
}

/*
================
ModAssimBorgAdapt_CheckAdapted
================
*/
static qboolean ModAssimBorgAdapt_CheckAdapted( int clientNum, int mod, int damage ) {
	gclient_t *client = &level.clients[clientNum];

	if ( !EF_WARN_ASSERT( mod != MOD_ASSIMILATE ) ) {
		// shouldn't happen since only non-borg should be hit in WP_Assimilate
		return qfalse;
	}

	if ( mod == MOD_BORG || mod == MOD_BORG_ALT ) {
		// always adapted to borg weapons
		client->ps.powerups[PW_BORG_ADAPT] = level.time + 250;
	}

	else {
		weapon_t weaponType = ModAssimBorgAdapt_AdaptWeaponType( mod );
		if ( weaponType != WP_NONE ) {
			MOD_STATE->hits[weaponType]++;
			if ( MOD_STATE->hits[weaponType] > BORG_ADAPT_NUM_HITS ) {
				client->ps.powerups[PW_BORG_ADAPT] = level.time + 250;
			}
		}
	}

	// For consistency with original behavior, block if any adaptation was recently
	// triggered, even if it wasn't from this attack.
	if ( client->ps.powerups[PW_BORG_ADAPT] ) {
		return qtrue;
	}
	return qfalse;
}

/*
================
(ModFN) CheckBorgAdapt

Check for borg shield adaptation.
================
*/
static qboolean MOD_PREFIX(CheckBorgAdapt)( MODFN_CTV, gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	if( targ->client && targ->client->sess.sessionClass == PC_BORG ) {
		int clientNum = targ - g_entities;
		return ModAssimBorgAdapt_CheckAdapted( clientNum, mod, damage );
	}

	return qfalse;
}

/*
================
ModAssimBorgAdapt_Init
================
*/
void ModAssimBorgAdapt_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( CheckBorgAdapt, ++modePriorityLevel );
	}
}
