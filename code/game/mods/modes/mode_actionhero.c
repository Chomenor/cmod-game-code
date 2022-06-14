/*
* Action Hero Mode
* 
* In this FFA variant, one player is selected at random to be the "Action Hero", which
* comes with significant health and weapon bonuses. They remain the Action Hero until
* they are killed, at which point the player who killed them becomes the new Action Hero.
*/

#include "mods/g_mod_local.h"

#define PREFIX( x ) ModActionHero_##x
#define MOD_STATE PREFIX( state )

static struct {
	int actionHeroClientNum;

	// For mod function stacking
	ModFNType_UpdateSessionClass Prev_UpdateSessionClass;
	ModFNType_EffectiveHandicap Prev_EffectiveHandicap;
	ModFNType_SpawnConfigureClient Prev_SpawnConfigureClient;
	ModFNType_SpawnCenterPrintMessage Prev_SpawnCenterPrintMessage;
	ModFNType_PostClientSpawn Prev_PostClientSpawn;
	ModFNType_PostPlayerDie Prev_PostPlayerDie;
	ModFNType_PrePlayerLeaveTeam Prev_PrePlayerLeaveTeam;
	ModFNType_CanItemBeDropped Prev_CanItemBeDropped;
	ModFNType_AddRegisteredItems Prev_AddRegisteredItems;
	ModFNType_PostRunFrame Prev_PostRunFrame;
} *MOD_STATE;

/*
================
ModActionhero_SetHero
================
*/
LOGFUNCTION_SVOID( ModActionhero_SetHero, ( int clientNum ), ( clientNum ), "G_ACTIONHERO" ) {
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
LOGFUNCTION_SVOID( ModActionHero_CheckReplaceHero, ( int exclude_client ), ( exclude_client ), "" ) {
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

		for ( i = 0; i < level.maxclients; i++ ) {
			gclient_t *client = &level.clients[i];
			if ( client->pers.connected == CON_CONNECTED && !modfn.SpectatorClient( i ) && i != exclude_client ) {
				candidates[count++] = i;
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
LOGFUNCTION_SVOID( PREFIX(PostPlayerDie), ( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int meansOfDeath, int *awardPoints ),
		( self, inflictor, attacker, meansOfDeath, awardPoints ), "G_MODFN_POSTPLAYERDIE" ) {
	int clientNum = self - g_entities;
	gclient_t *client = &level.clients[clientNum];

	MOD_STATE->Prev_PostPlayerDie( self, inflictor, attacker, meansOfDeath, awardPoints );

	if ( clientNum == MOD_STATE->actionHeroClientNum ) {
		// Make me no longer a hero... *sniff*...
		MOD_STATE->actionHeroClientNum = -1;
		self->client->ps.persistant[PERS_CLASS] = self->client->sess.sessionClass = PC_NOCLASS;
		ClientUserinfoChanged( clientNum );

		if ( attacker && attacker->client && attacker != self ) {
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
LOGFUNCTION_SVOID( PREFIX(PrePlayerLeaveTeam), ( int clientNum, team_t oldTeam ),
		( clientNum, oldTeam ), "G_MODFN_PREPLAYERLEAVETEAM" ) {
	MOD_STATE->Prev_PrePlayerLeaveTeam( clientNum, oldTeam );

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
LOGFUNCTION_SVOID( PREFIX(UpdateSessionClass), ( int clientNum ),
		( clientNum ), "G_MODFN_UPDATESESSIONCLASS" ) {
	gclient_t *client = &level.clients[clientNum];

	if ( clientNum == MOD_STATE->actionHeroClientNum ) {
		// Set action hero to action hero class
		client->sess.sessionClass = PC_ACTIONHERO;
	} else {
		// Validate class normally for others
		// If PC_ACTIONHERO is set it should be cleared here
		MOD_STATE->Prev_UpdateSessionClass( clientNum );
	}
}

/*
================
(ModFN) EffectiveHandicap

Force 1.5x "handicap" for action hero, consistent with original behavior.
================
*/
LOGFUNCTION_SRET( int, PREFIX(EffectiveHandicap), ( int clientNum, effectiveHandicapType_t type ),
		( clientNum, type ), "G_MODFN_EFFECTIVEHANDICAP" ) {
	gclient_t *client = &level.clients[clientNum];
	pclass_t pclass = client->sess.sessionClass;

	if ( pclass == PC_ACTIONHERO ) {
		if ( type == EH_DAMAGE || type == EH_MAXHEALTH ) {
			return 150;
		}
		return 100;
	}

	return MOD_STATE->Prev_EffectiveHandicap( clientNum, type );
}

/*
================
(ModFN) SpawnConfigureClient
================
*/
LOGFUNCTION_SVOID( PREFIX(SpawnConfigureClient), ( int clientNum ), ( clientNum ), "G_MODFN_SPAWNCONFIGURECLIENT" ) {
	gentity_t *ent = &g_entities[clientNum];
	gclient_t *client = &level.clients[clientNum];

	MOD_STATE->Prev_SpawnConfigureClient( clientNum );

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
LOGFUNCTION_SVOID( PREFIX(SpawnCenterPrintMessage), ( int clientNum, clientSpawnType_t spawnType ),
		( clientNum, spawnType ), "G_MODFN_SPAWNCENTERPRINTMESSAGE" ) {
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
		MOD_STATE->Prev_SpawnCenterPrintMessage( clientNum, spawnType );
	}
}

/*
===========
(ModFN) PostClientSpawn

Notify other clients when action hero is spawning.
============
*/
LOGFUNCTION_SVOID( PREFIX(PostClientSpawn), ( int clientNum, clientSpawnType_t spawnType ),
		( clientNum, spawnType ), "G_MODFN_POSTCLIENTSPAWN" ) {
	gclient_t *client = &level.clients[clientNum];

	MOD_STATE->Prev_PostClientSpawn( clientNum, spawnType );

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
LOGFUNCTION_SRET( qboolean, PREFIX(CanItemBeDropped), ( gitem_t *item, int clientNum ),
		( item, clientNum ), "G_MODFN_CANITEMBEDROPPED" ) {
	gclient_t *client = &level.clients[clientNum];

	if ( client->sess.sessionClass == PC_ACTIONHERO && item->giType != IT_TEAM ) {
		return qfalse;
	}

	return MOD_STATE->Prev_CanItemBeDropped( item, clientNum );
}

/*
================
(ModFN) AddRegisteredItems
================
*/
LOGFUNCTION_SVOID( PREFIX(AddRegisteredItems), ( void ), (), "G_MODFN_ADDREGISTEREDITEMS" ) {
	MOD_STATE->Prev_AddRegisteredItems();

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
LOGFUNCTION_SVOID( PREFIX(PostRunFrame), ( void ), (), "G_MODFN_POSTRUNFRAME" ) {
	MOD_STATE->Prev_PostRunFrame();
	ModActionHero_CheckReplaceHero( -1 );
}

/*
================
ModActionHero_Init
================
*/

#define INIT_FN_STACKABLE( name ) \
	MOD_STATE->Prev_##name = modfn.name; \
	modfn.name = PREFIX(name);

#define INIT_FN_OVERRIDE( name ) \
	modfn.name = PREFIX(name);

#define INIT_FN_BASE( name ) \
	EF_WARN_ASSERT( modfn.name == ModFNDefault_##name ); \
	modfn.name = PREFIX(name);

LOGFUNCTION_VOID( ModActionHero_Init, ( void ), (), "G_MOD_INIT G_ACTIONHERO" ) {
	if ( EF_WARN_ASSERT( !MOD_STATE ) ) {
		modcfg.mods_enabled.actionhero = qtrue;
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MOD_STATE->actionHeroClientNum = -1;

		// Register mod functions
		INIT_FN_STACKABLE( UpdateSessionClass );
		INIT_FN_STACKABLE( EffectiveHandicap );
		INIT_FN_STACKABLE( SpawnConfigureClient );
		INIT_FN_STACKABLE( SpawnCenterPrintMessage );
		INIT_FN_STACKABLE( PostClientSpawn );
		INIT_FN_STACKABLE( PostPlayerDie );
		INIT_FN_STACKABLE( PrePlayerLeaveTeam );
		INIT_FN_STACKABLE( CanItemBeDropped );
		INIT_FN_STACKABLE( AddRegisteredItems );
		INIT_FN_STACKABLE( PostRunFrame );

		// Make sure the gametype is set to FFA
		if ( trap_Cvar_VariableIntegerValue( "g_gametype" ) != GT_FFA &&
				trap_Cvar_VariableIntegerValue( "g_gametype" ) != GT_SINGLE_PLAYER ) {
			trap_Cvar_Set( "g_gametype", "0" );
		}
	}
}
