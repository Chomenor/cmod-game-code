/*
* Ping Compensation - Client Prediction
* 
* This module provides server-side support for weapon fire prediction features on the client.
*/

#define MOD_NAME ModPCClientPredict

#include "mods/features/pingcomp/pc_local.h"

#define PREDICTION_ENABLED ( ModPingcomp_Static_PingCompensationEnabled() && MOD_STATE->g_unlaggedPredict.integer )

typedef struct {
	predictableRNG_t rng;
} PingcompClientPredict_client_t;

static struct {
	trackedCvar_t g_unlaggedPredict;

	PingcompClientPredict_client_t clients[MAX_CLIENTS];
} *MOD_STATE;

/*
======================
(ModFN) WeaponPredictableRNG

Only use the predictable functions if prediction is actually enabled (it could have slight
cheating security implications).
======================
*/
static unsigned int MOD_PREFIX(WeaponPredictableRNG)( MODFN_CTV, int clientNum ) {
	if ( PREDICTION_ENABLED && G_AssertConnectedClient( clientNum ) ) {
		return BG_PredictableRNG_Rand( &MOD_STATE->clients[clientNum].rng, level.clients[clientNum].ps.commandTime );
	}

	return MODFN_NEXT( WeaponPredictableRNG, ( MODFN_NC, clientNum ) );
}

/*
==============
ModPCClientPredict_AddWeaponConstant
==============
*/
static void ModPCClientPredict_AddWeaponConstant( char *buffer, int bufSize, weaponConstant_t wc,
		int defaultValue, const char *infoKey ) {
	int value = modfn.AdjustWeaponConstant( wc, defaultValue );
	if ( value != defaultValue ) {
		Q_strcat( buffer, bufSize, va( " %s:%i", infoKey, value ) );
	}
}

/*
==============
ModPCClientPredict_AddInfo
==============
*/
static void ModPCClientPredict_AddInfo( char *info ) {
	char buffer[256];
	const char *version;
	
	// If uam is enabled, use the new version string to block old client versions from
	// enabling weapon prediction, to avoid incompatibilities.
	version = "uxb09a9y";
	if ( modcfg.mods_enabled.uam ) {
		version = "uxb09a9y-2";
	}
	Com_sprintf( buffer, sizeof( buffer ), "ver:%s", version );

	ModPCClientPredict_AddWeaponConstant( buffer, sizeof( buffer ), WC_USE_IMOD_RIFLE, 0, "imodRifle" );

	if ( ModPingcomp_Static_ProjectileCompensationEnabled() ) {
		Q_strcat( buffer, sizeof( buffer ), " proj:1" );
		ModPCClientPredict_AddWeaponConstant( buffer, sizeof( buffer ), WC_USE_TRIPMINES, 0, "tm" );
		ModPCClientPredict_AddWeaponConstant( buffer, sizeof( buffer ), WC_SCAV_VELOCITY, 1500, "ps4p" );
		ModPCClientPredict_AddWeaponConstant( buffer, sizeof( buffer ), WC_STASIS_VELOCITY, 1100, "ps5p" );
		ModPCClientPredict_AddWeaponConstant( buffer, sizeof( buffer ), WC_DN_SEARCH_DIST, 256, "ps9a" );
		ModPCClientPredict_AddWeaponConstant( buffer, sizeof( buffer ), WC_BORG_PROJ_VELOCITY, 1000, "psbp" );

		if ( modfn_lcl.AdjustModConstant( MC_SEEKER_ACCELERATOR_MODE, 0 ) ) {
			Q_strcat( buffer, sizeof( buffer ), " seekerAccel:1" );
		}
	}

	Info_SetValueForKey( info, "weaponPredict", buffer );
}

/*
==============
ModPCClientPredict_SetNoDamageFlags

Set flags to communicate entity takedamage values to client for prediction purposes.
==============
*/
static void ModPCClientPredict_SetNoDamageFlags( void ) {
	int i;
	for(i=0; i<MAX_GENTITIES; ++i) {
		if ( g_entities[i].r.linked ) {
			gentity_t *ent = &g_entities[i];
			if ( !ent->takedamage && ent->s.solid ) {
				ent->s.eFlags |= EF_PINGCOMP_NO_DAMAGE;
			} else {
				ent->s.eFlags &= ~EF_PINGCOMP_NO_DAMAGE;
			}
		}
	}
}

/*
================
ModPCClientPredict_CvarCallback

Make sure any configstrings for client prediction are updated.
================
*/
static void ModPCClientPredict_CvarCallback( trackedCvar_t *cvar ) {
	ModModcfgCS_Static_Update();
}

/*
==============
(ModFN) AddModConfigInfo
==============
*/
static void MOD_PREFIX(AddModConfigInfo)( MODFN_CTV, char *info ) {
	MODFN_NEXT( AddModConfigInfo, ( MODFN_NC, info ) );

	if ( PREDICTION_ENABLED ) {
		ModPCClientPredict_AddInfo( info );
	}
}

/*
================
(ModFN) PostRunFrame
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );

	if ( PREDICTION_ENABLED ) {
		ModPCClientPredict_SetNoDamageFlags();
	}
}

/*
================
ModPCClientPredict_Init
================
*/
void ModPCClientPredict_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( WeaponPredictableRNG, MODPRIORITY_GENERAL );
		MODFN_REGISTER( AddModConfigInfo, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PostRunFrame, MODPRIORITY_GENERAL );

		G_RegisterTrackedCvar( &MOD_STATE->g_unlaggedPredict, "g_unlaggedPredict", "1", 0, qfalse );
		G_RegisterCvarCallback( &MOD_STATE->g_unlaggedPredict, ModPCClientPredict_CvarCallback, qfalse );

		ModModcfgCS_Init();
	}
}
