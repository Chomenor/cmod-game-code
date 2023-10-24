/*
* Razor Mode
* 
* "Razor Arena" is a recreation of Pinball mod by Lt. Cmdr. Salinga, in which players
* use knockback to push opponents into the edge of the map to get kills.
* 
* Enabled by setting "g_mod_razor" to 1.
*/

#define MOD_NAME ModRazor

#include "mods/modes/razor/razor_local.h"

static struct {
	int _unused;
} *MOD_STATE;

/*
==================
(ModFN) AdjustGeneralConstant
==================
*/
static int MOD_PREFIX(AdjustGeneralConstant)( MODFN_CTV, generalConstant_t gcType, int defaultValue ) {
	switch ( gcType ) {
		case GC_TOSS_ITEMS_ON_SUICIDE:
			return 1;
	}

	return MODFN_NEXT( AdjustGeneralConstant, ( MODFN_NC, gcType, defaultValue ) );
}

/*
==============
(ModFN) RunPlayerMove
==============
*/
static void MOD_PREFIX(RunPlayerMove)( MODFN_CTV, int clientNum, qboolean spectator ) {
	gclient_t *client = &level.clients[clientNum];

	// Override the regular speed calculation in ClientThink_real.
	client->ps.speed = tonextint( g_speed.value );
	client->ps.speed = tonextint( client->ps.speed * 1.6f );
	if ( client->ps.powerups[PW_FLIGHT] ) {
		client->ps.speed = tonextint( client->ps.speed * 0.9f );
	}

	MODFN_NEXT( RunPlayerMove, ( MODFN_NC, clientNum, spectator ) );
}

/*
============
(ModFN) ModifyDamageFlags
============
*/
static int MOD_PREFIX(ModifyDamageFlags)( MODFN_CTV, gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	dflags = MODFN_NEXT( ModifyDamageFlags, ( MODFN_NC, targ, inflictor, attacker, dir, point, damage, dflags, mod ) );

	// Disable falling damage.
	if ( mod == MOD_FALLING ) {
		dflags |= DAMAGE_TRIGGERS_ONLY;
	}

	// Emulate some sliding behavior in Pinball when touching forcefield or wall with gold armor.
	if ( damage >= 1000 && targ->client && !targ->client->ps.pm_time ) {
		targ->client->ps.pm_time = 200;
		targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	}

	return dflags;
}

/*
============
(ModFN) CanItemBeDropped
============
*/
static qboolean MOD_PREFIX(CanItemBeDropped)( MODFN_CTV, gitem_t *item, int clientNum ) {
	// Enable all powerup and holdable drops in both teams and ffa.
	if ( item->giType == IT_POWERUP || item->giType == IT_HOLDABLE ) {
		return qtrue;
	}

	// Return rather than drop flags on death (this is a bit hacky).
	if ( item->giType == IT_TEAM && level.clients[clientNum].ps.pm_type == PM_DEAD ) {
		if ( item->giTag == PW_REDFLAG ) {
			Team_ReturnFlag( TEAM_RED );
			return qfalse;
		} else if ( item->giTag == PW_BLUEFLAG ) {
			Team_ReturnFlag( TEAM_BLUE );
			return qfalse;
		}
	}

	return MODFN_NEXT( CanItemBeDropped, ( MODFN_NC, item, clientNum ) );
}

/*
============
(ModFN) DroppedItemExpirationTime

Dropped non-weapon items last forever.
============
*/
static int MOD_PREFIX(ItemDropExpireTime)( MODFN_CTV, const gitem_t *item ) {
	if ( item->giType != IT_WEAPON ) {
		return 0;
	}

	return MODFN_NEXT( ItemDropExpireTime, ( MODFN_NC, item ) );
}

/*
================
(ModFN) IntermissionReadyConfig

Play beep sound when players toggle ready in intermission.
================
*/
static void MOD_PREFIX(IntermissionReadyConfig)( MODFN_CTV, modIntermissionReady_config_t *config ) {
	MODFN_NEXT( IntermissionReadyConfig, ( MODFN_NC, config ) );
	config->readySound = qtrue;
}

/*
================
ModRazor_Init
================
*/
void ModRazor_Init( void ) {
	if ( !MOD_STATE ) {
		modcfg.mods_enabled.razor = qtrue;
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		if ( G_ModUtils_GetLatchedValue( "g_pModDisintegration", "0", 0 ) ) {
			// Set disintegration to active, but don't actually load the real disintegration
			// module since Razor uses its own variant.
			modcfg.mods_enabled.disintegration = qtrue;
		}

		if ( G_ModUtils_GetLatchedValue( "g_pModElimination", "0", 0 ) ) {
			ModElimination_Init();
		}

		ModRazorDamage_Init();
		ModRazorDifficulty_Init();
		ModRazorGreeting_Init();
		ModRazorItems_Init();
		ModRazorPowerups_Init();
		ModRazorScoring_Init();
		ModRazorSounds_Init();
		ModRazorWeapons_Init();
		ModIntermissionReady_Init();

		// Set mod name for server browsers
		trap_Cvar_Set( "gamename", "Razor" );

		MODFN_REGISTER( AdjustGeneralConstant, ++modePriorityLevel );
		MODFN_REGISTER( RunPlayerMove, ++modePriorityLevel );
		MODFN_REGISTER( ModifyDamageFlags, ++modePriorityLevel );
		MODFN_REGISTER( CanItemBeDropped, ++modePriorityLevel );
		MODFN_REGISTER( ItemDropExpireTime, ++modePriorityLevel );
		MODFN_REGISTER( IntermissionReadyConfig, ++modePriorityLevel );

		ModIntermissionReady_Shared_UpdateConfig();
	}
}
