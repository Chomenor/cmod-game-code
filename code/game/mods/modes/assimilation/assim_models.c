/*
* Assimilation Models
* 
* This module handles player model conversions for Assimilation mode. All borg players
* should have borg models, non-borg players should have non-borg models, and the borg
* queen has a specific unique model.
*/

#define MOD_PREFIX( x ) ModAssimModels_##x

#include "mods/modes/assimilation/assim_local.h"

static struct {
	ModFNType_ConvertPlayerModel Prev_ConvertPlayerModel;
	ModFNType_RandomPlayerModel Prev_RandomPlayerModel;
} *MOD_STATE;

/*
================
(ModFN) ConvertPlayerModel

Verifies and converts model to meet borg/non-borg team requirements. Returns empty string
to select random model instead.
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(ConvertPlayerModel),
		( int clientNum, const char *userinfo, const char *source_model, char *output, unsigned int outputSize ),
		( clientNum, userinfo, source_model, output, outputSize ), "G_PLAYERMODELS G_MODFN_CONVERTPLAYERMODEL" ) {
	gclient_t *client = &level.clients[clientNum];

	// Don't change model for spectators
	if ( modfn.SpectatorClient( clientNum ) ) {
		MOD_STATE->Prev_ConvertPlayerModel( clientNum, userinfo, source_model, output, outputSize );
		return;
	}

	if ( client->sess.sessionClass == PC_BORG ) {
		// Use specific model for queen
		if ( modfn.IsBorgQueen( clientNum ) ) {
			Q_strncpyz( output, "borgqueen", outputSize );
			return;
		}

		// Check if source model is already borg
		if ( Q_strncmp( "borgqueen", source_model, 9 ) &&
				ModModelGroups_Shared_ListContainsRace( ModModelGroups_Shared_SearchGroupList( source_model ), "borg" ) ) {
			Q_strncpyz( output, source_model, outputSize );
			return;
		}

		// Convert certain specific models to borg versions
		if ( Q_strncmp( "janeway", source_model, 7 ) == 0 ) {
			Q_strncpyz( output, "borg-janeway", outputSize );
			return;
		} else if ( Q_strncmp( "torres", source_model, 6 ) == 0 ) {
			Q_strncpyz( output, "borg-torres", outputSize );
			return;
		} else if ( Q_strncmp( "tuvok", source_model, 5 ) == 0 ) {
			Q_strncpyz( output, "borg-tuvok", outputSize );
			return;
		} else if ( Q_strncmp( "seven", source_model, 5 ) == 0 ) {
			Q_strncpyz( output, "sevenofnine", outputSize );
			return;
		}

		// Fall back to random selection
		*output = '\0';
		return;
	}

	else {
		// Perform base conversions
		MOD_STATE->Prev_ConvertPlayerModel( clientNum, userinfo, source_model, output, outputSize );

		// Ensure model isn't borg
		if ( !Q_strncmp( "borgqueen", output, 9 ) ||
				ModModelGroups_Shared_ListContainsRace( ModModelGroups_Shared_SearchGroupList( output ), "borg" ) ) {
			*output = '\0';
			return;
		}
	}
}

/*
================
(ModFN) RandomPlayerModel

Selects random model that meets borg/non-borg team requirements. Returns empty string on error.
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(RandomPlayerModel), ( int clientNum, const char *userinfo, char *output, unsigned int outputSize ),
		( clientNum, userinfo, output, outputSize ), "G_PLAYERMODELS G_MODFN_RANDOMPLAYERMODEL" ) {
	gclient_t *client = &level.clients[clientNum];

	// Don't change model for spectators
	if ( modfn.SpectatorClient( clientNum ) ) {
		MOD_STATE->Prev_RandomPlayerModel( clientNum, userinfo, output, outputSize );
		return;
	}

	if ( client->sess.sessionClass == PC_BORG ) {
		const char *race = "Borg";
		const char *oldModel = Info_ValueForKey( userinfo, "model" );
		int tries = 0;

		// Try to match sex of original model
		if ( !Q_strncmp( "borgqueen", oldModel, 9 ) ) {
			const char *sex = Info_ValueForKey( userinfo, "sex" );
			if ( sex[0] == 'm' ) {
				race = "BorgMale";
			} else if ( sex[0] == 'f' ) {
				race = "BorgFemale";
			}
		} else if ( ModModelGroups_Shared_ListContainsRace( ModModelGroups_Shared_SearchGroupList( oldModel ), "male" ) ) {
			race = "BorgMale";
		} else if ( ModModelGroups_Shared_ListContainsRace( ModModelGroups_Shared_SearchGroupList( oldModel ), "female" ) ) {
			race = "BorgFemale";
		}

		// Try to get a borg model that isn't the queen
		while ( 1 ) {
			ModModelGroups_Shared_RandomModelForRace( race, output, outputSize );

			if ( Q_strncmp( "borgqueen", output, 9 ) ) {
				return;
			}

			if ( tries++ > 10 ) {
				*output = '\0';
				return;
			}
		}
	}

	else {
		// Give the base selection a chance to pick a non-borg model
		// (maybe a team group specified for the non-borg team)
		MOD_STATE->Prev_RandomPlayerModel( clientNum, userinfo, output, outputSize );

		// Select non-borg model
		if ( !*output || !Q_strncmp( "borgqueen", output, 9 ) ||
				ModModelGroups_Shared_ListContainsRace( ModModelGroups_Shared_SearchGroupList( output ), "borg" ) ) {
			ModModelGroups_Shared_RandomModelForRace( "HazardTeam", output, outputSize );
		}
	}
}

/*
================
ModAssimModels_Init
================
*/
LOGFUNCTION_VOID( ModAssimModels_Init, ( void ), (), "G_MOD_INIT G_ASSIMILATION" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		INIT_FN_STACKABLE_LCL( ConvertPlayerModel );
		INIT_FN_STACKABLE_LCL( RandomPlayerModel );

		ModModelGroups_Init();
		ModModelSelection_Init();
	}
}
