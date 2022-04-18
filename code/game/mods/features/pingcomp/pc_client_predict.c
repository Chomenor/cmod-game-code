/*
* Ping Compensation - Client Prediction
* 
* This module provides server-side support for weapon fire prediction features on the client.
*/

#include "mods/features/pingcomp/pc_local.h"

#define PREFIX( x ) ModPCClientPredict_##x
#define MOD_STATE PREFIX( state )

#define PREDICTION_ENABLED ( ModPingcomp_Static_PingCompensationEnabled() && MOD_STATE->g_unlaggedPredict.integer )

typedef struct {
	predictableRNG_t rng;
} PingcompClientPredict_client_t;

static struct {
	trackedCvar_t g_unlaggedPredict;

	PingcompClientPredict_client_t clients[MAX_CLIENTS];

	// For mod function stacking
	ModFNType_WeaponPredictableRNG Prev_WeaponPredictableRNG;
	ModFNType_AddModConfigInfo Prev_AddModConfigInfo;
	ModFNType_PostRunFrame Prev_PostRunFrame;
} *MOD_STATE;

/*
======================
(ModFN) WeaponPredictableRNG

Only use the predictable functions if prediction is actually enabled (it could have slight
cheating security implications).
======================
*/
LOGFUNCTION_SRET( unsigned int, PREFIX(WeaponPredictableRNG), ( int clientNum ), ( clientNum ), "G_MODFN_WEAPONPREDICTABLERNG" ) {
	if ( PREDICTION_ENABLED && G_AssertConnectedClient( clientNum ) ) {
		return BG_PredictableRNG_Rand( &MOD_STATE->clients[clientNum].rng, level.clients[clientNum].ps.commandTime );
	}

	return MOD_STATE->Prev_WeaponPredictableRNG( clientNum );
}

/*
==============
ModPCClientPredict_AddInfo
==============
*/
static void ModPCClientPredict_AddInfo( char *info ) {
	char buffer[256];
	Q_strncpyz( buffer, "ver:" BG_WEAPON_PREDICT_VERSION, sizeof( buffer ) );

	if ( ModPingcomp_Static_ProjectileCompensationEnabled() ) {
		Q_strcat( buffer, sizeof( buffer ), " proj:1" );
		if ( g_pModSpecialties.integer ) {
			Q_strcat( buffer, sizeof( buffer ), " tm:1" );
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
	G_UpdateModConfigInfo();
}

/*
==============
(ModFN) AddModConfigInfo
==============
*/
LOGFUNCTION_SVOID( PREFIX(AddModConfigInfo), ( char *info ), ( info ), "G_MODFN_ADDMODCONFIGINFO" ) {
	MOD_STATE->Prev_AddModConfigInfo( info );

	if ( PREDICTION_ENABLED ) {
		ModPCClientPredict_AddInfo( info );
	}
}

/*
================
(ModFN) PostRunFrame
================
*/
LOGFUNCTION_SVOID( PREFIX(PostRunFrame), ( void ), (), "G_MODFN_POSTRUNFRAME" ) {
	MOD_STATE->Prev_PostRunFrame();

	if ( PREDICTION_ENABLED ) {
		ModPCClientPredict_SetNoDamageFlags();
	}
}

/*
================
ModPCClientPredict_Init
================
*/

#define INIT_FN_STACKABLE( name ) \
	MOD_STATE->Prev_##name = modfn.name; \
	modfn.name = PREFIX(name);

#define INIT_FN_OVERRIDE( name ) \
	modfn.name = PREFIX(name);

LOGFUNCTION_VOID( ModPCClientPredict_Init, ( void ), (), "G_MOD_INIT" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		INIT_FN_STACKABLE( WeaponPredictableRNG );
		INIT_FN_STACKABLE( AddModConfigInfo );
		INIT_FN_STACKABLE( PostRunFrame );

		G_RegisterTrackedCvar( &MOD_STATE->g_unlaggedPredict, "g_unlaggedPredict", "1", 0, qfalse );
		G_RegisterCvarCallback( &MOD_STATE->g_unlaggedPredict, ModPCClientPredict_CvarCallback, qfalse );
	}
}
