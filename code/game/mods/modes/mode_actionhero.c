/*
* Action Hero Mode
* 
* In this FFA variant, one player is selected at random to be the "Action Hero", which
* comes with significant health and weapon bonuses. They remain the Action Hero until
* they are killed, at which point the player who killed them becomes the new Action Hero.
*/

#define MOD_NAME ModActionHero

#include "mods/g_mod_local.h"

static struct {
	int actionHeroClientNum;
} *MOD_STATE;

/*
================
ModActionhero_SetHero
================
*/
static void ModActionhero_SetHero( int clientNum ) {
	if ( clientNum == MOD_STATE->actionHeroClientNum ) {
		return;
	}

	if ( MOD_STATE->actionHeroClientNum != -1 ) {
		// In case there is an existing hero, reset it
		int oldHero = MOD_STATE->actionHeroClientNum;
		MOD_STATE->actionHeroClientNum = -1;
		if ( level.clients[oldHero].pers.connected == CON_CONNECTED ) {
			ClientSpawn( &g_entities[oldHero], CST_RESPAWN );
		}
	}

	if ( clientNum != -1 && G_AssertConnectedClient( clientNum ) ) {
		// Set new hero
		MOD_STATE->actionHeroClientNum = clientNum;
		ClientSpawn( &g_entities[clientNum], CST_RESPAWN );
	}
}

/*
================
ModActionHero_CheckReplaceHero

Checks if current action hero is valid, and picks new one if necessary.
================
*/
static void ModActionHero_CheckReplaceHero( int exclude_client ) {
	// Check if existing hero is invalid.
	// Shouldn't normally happen, as hero exits should be processed immediately.
	if ( MOD_STATE->actionHeroClientNum != -1 ) {
		const gclient_t *client = &level.clients[MOD_STATE->actionHeroClientNum];
		if ( client->pers.connected != CON_CONNECTED || client->ps.pm_type == PM_DEAD || MOD_STATE->actionHeroClientNum == exclude_client ) {
			G_DedPrintf( "action hero: WARNING - ModActionHero_CheckReplaceHero - Unexpected hero exit\n" );
			MOD_STATE->actionHeroClientNum = -1;
		}
	}

	// Wait until at least 10 seconds into map and 3 seconds into game to select hero.
	if ( MOD_STATE->actionHeroClientNum == -1 && level.matchState < MS_INTERMISSION_QUEUED &&
			!level.warmupTime && level.time >= 10000 && level.time - level.startTime >= 3000 ) {
		int candidates[MAX_CLIENTS];
		int count = 0;
		int i;

		if ( g_gametype.integer >= GT_TEAM && exclude_client >= 0 ) {
			// try to pick from the team opposite the action hero that just died/quit
			team_t oldTeam = level.clients[exclude_client].sess.sessionTeam;
			for ( i = 0; i < level.maxclients; i++ ) {
				gclient_t *client = &level.clients[i];
				if ( client->pers.connected == CON_CONNECTED && !modfn.SpectatorClient( i ) &&
						level.clients[i].sess.sessionTeam != oldTeam ) {
					candidates[count++] = i;
				}
			}
		}

		if ( count <= 0 ) {
			// try to pick any player
			for ( i = 0; i < level.maxclients; i++ ) {
				gclient_t *client = &level.clients[i];
				if ( client->pers.connected == CON_CONNECTED && !modfn.SpectatorClient( i ) && i != exclude_client ) {
					candidates[count++] = i;
				}
			}
		}

		if ( count > 0 ) {
			ModActionhero_SetHero( candidates[irandom( 0, count - 1 )] );
		}
	}
}

/*
================
(ModFN) PostPlayerDie

Select a new action hero if the existing one was killed.
================
*/
static void MOD_PREFIX(PostPlayerDie)( MODFN_CTV, gentity_t *self, gentity_t *inflictor, gentity_t *attacker,
		int meansOfDeath, int *awardPoints ) {
	int clientNum = self - g_entities;
	gclient_t *client = &level.clients[clientNum];

	MODFN_NEXT( PostPlayerDie, ( MODFN_NC, self, inflictor, attacker, meansOfDeath, awardPoints ) );

	if ( clientNum == MOD_STATE->actionHeroClientNum ) {
		// Make me no longer a hero... *sniff*...
		MOD_STATE->actionHeroClientNum = -1;
		self->client->ps.persistant[PERS_CLASS] = self->client->sess.sessionClass = PC_NOCLASS;
		ClientUserinfoChanged( clientNum );

		if ( attacker && attacker->client && attacker != self && !OnSameTeam( attacker, self ) ) {
			// killer of action hero becomes action hero
			ModActionhero_SetHero( attacker - g_entities );

			if ( *awardPoints == 1 ) {
				// killing action hero gives 5 points instead of the normal +1
				*awardPoints = 5;
			}
		} else {
			// other kind of hero death picks a random action hero
			ModActionHero_CheckReplaceHero( clientNum );
		}
	}
}

/*
================
(ModFN) PrePlayerLeaveTeam

Check for action hero disconnecting or changing teams.
================
*/
static void MOD_PREFIX(PrePlayerLeaveTeam)( MODFN_CTV, int clientNum, team_t oldTeam ) {
	MODFN_NEXT( PrePlayerLeaveTeam, ( MODFN_NC, clientNum, oldTeam ) );

	if ( clientNum == MOD_STATE->actionHeroClientNum ) {
		MOD_STATE->actionHeroClientNum = -1;
		ModActionHero_CheckReplaceHero( clientNum );
	}
}

/*
================
(ModFN) UpdateSessionClass

Checks if current player class is valid, and picks a new one if necessary.
================
*/
static void MOD_PREFIX(UpdateSessionClass)( MODFN_CTV, int clientNum ) {
	gclient_t *client = &level.clients[clientNum];

	if ( clientNum == MOD_STATE->actionHeroClientNum ) {
		// Set action hero to action hero class
		client->sess.sessionClass = PC_ACTIONHERO;
	} else {
		// Validate class normally for others
		// If PC_ACTIONHERO is set it should be cleared here
		MODFN_NEXT( UpdateSessionClass, ( MODFN_NC, clientNum ) );
	}
}

/*
================
(ModFN) EffectiveHandicap

Force 1.5x "handicap" for action hero, consistent with original behavior.
================
*/
static int MOD_PREFIX(EffectiveHandicap)( MODFN_CTV, int clientNum, effectiveHandicapType_t type ) {
	gclient_t *client = &level.clients[clientNum];
	pclass_t pclass = client->sess.sessionClass;

	if ( pclass == PC_ACTIONHERO ) {
		if ( type == EH_DAMAGE || type == EH_MAXHEALTH ) {
			return 150;
		}
		return 100;
	}

	return MODFN_NEXT( EffectiveHandicap, ( MODFN_NC, clientNum, type ) );
}

/*
================
(ModFN) SpawnConfigureClient
================
*/
static void MOD_PREFIX(SpawnConfigureClient)( MODFN_CTV, int clientNum ) {
	gentity_t *ent = &g_entities[clientNum];
	gclient_t *client = &level.clients[clientNum];

	MODFN_NEXT( SpawnConfigureClient, ( MODFN_NC, clientNum ) );

	if ( client->sess.sessionClass == PC_ACTIONHERO ) {
		client->ps.stats[STAT_WEAPONS] = ( 1 << WP_PHASER );
		client->ps.ammo[WP_PHASER] = PHASER_AMMO_MAX;
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_COMPRESSION_RIFLE );
		client->ps.ammo[WP_COMPRESSION_RIFLE] = Max_Ammo[WP_COMPRESSION_RIFLE];
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_IMOD );
		client->ps.ammo[WP_IMOD] = Max_Ammo[WP_IMOD];
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_SCAVENGER_RIFLE );
		client->ps.ammo[WP_SCAVENGER_RIFLE] = Max_Ammo[WP_SCAVENGER_RIFLE];
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_STASIS );
		client->ps.ammo[WP_STASIS] = Max_Ammo[WP_STASIS];
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_GRENADE_LAUNCHER );
		client->ps.ammo[WP_GRENADE_LAUNCHER] = Max_Ammo[WP_GRENADE_LAUNCHER];
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_TETRION_DISRUPTOR );
		client->ps.ammo[WP_TETRION_DISRUPTOR] = Max_Ammo[WP_TETRION_DISRUPTOR];
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_QUANTUM_BURST );
		client->ps.ammo[WP_QUANTUM_BURST] = Max_Ammo[WP_QUANTUM_BURST];
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_DREADNOUGHT );
		client->ps.ammo[WP_DREADNOUGHT] = Max_Ammo[WP_DREADNOUGHT];

		ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH];
		client->ps.stats[STAT_ARMOR] = 100;

		client->ps.powerups[PW_REGEN] = level.time - ( level.time % 1000 );
		client->ps.powerups[PW_REGEN] += 1800 * 1000;
	}
}

/*
===========
(ModFN) SpawnCenterPrintMessage

Prints info messages to client during ClientSpawn.
============
*/
static void MOD_PREFIX(SpawnCenterPrintMessage)( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ) {
	gclient_t *client = &level.clients[clientNum];

	if ( client->sess.sessionClass == PC_ACTIONHERO ) {
		trap_SendServerCommand( clientNum, "cp \"You are the Action Hero!\"" );
	}

	else if ( client->sess.sessionTeam != TEAM_SPECTATOR && spawnType != CST_RESPAWN &&
			MOD_STATE->actionHeroClientNum != -1 && G_AssertConnectedClient( MOD_STATE->actionHeroClientNum ) ) {
		gclient_t *ah = &level.clients[MOD_STATE->actionHeroClientNum];
		trap_SendServerCommand( clientNum, va( "cp \"Action Hero is %s!\"", ah->pers.netname ) );
	}

	else {
		MODFN_NEXT( SpawnCenterPrintMessage, ( MODFN_NC, clientNum, spawnType ) );
	}
}

/*
===========
(ModFN) PostClientSpawn

Notify other clients when action hero is spawning.
============
*/
static void MOD_PREFIX(PostClientSpawn)( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ) {
	gclient_t *client = &level.clients[clientNum];

	MODFN_NEXT( PostClientSpawn, ( MODFN_NC, clientNum, spawnType ) );

	if ( client->sess.sessionClass == PC_ACTIONHERO ) {
		int i;
		for ( i = 0; i < level.maxclients; ++i ) {
			gclient_t *msg_client = &level.clients[i];
			if ( msg_client->pers.connected == CON_CONNECTED && i != clientNum ) {
				trap_SendServerCommand( i, va( "cp \"%.15s" S_COLOR_WHITE " is the Action Hero!\n\"", client->pers.netname ) );
			}
		}
	}
}

/*
============
(ModFN) CanItemBeDropped

Action hero doesn't drop items.
============
*/
static qboolean MOD_PREFIX(CanItemBeDropped)( MODFN_CTV, gitem_t *item, int clientNum ) {
	gclient_t *client = &level.clients[clientNum];

	if ( client->sess.sessionClass == PC_ACTIONHERO && item->giType != IT_TEAM ) {
		return qfalse;
	}

	return MODFN_NEXT( CanItemBeDropped, ( MODFN_NC, item, clientNum ) );
}

/*
================
(ModFN) AddRegisteredItems
================
*/
static void MOD_PREFIX(AddRegisteredItems)( MODFN_CTV ) {
	MODFN_NEXT( AddRegisteredItems, ( MODFN_NC ) );

	RegisterItem( BG_FindItemForWeapon( WP_IMOD ) );
	RegisterItem( BG_FindItemForWeapon( WP_SCAVENGER_RIFLE ) );
	RegisterItem( BG_FindItemForWeapon( WP_STASIS ) );
	RegisterItem( BG_FindItemForWeapon( WP_GRENADE_LAUNCHER ) );
	RegisterItem( BG_FindItemForWeapon( WP_TETRION_DISRUPTOR ) );
	RegisterItem( BG_FindItemForWeapon( WP_QUANTUM_BURST ) );
	RegisterItem( BG_FindItemForWeapon( WP_DREADNOUGHT ) );
}

/*
================
(ModFN) PostRunFrame
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );
	ModActionHero_CheckReplaceHero( -1 );
}

/*
================
ModActionHero_Init
================
*/
void ModActionHero_Init( void ) {
	if ( EF_WARN_ASSERT( !MOD_STATE ) ) {
		modcfg.mods_enabled.actionhero = qtrue;
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MOD_STATE->actionHeroClientNum = -1;

		// Register mod functions
		MODFN_REGISTER( UpdateSessionClass, ++modePriorityLevel );
		MODFN_REGISTER( EffectiveHandicap, ++modePriorityLevel );
		MODFN_REGISTER( SpawnConfigureClient, ++modePriorityLevel );
		MODFN_REGISTER( SpawnCenterPrintMessage, ++modePriorityLevel );
		MODFN_REGISTER( PostClientSpawn, ++modePriorityLevel );
		MODFN_REGISTER( PostPlayerDie, ++modePriorityLevel );
		MODFN_REGISTER( PrePlayerLeaveTeam, ++modePriorityLevel );
		MODFN_REGISTER( CanItemBeDropped, ++modePriorityLevel );
		MODFN_REGISTER( AddRegisteredItems, ++modePriorityLevel );
		MODFN_REGISTER( PostRunFrame, ++modePriorityLevel );
	}
}
