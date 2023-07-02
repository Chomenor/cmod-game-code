/*
* Specialties Mode
* 
* In this mode players can select from one of several classes: Heavy Weapons Specialist,
* Demolitionist, Technician, Medic, Sniper, and Infiltrator. It is usually played in
* combination with the CTF gametype.
*/

#define MOD_NAME ModSpecialties

#include "mods/modes/specialties/specs_local.h"

typedef struct {
	// time of last class change
	int classChangeTime;
} specialties_client_t;

static struct {
	specialties_client_t clients[MAX_CLIENTS];
	trackedCvar_t g_classChangeDebounceTime;
} *MOD_STATE;

/*
================
(ModFN) InitClientSession
================
*/
static void MOD_PREFIX(InitClientSession)( MODFN_CTV, int clientNum, qboolean initialConnect, const info_string_t *info ) {
	specialties_client_t *modclient = &MOD_STATE->clients[clientNum];

	MODFN_NEXT( InitClientSession, ( MODFN_NC, clientNum, initialConnect, info ) );
	memset( modclient, 0, sizeof( *modclient ) );
}

/*
================
(ModFN) PreClientSpawn

Reset class change time limit when player joins a new team.
================
*/
static void MOD_PREFIX(PreClientSpawn)( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ) {
	specialties_client_t *modclient = &MOD_STATE->clients[clientNum];

	MODFN_NEXT( PreClientSpawn, ( MODFN_NC, clientNum, spawnType ) );

	if ( spawnType != CST_RESPAWN ) {
		modclient->classChangeTime = 0;
	}
}

/*
================
ModSpecialties_IsSpecialtiesClass
================
*/
static qboolean ModSpecialties_IsSpecialtiesClass( pclass_t pclass ) {
	if( pclass == PC_INFILTRATOR || pclass == PC_SNIPER || pclass == PC_HEAVY || pclass == PC_DEMO
				|| pclass == PC_MEDIC || pclass == PC_TECH )
		return qtrue;
	return qfalse;
}

/*
================
(ModFN) GenerateClientSessionStructure

Save specialties class for next round.
================
*/
static void MOD_PREFIX(GenerateClientSessionStructure)( MODFN_CTV, int clientNum, clientSession_t *sess ) {
	gclient_t *client = &level.clients[clientNum];
	pclass_t pclass;

	MODFN_NEXT( GenerateClientSessionStructure, ( MODFN_NC, clientNum, sess ) );

	pclass = modfn.RealSessionClass( clientNum );
	if ( ModSpecialties_IsSpecialtiesClass( pclass ) ) {
		sess->sessionClass = pclass;
	}
}

/*
================
ModSpecialties_TellClassCmd
================
*/
static void ModSpecialties_TellClassCmd( int clientNum ) {
	gclient_t *client = &level.clients[clientNum];
	const char *className = "";

	switch ( client->sess.sessionClass ) {
		case PC_INFILTRATOR: //fast: low attack
			className = "Infiltrator";
			break;
		case PC_SNIPER: //sneaky: snipe only
			className = "Sniper";
			break;
		case PC_HEAVY: //slow: heavy attack
			className = "Heavy";
			break;
		case PC_DEMO: //go boom
			className = "Demo";
			break;
		case PC_MEDIC: //heal
			className = "Medic";
			break;
		case PC_TECH: //operate
			className = "Tech";
			break;
		default:
			trap_SendServerCommand( clientNum, "print \"Unknown current class!\n\"" );
			return;
	}

	trap_SendServerCommand( clientNum, va( "print \"class: %s\n\"", className ) );
}

/*
=================
ModSpecialties_BroadCastClassChange

Let everyone know about a team change
=================
*/
static void ModSpecialties_BroadcastClassChange( gclient_t *client )
{
	switch( client->sess.sessionClass )
	{
	case PC_INFILTRATOR:
		trap_SendServerCommand( -1, va("cp \"%.15s" S_COLOR_WHITE " is now an Infiltrator.\n\"", client->pers.netname) );
		break;
	case PC_SNIPER:
		trap_SendServerCommand( -1, va("cp \"%.15s" S_COLOR_WHITE " is now a Sniper.\n\"", client->pers.netname) );
		break;
	case PC_HEAVY:
		trap_SendServerCommand( -1, va("cp \"%.15s" S_COLOR_WHITE " is now a Heavy Weapons Specialist.\n\"", client->pers.netname) );
		break;
	case PC_DEMO:
		trap_SendServerCommand( -1, va("cp \"%.15s" S_COLOR_WHITE " is now a Demolitionist.\n\"", client->pers.netname) );
		break;
	case PC_MEDIC:
		trap_SendServerCommand( -1, va("cp \"%.15s" S_COLOR_WHITE " is now a Medic.\n\"", client->pers.netname) );
		break;
	case PC_TECH:
		trap_SendServerCommand( -1, va("cp \"%.15s" S_COLOR_WHITE " is now a Technician.\n\"", client->pers.netname) );
		break;
	}
}

/*
================
ModSpecialties_SetClassCmd
================
*/
static void ModSpecialties_SetClassCmd( int clientNum ) {
	gentity_t *ent = &g_entities[clientNum];
	gclient_t *client = &level.clients[clientNum];
	specialties_client_t *modclient = &MOD_STATE->clients[clientNum];
	pclass_t pclass = PC_NOCLASS;
	char s[MAX_TOKEN_CHARS];

	// Check if joining is disabled by mod
	if ( client->sess.sessionTeam != TEAM_SPECTATOR && !modfn.CheckJoinAllowed( clientNum, CJA_SETCLASS, TEAM_FREE ) ) {
		return;
	}

	// Apply time limit to repeat class changes
	if ( MOD_STATE->g_classChangeDebounceTime.integer > 0 && level.matchState == MS_ACTIVE &&
			modclient->classChangeTime && !modfn.SpectatorClient( clientNum ) &&
			modclient->classChangeTime + MOD_STATE->g_classChangeDebounceTime.integer * 1000 > level.time ) {
		int seconds, minutes = 0;
		seconds = ( modclient->classChangeTime + MOD_STATE->g_classChangeDebounceTime.integer * 1000 - level.time + 999 ) / 1000;
		if ( seconds >= 60 ) {
			minutes = floor( seconds / 60.0f );
			seconds -= minutes * 60;
			if ( minutes > 1 ) {
				if ( seconds ) {
					if ( seconds > 1 ) {
						trap_SendServerCommand( clientNum, va( "cp \"Cannot change classes again for %d minutes and %d seconds\"", minutes, seconds ) );
					} else {
						trap_SendServerCommand( clientNum, va( "cp \"Cannot change classes again for %d minutes\"", minutes ) );
					}
				} else {
					trap_SendServerCommand( clientNum, va( "cp \"Cannot change classes again for %d minutes\"", minutes ) );
				}
			} else {
				if ( seconds ) {
					if ( seconds > 1 ) {
						trap_SendServerCommand( clientNum, va( "cp \"Cannot change classes again for %d minute and %d seconds\"", minutes, seconds ) );
					} else {
						trap_SendServerCommand( clientNum, va( "cp \"Cannot change classes again for %d minute and %d second\"", minutes, seconds ) );
					}
				} else {
					trap_SendServerCommand( clientNum, va( "cp \"Cannot change classes again for %d minute\"", minutes ) );
				}
			}
		} else {
			if ( seconds > 1 ) {
				trap_SendServerCommand( clientNum, va( "cp \"Cannot change classes again for %d seconds\"", seconds ) );
			} else {
				trap_SendServerCommand( clientNum, va( "cp \"Cannot change classes again for %d second\"", seconds ) );
			}
		}
		return;
	}

	trap_Argv( 1, s, sizeof( s ) );

	if ( !Q_stricmp( s, "infiltrator" ) ) {
		pclass = PC_INFILTRATOR;
	} else if ( !Q_stricmp( s, "sniper" ) ) {
		pclass = PC_SNIPER;
	} else if ( !Q_stricmp( s, "heavy" ) ) {
		pclass = PC_HEAVY;
	} else if ( !Q_stricmp( s, "demo" ) ) {
		pclass = PC_DEMO;
	} else if ( !Q_stricmp( s, "medic" ) ) {
		pclass = PC_MEDIC;
	} else if ( !Q_stricmp( s, "tech" ) ) {
		pclass = PC_TECH;
	} else {
		pclass = irandom( PC_INFILTRATOR, PC_TECH );
	}

	if ( pclass == client->sess.sessionClass ) {
		// Make sure client has the correct class value in UI, just in case
		SetPlayerClassCvar( ent );
		return;
	}

	client->sess.sessionClass = pclass;
	ModSpecialties_BroadcastClassChange( client );

	// Kill him (makes sure he loses flags, etc)
	player_die( ent, NULL, NULL, 100000, MOD_RESPAWN );

	ClientBegin( clientNum );

	modclient->classChangeTime = level.time;
}

/*
================
(ModFN) ModClientCommand

Handle class command.
================
*/
static qboolean MOD_PREFIX(ModClientCommand)( MODFN_CTV, int clientNum, const char *cmd ) {
	gclient_t *client = &level.clients[clientNum];

	// Allow this command for connecting clients, because it can sometimes be called before
	// GAME_CLIENT_BEGIN when starting the game from UI.
	if ( !Q_stricmp( cmd, "class" ) && !level.intermissiontime && client->pers.connected >= CON_CONNECTING ) {
		if ( trap_Argc() != 2 ) {
			ModSpecialties_TellClassCmd( clientNum );
		} else {
			ModSpecialties_SetClassCmd( clientNum );
		}
		return qtrue;
	}

	return MODFN_NEXT( ModClientCommand, ( MODFN_NC, clientNum, cmd ) );
}

/*
================
(ModFN) UpdateSessionClass

Pick random class for players coming into the game.
================
*/
static void MOD_PREFIX(UpdateSessionClass)( MODFN_CTV, int clientNum ) {
	gentity_t *ent = &g_entities[clientNum];
	gclient_t *client = &level.clients[clientNum];

	if( !ModSpecialties_IsSpecialtiesClass( client->sess.sessionClass ) ) {
		client->sess.sessionClass = irandom( PC_INFILTRATOR, PC_TECH );
	}
}

/*
================
(ModFN) EffectiveHandicap

Force handicap values for infiltrator and heavy. Also ignore handicap for other classes because
the scoreboard doesn't support displaying it and for consistency with original behavior.
================
*/
static int MOD_PREFIX(EffectiveHandicap)( MODFN_CTV, int clientNum, effectiveHandicapType_t type ) {
	gclient_t *client = &level.clients[clientNum];

	if ( ModSpecialties_IsSpecialtiesClass( client->sess.sessionClass ) ) {
		if ( client->sess.sessionClass == PC_INFILTRATOR && ( type == EH_DAMAGE || type == EH_MAXHEALTH ) ) {
			return 50;
		}
		if ( client->sess.sessionClass == PC_HEAVY && ( type == EH_DAMAGE || type == EH_MAXHEALTH ) ) {
			return 200;
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
	specialties_client_t *modclient = &MOD_STATE->clients[clientNum];

	MODFN_NEXT( SpawnConfigureClient, ( MODFN_NC, clientNum ) );

	if ( ModSpecialties_IsSpecialtiesClass( client->sess.sessionClass ) ) {
		// STAT_MAX_HEALTH should already be determined via EffectiveHandicap
		ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH];

		switch ( client->sess.sessionClass )
		{
		case PC_INFILTRATOR:
			client->ps.stats[STAT_WEAPONS] = ( 1 << WP_PHASER );
			client->ps.ammo[WP_PHASER] = PHASER_AMMO_MAX;

			client->ps.stats[STAT_ARMOR] = 0;

			client->ps.stats[STAT_HOLDABLE_ITEM] = BG_FindItemForHoldable( HI_TRANSPORTER ) - bg_itemlist;

			//INFILTRATOR gets permanent speed
			ent->client->ps.powerups[PW_HASTE] = level.time - ( level.time % 1000 );
			ent->client->ps.powerups[PW_HASTE] += 1800 * 1000;
			break;
		case PC_SNIPER:
			client->ps.stats[STAT_WEAPONS] = ( 1 << WP_IMOD );
			client->ps.ammo[WP_IMOD] = Max_Ammo[WP_IMOD];
			client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_COMPRESSION_RIFLE );
			client->ps.ammo[WP_COMPRESSION_RIFLE] = Max_Ammo[WP_COMPRESSION_RIFLE];

			client->ps.stats[STAT_ARMOR] = 25;

			client->ps.stats[STAT_HOLDABLE_ITEM] = BG_FindItemForHoldable( HI_DECOY ) - bg_itemlist;
			client->ps.stats[STAT_USEABLE_PLACED] = 0;
			break;
		case PC_HEAVY:
			client->ps.stats[STAT_WEAPONS] = ( 1 << WP_TETRION_DISRUPTOR );
			client->ps.ammo[WP_TETRION_DISRUPTOR] = Max_Ammo[WP_TETRION_DISRUPTOR];
			client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_QUANTUM_BURST );
			client->ps.ammo[WP_QUANTUM_BURST] = Max_Ammo[WP_QUANTUM_BURST];

			client->ps.stats[STAT_ARMOR] = 100;
			break;
		case PC_DEMO:
			// Start countdown to receive detpack
			ModPendingItem_Shared_SchedulePendingItem( clientNum, HI_DETPACK, 10000 );

			client->ps.stats[STAT_WEAPONS] = ( 1 << WP_SCAVENGER_RIFLE );
			client->ps.ammo[WP_SCAVENGER_RIFLE] = Max_Ammo[WP_SCAVENGER_RIFLE];
			client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_GRENADE_LAUNCHER );
			client->ps.ammo[WP_GRENADE_LAUNCHER] = Max_Ammo[WP_GRENADE_LAUNCHER];

			client->ps.stats[STAT_ARMOR] = 50;
			break;
		case PC_MEDIC:
			client->ps.stats[STAT_WEAPONS] = ( 1 << WP_PHASER );
			client->ps.ammo[WP_PHASER] = PHASER_AMMO_MAX;
			client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_VOYAGER_HYPO );
			client->ps.ammo[WP_VOYAGER_HYPO] = client->ps.ammo[WP_PHASER];

			client->ps.stats[STAT_ARMOR] = 75;

			client->ps.stats[STAT_HOLDABLE_ITEM] = BG_FindItemForHoldable( HI_MEDKIT ) - bg_itemlist;

			ent->client->ps.powerups[PW_REGEN] = level.time - ( level.time % 1000 );
			ent->client->ps.powerups[PW_REGEN] += 1800 * 1000;
			break;
		case PC_TECH:
			client->ps.stats[STAT_WEAPONS] = ( 1 << WP_PHASER );
			client->ps.ammo[WP_PHASER] = PHASER_AMMO_MAX;
			client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_DREADNOUGHT );
			client->ps.ammo[WP_DREADNOUGHT] = Max_Ammo[WP_DREADNOUGHT];

			client->ps.stats[STAT_ARMOR] = 50;

			client->ps.stats[STAT_HOLDABLE_ITEM] = BG_FindItemForHoldable( HI_SHIELD ) - bg_itemlist;

			//tech gets permanent seeker
			ent->client->ps.powerups[PW_SEEKER] = level.time - ( level.time % 1000 );
			ent->client->ps.powerups[PW_SEEKER] += 1800 * 1000;
			ent->count = 1;//can give away one invincibility
			//can also place ammo stations, register the model and sound for them
			G_ModelIndex( "models/mapobjects/dn/powercell.md3" );
			G_SoundIndex( "sound/player/suitenergy.wav" );
			break;
		}
	}
}

/*
================
(ModFN) CheckItemSpawnDisabled
================
*/
static qboolean MOD_PREFIX(CheckItemSpawnDisabled)( MODFN_CTV, gitem_t *item ) {
	switch ( item->giType ) {
		case IT_ARMOR: //given to classes
		case IT_WEAPON: //spread out among classes
		case IT_HOLDABLE: //spread out among classes
		case IT_HEALTH: //given by medics
		case IT_AMMO: //given by technician
			return qtrue;
		case IT_POWERUP:
			switch ( item->giTag ) {
				case PW_HASTE:
				case PW_INVIS:
				case PW_REGEN:
				case PW_SEEKER:
					return qtrue;
			}
	}

	return MODFN_NEXT( CheckItemSpawnDisabled, ( MODFN_NC, item ) );
}

/*
============
(ModFN) CanItemBeDropped

Disable dropping any items except flag.
============
*/
static qboolean MOD_PREFIX(CanItemBeDropped)( MODFN_CTV, gitem_t *item, int clientNum ) {
	if ( item->giType != IT_TEAM ) {
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

	//weapons
	RegisterItem( BG_FindItemForWeapon( WP_IMOD ) );	//PC_SNIPER
	RegisterItem( BG_FindItemForWeapon( WP_TETRION_DISRUPTOR ) );	//PC_HEAVY
	RegisterItem( BG_FindItemForWeapon( WP_QUANTUM_BURST ) );	//PC_HEAVY
	RegisterItem( BG_FindItemForWeapon( WP_SCAVENGER_RIFLE ) );	//PC_DEMO
	RegisterItem( BG_FindItemForWeapon( WP_GRENADE_LAUNCHER ) );//PC_DEMO
	RegisterItem( BG_FindItemForWeapon( WP_DREADNOUGHT ) );		//PC_TECH
	RegisterItem( BG_FindItemForWeapon( WP_VOYAGER_HYPO ) );	//PC_MEDIC
	//items
	RegisterItem( BG_FindItemForHoldable( HI_TRANSPORTER ) );	//PC_INFILTRATOR, PC_BORG
	RegisterItem( BG_FindItemForHoldable( HI_DECOY ) );	//PC_SNIPER
	RegisterItem( BG_FindItemForHoldable( HI_DETPACK ) );	//PC_DEMO
	RegisterItem( BG_FindItemForHoldable( HI_MEDKIT ) );	//PC_MEDIC
	RegisterItem( BG_FindItemForHoldable( HI_SHIELD ) );	//PC_TECH
	//power ups
	RegisterItem( BG_FindItemForPowerup( PW_HASTE ) );	//PC_INFILTRATOR
	RegisterItem( BG_FindItemForPowerup( PW_REGEN ) );	//PC_MEDIC, PC_ACTIONHERO
	RegisterItem( BG_FindItemForPowerup( PW_SEEKER ) );	//PC_TECH. PC_VIP
	RegisterItem( BG_FindItemForPowerup( PW_INVIS ) );	//PC_TECH
	//Tech power stations
	G_ModelIndex( "models/mapobjects/dn/powercell.md3" );
	G_ModelIndex( "models/mapobjects/dn/powercell2.md3" );
	G_SoundIndex( "sound/player/suitenergy.wav" );
	G_SoundIndex( "sound/weapons/noammo.wav" );
	G_SoundIndex( "sound/weapons/explosions/cargoexplode.wav" );
	G_SoundIndex( "sound/items/respawn1.wav" );
}

/*
================
(ModFN) KnockbackMass

Determine class-specific mass value for knockback calculations.
================
*/
static float MOD_PREFIX(KnockbackMass)( MODFN_CTV, gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	if ( targ->client->sess.sessionClass == PC_INFILTRATOR )
		return 100;
	if ( targ->client->sess.sessionClass == PC_HEAVY )
		return 400;
	return MODFN_NEXT( KnockbackMass, ( MODFN_NC, targ, inflictor, attacker, dir, point, damage, dflags, mod ) );
}

/*
==================
(ModFN) AdjustGeneralConstant
==================
*/
static int MOD_PREFIX(AdjustGeneralConstant)( MODFN_CTV, generalConstant_t gcType, int defaultValue ) {
	if ( gcType == GC_ALLOW_BOT_CLASS_SPECIFIER )
		return 1;

	return MODFN_NEXT( AdjustGeneralConstant, ( MODFN_NC, gcType, defaultValue ) );
}

/*
======================
(ModFN) AdjustWeaponConstant

Enable tripmines for the demolitionist's grenade launcher.
======================
*/
static int MOD_PREFIX(AdjustWeaponConstant)( MODFN_CTV, weaponConstant_t wcType, int defaultValue ) {
	if ( wcType == WC_USE_TRIPMINES )
		return 1;
	if ( wcType == WC_GRENADE_SPLASH_RAD )
		return 320;
	if ( wcType == WC_GRENADE_SPLASH_DAM )
		return 150;

	return MODFN_NEXT( AdjustWeaponConstant, ( MODFN_NC, wcType, defaultValue ) );
}

/*
==============
(ModFN) ModifyAmmoUsage

Tripmines use more ammo.
==============
*/
static int MOD_PREFIX(ModifyAmmoUsage)( MODFN_CTV, int defaultValue, int weapon, qboolean alt ) {
	if ( weapon == WP_GRENADE_LAUNCHER && alt ) {
		return 3;
	}

	return MODFN_NEXT( ModifyAmmoUsage, ( MODFN_NC, defaultValue, weapon, alt ) );
}

/*
================
(ModFN) ConvertPlayerModel

Set special player models for specialties classes.
================
*/
static void MOD_PREFIX(ConvertPlayerModel)( MODFN_CTV, int clientNum, const char *userinfo, const char *source_model,
		char *output, unsigned int outputSize ) {
	gclient_t *client = &level.clients[clientNum];
	char *oldModel;

	switch ( client->sess.sessionClass ) {
		case PC_INFILTRATOR:
			oldModel = Info_ValueForKey( userinfo, "model" );
			if ( ModModelGroups_Shared_ListContainsRace( ModModelGroups_Shared_SearchGroupList( oldModel ), "female" ) ) {
				Q_strncpyz( output, "alexandria", outputSize );
			} else {
				Q_strncpyz( output, "munro", outputSize );
			}
			return;
		case PC_SNIPER:
			Q_strncpyz( output, "telsia", outputSize );
			return;
		case PC_HEAVY:
			Q_strncpyz( output, "biessman", outputSize );
			return;
		case PC_DEMO:
			Q_strncpyz( output, "chang", outputSize );
			return;
		case PC_MEDIC:
			Q_strncpyz( output, "jurot", outputSize );
			return;
		case PC_TECH:
			Q_strncpyz( output, "chell", outputSize );
			return;
		default:
			MODFN_NEXT( ConvertPlayerModel, ( MODFN_NC, clientNum, userinfo, source_model, output, outputSize ) );
	}
}

/*
================
(ModFN) ForcefieldTouchResponse

Allow technician to pass through enemy forcefields.
================
*/
static modForcefield_touchResponse_t MOD_PREFIX(ForcefieldTouchResponse)(
		MODFN_CTV, forcefieldRelation_t relation, int clientNum, gentity_t *forcefield ) {
	modForcefield_touchResponse_t response =
			MODFN_NEXT( ForcefieldTouchResponse, ( MODFN_NC, relation, clientNum, forcefield ) );
	if ( ( response == FFTR_BLOCK || response == FFTR_KILL ) &&
			clientNum >= 0 && level.clients[clientNum].sess.sessionClass == PC_TECH ) {
		return FFTR_PASS;
	}
	return response;
}

/*
================
ModSpecialties_Init
================
*/
void ModSpecialties_Init( void ) {
	if ( EF_WARN_ASSERT( !MOD_STATE ) ) {
		modcfg.mods_enabled.specialties = qtrue;
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		G_RegisterTrackedCvar( &MOD_STATE->g_classChangeDebounceTime, "g_classChangeDebounceTime", "180", CVAR_ARCHIVE, qfalse );

		// Register mod functions
		MODFN_REGISTER( InitClientSession, ++modePriorityLevel );
		MODFN_REGISTER( PreClientSpawn, ++modePriorityLevel );
		MODFN_REGISTER( GenerateClientSessionStructure, ++modePriorityLevel );
		MODFN_REGISTER( ModClientCommand, ++modePriorityLevel );
		MODFN_REGISTER( UpdateSessionClass, ++modePriorityLevel );
		MODFN_REGISTER( EffectiveHandicap, ++modePriorityLevel );
		MODFN_REGISTER( SpawnConfigureClient, ++modePriorityLevel );
		MODFN_REGISTER( CheckItemSpawnDisabled, ++modePriorityLevel );
		MODFN_REGISTER( CanItemBeDropped, ++modePriorityLevel );
		MODFN_REGISTER( AddRegisteredItems, ++modePriorityLevel );
		MODFN_REGISTER( KnockbackMass, ++modePriorityLevel );
		MODFN_REGISTER( AdjustGeneralConstant, ++modePriorityLevel );
		MODFN_REGISTER( AdjustWeaponConstant, ++modePriorityLevel );
		MODFN_REGISTER( ModifyAmmoUsage, ++modePriorityLevel );
		MODFN_REGISTER( ConvertPlayerModel, ++modePriorityLevel );
		MODFN_REGISTER( ForcefieldTouchResponse, ++modePriorityLevel );

		ModModelGroups_Init();
		ModModelSelection_Init();

		// Pending item support for demolitionist detpack
		ModPendingItem_Init();

		// For technician forcefield handling
		ModForcefield_Init();
	}
}
