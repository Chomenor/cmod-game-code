// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"
#include "cg_text.h"

void CG_Init( int serverMessageNum, int serverCommandSequence );
void CG_Shutdown( void );
void CG_LoadIngameText(void);
void CG_LoadObjectivesForMap(void);
void BG_LoadItemNames(void);

extern void FX_InitSinTable(void);

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
#ifdef _MSC_VER
__declspec(dllexport)
#endif
intptr_t vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6 ) {
	switch ( command ) {
	case CG_INIT:
		CG_Init( arg0, arg1 );
		return 0;
	case CG_SHUTDOWN:
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand();
	case CG_DRAW_ACTIVE_FRAME:
		CG_DrawActiveFrame( arg0, arg1, arg2 );
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();
	case CG_LAST_ATTACKER:
		return CG_LastAttacker();
	default:
		CG_Error( "vmMain: unknown command %i", command );
		break;
	}
	return -1;
}


cg_t				cg;
cgs_t				cgs;
centity_t			cg_entities[MAX_GENTITIES];
weaponInfo_t		cg_weapons[MAX_WEAPONS];
itemInfo_t			cg_items[MAX_ITEMS];

vmCvar_t	cg_centertime;
vmCvar_t	cg_runpitch;
vmCvar_t	cg_runroll;
vmCvar_t	cg_bobup;
vmCvar_t	cg_bobpitch;
vmCvar_t	cg_bobroll;
vmCvar_t	cg_swingSpeed;
vmCvar_t	cg_shadows;
vmCvar_t	cg_gibs;
vmCvar_t	cg_drawTimer;
vmCvar_t	cg_drawFPS;
vmCvar_t	cg_drawSnapshot;
vmCvar_t	cg_draw3dIcons;
vmCvar_t	cg_drawIcons;
vmCvar_t	cg_drawAmmoWarning;
vmCvar_t	cg_drawCrosshair;
vmCvar_t	cg_drawCrosshairNames;
vmCvar_t	cg_drawRewards;
vmCvar_t	cg_crosshairSize;
vmCvar_t	cg_crosshairX;
vmCvar_t	cg_crosshairY;
vmCvar_t	cg_crosshairHealth;
vmCvar_t	cg_draw2D;
vmCvar_t	cg_drawStatus;
vmCvar_t	cg_animSpeed;
vmCvar_t	cg_debugAnim;
vmCvar_t	cg_debugPosition;
vmCvar_t	cg_debugEvents;
vmCvar_t	cg_errorDecay;
vmCvar_t	cg_nopredict;
vmCvar_t	cg_noPlayerAnims;
vmCvar_t	cg_showmiss;
vmCvar_t	cg_footsteps;
vmCvar_t	cg_addMarks;
vmCvar_t	cg_viewsize;
vmCvar_t	cg_drawGun;
vmCvar_t	cg_gun_frame;
vmCvar_t	cg_gun_x;
vmCvar_t	cg_gun_y;
vmCvar_t	cg_gun_z;
vmCvar_t	cg_autoswitch;
vmCvar_t	cg_ignore;
vmCvar_t	cg_simpleItems;
vmCvar_t	cg_fov;
vmCvar_t	cg_zoomFov;
vmCvar_t	cg_thirdPerson;
vmCvar_t	cg_thirdPersonRange;
vmCvar_t	cg_thirdPersonAngle;
vmCvar_t	cg_stereoSeparation;
vmCvar_t	cg_lagometer;
vmCvar_t	cg_drawAttacker;
vmCvar_t	cg_synchronousClients;
vmCvar_t 	cg_teamChatTime;
vmCvar_t 	cg_teamChatHeight;
vmCvar_t 	cg_stats;
vmCvar_t 	cg_reportDamage;
vmCvar_t 	cg_buildScript;
vmCvar_t 	cg_forceModel;
vmCvar_t	cg_paused;
vmCvar_t	cg_blood;
vmCvar_t	cg_predictItems;
vmCvar_t	cg_deferPlayers;
vmCvar_t	cg_drawTeamOverlay;
vmCvar_t	cg_teamOverlayUserinfo;
vmCvar_t	ui_playerclass;

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
} cvarTable_t;

cvarTable_t		cvarTable[] = {
	{ &cg_ignore, "cg_ignore", "0", 0 },	// used for debugging
	{ &cg_autoswitch, "cg_autoswitch", "1", CVAR_ARCHIVE },
	{ &cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE },
	{ &cg_zoomFov, "cg_zoomfov", "22.5", CVAR_ARCHIVE },
	{ &cg_fov, "cg_fov", "80", CVAR_ARCHIVE },
	{ &cg_viewsize, "cg_viewsize", "100", CVAR_ARCHIVE },
	{ &cg_stereoSeparation, "cg_stereoSeparation", "0.4", CVAR_ARCHIVE  },
	{ &cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE  },
	{ &cg_gibs, "cg_gibs", "0", CVAR_ARCHIVE  },	//no gibs in trek
	{ &cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE  },
	{ &cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE  },
	{ &cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE  },
	{ &cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE  },
	{ &cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE  },
	{ &cg_draw3dIcons, "cg_draw3dIcons", "1", CVAR_ARCHIVE  },
	{ &cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE  },
	{ &cg_drawAmmoWarning, "cg_drawAmmoWarning", "1", CVAR_ARCHIVE  },
	{ &cg_drawAttacker, "cg_drawAttacker", "1", CVAR_ARCHIVE  },
	{ &cg_drawCrosshair, "cg_drawCrosshair", "1", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE },
	{ &cg_drawRewards, "cg_drawRewards", "1", CVAR_ARCHIVE },
	{ &cg_crosshairSize, "cg_crosshairSize", "24", CVAR_ARCHIVE },
	{ &cg_crosshairHealth, "cg_crosshairHealth", "1", CVAR_ARCHIVE },
	{ &cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE },
	{ &cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE },
	{ &cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE },
	{ &cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE },
	{ &cg_lagometer, "cg_lagometer", "0", CVAR_ARCHIVE },
	{ &cg_gun_x, "cg_gunX", "0", CVAR_CHEAT },
	{ &cg_gun_y, "cg_gunY", "0", CVAR_CHEAT },
	{ &cg_gun_z, "cg_gunZ", "0", CVAR_CHEAT },
	{ &cg_centertime, "cg_centertime", "3", CVAR_CHEAT },
	{ &cg_runpitch, "cg_runpitch", "0.002", CVAR_ARCHIVE},
	{ &cg_runroll, "cg_runroll", "0.005", CVAR_ARCHIVE },
	{ &cg_bobup , "cg_bobup", "0.005", CVAR_ARCHIVE },
	{ &cg_bobpitch, "cg_bobpitch", "0.002", CVAR_ARCHIVE },
	{ &cg_bobroll, "cg_bobroll", "0.002", CVAR_ARCHIVE },
	{ &cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT },
	{ &cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT },
	{ &cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT },
	{ &cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT },
	{ &cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT },
	{ &cg_errorDecay, "cg_errordecay", "100", 0 },
	{ &cg_nopredict, "cg_nopredict", "0", 0 },
	{ &cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT },
	{ &cg_showmiss, "cg_showmiss", "0", 0 },
	{ &cg_footsteps, "cg_footsteps", "1", CVAR_CHEAT },
	{ &cg_thirdPersonRange, "cg_thirdPersonRange", "40", 0 },
	{ &cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT },
	{ &cg_thirdPerson, "cg_thirdPerson", "0", 0 },
	{ &cg_teamChatTime, "cg_teamChatTime", "3000", CVAR_ARCHIVE  },
	{ &cg_teamChatHeight, "cg_teamChatHeight", "0", CVAR_ARCHIVE  },
	{ &cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE  },
	{ &cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE },
	{ &cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE },
	{ &cg_drawTeamOverlay, "cg_drawTeamOverlay", "0", CVAR_ARCHIVE },
	{ &cg_teamOverlayUserinfo, "teamoverlay", "0", CVAR_ROM | CVAR_USERINFO },
	{ &cg_stats, "cg_stats", "0", 0 },
	{ &cg_reportDamage, "cg_reportDamage", "0", 0},

	// the following variables are created in other parts of the system,
	// but we also reference them here

	{ &cg_buildScript, "com_buildScript", "0", 0 },	// force loading of all possible data amd error on failures
	{ &cg_paused, "cl_paused", "0", CVAR_ROM },
	{ &cg_blood, "com_blood", "0", CVAR_ARCHIVE },	//no blood in trek
	{ &cg_synchronousClients, "g_synchronousClients", "0", 0 },	// communicated by systeminfo
	{ &ui_playerclass, "ui_playerclass", "0", 0 },	// player class
};

int		cvarTableSize = sizeof( cvarTable ) / sizeof( cvarTable[0] );

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	char		var[MAX_TOKEN_CHARS];

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags );
	}

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer( "sv_running", var, sizeof( var ) );
	cgs.localServer = atoi( var );

	// Register the ignored players cvar. We can't use a vmCvar_t anyways as we need
	// way larger string values than just 256.
	trap_Cvar_Register( NULL, IGNORE_CVARNAME, "", CVAR_ROM );
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Update( cv->vmCvar );
	}

	// check for modications here

	// If team overlay is on, ask for updates from the server.  If its off,
	// let the server know so we don't receive it
	if ( drawTeamOverlayModificationCount != cg_drawTeamOverlay.modificationCount ) {
		drawTeamOverlayModificationCount = cg_drawTeamOverlay.modificationCount;

		if ( cg_drawTeamOverlay.integer > 0 ) {
			trap_Cvar_Set( "teamoverlay", "1" );
		} else {
			trap_Cvar_Set( "teamoverlay", "0" );
		}
	}
}


int CG_CrosshairPlayer( void ) {
	if ( cg.time > ( cg.crosshairClientTime + 1000 ) ) {
		return -1;
	}
	return cg.crosshairClientNum;
}


int CG_LastAttacker( void ) {
	if ( !cg.attackerTime ) {
		return -1;
	}
	return cg.snap->ps.persistant[PERS_ATTACKER];
}


void QDECL CG_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	vsprintf (text, msg, argptr);
	va_end (argptr);

	trap_Print( text );
}

void QDECL CG_Error( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	vsprintf (text, msg, argptr);
	va_end (argptr);

	trap_Error( text );
}

#ifndef CGAME_HARD_LINKED
// this is only here so the functions in q_shared.c and bg_*.c can link (FIXME)

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	CG_Error( "%s", text);
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	vsprintf (text, msg, argptr);
	va_end (argptr);

	CG_Printf ("%s", text);
}

#endif



/*
================
CG_Argv
================
*/
const char *CG_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}


//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds( int itemNum ) {
	gitem_t			*item;
	char			data[MAX_QPATH];
	char			*s, *start;
	int				len;

	item = &bg_itemlist[ itemNum ];

	if ( item->pickup_sound )
	{
		trap_S_RegisterSound( item->pickup_sound );
	}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if (!s || !s[0])
		return;

	while (*s) {
		start = s;
		while (*s && *s != ' ') {
			s++;
		}

		len = s-start;
		if (len >= MAX_QPATH || len < 5) {
			CG_Error( "PrecacheItem: %s has bad precache string",
				item->classname);
			return;
		}
		memcpy (data, start, len);
		data[len] = 0;
		if ( *s ) {
			s++;
		}

		if ( !strcmp(data+len-3, "wav" )) {
			trap_S_RegisterSound( data );
		}
	}
}


/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds( void )
{
	int		i;
	char	items[MAX_ITEMS+1];
	char	name[MAX_QPATH];
	const char	*soundName;

	cg.loadLCARSStage = 1;	// Loading bar stage 1
	CG_LoadingString( "sounds" );

	if ( cgs.timelimit || cg_buildScript.integer ) {	// should we always load this?
		cgs.media.oneMinuteSound = trap_S_RegisterSound( "sound/voice/computer/misc/1_minute.wav" );
		cgs.media.fiveMinuteSound = trap_S_RegisterSound( "sound/voice/computer/misc/5_minute.wav" );
		cgs.media.suddenDeathSound = trap_S_RegisterSound( "sound/voice/computer/misc/sudden_death.wav" );
	}

	if ( cgs.fraglimit || cg_buildScript.integer ) {
		cgs.media.oneFragSound = trap_S_RegisterSound( "sound/voice/computer/misc/1_frag.wav" );
		cgs.media.twoFragSound = trap_S_RegisterSound( "sound/voice/computer/misc/2_frags.wav" );
		cgs.media.threeFragSound = trap_S_RegisterSound( "sound/voice/computer/misc/3_frags.wav" );
	}

//	if ( cgs.gametype == GT_TOURNAMENT || cg_buildScript.integer ) {
//  We always need this since a warmup can be enabled in any game mode
		cgs.media.count3Sound = trap_S_RegisterSound( "sound/voice/computer/misc/three.wav" );
		cgs.media.count2Sound = trap_S_RegisterSound( "sound/voice/computer/misc/two.wav" );
		cgs.media.count1Sound = trap_S_RegisterSound( "sound/voice/computer/misc/one.wav" );
		cgs.media.countFightSound = trap_S_RegisterSound( "sound/voice/computer/misc/fight.wav" );
		cgs.media.countPrepareSound = trap_S_RegisterSound( "sound/voice/computer/misc/prepare.wav" );
//	}

	if ( cgs.gametype >= GT_TEAM || cg_buildScript.integer ) {
		cgs.media.redLeadsSound = trap_S_RegisterSound( "sound/voice/computer/misc/redleads.wav" );
		cgs.media.blueLeadsSound = trap_S_RegisterSound( "sound/voice/computer/misc/blueleads.wav" );
		cgs.media.teamsTiedSound = trap_S_RegisterSound( "sound/voice/computer/misc/teamstied.wav" );
		cgs.media.hitTeamSound = trap_S_RegisterSound( "sound/feedback/hit_teammate.wav" );
	}

	if (cgs.gametype == GT_CTF || cg_buildScript.integer)
	{
		cgs.media.ctfStealSound = trap_S_RegisterSound("sound/voice/computer/misc/flagtk_blu.wav");
		cgs.media.ctfReturnSound = trap_S_RegisterSound("sound/voice/computer/misc/flagret_blu.wav");
		cgs.media.ctfScoreSound = trap_S_RegisterSound("sound/voice/computer/misc/flagcap_blu.wav");
		cgs.media.ctfYouStealVoiceSound = trap_S_RegisterSound("sound/voice/computer/misc/stolen.wav");
		cgs.media.ctfYouDroppedVoiceSound = trap_S_RegisterSound("sound/voice/computer/misc/dropped_e.wav");
		cgs.media.ctfYouReturnVoiceSound = trap_S_RegisterSound("sound/voice/computer/misc/returned.wav");
		cgs.media.ctfYouScoreVoiceSound = trap_S_RegisterSound("sound/voice/computer/misc/scored.wav");
		cgs.media.ctfTheyStealVoiceSound = trap_S_RegisterSound("sound/voice/computer/misc/stolen_e.wav");
		cgs.media.ctfTheyDroppedVoiceSound = trap_S_RegisterSound("sound/voice/computer/misc/dropped.wav");	// Note the flip, because YOU dropped THEIR flag
		cgs.media.ctfTheyReturnVoiceSound = trap_S_RegisterSound("sound/voice/computer/misc/returned_e.wav");
		cgs.media.ctfTheyScoreVoiceSound = trap_S_RegisterSound("sound/voice/computer/misc/scored_e.wav");
	}

	cgs.media.interfaceSnd1 = trap_S_RegisterSound( "sound/interface/button4.wav" );

	cgs.media.selectSound = trap_S_RegisterSound( "sound/weapons/change.wav" );
	cgs.media.wearOffSound = trap_S_RegisterSound( "sound/items/wearoff.wav" );
	cgs.media.useNothingSound = trap_S_RegisterSound( "sound/items/use_nothing.wav" );

	cgs.media.holoOpenSound = trap_S_RegisterSound( "sound/movers/doors/holoopen.wav" );
	cgs.media.teleInSound = trap_S_RegisterSound( "sound/world/transin.wav" );
	cgs.media.teleOutSound = trap_S_RegisterSound( "sound/world/transout.wav" );
	cgs.media.respawnSound = trap_S_RegisterSound( "sound/items/respawn1.wav" );

	cgs.media.noAmmoSound = trap_S_RegisterSound( "sound/weapons/noammo.wav" );

	cgs.media.talkSound = trap_S_RegisterSound( "sound/interface/communicator.wav" );
	cgs.media.landSound = trap_S_RegisterSound( "sound/player/land1.wav");

	cgs.media.hitSound = trap_S_RegisterSound( "sound/feedback/hit.wav" );
	cgs.media.shieldHitSound = trap_S_RegisterSound( "sound/feedback/shieldHit.wav" );
	cgs.media.shieldPierceSound = trap_S_RegisterSound( "sound/feedback/shieldPierce.wav" );

	cgs.media.rewardImpressiveSound		= trap_S_RegisterSound( "sound/voice/computer/misc/impressive.wav" );
	cgs.media.rewardExcellentSound		= trap_S_RegisterSound( "sound/voice/computer/misc/excellent.wav" );
	cgs.media.rewardDeniedSound			= trap_S_RegisterSound( "sound/voice/computer/misc/denied.wav" );
	cgs.media.rewardFirstStrikeSound	= trap_S_RegisterSound( "sound/voice/computer/misc/1ststrike.wav");
	cgs.media.rewardAceSound			= trap_S_RegisterSound( "sound/voice/computer/misc/ace.wav");
	cgs.media.rewardExpertSound			= trap_S_RegisterSound( "sound/voice/computer/misc/expert.wav");
	cgs.media.rewardMasterSound			= trap_S_RegisterSound( "sound/voice/computer/misc/master.wav");
	cgs.media.rewardChampionSound		= trap_S_RegisterSound( "sound/voice/computer/misc/champion.wav");

	cgs.media.takenLeadSound = trap_S_RegisterSound( "sound/voice/computer/misc/takenlead.wav");
	cgs.media.tiedLeadSound = trap_S_RegisterSound( "sound/voice/computer/misc/tiedlead.wav");
	cgs.media.lostLeadSound = trap_S_RegisterSound( "sound/voice/computer/misc/lostlead.wav");

	cgs.media.watrInSound = trap_S_RegisterSound( "sound/player/watr_in.wav");
	cgs.media.watrOutSound = trap_S_RegisterSound( "sound/player/watr_out.wav");
	cgs.media.watrUnSound = trap_S_RegisterSound( "sound/player/watr_un.wav");

	cgs.media.jumpPadSound = trap_S_RegisterSound ("sound/items/damage3.wav" );

	cgs.media.poweruprespawnSound = trap_S_RegisterSound ("sound/items/poweruprespawn.wav");
	cgs.media.disintegrateSound = trap_S_RegisterSound( "sound/weapons/prifle/disint.wav" );
	cgs.media.disintegrate2Sound = trap_S_RegisterSound( "sound/weapons/prifle/disint2.wav" );
	cgs.media.playerExplodeSound = trap_S_RegisterSound( "sound/weapons/explosions/fireball.wav" );

	cgs.media.holoInitSound = trap_S_RegisterSound("sound/voice/computer/misc/proginit.wav");
	cgs.media.holoDoorSound = trap_S_RegisterSound("sound/movers/doors/holoopen.wav");
	cgs.media.holoFadeSound = trap_S_RegisterSound("sound/movers/holodeckdecloak.wav");

	cgs.media.phaserEmptySound = trap_S_RegisterSound("sound/weapons/phaser/phaserempty.wav");


	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/step%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/borg%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_BORG][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/reaver%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_REAVER][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/species%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SPECIES][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/warbot%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_WARBOT][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/boot%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_BOOT][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/splash%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/clank%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound (name);
	}

	cg.loadLCARSStage = 2;	// Loading bar stage 2
	CG_LoadingString( "item sounds" );

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		if ( items[ i ] == '1' || cg_buildScript.integer ) {
			CG_RegisterItemSounds( i );
		}
	}

	for ( i = 1 ; i < MAX_SOUNDS ; i++ ) {
		soundName = CG_ConfigString( CS_SOUNDS+i );
		if ( !soundName[0] ) {
			break;
		}
		if ( soundName[0] == '*' ) {
			continue;	// custom sound
		}
		cgs.gameSounds[i] = trap_S_RegisterSound( soundName );
	}

	// FIXME: only needed with item
	cgs.media.flightSound			= trap_S_RegisterSound( "sound/items/flight.wav" );
	cgs.media.medkitSound			= trap_S_RegisterSound ("sound/items/use_medkit.wav");
	cgs.media.quadSound				= trap_S_RegisterSound("sound/items/damage3.wav");
	cgs.media.grenadeExplodeSound	= trap_S_RegisterSound(SOUND_DIR "glauncher/explode.wav");//detpack

	cgs.media.metalChunkSound	= trap_S_RegisterSound( "sound/weapons/explosions/metalexplode.wav" );
	cgs.media.glassChunkSound	= trap_S_RegisterSound( "sound/weapons/explosions/glassbreak1.wav" );
	cgs.media.woodChunkSound	= trap_S_RegisterSound( "sound/weapons/explosions/metalexplode.wav" );
	cgs.media.stoneChunkSound	= trap_S_RegisterSound( "sound/weapons/explosions/metalexplode.wav" );


//	cgs.media.sfx_rockexp = trap_S_RegisterSound ("sound/weapons/rocket/rocklx1a.wav");

	// trek sounds
	cgs.media.envSparkSound1 = trap_S_RegisterSound ("sound/ambience/spark1.wav");
	cgs.media.envSparkSound2 = trap_S_RegisterSound ("sound/ambience/spark2.wav");
	cgs.media.envSparkSound3 = trap_S_RegisterSound ("sound/ambience/spark3.wav");
	cgs.media.defaultPickupSound = trap_S_RegisterSound ("sound/items/n_health.wav");
	cgs.media.invulnoProtectSound = trap_S_RegisterSound("sound/items/protect3.wav");
	cgs.media.regenSound = trap_S_RegisterSound("sound/items/regen.wav");
	cgs.media.waterDropSound1 = trap_S_RegisterSound("sound/ambience/waterdrop1.wav");
	cgs.media.waterDropSound2 = trap_S_RegisterSound("sound/ambience/waterdrop2.wav");
	cgs.media.waterDropSound3 = trap_S_RegisterSound("sound/ambience/waterdrop3.wav");

	// Zoom
	cgs.media.zoomStart = trap_S_RegisterSound( "sound/interface/zoomstart.wav" );
	cgs.media.zoomLoop = trap_S_RegisterSound( "sound/interface/zoomloop.wav" );
	cgs.media.zoomEnd = trap_S_RegisterSound( "sound/interface/zoomend.wav" );
}


//===================================================================================

static void PrecacheAwardsAssets()
{
	// kef -- precaching bot victory sounds (e.g. Desperado_wins.wav) in PlayerModel_BuildList()

	trap_R_RegisterShaderNoMip("menu/medals/medal_efficiency");
	trap_R_RegisterShaderNoMip("menu/medals/medal_sharpshooter");
	trap_R_RegisterShaderNoMip("menu/medals/medal_untouchable");
	trap_R_RegisterShaderNoMip("menu/medals/medal_logistics");
	trap_R_RegisterShaderNoMip("menu/medals/medal_tactician");
	trap_R_RegisterShaderNoMip("menu/medals/medal_demolitionist");
	trap_R_RegisterShaderNoMip("menu/medals/medal_ace");
	trap_R_RegisterShaderNoMip("menu/medals/medal_teammvp");
	trap_R_RegisterShaderNoMip("menu/medals/medal_section31");

	trap_S_RegisterSound("sound/voice/computer/misc/effic.wav");
	trap_S_RegisterSound("sound/voice/computer/misc/sharp.wav");
	trap_S_RegisterSound("sound/voice/computer/misc/untouch.wav");
	trap_S_RegisterSound("sound/voice/computer/misc/log.wav");
	trap_S_RegisterSound("sound/voice/computer/misc/tact.wav");
	trap_S_RegisterSound("sound/voice/computer/misc/demo.wav");
	trap_S_RegisterSound("sound/voice/computer/misc/ace.wav");
	trap_S_RegisterSound("sound/voice/computer/misc/sec31.wav");

	trap_R_RegisterShaderNoMip("menu/medals/medal_teammvp");
	trap_R_RegisterShaderNoMip("menu/medals/medal_teamdefender");
	trap_R_RegisterShaderNoMip("menu/medals/medal_teamwarrior");
	trap_R_RegisterShaderNoMip("menu/medals/medal_teamcarrier");
	trap_R_RegisterShaderNoMip("menu/medals/medal_teaminterceptor");
	trap_R_RegisterShaderNoMip("menu/medals/medal_teambravery");

	trap_S_RegisterSound("sound/voice/computer/misc/mvp.wav");
	trap_S_RegisterSound("sound/voice/computer/misc/defender.wav");
	trap_S_RegisterSound("sound/voice/computer/misc/warrior.wav");
	trap_S_RegisterSound("sound/voice/computer/misc/carrier.wav");
	trap_S_RegisterSound("sound/voice/computer/misc/intercept.wav");
	trap_S_RegisterSound("sound/voice/computer/misc/bravery.wav");

	trap_R_RegisterShaderNoMip("menu/medals/medal_ace");
	trap_R_RegisterShaderNoMip("menu/medals/medal_expert");
	trap_R_RegisterShaderNoMip("menu/medals/medal_master");
	trap_R_RegisterShaderNoMip("menu/medals/medal_champion");

	trap_S_RegisterSound("sound/voice/computer/misc/ace.wav");
	trap_S_RegisterSound("sound/voice/computer/misc/expert.wav");
	trap_S_RegisterSound("sound/voice/computer/misc/master.wav");
	trap_S_RegisterSound("sound/voice/computer/misc/champion.wav");

	trap_S_RegisterSound("sound/voice/computer/misc/commendations.wav");
	trap_S_RegisterSound("sound/voice/computer/misc/progcomp.wav");
	trap_S_RegisterSound("sound/voice/computer/misc/2nd.wav");
	trap_S_RegisterSound("sound/voice/computer/misc/3rd.wav");
	trap_S_RegisterSound("sound/voice/computer/misc/notPlace.wav");
	trap_S_RegisterSound( "sound/voice/computer/misc/youwin.wav" );
	trap_S_RegisterSound( "sound/voice/computer/misc/blueteam_wins.wav" );
	trap_S_RegisterSound( "sound/voice/computer/misc/redteam_wins.wav" );
	trap_S_RegisterSound( "sound/voice/computer/misc/teamstied.wav" );
	trap_S_RegisterSound( "sound/voice/computer/misc/yourteam_wins.wav" );

	trap_R_RegisterShader("icons/icon_ready_on");
	trap_R_RegisterShader("icons/icon_ready_off");
}

/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics( void ) {
	int			i;
	char		items[MAX_ITEMS+1];
	char		temp_skin[100];
	static char		*sb_nums[11] = {
		"gfx/2d/numbers/zero",
		"gfx/2d/numbers/one",
		"gfx/2d/numbers/two",
		"gfx/2d/numbers/three",
		"gfx/2d/numbers/four",
		"gfx/2d/numbers/five",
		"gfx/2d/numbers/six",
		"gfx/2d/numbers/seven",
		"gfx/2d/numbers/eight",
		"gfx/2d/numbers/nine",
		"gfx/2d/numbers/minus",
	};

	static char		*sb_t_nums[11] = {
		"gfx/2d/numbers/t_zero",
		"gfx/2d/numbers/t_one",
		"gfx/2d/numbers/t_two",
		"gfx/2d/numbers/t_three",
		"gfx/2d/numbers/t_four",
		"gfx/2d/numbers/t_five",
		"gfx/2d/numbers/t_six",
		"gfx/2d/numbers/t_seven",
		"gfx/2d/numbers/t_eight",
		"gfx/2d/numbers/t_nine",
		"gfx/2d/numbers/t_minus",
	};


	// clear any references to old media
	memset( &cg.refdef, 0, sizeof( cg.refdef ) );
	trap_R_ClearScene();

	cg.loadLCARSStage = 3;	// Loading bar stage 3
	CG_LoadingString( cgs.mapname );

	trap_R_LoadWorldMap( cgs.mapname );

	// precache status bar pics
	cg.loadLCARSStage = 4;	// Loading bar stage 4
	CG_LoadingString( "game media" );

	for ( i=0 ; i<11 ; i++) {
		cgs.media.numberShaders[i] = trap_R_RegisterShaderNoMip( sb_nums[i] );
		cgs.media.smallnumberShaders[i] = trap_R_RegisterShaderNoMip( sb_t_nums[i] );
	}

	cgs.media.botSkillShaders[0] = trap_R_RegisterShaderNoMip( "menu/art/skill1.tga" );
	cgs.media.botSkillShaders[1] = trap_R_RegisterShaderNoMip( "menu/art/skill2.tga" );
	cgs.media.botSkillShaders[2] = trap_R_RegisterShaderNoMip( "menu/art/skill3.tga" );
	cgs.media.botSkillShaders[3] = trap_R_RegisterShaderNoMip( "menu/art/skill4.tga" );
	cgs.media.botSkillShaders[4] = trap_R_RegisterShaderNoMip( "menu/art/skill5.tga" );

	cgs.media.pClassShaders[PC_NOCLASS] = trap_R_RegisterShaderNoMip( "menu/art/pc_noclass.tga" );//PC_NOCLASS,//default
	if ( cgs.pModSpecialties || cg_buildScript.integer )
	{
		cgs.media.pClassShaders[PC_INFILTRATOR] = trap_R_RegisterShaderNoMip( "menu/art/pc_infiltrator.tga" );//PC_INFILTRATOR,//fast, low attack
		cgs.media.pClassShaders[PC_SNIPER] = trap_R_RegisterShaderNoMip( "menu/art/pc_sniper.tga" );//PC_SNIPER,//sneaky, snipe only
		cgs.media.pClassShaders[PC_HEAVY] = trap_R_RegisterShaderNoMip( "menu/art/pc_heavy.tga" );//PC_HEAVY,//slow, heavy attack
		cgs.media.pClassShaders[PC_DEMO] = trap_R_RegisterShaderNoMip( "menu/art/pc_demo.tga" );//PC_DEMO,//go boom
		cgs.media.pClassShaders[PC_MEDIC] = trap_R_RegisterShaderNoMip( "menu/art/pc_medic.tga" );//PC_MEDIC,//heal
		cgs.media.pClassShaders[PC_TECH] = trap_R_RegisterShaderNoMip( "menu/art/pc_tech.tga" );//PC_TECH,//operate
		cgs.media.pClassShaders[PC_VIP] = trap_R_RegisterShaderNoMip( "menu/art/pc_vip.tga" );//PC_VIP,//for escorts
	}
	if ( cgs.pModActionHero || cg_buildScript.integer )
	{
		cgs.media.pClassShaders[PC_ACTIONHERO] = trap_R_RegisterShaderNoMip( "menu/art/pc_hero.tga" );//PC_ACTIONHERO,//has everything
		cgs.media.heroSpriteShader = trap_R_RegisterShader( "sprites/class_hero" );
	}

	if ( cgs.pModAssimilation || cg_buildScript.integer )
	{//borg beam
		cgs.media.whiteLaserShader		= trap_R_RegisterShader( "gfx/effects/whitelaser" );
		cgs.media.borgEyeFlareShader	= trap_R_RegisterShader( "gfx/misc/borgeyeflare" );
		cgs.media.pClassShaders[PC_BORG] = trap_R_RegisterShaderNoMip( "menu/art/pc_borg.tga" );//PC_BORG,//special weapons, slower, adapting shields
		cgs.media.borgIconShader = trap_R_RegisterShaderNoMip( "icons/icon_borg.tga" );
		cgs.media.borgQueenIconShader = trap_R_RegisterShaderNoMip( "icons/icon_borgqueen.tga" );
	}

	cgs.media.deferShader = trap_R_RegisterShaderNoMip( "gfx/2d/defer.tga" );
	cgs.media.eliminatedShader = trap_R_RegisterShaderNoMip( "gfx/2d/eliminated.tga" );

	cgs.media.smokePuffRageProShader = trap_R_RegisterShader( "smokePuffRagePro" );
	cgs.media.lagometerShader = trap_R_RegisterShader("lagometer" );
	cgs.media.connectionShader = trap_R_RegisterShader( "disconnected" );

	cgs.media.waterBubbleShader = trap_R_RegisterShader( "waterBubble" );

	cgs.media.selectShader = trap_R_RegisterShader( "gfx/2d/select" );

	for ( i = 0 ; i < NUM_CROSSHAIRS ; i++ ) {
		cgs.media.crosshairShader[i] = trap_R_RegisterShaderNoMip( va("gfx/2d/crosshair%c", 'a'+i) );
	}

	cgs.media.backTileShader = trap_R_RegisterShader( "gfx/2d/backtile" );
	cgs.media.noammoShader = trap_R_RegisterShader( "icons/noammo" );

	// powerup shaders
	cgs.media.quadShader				= trap_R_RegisterShader("powerups/quad" );
	cgs.media.quadWeaponShader			= trap_R_RegisterShader("powerups/quadWeapon" );
	cgs.media.battleSuitShader			= trap_R_RegisterShader("powerups/battleSuit" );
	cgs.media.battleWeaponShader		= trap_R_RegisterShader("powerups/battleWeapon" );
	cgs.media.invisShader				= trap_R_RegisterShader("powerups/invisibility" );
	cgs.media.regenShader				= trap_R_RegisterShader("powerups/regen" );
	cgs.media.hastePuffShader			= trap_R_RegisterShader("hasteSmokePuff" );
	cgs.media.flightPuffShader			= trap_R_RegisterShader("flightSmokePuff" );
	cgs.media.borgFullBodyShieldShader	= trap_R_RegisterShader( "gfx/effects/borgfullbodyshield" );
	cgs.media.borgFlareShader			= trap_R_RegisterShader( "gfx/misc/borgflare" );
	cgs.media.disruptorShader		= trap_R_RegisterShader( "powerups/disrupt");
	cgs.media.explodeShellShader	= trap_R_RegisterShader( "powerups/explode");
	cgs.media.quantumDisruptorShader= trap_R_RegisterShader( "powerups/quantum_disruptor_hm");

	cgs.media.seekerModel = trap_R_RegisterModel("models/powerups/trek/flyer.md3" );
	cgs.media.holoDoorModel = trap_R_RegisterModel("models/mapobjects/podium/hm_room.md3" );

	// Used in any explosion-oriented death.
	for (i = 0; i < NUM_CHUNKS; i++)
	{
		cgs.media.chunkModels[MT_METAL][i] = trap_R_RegisterModel( va( "models/chunks/generic/chunks_%i.md3", i+1 ) );
		cgs.media.chunkModels[MT_GLASS][i] = trap_R_RegisterModel( va( "models/chunks/glass/glchunks_%i.md3", i+1 ) );
		cgs.media.chunkModels[MT_WOOD][i] = trap_R_RegisterModel( va( "models/chunks/generic/chunks_%i.md3", i+1 ) );
		cgs.media.chunkModels[MT_STONE][i] = trap_R_RegisterModel( va( "models/chunks/generic/chunks_%i.md3", i+1 ) );
	}

	if ( cgs.gametype == GT_CTF || cg_buildScript.integer ) {
		cgs.media.redFlagModel = trap_R_RegisterModel( "models/flags/flag_red.md3" );//must match bg_misc item and botfiles/items.c
		cgs.media.blueFlagModel = trap_R_RegisterModel( "models/flags/flag_blue.md3" );//must match bg_misc item and botfiles/items.c
		cgs.media.redFlagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_red1" );
		cgs.media.redFlagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_red2" );
		cgs.media.redFlagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_red3" );
		cgs.media.blueFlagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_blu1" );
		cgs.media.blueFlagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_blu2" );
		cgs.media.blueFlagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_blu3" );

		// this determines the normal shaders / skins used by the ctf flags
		if (Q_stricmp("", CG_ConfigString( CS_RED_GROUP)))
		{
			// try loading the group based flag skin
			Com_sprintf(temp_skin, sizeof(temp_skin),"models/flags/%s_red", CG_ConfigString( CS_RED_GROUP));
			cgs.media.redFlagShader[3] = trap_R_RegisterShader3D( temp_skin );
			// did it load?
			if (!cgs.media.redFlagShader[3])
			{
				//no, go with default skin
				cgs.media.redFlagShader[3] = trap_R_RegisterShader3D( "models/flags/default_red" );
			}
		}
		else
		{
			cgs.media.redFlagShader[3] = trap_R_RegisterShader3D( "models/flags/default_red" );
		}

		if (Q_stricmp("", CG_ConfigString( CS_BLUE_GROUP)))
		{
			// try loading the group based flag skin
			Com_sprintf(temp_skin, sizeof(temp_skin),"models/flags/%s_blue", CG_ConfigString( CS_BLUE_GROUP));
			cgs.media.blueFlagShader[3] = trap_R_RegisterShader3D( temp_skin );
			// did it load?
			if (!cgs.media.blueFlagShader[3])
			{
				//no, go with default skin
				cgs.media.blueFlagShader[3] = trap_R_RegisterShader3D( "models/flags/default_blue" );
			}
		}
		else
		{
			cgs.media.blueFlagShader[3] = trap_R_RegisterShader3D( "models/flags/default_blue" );
		}

	}

	if ( cgs.gametype >= GT_TEAM || cg_buildScript.integer ) {
		cgs.media.teamRedShader = trap_R_RegisterShader( "sprites/team_red" );
		cgs.media.teamBlueShader = trap_R_RegisterShader( "sprites/team_blue" );
		cgs.media.redQuadShader = trap_R_RegisterShader("powerups/blueflag" );
		cgs.media.teamStatusBar = trap_R_RegisterShader( "gfx/2d/colorbar.tga" );
	}

	cgs.media.chatShader = trap_R_RegisterShader( "sprites/chat" );

	cgs.media.bloodExplosionShader = trap_R_RegisterShader( "bloodExplosion" );

	cgs.media.ringFlashModel = trap_R_RegisterModel("models/weaphits/ring02.md3");
	cgs.media.teleportEffectModel = trap_R_RegisterModel( "models/misc/telep.md3" );
	cgs.media.teleportEffectShader = trap_R_RegisterShader( "playerTeleport" );

	cgs.media.doorbox = trap_R_RegisterModel( "models/mapobjects/podium/hm_room.md3");

	cgs.media.shieldActivateShaderBlue = trap_R_RegisterShader( "gfx/misc/blue_portashield" );
	cgs.media.shieldDamageShaderBlue = trap_R_RegisterShader( "gfx/misc/blue_dmgshield" );
	cgs.media.shieldActivateShaderRed = trap_R_RegisterShader( "gfx/misc/red_portashield" );
	cgs.media.shieldDamageShaderRed = trap_R_RegisterShader( "gfx/misc/red_dmgshield" );

	cgs.media.weaponPlaceholderShader	= trap_R_RegisterShader("powerups/placeholder" );
	cgs.media.rezOutShader				= trap_R_RegisterShader("powerups/rezout");
	cgs.media.electricBodyShader		= trap_R_RegisterShader("gfx/misc/electric");

	cgs.media.medalImpressive = trap_R_RegisterShaderNoMip( "medal_impressive" );
	cgs.media.medalExcellent = trap_R_RegisterShaderNoMip( "medal_excellent" );
	cgs.media.medalFirstStrike = trap_R_RegisterShaderNoMip( "medal_firststrike" );
	cgs.media.medalAce = trap_R_RegisterShaderNoMip( "medal_ace" );
	cgs.media.medalExpert = trap_R_RegisterShaderNoMip( "medal_expert" );
	cgs.media.medalMaster = trap_R_RegisterShaderNoMip( "medal_master" );
	cgs.media.medalChampion = trap_R_RegisterShaderNoMip( "medal_champion" );

	cgs.media.scoreboardEndcap = trap_R_RegisterShaderNoMip( "menu/common/halfround_r_24");
	cgs.media.corner_12_24 = trap_R_RegisterShaderNoMip( "menu/common/corner_ll_24_12");
	cgs.media.corner_8_16_b = trap_R_RegisterShaderNoMip( "menu/common/corner_lr_8_16_b");

	cgs.media.weaponcap1 = trap_R_RegisterShaderNoMip("gfx/interface/cap4");
	cgs.media.weaponcap2 = trap_R_RegisterShaderNoMip("gfx/interface/cap5");

	cgs.media.weaponbox =  trap_R_RegisterShaderNoMip("gfx/interface/weapon_box");
	cgs.media.weaponbox2 =  trap_R_RegisterShaderNoMip("gfx/interface/weapon_box2");

	memset( cg_items, 0, sizeof( cg_items ) );
	memset( cg_weapons, 0, sizeof( cg_weapons ) );

	cg.loadLCARSStage = 5;	// Loading bar stage 5
	//don't need a 	CG_LoadingString because there will be one in the LoadingItem()

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		if ( items[ i ] == '1' || cg_buildScript.integer ) {
			CG_LoadingItem( i );
			CG_RegisterItemVisuals( i );
		}
	}

	// wall marks
	cgs.media.holeMarkShader = trap_R_RegisterShader( "gfx/damage/hole_lg_mrk" );
	cgs.media.energyMarkShader = trap_R_RegisterShader( "gfx/damage/plasma_mrk" );
	cgs.media.shadowMarkShader = trap_R_RegisterShader( "markShadow" );
	cgs.media.wakeMarkShader = trap_R_RegisterShader( "wake" );

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	for ( i = 1 ; i < cgs.numInlineModels ; i++ ) {
		char	name[10];
		vec3_t			mins, maxs;
		int				j;

		Com_sprintf( name, sizeof(name), "*%i", i );
		cgs.inlineDrawModel[i] = trap_R_RegisterModel( name );
		trap_R_ModelBounds( cgs.inlineDrawModel[i], mins, maxs );
		for ( j = 0 ; j < 3 ; j++ ) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * ( maxs[j] - mins[j] );
		}
	}

	cg.loadLCARSStage = 6;	// Loading bar stage 6
	CG_LoadingString( "Game Models" );

	// register all the server specified models
	for (i=1 ; i<MAX_MODELS ; i++) {
		const char		*modelName;

		modelName = CG_ConfigString( CS_MODELS+i );
		if ( !modelName[0] ) {
			break;
		}
		cgs.gameModels[i] = trap_R_RegisterModel( modelName );
	}


	cg.loadLCARSStage = 7;	// Loading bar stage 7
	CG_LoadingString( "Interface" );

	// Registering interface graphics
	for (i=0;i<IG_MAX;++i)
	{
		if (interface_graphics[i].file)
		{
			interface_graphics[i].graphic = trap_R_RegisterShaderNoMip(interface_graphics[i].file);
		}

		// Turn everything off at first
		if ((interface_graphics[i].type == SG_GRAPHIC) || (interface_graphics[i].type == SG_NUMBER))
		{
			interface_graphics[i].type = SG_OFF;
		}
	}

	interface_graphics[IG_GROW].type = SG_OFF;

	// Trek stuff
	cgs.media.sparkShader				= trap_R_RegisterShader( "gfx/misc/spark" );
	cgs.media.spark2Shader				= trap_R_RegisterShader( "gfx/misc/spark2" );
	cgs.media.sunnyFlareShader			= trap_R_RegisterShader( "gfx/misc/sunny_flare" );
	cgs.media.scavMarkShader			= trap_R_RegisterShader( "gfx/damage/burnmark4" );
	cgs.media.steamShader				= trap_R_RegisterShader( "gfx/misc/steam" );
	cgs.media.smokeShader				= trap_R_RegisterShader( "gfx/misc/smoke" );
	cgs.media.explosionModel				= trap_R_RegisterModel ( "models/weaphits/explosion.md3" );
	cgs.media.electricalExplosionFastShader = trap_R_RegisterShader( "electricalExplosionFast" );
	cgs.media.electricalExplosionSlowShader	= trap_R_RegisterShader( "electricalExplosionSlow" );
	cgs.media.surfaceExplosionShader		= trap_R_RegisterShader( "surfaceExplosion" );
	cgs.media.purpleParticleShader		= trap_R_RegisterShader( "gfx/misc/purpleparticle" );
	cgs.media.blueParticleShader		= trap_R_RegisterShader( "gfx/misc/blueparticle" );
	cgs.media.ltblueParticleShader		= trap_R_RegisterShader( "gfx/misc/ltblueparticle" );
	cgs.media.yellowParticleShader		= trap_R_RegisterShader( "gfx/misc/yellowparticle" );
	cgs.media.orangeParticleShader		= trap_R_RegisterShader( "gfx/misc/orangeparticle" );
	cgs.media.dkorangeParticleShader	= trap_R_RegisterShader( "gfx/misc/dkorangeparticle" );
	cgs.media.redFlareShader			= trap_R_RegisterShader( "gfx/misc/red_flare" );
	cgs.media.redRingShader				= trap_R_RegisterShader( "gfx/misc/red_ring" );
	cgs.media.redRing2Shader			= trap_R_RegisterShader( "gfx/misc/red_ring2" );
	cgs.media.nukeModel					= trap_R_RegisterModel ( "models/weaphits/nuke.md3" );
	cgs.media.bigShockShader			= trap_R_RegisterShader( "gfx/misc/bigshock" );
	cgs.media.IMODMarkShader			= trap_R_RegisterShader( "gfx/damage/burnmark2" );
	cgs.media.bolt2Shader				= trap_R_RegisterShader( "gfx/effects/electrica" );
	cgs.media.holoOuchShader			= trap_R_RegisterShader( "powerups/holoOuch" );
	cgs.media.painBlobShader			= trap_R_RegisterShader( "gfx/misc/painblob" );
	cgs.media.painShieldBlobShader		= trap_R_RegisterShader( "gfx/misc/painshieldblob" );
	cgs.media.shieldBlobShader			= trap_R_RegisterShader( "gfx/misc/shieldblob" );
	cgs.media.halfShieldShader			= trap_R_RegisterShader( "halfShieldShell" );
	cgs.media.holoDecoyShader			= trap_R_RegisterShader( "powerups/holodecoy" );
	cgs.media.trans1Shader				= trap_R_RegisterShader( "gfx/misc/trans1" );
	cgs.media.trans2Shader				= trap_R_RegisterShader( "gfx/misc/trans2" );
	for ( i = 0; i < 4; i++ ) {
		cgs.media.borgLightningShaders[i] = trap_R_RegisterShader( va( "gfx/misc/blightning%i", i+1 ) );
	}
	// detpack
	cgs.media.orangeTrailShader			= trap_R_RegisterShader( "gfx/misc/orangetrail" );
	cgs.media.compressionMarkShader		= trap_R_RegisterShader( "gfx/damage/burnmark1" );
	cgs.media.waterDropShader			= trap_R_RegisterShader( "gfx/misc/drop" );
	cgs.media.oilDropShader				= trap_R_RegisterShader( "gfx/misc/oildrop" );
	cgs.media.greenDropShader			= trap_R_RegisterShader( "gfx/misc/greendrop" );
	cgs.media.detpackPlacedIcon			= trap_R_RegisterShader( "icons/icon_detpack_use"); // placed icon

	// Zoom interface
	cgs.media.zoomMaskShader			= trap_R_RegisterShader( "gfx/misc/zoom_mask2" );
	cgs.media.zoomBarShader				= trap_R_RegisterShader( "gfx/2d/zoom_ctrl" );
	cgs.media.zoomArrowShader			= trap_R_RegisterShader( "gfx/2d/arrow" );
	cgs.media.ammoslider				= trap_R_RegisterShaderNoMip( "gfx/interface/ammobar" );
	cgs.media.zoomInsertShader			= trap_R_RegisterShaderNoMip( "gfx/misc/zoom_insert" );


	cgs.media.testDetpackShader3		= trap_R_RegisterShader( "gfx/misc/detpack3" );
	cgs.media.testDetpackRingShader1	= trap_R_RegisterShader( "gfx/misc/detpackring1" );
	cgs.media.testDetpackRingShader2	= trap_R_RegisterShader( "gfx/misc/detpackring2" );
	cgs.media.testDetpackRingShader3	= trap_R_RegisterShader( "gfx/misc/detpackring3" );
	cgs.media.testDetpackRingShader4	= trap_R_RegisterShader( "gfx/misc/detpackring4" );
	cgs.media.testDetpackRingShader5	= trap_R_RegisterShader( "gfx/misc/detpackring5" );
	cgs.media.testDetpackRingShader6	= trap_R_RegisterShader( "gfx/misc/detpackring6" );

	if (cg_buildScript.integer)
	{
		PrecacheAwardsAssets();
	}
}

/*
===================
CG_RegisterClients

===================
*/
static void CG_RegisterClients( void ) {
	int		i;

	cg.loadLCARSStage = 8;	// Loading bar stage 8
	CG_LoadingString( "clients" );

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		const char		*clientInfo;

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0] ) {
			continue;
		}
		CG_LoadingClient( i );
		CG_NewClientInfo( i );
	}
	cg.loadLCARSStage = 9;	// Loading bar stage 9
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char *CG_ConfigString( int index ) {
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		CG_Error( "CG_ConfigString: bad index: %i", index );
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[ index ];
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic( void ) {
	char	*s;
	char	parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char *)CG_ConfigString( CS_MUSIC );
	Q_strncpyz( parm1, COM_Parse( &s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( &s ), sizeof( parm2 ) );

	trap_S_StartBackgroundTrack( parm1, parm2 );
}

extern int altAmmoUsage[];
void CG_InitModRules( void )
{
	if ( cgs.pModDisintegration )
	{//don't use up ammo in disintegration mode
		altAmmoUsage[WP_COMPRESSION_RIFLE] = 0;
	}
	if ( cgs.pModSpecialties )
	{//tripwires use more ammo
		altAmmoUsage[WP_GRENADE_LAUNCHER] = 3;
	}
}

/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
void CG_Init( int serverMessageNum, int serverCommandSequence ) {
	const char	*s;

	// clear everything
	memset( &cgs, 0, sizeof( cgs ) );
	memset( &cg, 0, sizeof( cg ) );
	memset( cg_entities, 0, sizeof(cg_entities) );
	memset( cg_weapons, 0, sizeof(cg_weapons) );
	memset( cg_items, 0, sizeof(cg_items) );

	init_tonextint(qfalse);

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	CG_LoadIngameText();

	CG_LoadFonts();

	// Loading graphics
	cgs.media.loading1		= trap_R_RegisterShaderNoMip( "menu/loading/smpiece1.tga" );
	cgs.media.loading2		= trap_R_RegisterShaderNoMip( "menu/loading/smpiece2.tga" );
	cgs.media.loading3		= trap_R_RegisterShaderNoMip( "menu/loading/smpiece3.tga" );
	cgs.media.loading4		= trap_R_RegisterShaderNoMip( "menu/loading/smpiece4.tga" );
	cgs.media.loading5		= trap_R_RegisterShaderNoMip( "menu/loading/smpiece5.tga" );
	cgs.media.loading6		= trap_R_RegisterShaderNoMip( "menu/loading/smpiece6.tga" );
	cgs.media.loading7		= trap_R_RegisterShaderNoMip( "menu/loading/smpiece7.tga" );
	cgs.media.loading8		= trap_R_RegisterShaderNoMip( "menu/loading/smpiece8.tga" );
	cgs.media.loading9		= trap_R_RegisterShaderNoMip( "menu/loading/smpiece9.tga" );
	cgs.media.loadingcircle = trap_R_RegisterShaderNoMip( "menu/loading/arrowpiece.tga" );
	cgs.media.loadingquarter= trap_R_RegisterShaderNoMip( "menu/loading/quarter.tga" );
	cgs.media.loadingcorner	= trap_R_RegisterShaderNoMip( "menu/common/corner_lr_8_16.tga" );
	cgs.media.loadingtrim	= trap_R_RegisterShaderNoMip( "menu/loading/trimupper.tga" );
	cgs.media.circle		= trap_R_RegisterShaderNoMip( "menu/common/circle.tga" );
	cgs.media.circle2		= trap_R_RegisterShaderNoMip( "menu/objectives/circle_out.tga" );
	cgs.media.corner_12_18	= trap_R_RegisterShaderNoMip( "menu/common/corner_ll_12_18.tga" );
	cgs.media.halfroundr_22	= trap_R_RegisterShaderNoMip( "menu/common/halfroundr_22.tga" );

	cgs.media.corner_ul_20_30= trap_R_RegisterShaderNoMip( "menu/common/corner_ul_20_30.tga" );
	cgs.media.corner_ll_8_30= trap_R_RegisterShaderNoMip( "menu/common/corner_ll_8_30.tga" );

	cg.loadLCARSStage		= 0;
	cg.loadLCARScnt			= 0;
	// load a few needed things before we do any screen updates
	cgs.media.charsetShader		= trap_R_RegisterShaderNoMip( "gfx/2d/charsgrid_med" );
	cgs.media.whiteShader		= trap_R_RegisterShader( "white" );
	cgs.media.charsetPropTiny = trap_R_RegisterShaderNoMip("gfx/2d/chars_tiny");
	cgs.media.charsetProp		= trap_R_RegisterShaderNoMip("gfx/2d/chars_medium");
	cgs.media.charsetPropBig	= trap_R_RegisterShaderNoMip("gfx/2d/chars_big");
//	cgs.media.charsetPropGlow	= trap_R_RegisterShaderNoMip( "menu/art/font1_prop_glo.tga" );
	cgs.media.charsetPropB		= trap_R_RegisterShaderNoMip( "gfx/2d/chars_medium.tga" );

	CG_RegisterCvars();

	CG_InitConsoleCommands();

	BG_LoadItemNames();

	cg.weaponSelect = WP_PHASER;

	cgs.redflag = cgs.blueflag = -1; // For compatibily, default to unset for
	// old servers

	// get the rendering configuration from the client system
	trap_GetGlconfig( &cgs.glconfig );
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0;

	// get the gamestate from the client system
	trap_GetGameState( &cgs.gameState );

	// check version
	s = CG_ConfigString( CS_GAME_VERSION );
	if ( strcmp( s, GAME_VERSION ) ) {
		CG_Error( "Client/Server game mismatch: %s/%s", GAME_VERSION, s );
	}

	s = CG_ConfigString( CS_LEVEL_START_TIME );
	cgs.levelStartTime = atoi( s );

	CG_ParseServerinfo();

	CG_ParseModConfig();

	// load the new map
	CG_LoadingString( "collision map" );

	trap_CM_LoadMap( cgs.mapname );

	cg.loading = qtrue;		// force players to load instead of defer

	CG_LoadObjectivesForMap();

	CG_RegisterSounds();

	CG_RegisterGraphics();

	CG_RegisterClients();		// if low on memory, some clients will be deferred

	cg.loading = qfalse;	// future players will be deferred

	CG_InitLocalEntities();

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	CG_LoadingString( "" );

	// To get the interface timing started
	cg.interfaceStartupTime = 0;
	cg.interfaceStartupDone = 0;

	CG_InitModRules();
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown( void ) {
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
}


#define MAXINGAMETEXT 5000
char ingameText[MAXINGAMETEXT];

/*
=================
CG_ParseIngameText
=================
*/
void CG_ParseIngameText(void)
{
	char	*token;
	char *buffer;
	int i;
	int len;

	COM_BeginParseSession();

	buffer = ingameText;
	i = 1;	// Zero is null string
	while ( buffer )
	{
		token = COM_ParseExt( &buffer, qtrue );

		len = strlen(token);
		if (len)
		{
			ingame_text[i] = (buffer - (len + 1));	// The +1 is to get rid of the " at the beginning of the sting.
			*(buffer - 1) = '\0';		//	Place an string end where is belongs.

			++i;
		}

		if (i> IGT_MAX)
		{
			Com_Printf( S_COLOR_RED "CG_ParseIngameText : too many values!\n");
			return;
		}
	}

	if (i != IGT_MAX)
	{
		Com_Printf( S_COLOR_RED "CG_ParseIngameText : not enough lines! Read %d of %d!\n",i,IGT_MAX);
		for(;i<IGT_MAX;i++) {
			ingame_text[i] = "?";
		}
	}

}

/*
CG_LanguageFilename - create a filename with an extension based on the value in g_language
*/
void CG_LanguageFilename(char *baseName,char *baseExtension,char *finalName)
{
	char	language[32];
	fileHandle_t	file;

	trap_Cvar_VariableStringBuffer("g_language", language, 32 );

	// If it's English then no extension
	if (language[0]=='\0' || Q_stricmp ("ENGLISH",language)==0)
	{
		Com_sprintf(finalName,MAX_QPATH,"%s.%s",baseName,baseExtension);
	}
	else
	{
		Com_sprintf(finalName,MAX_QPATH,"%s_%s.%s",baseName,language,baseExtension);

		//Attempt to load the file
		trap_FS_FOpenFile( finalName, &file, FS_READ );

		if ( file == 0 )	//	This extension doesn't exist, go English.
		{
			Com_sprintf(finalName,MAX_QPATH,"%s.%s",baseName,baseExtension);	//the caller will give the error if this isn't there
		}
		else
		{
			trap_FS_FCloseFile( file );
		}
	}
}

/*
=================
CG_LoadIngameText
=================
*/
void CG_LoadIngameText(void)
{
	int len;
	fileHandle_t	f;
	char fileName[MAX_QPATH];

	CG_LanguageFilename("ext_data/mp_ingametext","dat",fileName);

	len = trap_FS_FOpenFile( fileName, &f, FS_READ );

	if ( !f )
	{
		Com_Printf( S_COLOR_RED "CG_LoadIngameText : mp_ingametext.dat file not found!\n");
		return;
	}

	if (len > MAXINGAMETEXT)
	{
		Com_Printf( S_COLOR_RED "CG_LoadIngameText : mp_ingametext.dat file bigger than %d!\n",MAXINGAMETEXT);
		return;
	}

	// initialise the data area
	memset(ingameText, 0, sizeof(ingameText));

	trap_FS_Read( ingameText, len, f );

	trap_FS_FCloseFile( f );


	CG_ParseIngameText();

}

/*
=================
CG_LoadObjectivesForMap
=================
*/
void CG_LoadObjectivesForMap(void)
{
	int		len, objnum = 0;
	char	*token;
	char	*buf;
	fileHandle_t	f;
	char	fileName[MAX_QPATH];
	char	fullFileName[MAX_QPATH];
	char	objtext[MAX_OBJ_TEXT_LENGTH];

	COM_StripExtension(cgs.mapname, fileName, sizeof(fileName));
	CG_LanguageFilename( fileName, "efo", fullFileName);

	len = trap_FS_FOpenFile( fullFileName, &f, FS_READ );

	if ( len > MAX_OBJ_TEXT_LENGTH )
	{
		Com_Printf( S_COLOR_RED "CG_LoadObjectivesForMap : %s file bigger than %d!\n", fileName, MAX_OBJ_TEXT_LENGTH );
		return;
	}

	trap_FS_Read( objtext, len, f );

	trap_FS_FCloseFile( f );

	buf = objtext;
	//Now parse out each objective
	while ( 1 )
	{
		token = COM_ParseExt( &buf, qtrue );
		if ( !token[0] ) {
			break;
		}

		// Normal objective text
		if ( !Q_strncmp( token, "obj", 3 ) )
		{
			objnum = atoi( &token[3] );

			if ( objnum < 1 || objnum == MAX_OBJECTIVES ) {
				Com_Printf( "Invalid objective number (%d), valid range is 1 to %d\n", objnum, MAX_OBJECTIVES );
				break;
			}

			//Now read the objective text into the current objective
			token = COM_ParseExt( &buf, qfalse );
			Q_strncpyz( cgs.objectives[objnum-1].text, token, sizeof(cgs.objectives[objnum-1].text) );
		}

		else if ( !Q_strncmp( token, "abridged_obj", 12 ) )
		{
			objnum = atoi( &token[12] );

			if ( objnum < 1 || objnum == MAX_OBJECTIVES )
			{
				Com_Printf( "Invalid objective number (%d), valid range is 1 to %d\n", objnum, MAX_OBJECTIVES );
				break;
			}

			//Now read the objective text into the current objective
			token = COM_ParseExt( &buf, qfalse );
			Q_strncpyz( cgs.objectives[objnum-1].abridgedText, token, sizeof(cgs.objectives[objnum-1].abridgedText) );
		}
	}
}
