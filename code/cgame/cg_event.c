// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_event.c -- handle entity events at snapshot or playerstate transitions

#include "cg_local.h"
#include "fx_local.h"
#include "cg_text.h"
#include "cg_screenfx.h"

//==========================================================================

/*
===================
CG_PlaceString

Also called by scoreboard drawing
===================
*/
const char	*CG_PlaceString( int rank ) {
	static char	str[64];
	static char	str2[64];
	char	*s, *t;

	if ( rank & RANK_TIED_FLAG ) {
		rank &= ~RANK_TIED_FLAG;
		t = ingame_text[IGT_TIEDFOR];
	} else {
		t = "";
	}

	if ( rank == 1 ) {
//		s = S_COLOR_BLUE "1st" S_COLOR_WHITE;		// draw in blue
		Com_sprintf( str2, sizeof( str2 ), "%s%s%s",S_COLOR_BLUE, ingame_text[IGT_1ST],S_COLOR_WHITE );
		s = str2;
	} else if ( rank == 2 ) {
//		s = S_COLOR_RED "2nd" S_COLOR_WHITE;		// draw in red
		Com_sprintf( str2, sizeof( str2 ), "%s%s%s",S_COLOR_RED, ingame_text[IGT_2ND],S_COLOR_WHITE );
		s = str2;
	} else if ( rank == 3 ) {
//		s = S_COLOR_YELLOW "3rd" S_COLOR_WHITE;		// draw in yellow
		Com_sprintf( str2, sizeof( str2 ), "%s%s%s",S_COLOR_YELLOW, ingame_text[IGT_3RD],S_COLOR_WHITE );
		s = str2;
	} else if ( rank == 11 ) {
//		s = "11th";
		Com_sprintf( str2, sizeof( str2 ), "%s",ingame_text[IGT_11TH]);
		s = str2;
	} else if ( rank == 12 ) {
//		s = "12th";
		Com_sprintf( str2, sizeof( str2 ), "%s",ingame_text[IGT_12TH]);
		s = str2;
	} else if ( rank == 13 ) {
//		s = "13th";
		Com_sprintf( str2, sizeof( str2 ), "%s",ingame_text[IGT_13TH]);
		s = str2;
	} else if ( rank % 10 == 1 ) {
//		s = va("%ist", rank);
		Com_sprintf( str2, sizeof( str2 ), "%i%s",rank,ingame_text[IGT_NUM_ST]);
		s = str2;
	} else if ( rank % 10 == 2 ) {
//		s = va("%ind", rank);
		Com_sprintf( str2, sizeof( str2 ), "%i%s",rank,ingame_text[IGT_NUM_ND]);
		s = str2;
	} else if ( rank % 10 == 3 ) {
//		s = va("%ird", rank);
		Com_sprintf( str2, sizeof( str2 ), "%i%s",rank,ingame_text[IGT_NUM_RD]);
		s = str2;
	} else {
//		s = va("%ith", rank);
		Com_sprintf( str2, sizeof( str2 ), "%i%s",rank,ingame_text[IGT_NUM_TH]);
		s = str2;
	}

	Com_sprintf( str, sizeof( str ), "%s%s", t, s );
	return str;
}

/*
=============
CG_Obituary
=============
*/
static void CG_Obituary( entityState_t *ent ) {
	int			mod;
	int			target, attacker;
	char		*method;
	const char	*targetInfo;
	const char	*attackerInfo;
	char		targetName[32];
	char		attackerName[32];

	target = ent->otherEntityNum;
	attacker = ent->otherEntityNum2;
	mod = ent->eventParm;

	if ( target < 0 || target >= MAX_CLIENTS ) 
	{
		CG_Error( "CG_Obituary: target out of range" );
	}

	if ( attacker < 0 || attacker >= MAX_CLIENTS ) 
	{
		attacker = ENTITYNUM_WORLD;
		attackerInfo = NULL;
	} 
	else 
	{
		attackerInfo = CG_ConfigString( CS_PLAYERS + attacker );
	}

	targetInfo = CG_ConfigString( CS_PLAYERS + target );
	if ( !targetInfo ) 
	{
		return;
	}
	Q_strncpyz( targetName, Info_ValueForKey( targetInfo, "n" ), sizeof(targetName) - 2);
//	strcat( targetName, S_COLOR_WHITE );

	switch( mod ) 
	{
	case MOD_WATER:
		method = ingame_text[IGT_DROWNING];
		break;
	case MOD_SLIME:
		method = ingame_text[IGT_CORROSION];
		break;
	case MOD_LAVA:
		method = ingame_text[IGT_BOILING];
		break;
	case MOD_CRUSH:
		method = ingame_text[IGT_COMPRESSION];
		break;
	case MOD_TELEFRAG:
		method = ingame_text[IGT_TRANSPORTERACCIDENT];
		break;
	case MOD_FALLING:
		method = ingame_text[IGT_IMPACT];
		break;
	case MOD_SUICIDE:
	case MOD_RESPAWN:
		method = ingame_text[IGT_SUICIDE];
		break;
	case MOD_TARGET_LASER:
		method = ingame_text[IGT_LASERBURNS];
		break;
	case MOD_TRIGGER_HURT:
		method = ingame_text[IGT_MISADVENTURE];
		break;
	case MOD_EXPLOSION:
		method = ingame_text[IGT_DESTROYED];
		break;
	case MOD_PHASER:
	case MOD_PHASER_ALT:
		method = ingame_text[IGT_PHASERBURNS];
		break;
	case MOD_CRIFLE:
	case MOD_CRIFLE_SPLASH:
	case MOD_CRIFLE_ALT_SPLASH:
		method = ingame_text[IGT_ENERGYSCARS];
		break;
	case MOD_CRIFLE_ALT:
		method = ingame_text[IGT_SNIPED];
		break;
	case MOD_IMOD:
	case MOD_IMOD_ALT:
		method = ingame_text[IGT_INFINITEMODULATION];
		break;
	case MOD_SCAVENGER:
		method = ingame_text[IGT_GUNNEDDOWN];
		break;
	case MOD_SCAVENGER_ALT:
	case MOD_SCAVENGER_ALT_SPLASH:
		method = ingame_text[IGT_SCAVENGED];
		break;
	case MOD_STASIS:
	case MOD_STASIS_ALT:
		method =  ingame_text[IGT_PERMANENTSTASIS];
		break;
	case MOD_GRENADE:
	case MOD_GRENADE_SPLASH:
		method = ingame_text[IGT_BLASTED];
		break;
	case MOD_GRENADE_ALT:
	case MOD_GRENADE_ALT_SPLASH:
		method = ingame_text[IGT_MINED];
		break;
	case MOD_TETRION:
		method = ingame_text[IGT_PERFORATED];
		break;
	case MOD_TETRION_ALT:
		method = ingame_text[IGT_DISRUPTED];
		break;
	case MOD_DREADNOUGHT:
		method = ingame_text[IGT_WELDED];
		break;
	case MOD_DREADNOUGHT_ALT:
		method = ingame_text[IGT_DEGAUSSED];
		break;
	case MOD_QUANTUM:
	case MOD_QUANTUM_SPLASH:
		method = ingame_text[IGT_DESTROYED];
		break;
	case MOD_QUANTUM_ALT:
	case MOD_QUANTUM_ALT_SPLASH:
		method = ingame_text[IGT_ANNIHILATED];
		break;
	case MOD_DETPACK:
		method = ingame_text[IGT_VAPORIZED];
		break;
	case MOD_KNOCKOUT:
		method = ingame_text[IGT_KNOCKOUT];
		break;
	case MOD_SEEKER:
		method = ingame_text[IGT_AUTOGUNNED];
		break;
	case MOD_ASSIMILATE:
		method = ingame_text[IGT_ASSIMILATED];
		break;
	case MOD_BORG:
	case MOD_BORG_ALT:
		method = ingame_text[IGT_ZAPPED];
		break;
	default:
		method =ingame_text[IGT_UNKNOWN];
		break;
	}


	if ( target == cg.snap->ps.clientNum )
	{
		cg.mod = mod;
	}

	// If killed self, send "Casualty" message.
	if (attacker == target || attacker == ENTITYNUM_WORLD || !attackerInfo) 
	{	// Killed self
		CG_Printf(S_COLOR_CYAN"%s: "S_COLOR_WHITE"%10s   "S_COLOR_CYAN"%s: "S_COLOR_WHITE"%s\n", ingame_text[IGT_CASUALTY],targetName,ingame_text[IGT_METHOD], method);
		return;
	}

	// check for kill messages from the current clientNum
	if ( attacker == cg.snap->ps.clientNum ) {
		char	*s;

		if ( cgs.gametype < GT_TEAM ) 
		{
			s = va("%s %s\n%s %s %i", ingame_text[IGT_YOUELIMINATED],targetName, 
					CG_PlaceString( cg.snap->ps.persistant[PERS_RANK] + 1 ),ingame_text[IGT_PLACEWITH],
					cg.snap->ps.persistant[PERS_SCORE] );
		} 
		else 
		{
			//Assimilated message as well
			if ( mod == MOD_ASSIMILATE )
			{
				s = va("%s %s", ingame_text[IGT_YOUASSIMILATED],targetName );
			}
			else
			{
				s = va("%s %s", ingame_text[IGT_YOUELIMINATED],targetName );
			}
		}
		CG_CenterPrint( s, SCREEN_HEIGHT * 0.25, BIGCHAR_WIDTH );

		// print the text message as well
	}

	// check for double client messages
	if ( !attackerInfo ) 
	{
		attacker = ENTITYNUM_WORLD;
		strcpy( attackerName, "noname" );
	} 
	else 
	{
		Q_strncpyz( attackerName, Info_ValueForKey( attackerInfo, "n" ), sizeof(attackerName) - 2);
//		strcat( attackerName, S_COLOR_WHITE );
		// check for kill messages about the current clientNum
		if ( target == cg.snap->ps.clientNum ) {
			Q_strncpyz( cg.killerName, attackerName, sizeof( cg.killerName ) );
		}
	}

	CG_Printf(S_COLOR_CYAN"%s: "S_COLOR_WHITE"%10s   "S_COLOR_CYAN"%s: "S_COLOR_WHITE"%10s   "S_COLOR_CYAN"%s: "S_COLOR_WHITE"%s\n", 
			ingame_text[IGT_OBITELIMINATED],targetName,ingame_text[IGT_CREDIT],attackerName, ingame_text[IGT_METHOD], method);
}

//==========================================================================

/*
===============
CG_UseItem
===============
*/
static void CG_UseItem( centity_t *cent ) {
	int			itemNum;
	entityState_t *es;

	es = &cent->currentState;
	
	itemNum = (es->event & ~EV_EVENT_BITS) - EV_USE_ITEM0;
	if ( itemNum < 0 || itemNum > HI_NUM_HOLDABLE ) {
		itemNum = 0;
	}
	// print a message if the local player
/*	if ( es->number == cg.snap->ps.clientNum ) {
		if ( !itemNum ) {
			CG_CenterPrint( "No item to use", SCREEN_HEIGHT * 0.25, BIGCHAR_WIDTH );
		} else {
			gitem_t		*item;
			item = BG_FindItemForHoldable( itemNum );
			CG_CenterPrint( va("Use %s", item->pickup_name), SCREEN_HEIGHT * 0.25, BIGCHAR_WIDTH );
		}
	}
*/
	switch ( itemNum ) {
	default:
	case HI_NONE:
		trap_S_StartSound (NULL, es->number, CHAN_BODY, cgs.media.useNothingSound );
		break;

	case HI_TRANSPORTER:
		break;

	case HI_MEDKIT:
		trap_S_StartSound (NULL, es->number, CHAN_BODY, cgs.media.medkitSound );
		break;

	case HI_DETPACK:
		break;

		//--------------------------------------- TEMP DECOY
	case HI_DECOY:
		/*
		if ( es->number == cg.snap->ps.clientNum ) {
			CG_CenterPrint( "Decoy Placed\n",SCREEN_HEIGHT * 0.25, BIGCHAR_WIDTH  );
		}
		*/
		break;
	}

}

/*
================
CG_ItemPickup

A new item was picked up this frame
================
*/
static void CG_ItemPickup( int itemNum ) {
	cg.itemPickup = itemNum;
	cg.itemPickupTime = cg.time;
	cg.itemPickupBlendTime = cg.time;
	// see if it should be the grabbed weapon

	if (cg.snap->ps.stats[STAT_WEAPONS] & (1 << itemNum))
		return;

	if ( bg_itemlist[itemNum].giType == IT_WEAPON )
	{
		int nCurWpn = cg.predictedPlayerState.weapon;
		int nNewWpn = bg_itemlist[itemNum].giTag;

		// kef -- check cg_autoswitch...
		//
		// 0 == no switching
		// 1 == automatically switch to best SAFE weapon
		// 2 == automatically switch to best weapon, safe or otherwise
		//
		// NOTE: automatically switching to any weapon you pick up is stupid and annoying and we won't do it.
		//

		switch(cg_autoswitch.integer)
		{
		case 1:
			// safe switching
			if (	(nNewWpn > nCurWpn) &&
					!(nNewWpn == WP_GRENADE_LAUNCHER) &&
					!(nNewWpn == WP_QUANTUM_BURST) )
			{
				// switch to new wpn
				cg.weaponSelectTime = cg.time;
				cg.weaponSelect = nNewWpn;
			}
			break;
		case 2:
			// best, even if unsafe.
			if (nNewWpn > nCurWpn)
			{
				// switch to new wpn
				cg.weaponSelectTime = cg.time;
				cg.weaponSelect = nNewWpn;
			}
			break;
		case 3:
			// Always switch
			// switch to new wpn
			cg.weaponSelectTime = cg.time;
			cg.weaponSelect = nNewWpn;
			break;
		case 0:
		default:
			// Don't switch.
			break;
		}

	}
}


/*
================
CG_PainEvent

Also called by playerstate transition
================
*/
void CG_PainEvent( centity_t *cent, int health ) {
	char	*snd;

	// don't do more than two pain sounds a second
	if ( cg.time - cent->pe.painTime < 500 ) {
		return;
	}

	if ( health < 25 ) {
		snd = "*pain25.wav";
	} else if ( health < 50 ) {
		snd = "*pain50.wav";
	} else if ( health < 75 ) {
		snd = "*pain75.wav";
	} else {
		snd = "*pain100.wav";
	}
	trap_S_StartSound( NULL, cent->currentState.number, CHAN_VOICE, 
		CG_CustomSound( cent->currentState.number, snd ) );

	// save pain time for programitic twitch animation
	cent->pe.painTime = cg.time;
	cent->pe.painDirection ^= 1;
}

/*
==============
CG_EntityEvent

An entity has an event value
also called by CG_CheckPlayerstateEvents
==============
*/
#define	DEBUGNAME(x) if(cg_debugEvents.integer){CG_Printf(x"\n");}
void CG_EntityEvent( centity_t *cent, vec3_t position ) {
	entityState_t	*es;
	int				event;
	vec3_t			dir;
	const char		*s;
	int				clientNum;
	clientInfo_t	*ci;
	vec3_t		normal = { 0, 0, 1 };

	es = &cent->currentState;
	event = es->event & ~EV_EVENT_BITS;

	if ( cg_debugEvents.integer ) {
		CG_Printf( "ent:%3i  event:%3i ", es->number, event );
	}

	if ( !event ) {
		DEBUGNAME("ZEROEVENT");
		return;
	}

	clientNum = es->clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];

	switch ( event ) {
	//
	// movement generated events
	//
	case EV_FOOTSTEP:
		DEBUGNAME("EV_FOOTSTEP");
		if (cg_footsteps.integer) {
			trap_S_StartSound (NULL, es->number, CHAN_BODY, 
				cgs.media.footsteps[ ci->footsteps ][rand()&3] );
		}
		break;
	case EV_FOOTSTEP_METAL:
		DEBUGNAME("EV_FOOTSTEP_METAL");
		if (cg_footsteps.integer) {
			trap_S_StartSound (NULL, es->number, CHAN_BODY, 
				cgs.media.footsteps[ FOOTSTEP_METAL ][rand()&3] );
		}
		break;
	case EV_FOOTSPLASH:
		DEBUGNAME("EV_FOOTSPLASH");
		if (cg_footsteps.integer) {
			trap_S_StartSound (NULL, es->number, CHAN_BODY, 
				cgs.media.footsteps[ FOOTSTEP_SPLASH ][rand()&3] );
		}
		break;
	case EV_FOOTWADE:
		DEBUGNAME("EV_FOOTWADE");
		if (cg_footsteps.integer) {
			trap_S_StartSound (NULL, es->number, CHAN_BODY, 
				cgs.media.footsteps[ FOOTSTEP_SPLASH ][rand()&3] );
		}
		break;
	case EV_SWIM:
		DEBUGNAME("EV_SWIM");
		if (cg_footsteps.integer) {
			trap_S_StartSound (NULL, es->number, CHAN_BODY, 
				cgs.media.footsteps[ FOOTSTEP_SPLASH ][rand()&3] );
		}
		break;


	case EV_FALL_SHORT:
		DEBUGNAME("EV_FALL_SHORT");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.landSound );
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -8;
			cg.landTime = cg.time;
		}
		break;
	case EV_FALL_MEDIUM:
		DEBUGNAME("EV_FALL_MEDIUM");
		// use normal pain sound trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*pain100.wav" ) );
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -16;
			cg.landTime = cg.time;
		}
		break;
	case EV_FALL_FAR:
		DEBUGNAME("EV_FALL_FAR");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*fall1.wav" ) );
		cent->pe.painTime = cg.time;	// don't play a pain sound right after this
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -24;
			cg.landTime = cg.time;
		}
		break;

	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16:		// smooth out step up transitions
		DEBUGNAME("EV_STEP");
	{
		float	oldStep;
		int		delta;
		int		step;

		if ( clientNum != cg.predictedPlayerState.clientNum ) {
			break;
		}
		// if we are interpolating, we don't need to smooth steps
		if ( cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW) ||
			cg_nopredict.integer || cg_synchronousClients.integer ) {
			break;
		}
		// check for stepping up before a previous step is completed
		delta = cg.time - cg.stepTime;
		if (delta < STEP_TIME) {
			oldStep = cg.stepChange * (STEP_TIME - delta) / STEP_TIME;
		} else {
			oldStep = 0;
		}

		// add this amount
		step = 4 * (event - EV_STEP_4 + 1 );
		cg.stepChange = oldStep + step;
		if ( cg.stepChange > MAX_STEP_CHANGE ) {
			cg.stepChange = MAX_STEP_CHANGE;
		}
		cg.stepTime = cg.time;
		break;
	}

	case EV_JUMP_PAD:
		DEBUGNAME("EV_JUMP_PAD");
		// boing sound at origin, jump sound on player
		trap_S_StartSound ( cent->lerpOrigin, -1, CHAN_VOICE, cgs.media.jumpPadSound );
		trap_S_StartSound (NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*jump1.wav" ) );
		break;

	case EV_JUMP:
		DEBUGNAME("EV_JUMP");
		trap_S_StartSound (NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*jump1.wav" ) );
		break;
	case EV_TAUNT:
		DEBUGNAME("EV_TAUNT");
		if ( ci->numTaunts > 0 )
		{
			if ( ci->pClass == PC_BORG )
			{
				trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_AUTO, CG_CustomSound( es->number, va("*taunt%d.wav", irandom(1, ci->numTaunts) ) ) );
			}
			else
			{
				trap_S_StartSound (NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, va("*taunt%d.wav", irandom(1, ci->numTaunts) ) ) );
			}
		}
		break;
	case EV_WATER_TOUCH:
		DEBUGNAME("EV_WATER_TOUCH");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrInSound );
		break;
	case EV_WATER_LEAVE:
		DEBUGNAME("EV_WATER_LEAVE");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrOutSound );
		break;
	case EV_WATER_UNDER:
		DEBUGNAME("EV_WATER_UNDER");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrUnSound );
		break;
	case EV_WATER_CLEAR:
		DEBUGNAME("EV_WATER_CLEAR");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*gasp.wav" ) );
		break;

	case EV_ITEM_PICKUP:
		DEBUGNAME("EV_ITEM_PICKUP");
		{
			gitem_t	*item;
			int		index;

			index = es->eventParm;		// player predicted

			if ( index < 1 || index >= bg_numItems ) {
				break;
			}
			item = &bg_itemlist[ index ];

			// powerups and team items will have a separate global sound, this one
			// will be played at prediction time
			if ( item->giType == IT_POWERUP)
			{
				trap_S_StartSound (NULL, es->number, CHAN_AUTO,	cgs.media.defaultPickupSound );
			}
			else if (item->pickup_sound && (item->giType != IT_TEAM))
			{
				trap_S_StartSound (NULL, es->number, CHAN_AUTO,	trap_S_RegisterSound( item->pickup_sound ) );
			}

			// show icon and name on status bar
			if ( es->number == cg.snap->ps.clientNum ) {
				CG_ItemPickup( index );
			}
		}
		break;

	case EV_GLOBAL_ITEM_PICKUP:
		DEBUGNAME("EV_GLOBAL_ITEM_PICKUP");
		{
			gitem_t	*item;
			int		index;

			index = es->eventParm;		// player predicted

			if ( index < 1 || index >= bg_numItems ) {
				break;
			}
			item = &bg_itemlist[ index ];

			// if it's a IT_TEAM type, then its a CTF FLAG, and if so, we need to do different sounds for different people
			// not nice to do as a specific sound type, but without rebuilding the item structure there is no choice.
			if (item->giType == IT_TEAM)
			{
				// are we the same client as the one that fired this global sound call off in the first place?
				// if so, we have already handled the sound call in EV_ITEM_PICKUP
				if ( es->otherEntityNum != cg.snap->ps.clientNum )
				{
					clientInfo_t	*us;

					us = &cgs.clientinfo[ cg.snap->ps.clientNum ];
					
					if (us->team != TEAM_SPECTATOR)
					{
						// red or blue?
						if (item->giTag == PW_REDFLAG)
						{
							// red
							if (us->team == TEAM_RED)
							{	// Your flag has been touched.
								trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, cgs.media.ctfTheyStealVoiceSound);
							}
							else
							{
								trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, cgs.media.ctfYouStealVoiceSound);
							}
						}
						else
						{
							// no, Blue!
							if (us->team == TEAM_BLUE)
							{	// Your flag has been touched.
								trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, cgs.media.ctfTheyStealVoiceSound);
							}
							else
							{
								trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, cgs.media.ctfYouStealVoiceSound);
							}
						}
					}
					else
					{	// Spectators should hear generic steal sound with no voice
						trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, cgs.media.ctfStealSound);
					}
				}
				else
				{
					trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, trap_S_RegisterSound( item->pickup_sound ) );
				}

			}
			else if (item->pickup_sound)
			{	// powerup pickups are NOT global anymore, they are in the world.
				if (es->otherEntityNum==cg.snap->ps.clientNum)
				{	// YOU are the one who started this sound.
					trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_ITEM, trap_S_RegisterSound( item->pickup_sound ) );
				}
				else
				{	// Maybe it is too quiet for a pickup?
					trap_S_StartSound (NULL, es->number, CHAN_AUTO, trap_S_RegisterSound( item->pickup_sound ) );
				}
			}

			// show icon and name on status bar
			if ( es->number == cg.snap->ps.clientNum ) {
				CG_ItemPickup( index );
			}
		}
		break;

	//
	// weapon events
	//
	case EV_NOAMMO:
		DEBUGNAME("EV_NOAMMO");
//		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.noAmmoSound );
		if ( es->number == cg.snap->ps.clientNum ) {
			//primary fire switches weapons
			CG_OutOfAmmoChange(qfalse);
		}
		break;
	case EV_NOAMMO_ALT:
		DEBUGNAME("EV_NOAMMO_ALT");
//		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.noAmmoSound );
		if ( es->number == cg.snap->ps.clientNum ) {
			CG_OutOfAmmoChange(qtrue);
//			cg.lowAmmoWarning = 1000;//flash out of ammo message for 1 whole second
//			trap_S_StartLocalSound( cgs.media.noAmmoSound, CHAN_LOCAL_SOUND );
		}
		break;
	case EV_CHANGE_WEAPON:
		DEBUGNAME("EV_CHANGE_WEAPON");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.selectSound );
		break;
	case EV_FIRE_WEAPON:
		DEBUGNAME("EV_FIRE_WEAPON");
		cent->pe.empty = qfalse;
		CG_FireWeapon( cent, qfalse );
		break;
	case EV_ALT_FIRE:
		DEBUGNAME("EV_ALT_FIRE");
		cent->pe.empty = qfalse;
		CG_FireWeapon( cent, qtrue );
		break;

	case EV_USE:
		DEBUGNAME("EV_USE");
		//???
		break;

	case EV_FIRE_EMPTY_PHASER:
		DEBUGNAME("EV_FIRE_EMPTY_PHASER");
		cent->pe.lightningFiring = qtrue;
		cent->pe.empty = qtrue;
		CG_FireWeapon( cent, qfalse );
		break;

	case EV_USE_ITEM0:
	case EV_USE_ITEM1:
	case EV_USE_ITEM2:
	case EV_USE_ITEM3:
	case EV_USE_ITEM4:
	case EV_USE_ITEM5:
	case EV_USE_ITEM6:
	case EV_USE_ITEM7:
	case EV_USE_ITEM8:
	case EV_USE_ITEM9:
	case EV_USE_ITEM10:
	case EV_USE_ITEM11:
	case EV_USE_ITEM12:
	case EV_USE_ITEM13:
	case EV_USE_ITEM14:
	case EV_USE_ITEM15:
		DEBUGNAME("EV_USE_ITEMxx");
		CG_UseItem( cent );
		break;

	//=================================================================

	//
	// other events
	//
	case EV_PLAYER_TELEPORT_IN:
		DEBUGNAME("EV_PLAYER_TELEPORT_IN");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.teleInSound );
		CG_SpawnEffect( position);
		CG_AddFullScreenEffect(SCREENFX_TRANSPORTER, clientNum);
		break;

	case EV_PLAYER_TELEPORT_OUT:
		DEBUGNAME("EV_PLAYER_TELEPORT_OUT");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.teleOutSound );
		CG_SpawnEffect(  position);
		break;

	case EV_BORG_TELEPORT:
		DEBUGNAME("EV_BORG_TELEPORT");
		FX_BorgTeleport( position );
		// FIXME: Hmmm, sound?
		break;

	case EV_ITEM_POP:
		DEBUGNAME("EV_ITEM_POP");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.respawnSound );
		break;
	case EV_ITEM_RESPAWN:
		DEBUGNAME("EV_ITEM_RESPAWN");
		cent->miscTime = cg.time;	// scale up from this
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.respawnSound );
		break;

	case EV_GRENADE_BOUNCE:
		DEBUGNAME("EV_GRENADE_BOUNCE");
		UnVectorShort(cent->currentState.angles2);
		CG_BounceEffect( cent, es->weapon, position, cent->currentState.angles2 );
		break;

	//
	// missile impacts
	//
	case EV_MISSILE_STICK:
		DEBUGNAME("EV_MISSILE_STICK");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.grenadeAltStickSound );
		break;

	case EV_MISSILE_HIT:
		DEBUGNAME("EV_MISSILE_HIT");
		ByteToDir( es->eventParm, dir );
		CG_MissileHitPlayer( cent, es->weapon, position, dir);
		break;

	case EV_MISSILE_MISS:
		DEBUGNAME("EV_MISSILE_MISS");
		ByteToDir( es->eventParm, dir );
		CG_MissileHitWall( cent, es->weapon, position, dir );
		break;

	case EV_COMPRESSION_RIFLE:
		DEBUGNAME("EV_MISSILE_MISS");
		UnVectorShort(cent->currentState.origin2);
		FX_CompressionShot( cent->lerpOrigin, cent->currentState.origin2 );
		break;

	case EV_COMPRESSION_RIFLE_ALT:
		DEBUGNAME("EV_COMPRESSION_RIFLE_ALT");
		UnVectorShort(cent->currentState.origin2);
		FX_CompressionAltShot( cent->lerpOrigin, cent->currentState.origin2 );
		break;

	case EV_IMOD:
		DEBUGNAME("EV_IMOD");
		ByteToDir( es->eventParm, dir );		
		FX_IMODShot( cent->lerpOrigin, cent->currentState.origin2, dir );
		break;

	case EV_IMOD_ALTFIRE:
		DEBUGNAME("EV_IMOD_ALTFIRE");
		ByteToDir( es->eventParm, dir );		
		FX_AltIMODShot( cent->lerpOrigin, cent->currentState.origin2, dir );
		break;

	case EV_IMOD_HIT:
		DEBUGNAME("EV_IMOD_HIT");
		ByteToDir( es->eventParm, dir );
		FX_IMODExplosion( cent->lerpOrigin, dir );
		break;

	case EV_IMOD_ALTFIRE_HIT:
		DEBUGNAME("EV_IMOD_ALTFIRE_HIT");
		ByteToDir( es->eventParm, dir );
		FX_AltIMODExplosion( cent->lerpOrigin, dir );
		break;

	case EV_STASIS:
		DEBUGNAME("EV_STASIS");
		FX_StasisShot( cent, cent->lerpOrigin, cent->currentState.origin2 );
		break;

	case EV_BORG_ALT_WEAPON:
		DEBUGNAME("EV_BORG_ALT_WEAPON");
		FX_BorgTaser( cent->lerpOrigin, cent->currentState.origin2 );
		break;

	case EV_GRENADE_EXPLODE:
		DEBUGNAME("EV_GRENADE_EXPLODE");
		CG_MissileHitWall( cent, WP_GRENADE_LAUNCHER, position, normal );
		break;

	case EV_GRENADE_SHRAPNEL_EXPLODE:
		DEBUGNAME("EV_GRENADE_SHRAPNEL_EXPLODE");
		ByteToDir( es->eventParm, dir );		
		FX_GrenadeShrapnelExplode( position, dir );
		break;

	case EV_GRENADE_SHRAPNEL:
		DEBUGNAME("EV_GRENADE_SHRAPNEL");
		// just for beeping sound
		FX_GrenadeShrapnelBits(cent->lerpOrigin);
		break;

	case EV_DETPACK:
        DEBUGNAME("EV_DETPACK");
		FX_Detpack(cent->lerpOrigin);
		break;

	case EV_DREADNOUGHT_MISS:
		DEBUGNAME("EV_DREADNOUGHT_MISS");
		ByteToDir( es->eventParm, dir );		
		FX_DreadnoughtShotMiss( cent->lerpOrigin, dir );
		break;

	case EV_TETRION:
		DEBUGNAME("EV_TETRION");
		// lerpOrigin == muzzle
		// angles2 == firing direction
		UnVectorShort(cent->currentState.angles2);
	   	FX_TetrionShot( cent->lerpOrigin, cent->currentState.angles2 );
		break;

	case EV_SHIELD_HIT:
		DEBUGNAME("EV_SHIELD_HIT");
		ByteToDir(es->eventParm, dir);
		CG_PlayerShieldHit(es->otherEntityNum, dir, es->time2);
		break;

	//special effects

	case EV_FX_SPARK: 
		DEBUGNAME("EV_FX_SPARK");
		UnVectorShort(cent->currentState.angles);
		CG_Spark( cent->lerpOrigin, cent->currentState.angles, cent->currentState.time2 );
		break;

	case EV_FX_STEAM:
		DEBUGNAME("EV_FX_STEAM");
		UnVectorShort(cent->currentState.angles2);
		CG_Steam( cent->lerpOrigin, cent->currentState.angles2);
		break;

	case EV_FX_BOLT:
		DEBUGNAME("EV_FX_BOLT");
		CG_Bolt( cent );
		break;

	case EV_FX_TRANSPORTER_PAD:
		DEBUGNAME("EV_FX_TRANSPORTER_PAD");
		CG_TransporterPad(cent->lerpOrigin);
		break;

	case EV_FX_DRIP:
		DEBUGNAME("EV_FX_DRIP");
		CG_Drip(cent);
		break;

	case EV_FX_CHUNKS:
		DEBUGNAME("EV_FX_CHUNKS");
		UnVectorShort( cent->currentState.angles2 );
		CG_Chunks( cent->lerpOrigin, cent->currentState.angles2, cent->currentState.time2, cent->currentState.powerups );
		break;

	case EV_SCREENFX_TRANSPORTER:
		DEBUGNAME("EV_FULLSCREEN EFFECT");
		CG_AddFullScreenEffect(SCREENFX_TRANSPORTER, clientNum);
		break;

	case EV_GENERAL_SOUND:
		DEBUGNAME("EV_GENERAL_SOUND");
		if ( cgs.gameSounds[ es->eventParm ] ) {
			trap_S_StartSound (NULL, es->number, CHAN_VOICE, cgs.gameSounds[ es->eventParm ] );
		} else {
			s = CG_ConfigString( CS_SOUNDS + es->eventParm );
			trap_S_StartSound (NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, s ) );
		}
		break;

	case EV_GLOBAL_SOUND:	// play from the player's head so it never diminishes
		DEBUGNAME("EV_GLOBAL_SOUND");
		if ( cgs.gameSounds[ es->eventParm ] ) {
			trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.gameSounds[ es->eventParm ] );
		} else {
			s = CG_ConfigString( CS_SOUNDS + es->eventParm );
			trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_AUTO, CG_CustomSound( es->number, s ) );
		}
		break;

	case EV_TEAM_SOUND:	// play from the player's head so it never diminishes
		DEBUGNAME("EV_TEAM_SOUND");
		{
			clientInfo_t	*us = &cgs.clientinfo[ cg.snap->ps.clientNum ];

			// which kind of team sound is coming through?
			switch (es->eventParm)
			{
			case RETURN_FLAG_SOUND:
				if (us->team != TEAM_SPECTATOR)
				{
					if (us->team == es->otherEntityNum)
					{	// Your flag has been returned!
						trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, cgs.media.ctfYouReturnVoiceSound);
					}
					else
					{
						trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, cgs.media.ctfTheyReturnVoiceSound);
					}
				}
				else
				{	// Spectators should hear a generic return sound.
					trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, cgs.media.ctfReturnSound);
				}
				break;

			case DROPPED_FLAG_SOUND:
				if (us->team != TEAM_SPECTATOR)
				{
					if (us->team == es->otherEntityNum)
					{	// Your flag was dropped by the enemy.
						trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, cgs.media.ctfTheyDroppedVoiceSound);
					}
					else
					{
						trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, cgs.media.ctfYouDroppedVoiceSound);
					}
				}	// Spectators do not hear any sound when a flag is dropped.
				break;

			case SCORED_FLAG_SOUND:
				if (us->team != TEAM_SPECTATOR)
				{
					if (us->team == es->otherEntityNum)
					{	// Your flag has been touched, while you are holding the enemy flag.
						trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, cgs.media.ctfYouScoreVoiceSound);
					}
					else
					{
						trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, cgs.media.ctfTheyScoreVoiceSound);
					}
				}
				else
				{	// Spectators should hear a generic scored sound.
					trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, cgs.media.ctfScoreSound);
				}
				break;

			case SCORED_FLAG_NO_VOICE_SOUND:
				if (us->team == es->otherEntityNum)
				{
					trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.ctfScoreSound);
				}
				else
				{	// Spectators also hear this sound, a generic score sound.
					trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.ctfScoreSound);
				}
				break;
			}
		}
		break;

	case EV_PAIN:
		// local player sounds are triggered in CG_CheckLocalSounds,
		// so ignore events on the player
		DEBUGNAME("EV_PAIN");
		if ( cent->currentState.number != cg.snap->ps.clientNum ) {
			CG_PainEvent( cent, es->eventParm );
		}
		break;

	case EV_DEATH1:
	case EV_DEATH2:
	case EV_DEATH3:
		DEBUGNAME("EV_DEATHx");
		trap_S_StartSound( NULL, es->number, CHAN_VOICE, 
				CG_CustomSound( es->number, va("*death%i.wav", event - EV_DEATH1 + 1) ) );
		break;

	case EV_OBITUARY:
		DEBUGNAME("EV_OBITUARY");
		CG_Obituary( es );
		break;

	case EV_DISINTEGRATION:
		DEBUGNAME("EV_DISINTEGRATION");
		cg_entities[es->number].deathTime = cg.time;
		FX_Disruptor( cent->lerpOrigin, 1000 );
		if (irandom(0,1))
		{
			trap_S_StartSound (NULL, es->number, CHAN_VOICE, cgs.media.disintegrateSound);
		}
		else
		{
			trap_S_StartSound (NULL, es->number, CHAN_VOICE, cgs.media.disintegrate2Sound);
		}
		break;

	case EV_DISINTEGRATION2:
		DEBUGNAME("EV_DISINTEGRATION2");
		cg_entities[es->number].deathTime = cg.time;
		FX_QuantumColumns( cent->lerpOrigin );
		FX_ExplodeBits( cent->lerpOrigin);
		trap_S_StartSound (NULL, es->number, CHAN_VOICE, cgs.media.playerExplodeSound);
		break;

	case EV_EXPLODESHELL:
		DEBUGNAME("EV_EXPLODESHELL");
		cg_entities[es->number].deathTime = cg.time;
		FX_ExplodeBits( cent->lerpOrigin);
		trap_S_StartSound (NULL, es->number, CHAN_VOICE, cgs.media.playerExplodeSound);
		break;

	case EV_ARCWELD_DISINT:
		DEBUGNAME("EV_ARCWELD_DISINT");
		cg_entities[es->number].deathTime = cg.time;
		VectorSubtract( cg.refdef.vieworg, cent->lerpOrigin, dir );
		VectorNormalize( dir );
		break;

	//
	// powerup events
	//
	case EV_POWERUP_BATTLESUIT:
		DEBUGNAME("EV_POWERUP_BATTLESUIT");
		if ( es->number == cg.snap->ps.clientNum ) {
			cg.powerupActive = PW_BATTLESUIT;
			cg.powerupTime = cg.time;
		}
		trap_S_StartSound (NULL, es->number, CHAN_ITEM, cgs.media.invulnoProtectSound );
		break;
	case EV_POWERUP_REGEN:
		DEBUGNAME("EV_POWERUP_REGEN");
		if ( es->number == cg.snap->ps.clientNum ) {
			cg.powerupActive = PW_REGEN;
			cg.powerupTime = cg.time;
		}
		trap_S_StartSound (NULL, es->number, CHAN_ITEM, cgs.media.regenSound );
		break;
	case EV_POWERUP_SEEKER_FIRE:
		DEBUGNAME("EV_POWERUP_SEEKER_FIRE");
		CG_FireSeeker( cent ); 
		break;
	case EV_DEBUG_LINE:
		DEBUGNAME("EV_DEBUG_LINE");
		FX_AddLine(cent->lerpOrigin, cent->currentState.origin2, 1.0, 2.0, 0.0, 1.0, 0.0, 5000.0, (qhandle_t)0);
		break;

	case EV_OBJECTIVE_COMPLETE:
		if ( es->eventParm == 0  )
		{//Special code meaning clear all objectives
			int i;
			for ( i = 0; i < MAX_OBJECTIVES; i++ )
			{
				cgs.objectives[i].complete = qfalse;
			}
			return;
		}
		if ( es->eventParm < 0 || es->eventParm > MAX_OBJECTIVES )
		{//FIXME: error message?
			return;
		}
		cgs.objectives[es->eventParm-1].complete = qtrue;
		break;

	default:
		DEBUGNAME("UNKNOWN");
		CG_Error( "Unknown event: %i", event );
		break;
	}

}


/*
==============
CG_CheckEvents

==============
*/
void CG_CheckEvents( centity_t *cent ) {
	// check for event-only entities
	if ( cent->currentState.eType > ET_EVENTS ) {
		if ( cent->previousEvent ) {
			return;	// already fired
		}
		cent->previousEvent = 1;

		cent->currentState.event = cent->currentState.eType - ET_EVENTS;
	} else {
		// check for events riding with another entity
		if ( cent->currentState.event == cent->previousEvent ) {
			return;
		}
		cent->previousEvent = cent->currentState.event;
		if ( ( cent->currentState.event & ~EV_EVENT_BITS ) == 0 ) {
			return;
		}
	}

	// calculate the position at exactly the frame time
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin );
	CG_SetEntitySoundPosition( cent );

	CG_EntityEvent( cent, cent->lerpOrigin );
}

