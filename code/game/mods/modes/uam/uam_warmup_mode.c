/*
* UAM Warmup Mode
* 
* This module enables the core warmup behavior based on the Gladiator mod, in which players
* spawn with a hypo and jetpack and can move around the map.
* 
* This module does not change the length of the warmup or display any sequence of messages.
* That part of the warmup is handled by the uam_warmup_sequence module.
*/

#define MOD_NAME ModUAMWarmupMode

#include "mods/modes/uam/uam_local.h"

static struct {
	// Warmup mode is set to active in PostClientConnect, and remains active for the rest of
	// the session (until map restart/change).
	int active;
} *MOD_STATE;

#define IS_DETPACK( ent ) ( ( ent ) && ( ent )->inuse && ( ent )->s.eType == ET_USEABLE && ( ent )->s.modelindex == HI_DETPACK )

/*
================
(ModFN) AltFireConfig

Use standard fire default for hypo.
================
*/
static char MOD_PREFIX(AltFireConfig)( MODFN_CTV, weapon_t weapon ) {
	if ( weapon == WP_VOYAGER_HYPO ) {
		return 'n';
	}
	return MODFN_NEXT( AltFireConfig, ( MODFN_NC, weapon ) );
}

/*
===========
(ModFN) SetClientGhosting

Don't activate ghosting during warmup.
============
*/
static void MOD_PREFIX(SetClientGhosting)( MODFN_CTV, int clientNum, qboolean active ) {
	if ( MOD_STATE->active && active ) {
		return;
	}
	MODFN_NEXT( SetClientGhosting, ( MODFN_NC, clientNum, active ) );
}

/*
================
(ModFN) SpawnConfigureClient

Start with hypo, jetpack, and (optionally) detpack during warmup.
================
*/
static void MOD_PREFIX(SpawnConfigureClient)( MODFN_CTV, int clientNum ) {
	MODFN_NEXT( SpawnConfigureClient, ( MODFN_NC, clientNum ) );

	if ( MOD_STATE->active && !modfn.SpectatorClient( clientNum ) ) {
		gclient_t *client = &level.clients[clientNum];

		client->ps.stats[STAT_WEAPONS] = ( 1 << WP_VOYAGER_HYPO );
		client->ps.ammo[WP_VOYAGER_HYPO] = 100;
		client->ps.ammo[WP_PHASER] = 0;

		if ( ModUAMWarmupSequence_Shared_DetpackEnabled() ) {
			client->ps.stats[STAT_HOLDABLE_ITEM] = BG_FindItemForHoldable( HI_DETPACK ) - bg_itemlist;
		}

		client->ps.powerups[PW_FLIGHT] = level.time + 100000;
	}
}

/*
==================
(ModFN) AdjustGeneralConstant

Disable detpack knockback during warmup.
==================
*/
static int MOD_PREFIX(AdjustGeneralConstant)( MODFN_CTV, generalConstant_t gcType, int defaultValue ) {
	if ( gcType == GC_DETPACK_NO_SHOCKWAVE && MOD_STATE->active ) {
		return 1;
	}

	return MODFN_NEXT( AdjustGeneralConstant, ( MODFN_NC, gcType, defaultValue ) );
}

/*
============
(ModFN) ModifyDamageFlags

Disable most damage during warmup.
============
*/
static int MOD_PREFIX(ModifyDamageFlags)( MODFN_CTV, gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	dflags = MODFN_NEXT( ModifyDamageFlags, ( MODFN_NC, targ, inflictor, attacker, dir, point, damage, dflags, mod ) );

	// Allow detpacks to damage world elements, but not players or other detpacks.
	if ( MOD_STATE->active && ( mod != MOD_DETPACK || targ->client || IS_DETPACK( targ ) ) ) {
		dflags |= DAMAGE_TRIGGERS_ONLY;
	}

	return dflags;
}

/*
================
(ModFN) CanItemBeGrabbed

Disable item pickups during warmup.
================
*/
static qboolean MOD_PREFIX(CanItemBeGrabbed)( MODFN_CTV, gentity_t *item, int clientNum ) {
	if ( MOD_STATE->active ) {
		return qfalse;
	}

	return MODFN_NEXT( CanItemBeGrabbed, ( MODFN_NC, item, clientNum ) );
}

/*
============
(ModFN) CanItemBeDropped

Disable item drops during warmup.
============
*/
static qboolean MOD_PREFIX(CanItemBeDropped)( MODFN_CTV, gitem_t *item, int clientNum ) {
	if ( MOD_STATE->active ) {
		return qfalse;
	}

	return MODFN_NEXT( CanItemBeDropped, ( MODFN_NC, item, clientNum ) );
}

/*
=================
(ModFN) CheckSuicideAllowed

Disable suicide during warmup. This might already be done by elimination mod, but this check
is stricter because it blocks suicide during pre-warmup, not just during countdown.
=================
*/
static qboolean MOD_PREFIX(CheckSuicideAllowed)( MODFN_CTV, int clientNum ) {
	if ( MOD_STATE->active ) {
		return qfalse;
	}

	return MODFN_NEXT( CheckSuicideAllowed, ( MODFN_NC, clientNum ) );
}

/*
==============
(ModFN) RunPlayerMove

Disable actually firing the hypo during warmup.
==============
*/
static void MOD_PREFIX(RunPlayerMove)( MODFN_CTV, int clientNum, qboolean spectator ) {
	if ( MOD_STATE->active && !spectator ) {
		gclient_t *client = &level.clients[clientNum];
		int respawnedFlag = client->ps.pm_flags & PMF_RESPAWNED;
		int oldButtons = client->pers.cmd.buttons;

		// Check for clearing PMF_RESPAWNED here because clearing buttons below breaks the normal handling
		if ( respawnedFlag && client->ps.stats[STAT_HEALTH] > 0 &&
				!( client->pers.cmd.buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE) ) ) {
			respawnedFlag = 0;
		}

		// Run move with fire buttons cleared, so hypo doesn't actually fire
		client->pers.cmd.buttons &= ~( BUTTON_ATTACK | BUTTON_ALT_ATTACK);
		MODFN_NEXT( RunPlayerMove, ( MODFN_NC, clientNum, spectator ) );

		client->pers.cmd.buttons = oldButtons;
		client->ps.pm_flags |= respawnedFlag;
		client->ps.ammo[WP_PHASER] = 0;		// disables alt attack prediction
		return;
	}

	MODFN_NEXT( RunPlayerMove, ( MODFN_NC, clientNum, spectator ) );
}

/*
================
(ModFN) PostRunFrame
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );

	if ( MOD_STATE->active && level.matchState != MS_WARMUP ) {
		// If in warmup mode, but not actively counting down, keep the flight powerup timer at max
		int i;
		for ( i = 0; i < level.maxclients; ++i ) {
			if ( level.clients[i].pers.connected >= CON_CONNECTING && !modfn.SpectatorClient( i ) ) {
				level.clients[i].ps.powerups[PW_FLIGHT] = level.time + 100000;
			}
		}
	}

	if ( level.matchState >= MS_INTERMISSION_QUEUED && level.numConnectedClients < 2 && !level.exiting ) {
		// Similar to the MatchStateTransition check, but applies to intermission,
		// for consistency with original gladiator mod
		G_Printf( "uam: Restart due to insufficient players (intermission).\n" );
		level.exiting = qtrue;
		trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
		return;
	}
}

/*
================
(ModFN) MatchStateTransition
================
*/
static void MOD_PREFIX(MatchStateTransition)( MODFN_CTV, matchState_t oldState, matchState_t newState ) {
	if ( oldState >= MS_ACTIVE && newState < MS_ACTIVE && modfn.WarmupLength() > 0 ) {
		// Always restart if dropping back to warmup mode
		G_Printf( "uam: Restart due to insufficient players.\n" );
		level.exiting = qtrue;
		trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
		return;
	}

	MODFN_NEXT( MatchStateTransition, ( MODFN_NC, oldState, newState ) );
}

/*
================
(ModFN) PostClientConnect
================
*/
static void MOD_PREFIX(PostClientConnect)( MODFN_CTV, int clientNum, qboolean firstTime, qboolean isBot ) {
	if ( !MOD_STATE->active && modfn.WarmupLength() > 0 && level.matchState < MS_ACTIVE ) {
		int i;

		// Set warmup mode to active
		MOD_STATE->active = qtrue;

		// Disable turrets during warmup
		for ( i = 0; i < MAX_GENTITIES; ++i ) {
			gentity_t *ent = &g_entities[i];
			if ( ent->inuse && ent->classname && !Q_stricmp( ent->classname, "misc_turret" ) ) {
				ent->nextthink = 0;
			}
		}
	}

	MODFN_NEXT( PostClientConnect, ( MODFN_NC, clientNum, firstTime, isBot ) );
}

/*
================
(ModFN) AddRegisteredItems
================
*/
static void MOD_PREFIX(AddRegisteredItems)( MODFN_CTV ) {
	MODFN_NEXT( AddRegisteredItems, ( MODFN_NC ) );

	RegisterItem( BG_FindItemForPowerup( PW_FLIGHT ) );
	RegisterItem( BG_FindItemForHoldable( HI_DETPACK ) );
	RegisterItem( BG_FindItemForWeapon( WP_VOYAGER_HYPO ) );
}

/*
================
ModUAMWarmupMode_Init
================
*/
void ModUAMWarmupMode_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		ModAltFireConfig_Init();

		// Use high priority to make sure warmup behavior can override any mod functionality
		// meant for the regular game.
		MODFN_REGISTER( AltFireConfig, ++modePriorityLevel );
		MODFN_REGISTER( SetClientGhosting, MODPRIORITY_HIGH );
		MODFN_REGISTER( SpawnConfigureClient, MODPRIORITY_HIGH );
		MODFN_REGISTER( AdjustGeneralConstant, MODPRIORITY_HIGH );
		MODFN_REGISTER( ModifyDamageFlags, MODPRIORITY_HIGH );
		MODFN_REGISTER( CanItemBeGrabbed, MODPRIORITY_HIGH );
		MODFN_REGISTER( CanItemBeDropped, MODPRIORITY_HIGH );
		MODFN_REGISTER( CheckSuicideAllowed, MODPRIORITY_HIGH );
		MODFN_REGISTER( RunPlayerMove, MODPRIORITY_HIGH );
		MODFN_REGISTER( PostRunFrame, MODPRIORITY_HIGH );
		MODFN_REGISTER( MatchStateTransition, MODPRIORITY_HIGH );
		MODFN_REGISTER( PostClientConnect, MODPRIORITY_HIGH );
		MODFN_REGISTER( AddRegisteredItems, ++modePriorityLevel );
	}
}
