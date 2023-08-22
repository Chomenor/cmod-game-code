/*
* UAM Instagib Weapon Saving
* 
* Save whether the player is using imod or rifle in instagib mode so they
* don't have to switch it back every round.
*/

#define MOD_NAME ModUAMInstagibSaveWep

#include "mods/modes/uam/uam_local.h"

static struct {
	int currentWeapon[MAX_CLIENTS];
} *MOD_STATE;

#define VALID_WEAPON( wep ) ( ( wep ) == WP_COMPRESSION_RIFLE || ( wep ) == WP_IMOD )

/*
================
SaveWeaponPreference
================
*/
static void SaveWeaponPreference( int clientNum ) {
	int *weapon = &MOD_STATE->currentWeapon[clientNum];
	gclient_t *client = &level.clients[clientNum];

	if ( !modfn.SpectatorClient( clientNum ) && VALID_WEAPON( client->ps.weapon ) ) {
		*weapon = client->ps.weapon;
	}
}

/*
================
(ModFN) InitClientSession

Load saved weapon from previous session, or default to rifle on first connect.
================
*/
static void MOD_PREFIX(InitClientSession)( MODFN_CTV, int clientNum, qboolean initialConnect, const info_string_t *info ) {
	int *weapon = &MOD_STATE->currentWeapon[clientNum];
	MODFN_NEXT( InitClientSession, ( MODFN_NC, clientNum, initialConnect, info ) );

	*weapon = atoi( Info_ValueForKey( info->s, "currentWeapon" ) );
	if ( !VALID_WEAPON( *weapon ) ) {
		*weapon = WP_COMPRESSION_RIFLE;
	}
}

/*
================
(ModFN) GenerateClientSessionInfo
================
*/
static void MOD_PREFIX(GenerateClientSessionInfo)( MODFN_CTV, int clientNum, info_string_t *info ) {
	MODFN_NEXT( GenerateClientSessionInfo, ( MODFN_NC, clientNum, info ) );
	Info_SetValueForKey_Big( info->s, "currentWeapon", va( "%i", MOD_STATE->currentWeapon[clientNum] ) );
}

/*
================
(ModFN) PrePlayerLeaveTeam

Save weapon before leaving team.
================
*/
static void MOD_PREFIX(PrePlayerLeaveTeam)( MODFN_CTV, int clientNum, team_t oldTeam ) {
	MODFN_NEXT( PrePlayerLeaveTeam, ( MODFN_NC, clientNum, oldTeam ) );
	SaveWeaponPreference( clientNum );
}

/*
================
(ModFN) PostPlayerDie

Save weapon when killed.
================
*/
static void MOD_PREFIX(PostPlayerDie)( MODFN_CTV, gentity_t *self, gentity_t *inflictor, gentity_t *attacker,
		int meansOfDeath, int *awardPoints ) {
	int clientNum = self - g_entities;
	MODFN_NEXT( PostPlayerDie, ( MODFN_NC, self, inflictor, attacker, meansOfDeath, awardPoints ) );
	SaveWeaponPreference( clientNum );
}

/*
================
(ModFN) MatchStateTransition

Save weapon when entering intermission.
================
*/
static void MOD_PREFIX(MatchStateTransition)( MODFN_CTV, matchState_t oldState, matchState_t newState ) {
	MODFN_NEXT( MatchStateTransition, ( MODFN_NC, oldState, newState ) );

	if ( oldState < MS_INTERMISSION_QUEUED && newState >= MS_INTERMISSION_QUEUED ) {
		int i;
		for ( i = 0; i < level.maxclients; ++i ) {
			if ( level.clients[i].pers.connected == CON_CONNECTED ) {
				SaveWeaponPreference( i );
			}
		}
	}
}

/*
================
(ModFN) PostClientSpawn

Set active weapon to saved one when player spawns.
================
*/
static void MOD_PREFIX(PostClientSpawn)( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ) {
	int weapon = MOD_STATE->currentWeapon[clientNum];
	gclient_t *client = &level.clients[clientNum];

	MODFN_NEXT( PostClientSpawn, ( MODFN_NC, clientNum, spawnType ) );

	if ( client->ps.stats[STAT_WEAPONS] & ( 1 << weapon ) ) {
		client->ps.weapon = weapon;
	}
}

/*
================
ModUAMInstagibSaveWep_Init
================
*/
void ModUAMInstagibSaveWep_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( InitClientSession, ++modePriorityLevel );
		MODFN_REGISTER( GenerateClientSessionInfo, ++modePriorityLevel );
		MODFN_REGISTER( PrePlayerLeaveTeam, ++modePriorityLevel );
		MODFN_REGISTER( PostPlayerDie, ++modePriorityLevel );
		MODFN_REGISTER( MatchStateTransition, ++modePriorityLevel );
		MODFN_REGISTER( PostClientSpawn, ++modePriorityLevel );
	}
}
