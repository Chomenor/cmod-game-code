//
// cg_players.c -- handle the media and animation for player entities

#include "cg_local.h"
#include "cg_screenfx.h"
#include "fx_local.h"


const char	*cg_customSoundNames[MAX_CUSTOM_SOUNDS] = {
	"*death1.wav",
	"*death2.wav",
	"*death3.wav",
	"*jump1.wav",
	"*pain25.wav",
	"*pain50.wav",
	"*pain75.wav",
	"*pain100.wav",
	"*falling1.wav",
	"*gasp.wav",
	"*drown.wav",
	"*fall1.wav",
	"*taunt1.wav",
	"*taunt2.wav",
	"*taunt3.wav",
	"*taunt4.wav",
	"*taunt5.wav"
};


int timeParam;
int entNum;


/*
================
CG_CustomSound

================
*/
sfxHandle_t	CG_CustomSound( int clientNum, const char *soundName ) {
	clientInfo_t *ci;
	int			i;

	if ( soundName[0] != '*' ) {
		return trap_S_RegisterSound( soundName );
	}

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];

	for ( i = 0 ; i < MAX_CUSTOM_SOUNDS && cg_customSoundNames[i] ; i++ ) {
		if ( !strcmp( soundName, cg_customSoundNames[i] ) ) {
			return ci->sounds[i];
		}
	}

	CG_Error( "Unknown custom sound: %s", soundName );
	return 0;
}



/*
=============================================================================

CLIENT INFO

=============================================================================
*/

/*
======================
CG_ParseAnimationFile

Read a configuration file containing animation coutns and rates
models/players2/munro/animation.cfg, etc
======================
*/
static qboolean	CG_ParseAnimationFile( const char *filename, clientInfo_t *ci ) {
	char		*text_p, *prev;
	int			len;
	int			i;
	char		*token;
	float		fps;
	int			skip;
	char		text[20000];
	fileHandle_t	f;
	animation_t *animations;

	animations = ci->animations;

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return qfalse;
	}
	if ( len >= sizeof( text ) - 1 ) {
		CG_Printf( "File %s too long\n", filename );
		return qfalse;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;
	skip = 0;	// quite the compiler warning

	ci->footsteps = FOOTSTEP_NORMAL;
	VectorClear( ci->headOffset );
	ci->gender = GENDER_MALE;

	Q_strncpyz(ci->soundPath, ci->modelName, sizeof(ci->soundPath));

	// read optional parameters
	while ( 1 ) {
		prev = text_p;	// so we can unget
		token = COM_Parse( &text_p );
		if ( !token[0] ) {
			break;
		}
		if ( !Q_stricmp( token, "footsteps" ) ) {
			token = COM_Parse( &text_p );
			if ( !token[0] ) {
				break;
			}
			if ( !Q_stricmp( token, "default" ) || !Q_stricmp( token, "normal" ) ) {
				ci->footsteps = FOOTSTEP_NORMAL;
			} else if ( !Q_stricmp( token, "borg" ) ) {
				ci->footsteps = FOOTSTEP_BORG;
			} else if ( !Q_stricmp( token, "reaver" ) ) {
				ci->footsteps = FOOTSTEP_REAVER;
			} else if ( !Q_stricmp( token, "species" ) ) {
				ci->footsteps = FOOTSTEP_SPECIES;
			} else if ( !Q_stricmp( token, "warbot" ) ) {
				ci->footsteps = FOOTSTEP_WARBOT;
			} else if ( !Q_stricmp( token, "boot" ) ) {
				ci->footsteps = FOOTSTEP_BOOT;
			} else if ( !Q_stricmp( token, "flesh" ) ) {	// Old Q3 defaults, for compatibility.	-PJL
				ci->footsteps = FOOTSTEP_SPECIES;
			} else if ( !Q_stricmp( token, "mech" ) ) {		// Ditto
				ci->footsteps = FOOTSTEP_BORG;
			} else if ( !Q_stricmp( token, "energy" ) ) {	// Ditto
				ci->footsteps = FOOTSTEP_BORG;
			} else {
				CG_Printf( "Bad footsteps parm in %s: %s\n", filename, token );
			}
			continue;
		} else if ( !Q_stricmp( token, "headoffset" ) ) {
			for ( i = 0 ; i < 3 ; i++ ) {
				token = COM_Parse( &text_p );
				if ( !token[0] ) {
					break;
				}
				ci->headOffset[i] = atof( token );
			}
			continue;
		} else if ( !Q_stricmp( token, "sex" ) ) {
			token = COM_Parse( &text_p );
			if ( !token[0] ) {
				break;
			}
			if ( token[0] == 'f' || token[0] == 'F' ) {
				ci->gender = GENDER_FEMALE;
			} else if ( token[0] == 'n' || token[0] == 'N' ) {
				ci->gender = GENDER_NEUTER;
			} else {
				ci->gender = GENDER_MALE;
			}
			continue;
		} else if ( !Q_stricmp( token, "soundpath" ) ) {
			token = COM_Parse( &text_p );
			if ( !token[0] ) {
				break;
			}
			Q_strncpyz(ci->soundPath,token,sizeof (ci->soundPath) );
			continue;
		}

		// if it is a number, start parsing animations
		if ( token[0] >= '0' && token[0] <= '9' ) {
			text_p = prev;	// unget the token
			break;
		}
		Com_Printf( "unknown token '%s' is %s\n", token, filename );
	}

	// read information for each frame
	for ( i = 0 ; i < MAX_ANIMATIONS ; i++ ) {

		token = COM_Parse( &text_p );
		if ( !token[0] ) {
			break;
		}
		animations[i].firstFrame = atoi( token );
		// leg only frames are adjusted to not count the upper body only frames
		if ( i == LEGS_WALKCR ) {
			skip = animations[LEGS_WALKCR].firstFrame - animations[TORSO_GESTURE].firstFrame;
		}
		if ( i >= LEGS_WALKCR ) {
			animations[i].firstFrame -= skip;
		}

		token = COM_Parse( &text_p );
		if ( !token[0] ) {
			break;
		}
		animations[i].numFrames = atoi( token );

		token = COM_Parse( &text_p );
		if ( !token[0] ) {
			break;
		}
		animations[i].loopFrames = atoi( token );

		token = COM_Parse( &text_p );
		if ( !token[0] ) {
			break;
		}
		fps = atof( token );
		if ( fps == 0 ) {
			fps = 1;
		}
		animations[i].frameLerp = 1000 / fps;
		animations[i].initialLerp = 1000 / fps;
	}

	if ( i != MAX_ANIMATIONS ) {
		CG_Printf( "Error parsing animation file: %s", filename );
		return qfalse;
	}

	return qtrue;
}

/*
==========================
CG_RegisterClientSkin
==========================
*/
static qboolean	CG_RegisterClientSkin( clientInfo_t *ci, const char *modelName, const char *skinName ) {
	char		filename[MAX_QPATH];

	Com_sprintf( filename, sizeof( filename ), "models/players2/%s/lower_%s.skin", modelName, skinName );
	ci->legsSkin = trap_R_RegisterSkin( filename );

	Com_sprintf( filename, sizeof( filename ), "models/players2/%s/upper_%s.skin", modelName, skinName );
	ci->torsoSkin = trap_R_RegisterSkin( filename );

	Com_sprintf( filename, sizeof( filename ), "models/players2/%s/head_%s.skin", modelName, skinName );
	ci->headSkin = trap_R_RegisterSkin( filename );

	if ( !ci->legsSkin || !ci->torsoSkin || !ci->headSkin ) {
		return qfalse;
	}

	return qtrue;
}

/*
==========================
CG_RegisterClientModelname
==========================
*/
static qboolean CG_RegisterClientModelname( clientInfo_t *ci, const char *modelName, const char *skinName ) {
	char		filename[MAX_QPATH];

	// load cmodels before models so filecache works

	Com_sprintf( filename, sizeof( filename ), "models/players2/%s/lower.mdr", modelName );
	ci->legsModel = trap_R_RegisterModel( filename );
	if ( !ci->legsModel ) {
		Com_sprintf( filename, sizeof( filename ), "models/players2/%s/lower.md3", modelName );
		ci->legsModel = trap_R_RegisterModel( filename );
		if ( !ci->legsModel ) {
			Com_Printf( S_COLOR_RED"Failed to load model file %s\n", filename );
			return qfalse;
		}
	}
	Com_sprintf( filename, sizeof( filename ), "models/players2/%s/upper.mdr", modelName );
	ci->torsoModel = trap_R_RegisterModel( filename );
	if ( !ci->torsoModel ) {
		Com_sprintf( filename, sizeof( filename ), "models/players2/%s/upper.md3", modelName );
		ci->torsoModel = trap_R_RegisterModel( filename );
		if ( !ci->torsoModel ) {
			Com_Printf( "Failed to load model file %s\n", filename );
			return qfalse;
		}
	}

	Com_sprintf( filename, sizeof( filename ), "models/players2/%s/head.md3", modelName );
	ci->headModel = trap_R_RegisterModel( filename );
	if ( !ci->headModel ) {
		Com_Printf( "Failed to load model file %s\n", filename );
		return qfalse;
	}

	// if any skins failed to load, return failure
	if ( !CG_RegisterClientSkin( ci, modelName, skinName ) ) {
		Com_Printf( "Failed to load skin file: %s : %s\n", modelName, skinName );
		return qfalse;
	}

	// load the animations
	Com_sprintf( filename, sizeof( filename ), "models/players2/%s/animation.cfg", modelName );
	if ( !CG_ParseAnimationFile( filename, ci ) ) {
		Com_Printf( "Failed to load animation file %s\n", filename );
		return qfalse;
	}

	Com_sprintf( filename, sizeof( filename ), "models/players2/%s/icon_%s.jpg", modelName, skinName );
	ci->modelIcon = trap_R_RegisterShaderNoMip( filename );
	if ( !ci->modelIcon ) {
		Com_Printf( "Failed to load icon file: %s\n", filename );
		return qfalse;
	}

	return qtrue;
}

/*
====================
CG_ColorFromString
====================
*/
static void CG_ColorFromString( const char *v, vec3_t color ) {
	int val;

	VectorClear( color );

	val = atoi( v );

	if ( val < 1 || val > 7 ) {
		VectorSet( color, 1, 1, 1 );
		return;
	}

	if ( val & 1 ) {
		color[2] = 1.0f;
	}
	if ( val & 2 ) {
		color[1] = 1.0f;
	}
	if ( val & 4 ) {
		color[0] = 1.0f;
	}
}

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits.
This will usually be deferred to a safe time
===================
*/
static void CG_LoadClientInfo( clientInfo_t *ci , int clientNum) {
	const char	*dir, *fallback;
	int			i;
	const char	*s;
	char		temp_string[200];
	qboolean	noMoreTaunts, loadingTaunt;

	if ( !CG_RegisterClientModelname( ci, ci->modelName, ci->skinName ) ) {
		if ( cg_buildScript.integer ) {
			CG_Error( "CG_RegisterClientModelname( %s, %s ) failed", ci->modelName, ci->skinName );
		}

		// fall back
		if ( !( cgs.gametype >= GT_TEAM && CG_RegisterClientModelname( ci, DEFAULT_MODEL, ci->skinName ) ) &&
				!CG_RegisterClientModelname( ci, DEFAULT_MODEL, "default" ) ) {
			CG_Error( "DEFAULT_MODEL (%s) failed to register", DEFAULT_MODEL );
		}
	}

	// sounds
	dir = ci->soundPath;
	fallback = (ci->gender==GENDER_FEMALE)?"hm_female":"hm_male";

	ci->numTaunts = 0;
	noMoreTaunts = qfalse;
	for ( i = 0 ; i < MAX_CUSTOM_SOUNDS ; i++ ) {
		s = cg_customSoundNames[i];
		if ( !s ) {
			break;
		}
		if ( strstr( s, "taunt" ) != NULL ) {
			if ( noMoreTaunts )	{continue;}
			loadingTaunt = qtrue;
		} else {loadingTaunt=qfalse;}

		ci->sounds[i] = trap_S_RegisterSound( va("sound/voice/%s/misc/%s", dir, s + 1) );
		if ( !ci->sounds[i] ) {
			ci->sounds[i] = trap_S_RegisterSound( va("sound/player/%s/%s", fallback, s + 1) );
		}
		if ( loadingTaunt ) {//NOTE: this requires the taunts to not have any gaps
			if ( ci->sounds[i] ) {ci->numTaunts++;}
			else {noMoreTaunts=qtrue;}
		}
	}

	ci->deferred = qfalse;

	Com_sprintf(temp_string, sizeof(temp_string), "%s/%s", ci->modelName, ci->skinName);
	updateSkin(clientNum, temp_string);

	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
		for ( i = 0 ; i < MAX_GENTITIES ; i++ ) {
		if ( cg_entities[i].currentState.clientNum == clientNum
			&& cg_entities[i].currentState.eType == ET_PLAYER ) {
			CG_ResetPlayerEntity( &cg_entities[i] );
		}
	}
}

// we need to check here to see if the clientinfo model variable is the same as the one that is in the
// clientinfo block. This is because it is possible for the server to change skins on us when we hit a CTF
// teamplay game where groups are defined.
// most of the time this will not hit

void updateSkin(int clientNum, char *new_model)
{
	char		model_string[200];

	// create string to be checked against
	trap_Cvar_VariableStringBuffer("model", model_string, sizeof(model_string) );

	if (Q_stricmp(new_model, model_string) && cg.validPPS && (clientNum == cg.predictedPlayerState.clientNum))
	{
		trap_Cvar_Set_No_Modify ("model",new_model);
	}
}



/*
======================
CG_CopyClientInfoModel
======================
*/
static void CG_CopyClientInfoModel( clientInfo_t *from, clientInfo_t *to ) {
	VectorCopy( from->headOffset, to->headOffset );
	to->footsteps = from->footsteps;
	to->gender = from->gender;
	to->numTaunts = from->numTaunts;

	to->legsModel = from->legsModel;
	to->legsSkin = from->legsSkin;
	to->torsoModel = from->torsoModel;
	to->torsoSkin = from->torsoSkin;
	to->headModel = from->headModel;
	to->headSkin = from->headSkin;
	to->modelIcon = from->modelIcon;

	Q_strncpyz( to->soundPath, from->soundPath, sizeof (to->soundPath) );
	memcpy( to->animations, from->animations, sizeof( to->animations ) );
	memcpy( to->sounds, from->sounds, sizeof( to->sounds ) );
}

/*
======================
CG_ScanForExistingClientInfo
======================
*/
static qboolean CG_ScanForExistingClientInfo( clientInfo_t *ci ) {
	int		i;
	clientInfo_t	*match;

	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}
		if ( match->deferred ) {
			continue;
		}
		if ( !Q_stricmp( ci->modelName, match->modelName )
			&& !Q_stricmp( ci->skinName, match->skinName ) ) {
			// this clientinfo is identical, so use it's handles

			ci->deferred = qfalse;

			CG_CopyClientInfoModel( match, ci );

			return qtrue;
		}
	}

	// nothing matches, so defer the load
	return qfalse;
}

/*
======================
CG_SetDeferredClientInfo

We aren't going to load it now, so grab some other
client's info to use until we have some spare time.
======================
*/
static void CG_SetDeferredClientInfo( clientInfo_t *ci, int clientNum ) {
	int		i;
	clientInfo_t	*match;

	// if we are in teamplay, only grab a model if the skin is correct
	if ( cgs.gametype >= GT_TEAM ) {
		// this is ONLY for optimization - it's exactly the same effect as CG_LoadClientInfo
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			match = &cgs.clientinfo[ i ];
			if ( !match->infoValid ) {
				continue;
			}
			if ( Q_stricmp( ci->skinName, match->skinName ) ) {
				continue;
			}
			ci->deferred = qtrue;
			CG_CopyClientInfoModel( match, ci );
			return;
		}

		// load the full model, because we don't ever want to show
		// an improper team skin.  This will cause a hitch for the first
		// player, when the second enters.  Combat shouldn't be going on
		// yet, so it shouldn't matter
		CG_LoadClientInfo( ci, clientNum );
		return;
	}

	// find the first valid clientinfo and grab its stuff
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}

		ci->deferred = qtrue;
		CG_CopyClientInfoModel( match, ci );
		return;
	}

	// we should never get here...
	CG_Printf( "CG_SetDeferredClientInfo: no valid clients!\n" );

	CG_LoadClientInfo( ci ,clientNum);
}


/*
======================
CG_NewClientInfo
======================
*/
void CG_NewClientInfo( int clientNum ) {
	clientInfo_t *ci;
	clientInfo_t newInfo;
	const char	*configstring;
	const char	*v;
	char		*slash;

	ci = &cgs.clientinfo[clientNum];

	configstring = CG_ConfigString( clientNum + CS_PLAYERS );
	if ( !configstring[0] ) {
		memset( ci, 0, sizeof( *ci ) );
		return;		// player just left
	}

	// build into a temp buffer so the defer checks can use
	// the old value
	memset( &newInfo, 0, sizeof( newInfo ) );

	// isolate the player's name
	v = Info_ValueForKey(configstring, "n");
	Q_strncpyz( newInfo.name, v, sizeof( newInfo.name ) );

	if(CG_IsIgnored(newInfo.name) || (ci->infoValid && ci->ignore))
		newInfo.ignore = qtrue;

	// colors
	v = Info_ValueForKey( configstring, "c1" );
	CG_ColorFromString( v, newInfo.color );

	// bot skill
	v = Info_ValueForKey( configstring, "skill" );
	newInfo.botSkill = atoi( v );

	// handicap
	v = Info_ValueForKey( configstring, "hc" );
	newInfo.handicap = atoi( v );

	// wins
	v = Info_ValueForKey( configstring, "w" );
	newInfo.wins = atoi( v );

	// losses
	v = Info_ValueForKey( configstring, "l" );
	newInfo.losses = atoi( v );

	// team
	v = Info_ValueForKey( configstring, "t" );
	newInfo.team = atoi( v );

	// playerclass
	v = Info_ValueForKey( configstring, "p" );
	newInfo.pClass = atoi( v );

	// model
	v = Info_ValueForKey( configstring, "model" );
	if ( cg_forceModel.integer ) {
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char modelStr[MAX_QPATH];
		char *skin;


		trap_Cvar_VariableStringBuffer( "model", modelStr, sizeof( modelStr ) );
		if ( ( skin = strchr( modelStr, '/' ) ) == NULL) {
			skin = "default";
		} else {
			*skin++ = 0;
		}

		Q_strncpyz( newInfo.skinName, skin, sizeof( newInfo.skinName ) );
		Q_strncpyz( newInfo.modelName, modelStr, sizeof( newInfo.modelName ) );

//		Q_strncpyz( newInfo.modelName, DEFAULT_MODEL, sizeof( newInfo.modelName ) );
//		Q_strncpyz( newInfo.skinName, "default", sizeof( newInfo.skinName ) );

		if ( cgs.gametype >= GT_TEAM ) {
			// keep skin name
			slash = strchr( v, '/' );
			if ( slash ) {
				Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
			}
		}
	} else {
		Q_strncpyz( newInfo.modelName, v, sizeof( newInfo.modelName ) );

		slash = strchr( newInfo.modelName, '/' );
		if ( !slash || !slash[1] ) {//!slash[0]  means there is a slash but nothing after
			// modelName didn not include a skin name
			Q_strncpyz( newInfo.skinName, "default", sizeof( newInfo.skinName ) );
			if ( slash && !slash[1] )
			{//if we had a slash, but nothing after, clear it
				*slash = 0;
			}
		} else {
			Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
			// truncate modelName
			*slash = 0;
		}
	}

	// scan for an existing clientinfo that matches this modelname
	// so we can avoid loading checks if possible
	if ( !CG_ScanForExistingClientInfo( &newInfo ) ) {
		qboolean	forceDefer;

		forceDefer = trap_MemoryRemaining() < 2000000;

		// if we are defering loads, just have it pick the first valid
		if ( forceDefer ||
			( cg_deferPlayers.integer && !cg_buildScript.integer && !cg.loading &&
			((clientNum != cg.predictedPlayerState.clientNum) && cg.validPPS) ) ) {
			// keep whatever they had if it won't violate team skins
			if ( ci->infoValid &&
				( cgs.gametype < GT_TEAM || !Q_stricmp( newInfo.skinName, ci->skinName ) ) ) {
				CG_CopyClientInfoModel( ci, &newInfo );
				newInfo.deferred = qtrue;
			} else {
				// use whatever is available
				CG_SetDeferredClientInfo( &newInfo, clientNum );
			}
			// if we are low on memory, leave them with this model
			if ( forceDefer ) {
				CG_Printf( "Memory is low.  Using deferred model.\n" );
				newInfo.deferred = qfalse;
			}
		} else {
			CG_LoadClientInfo( &newInfo, clientNum );
		}
	}

	// replace whatever was there with the new one
	newInfo.infoValid = qtrue;
	*ci = newInfo;
}



/*
======================
CG_LoadDeferredPlayers

Called each frame when a player is dead
and the scoreboard is up
so deferred players can be loaded
======================
*/
void CG_LoadDeferredPlayers( void ) {
	int		i;
	clientInfo_t	*ci;

	// scan for a deferred player to load
	for ( i = 0, ci = cgs.clientinfo ; i < cgs.maxclients ; i++, ci++ ) {
		if ( ci->infoValid && ci->deferred ) {
			// if we are low on memory, leave it deferred
			if ( trap_MemoryRemaining() < 4000000 ) {
				CG_Printf( "Memory is low.  Using deferred model.\n" );
				ci->deferred = qfalse;
				continue;
			}
			CG_LoadClientInfo( ci, i );
//			break;
		}
	}
}

/*
=============================================================================

PLAYER ANIMATION

=============================================================================
*/


/*
===============
CG_SetLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetLerpFrameAnimation( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation ) {
	animation_t	*anim;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if ( newAnimation < 0 || newAnimation >= MAX_ANIMATIONS ) {
		CG_Error( "Bad animation number: %i", newAnimation );
	}

	anim = &ci->animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;

	if ( cg_debugAnim.integer ) {
		CG_Printf( "Anim: %i\n", newAnimation );
	}
}

/*
===============
CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
static void CG_RunLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale ) {
	int			f;
	animation_t	*anim;

	// debugging tool to get no animations
	if ( cg_animSpeed.integer == 0 ) {
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if ( newAnimation != lf->animationNumber || !lf->animation ) {
		CG_SetLerpFrameAnimation( ci, lf, newAnimation );
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if ( cg.time >= lf->frameTime ) {
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if ( !anim->frameLerp ) {
			return;		// shouldn't happen
		}
		if ( cg.time < lf->animationTime ) {
			lf->frameTime = lf->animationTime;		// initial lerp
		} else {
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f = ( lf->frameTime - lf->animationTime ) / anim->frameLerp;
		f *= speedScale;		// adjust for haste, etc
		if ( f >= anim->numFrames ) {
			f -= anim->numFrames;
			if ( anim->loopFrames ) {
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			} else {
				f = anim->numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		lf->frame = anim->firstFrame + f;
		if ( cg.time > lf->frameTime ) {
			lf->frameTime = cg.time;
			if ( cg_debugAnim.integer ) {
				CG_Printf( "Clamp lf->frameTime\n");
			}
		}
	}

	if ( lf->frameTime > cg.time + 200 ) {
		lf->frameTime = cg.time;
	}

	if ( lf->oldFrameTime > cg.time ) {
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if ( lf->frameTime == lf->oldFrameTime ) {
		lf->backlerp = 0;
	} else {
		lf->backlerp = 1.0 - (float)( cg.time - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}
}


/*
===============
CG_ClearLerpFrame
===============
*/
static void CG_ClearLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int animationNumber ) {
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation( ci, lf, animationNumber );
	lf->oldFrame = lf->frame = lf->animation->firstFrame;
}


/*
===============
CG_PlayerAnimation
===============
*/
static void CG_PlayerAnimation( centity_t *cent, int *legsOld, int *legs, float *legsBackLerp,
						int *torsoOld, int *torso, float *torsoBackLerp ) {
	clientInfo_t	*ci;
	int				clientNum;
	float			speedScale;

	clientNum = cent->currentState.clientNum;

	if ( cg_noPlayerAnims.integer ) {
		*legsOld = *legs = *torsoOld = *torso = 0;
		return;
	}

	if ( cent->currentState.powerups & ( 1 << PW_HASTE ) ) {
		speedScale = 1.5;
	} else {
		speedScale = 1;
	}

	ci = &cgs.clientinfo[ clientNum ];

	// do the shuffle turn frames locally
	if ( cent->pe.legs.yawing && ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == LEGS_IDLE ) {
		CG_RunLerpFrame( ci, &cent->pe.legs, LEGS_TURN, speedScale );
	} else {
		CG_RunLerpFrame( ci, &cent->pe.legs, cent->currentState.legsAnim, speedScale );
	}

	*legsOld = cent->pe.legs.oldFrame;
	*legs = cent->pe.legs.frame;
	*legsBackLerp = cent->pe.legs.backlerp;

	CG_RunLerpFrame( ci, &cent->pe.torso, cent->currentState.torsoAnim, speedScale );

	*torsoOld = cent->pe.torso.oldFrame;
	*torso = cent->pe.torso.frame;
	*torsoBackLerp = cent->pe.torso.backlerp;
}

/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

/*
==================
CG_SwingAngles
==================
*/
static void CG_SwingAngles( float destination, float swingTolerance, float clampTolerance,
					float speed, float *angle, qboolean *swinging ) {
	float	swing;
	float	move;
	float	scale;

	if ( !*swinging ) {
		// see if a swing should be started
		swing = AngleSubtract( *angle, destination );
		if ( swing > swingTolerance || swing < -swingTolerance ) {
			*swinging = qtrue;
		}
	}

	if ( !*swinging ) {
		return;
	}

	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract( destination, *angle );
	scale = fabs( swing );
	if ( scale < swingTolerance * 0.5 ) {
		scale = 0.5;
	} else if ( scale < swingTolerance ) {
		scale = 1.0;
	} else {
		scale = 2.0;
	}

	// swing towards the destination angle
	if ( swing >= 0 ) {
		move = cg.frametime * scale * speed;
		if ( move >= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	} else if ( swing < 0 ) {
		move = cg.frametime * scale * -speed;
		if ( move <= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	}

	// clamp to no more than tolerance
	swing = AngleSubtract( destination, *angle );
	if ( swing > clampTolerance ) {
		*angle = AngleMod( destination - (clampTolerance - 1) );
	} else if ( swing < -clampTolerance ) {
		*angle = AngleMod( destination + (clampTolerance - 1) );
	}
}

/*
=================
CG_AddPainTwitch
=================
*/
static void CG_AddPainTwitch( centity_t *cent, vec3_t torsoAngles ) {
	int		t;
	float	f;

	t = cg.time - cent->pe.painTime;
	if ( t >= PAIN_TWITCH_TIME ) {
		return;
	}

	f = 1.0 - (float)t / PAIN_TWITCH_TIME;

	if ( cent->pe.painDirection ) {
		torsoAngles[ROLL] += 20 * f;
	} else {
		torsoAngles[ROLL] -= 20 * f;
	}
}


/*
===============
CG_PlayerAngles

Handles seperate torso motion

  legs pivot based on direction of movement

  head always looks exactly at cent->lerpAngles

  if motion < 20 degrees, show in head only
  if < 45 degrees, also show in torso
===============
*/
static void CG_PlayerAngles( centity_t *cent, vec3_t legs[3], vec3_t torso[3], vec3_t head[3] ) {
	vec3_t		legsAngles, torsoAngles, headAngles;
	float		dest;
	static	int	movementOffsets[8] = { 0, 22, 45, -22, 0, 22, -45, -22 };
	vec3_t		velocity;
	float		speed;
	int			dir;

	VectorCopy( cent->lerpAngles, headAngles );
	headAngles[YAW] = AngleMod( headAngles[YAW] );
	VectorClear( legsAngles );
	VectorClear( torsoAngles );

	// --------- yaw -------------

	// allow yaw to drift a bit
	if ( ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) != LEGS_IDLE
		|| ( cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT ) != TORSO_STAND  ) {
		// if not standing still, always point all in the same direction
		cent->pe.torso.yawing = qtrue;	// always center
		cent->pe.torso.pitching = qtrue;	// always center
		cent->pe.legs.yawing = qtrue;	// always center
	}

	// adjust legs for movement dir
	if ( cent->currentState.eFlags & EF_DEAD ) {
		// don't let dead bodies twitch
		dir = 0;
	} else {
		dir = cent->currentState.angles2[YAW];
		if ( dir < 0 || dir > 7 ) {
			CG_Error( "Bad player movement angle" );
		}
	}
	legsAngles[YAW] = headAngles[YAW] + movementOffsets[ dir ];
	torsoAngles[YAW] = headAngles[YAW] + 0.25 * movementOffsets[ dir ];

	// torso
	CG_SwingAngles( torsoAngles[YAW], 25, 90, cg_swingSpeed.value, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
	CG_SwingAngles( legsAngles[YAW], 40, 90, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );

	torsoAngles[YAW] = cent->pe.torso.yawAngle;
	legsAngles[YAW] = cent->pe.legs.yawAngle;

	// --------- pitch -------------

	// only show a fraction of the pitch angle in the torso
	if ( headAngles[PITCH] > 180 ) {
		dest = (-360 + headAngles[PITCH]) * 0.75;
	} else {
		dest = headAngles[PITCH] * 0.75;
	}
	CG_SwingAngles( dest, 15, 30, 0.1, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching );
	torsoAngles[PITCH] = cent->pe.torso.pitchAngle;

	// --------- roll -------------


	// lean towards the direction of travel
	VectorCopy( cent->currentState.pos.trDelta, velocity );
	speed = VectorNormalize( velocity );
	if ( speed ) {
		vec3_t	axis[3];
		float	side;

		speed *= 0.05;

		AnglesToAxis( legsAngles, axis );
		side = speed * DotProduct( velocity, axis[1] );
		legsAngles[ROLL] -= side;

		side = speed * DotProduct( velocity, axis[0] );
		legsAngles[PITCH] += side;
	}

	// pain twitch
	CG_AddPainTwitch( cent, torsoAngles );

	// pull the angles back out of the hierarchial chain
	AnglesSubtract( headAngles, torsoAngles, headAngles );
	AnglesSubtract( torsoAngles, legsAngles, torsoAngles );
	AnglesToAxis( legsAngles, legs );
	AnglesToAxis( torsoAngles, torso );
	AnglesToAxis( headAngles, head );
}


//==========================================================================

/*
===============
CG_HasteTrail
===============
*/
static void CG_HasteTrail( centity_t *cent ) {
	localEntity_t	*smoke;
	vec3_t			origin, pos2;
	int				anim;

	if ( cent->trailTime > cg.time ) {
		return;
	}
	anim = cent->pe.legs.animationNumber & ~ANIM_TOGGLEBIT;
	if ( anim != LEGS_RUN && anim != LEGS_BACK ) {
		return;
	}

	cent->trailTime += 100;
	if ( cent->trailTime < cg.time ) {
		cent->trailTime = cg.time;
	}

	VectorCopy( cent->lerpOrigin, origin );
	origin[0] += flrandom(-5,5);
	origin[1] += flrandom(-5,5);
	origin[2] -= 15;

	AngleVectors(cent->lerpAngles, pos2, NULL, NULL);
	pos2[2]=0;
	VectorMA(origin, -22.0, pos2, pos2);

	smoke = FX_AddLine(origin, pos2, 1.0, 20.0, -12.0, 0.7, 0.0, 500, cgs.media.hastePuffShader);
}

/*
===============
CG_FlightTrail
===============
*/
static void CG_FlightTrail( centity_t *cent ) {
	localEntity_t	*smoke;
	vec3_t			origin;
	vec3_t			vel;
	vec3_t			startrgb={0.5, 0.5, 0.5};
	vec3_t			endrgb={0.0,0.0,0.0};

/*	if ( cent->trailTime > cg.time ) {
		return;
	}

	cent->trailTime += 100; // only draw a sprite every 0.01 s

	if ( cent->trailTime < cg.time ) {
		cent->trailTime = cg.time;
	}

*/
	VectorCopy( cent->lerpOrigin, origin );
	origin[2] -= flrandom(10,18);

	VectorSet(vel, flrandom(-10,10), flrandom(-10, 10), flrandom(-30,-50));

	smoke = FX_AddSprite2(origin, vel, qfalse, 4.0, 4.0, 0.5, 0.0, startrgb, endrgb, flrandom(0,360), 0, 500, cgs.media.flightPuffShader);
}

/*
===============
CG_TrailItem
===============
*/
static void CG_TrailItem( centity_t *cent, qhandle_t hModel ) {
	refEntity_t		ent;
	vec3_t			angles;
	float			frame;

	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson )
	{
		return;
	}

	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( cent->lerpAngles, angles );
	angles[PITCH] = 0;
	angles[ROLL] = 0;
	angles[YAW] += 180.0;		// It's facing the wrong way.
	AnglesToAxis( angles, ent.axis );

	VectorMA( cent->lerpOrigin, 12, ent.axis[0], ent.origin );
	ent.origin[2] += 4;

	// Make it animate.
	frame = (cg.time / 100.0);
	ent.renderfx|=RF_WRAP_FRAMES;

	ent.oldframe = (int)frame;
	ent.frame = (int)frame+1;
	ent.backlerp = (float)(ent.frame) - frame;

	// if the player is looking at himself in 3rd person, don't show the flag solid, 'cause he can't see!!!
	if (cent->currentState.number == cg.snap->ps.clientNum)
	{
		ent.shaderRGBA[3] = 128;
		ent.renderfx |= RF_FORCE_ENT_ALPHA;
	}

	VectorScale(ent.axis[0], 0.75, ent.axis[0]);
	VectorScale(ent.axis[1], 0.9, ent.axis[1]);
	VectorScale(ent.axis[2], 0.9, ent.axis[2]);
	ent.nonNormalizedAxes = qtrue;

#if 0	// This approach is used if you want the item to autorotate.  Since this is the flag, we don't.
	VectorScale( cg.autoAxis[0], 0.75, ent.axis[0] );
	VectorScale( cg.autoAxis[1], 0.75, ent.axis[1] );
	VectorScale( cg.autoAxis[2], 0.75, ent.axis[2] );
#endif

	ent.hModel = hModel;
	trap_R_AddRefEntityToScene( &ent );
}

/*
===============
CG_PlayerPowerups
===============
*/
static void CG_PlayerPowerups( centity_t *cent ) {
	int		powerups;

	powerups = cent->currentState.powerups;
	if ( !powerups ) {
		return;
	}

	// quad gives a dlight
	if ( powerups & ( 1 << PW_QUAD ) ) {
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.2, 0.2, 1.0 );
	}

	// invul gives a dlight
	if ( powerups & ( 1 << PW_BATTLESUIT ) ) {
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.8, 0.8, 0.2 );
	}

	// borg adapt gives a dlight
	if ( powerups & ( 1 << PW_BORG_ADAPT ) ) {
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.2, 1.0, 0.2 );
	}

	// flight plays a looped sound
	if ( powerups & ( 1 << PW_FLIGHT ) ) {
		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.flightSound );
	}

	// redflag
	if ( powerups & ( 1 << PW_REDFLAG ) ) {
		CG_TrailItem( cent, cgs.media.redFlagModel );
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1, 0.2, 0.2 );
	}

	// blueflag
	if ( powerups & ( 1 << PW_BLUEFLAG ) ) {
		CG_TrailItem( cent, cgs.media.blueFlagModel );
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.2, 0.2, 1 );
	}

	// haste leaves smoke trails
	if ( powerups & ( 1 << PW_HASTE ) && (cent->currentState.groundEntityNum==ENTITYNUM_WORLD)) {
		CG_HasteTrail( cent );
	}

	// haste leaves smoke trails
	if ( powerups & ( 1 << PW_FLIGHT ) && (cent->currentState.groundEntityNum!=ENTITYNUM_WORLD)) {
		CG_FlightTrail( cent );
	}

	// seeker coolness
	if ( powerups & ( 1 << PW_SEEKER ) )
	{
		CG_Seeker(cent);
	}
}




/*
===============
CG_PlayerFloatSprite

Float a sprite over the player's head
===============
*/
static void CG_PlayerFloatSprite( centity_t *cent, qhandle_t shader ) {
	int				rf;
	refEntity_t		ent;
	int				team;

	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson ) {
		rf = RF_THIRD_PERSON;		// only show in mirrors
	} else {
		rf = 0;
	}

	team = cgs.clientinfo[ cent->currentState.clientNum ].team;

	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.data.sprite.radius = 10;
	ent.renderfx = rf;
	if (team==TEAM_RED)
	{
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 64;
		ent.shaderRGBA[2] = 64;
	}
	else if (team==TEAM_BLUE)
	{
		ent.shaderRGBA[0] = 64;
		ent.shaderRGBA[1] = 64;
		ent.shaderRGBA[2] = 255;
	}
	else
	{
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
	}
	ent.shaderRGBA[3] = 255;
	trap_R_AddRefEntityToScene( &ent );
}



/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void CG_PlayerSprites( centity_t *cent ) {
	int		team;

	if ( cent->currentState.eFlags & EF_CONNECTION ) {
		CG_PlayerFloatSprite( cent, cgs.media.connectionShader );
		return;
	}

	//label the action hero
	if ( cgs.clientinfo[cent->currentState.number].pClass == PC_ACTIONHERO )
	{
		CG_PlayerFloatSprite( cent, cgs.media.heroSpriteShader );
		return;
	}

	//Special hack: if it's Borg who has regen going, must be Borg queen
	if ( cgs.clientinfo[cent->currentState.number].pClass == PC_BORG )
	{
		if ( (cg_entities[cent->currentState.number].currentState.powerups&(1<<PW_REGEN)) )
		{
			CG_PlayerFloatSprite( cent, cgs.media.borgQueenIconShader );
			return;
		}
	}

	if ( cent->currentState.eFlags & EF_TALK ) {
		CG_PlayerFloatSprite( cent, cgs.media.chatShader );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_FIRSTSTRIKE ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalFirstStrike );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_IMPRESSIVE ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalImpressive );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_EXCELLENT ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalExcellent );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_ACE ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalAce );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_EXPERT ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalExpert );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_MASTER ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalMaster );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_CHAMPION ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalChampion );
		return;
	}

	//NOTE: Borg *Queen* should have been caught above
	if ( cgs.clientinfo[cent->currentState.number].pClass == PC_BORG )
	{
		CG_PlayerFloatSprite( cent, cgs.media.borgIconShader );
		return;
	}

	team = cgs.clientinfo[ cent->currentState.clientNum ].team;
	if ( !(cent->currentState.eFlags & EF_DEAD) &&
			cg.snap->ps.persistant[PERS_TEAM] == team &&
			cgs.gametype >= GT_TEAM &&
			cent->currentState.number != cg.snap->ps.clientNum )	// Don't show a sprite above a player's own head in 3rd person.
	{
		if (team==TEAM_RED)
		{
			CG_PlayerFloatSprite( cent, cgs.media.teamRedShader );
		}
		else if (team==TEAM_BLUE)
		{
			CG_PlayerFloatSprite( cent, cgs.media.teamBlueShader );
		}
		// else don't show an icon.  There currently are no other team types.

		return;
	}

}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
#define	SHADOW_DISTANCE		128
static qboolean CG_PlayerShadow( centity_t *cent, float *shadowPlane ) {
	vec3_t		end, mins = {-7, -7, 0}, maxs = {7, 7, 2};
	trace_t		trace;
	float		alpha;

	*shadowPlane = 0;

	if ( cg_shadows.integer == 0 ) {
		return qfalse;
	}

	// no shadows when invisible
	if ( cent->currentState.powerups & ( 1 << PW_INVIS ) ) {
		return qfalse;
	}

	// send a trace down from the player to the ground
	VectorCopy( cent->lerpOrigin, end );
	end[2] -= SHADOW_DISTANCE;

	trap_CM_BoxTrace( &trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID );

	// no shadow if too high
	if ( trace.fraction == 1.0 ) {
		return qfalse;
	}

	*shadowPlane = trace.endpos[2] + 1;

	if ( cg_shadows.integer != 1 ) {	// no mark for stencil or projection shadows
		return qtrue;
	}

	// fade the shadow out with height
	alpha = 1.0 - trace.fraction;

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	CG_ImpactMark( cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal,
		cent->pe.legs.yawAngle, 1,1,1,alpha, qfalse, 16, qtrue );

	return qtrue;
}


/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
static void CG_PlayerSplash( centity_t *cent ) {
	vec3_t		start, end;
	trace_t		trace;
	int			contents;
	polyVert_t	verts[4];

	if ( !cg_shadows.integer ) {
		return;
	}

	VectorCopy( cent->lerpOrigin, end );
	end[2] -= 24;

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = trap_CM_PointContents( end, 0 );
	if ( !( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) ) {
		return;
	}

	VectorCopy( cent->lerpOrigin, start );
	start[2] += 32;

	// if the head isn't out of liquid, don't make a mark
	contents = trap_CM_PointContents( start, 0 );
	if ( contents & ( CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		return;
	}

	// trace down to find the surface
	trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) );

	if ( trace.fraction == 1.0 ) {
		return;
	}

	// create a mark polygon
	VectorCopy( trace.endpos, verts[0].xyz );
	verts[0].xyz[0] -= 32;
	verts[0].xyz[1] -= 32;
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[1].xyz );
	verts[1].xyz[0] -= 32;
	verts[1].xyz[1] += 32;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[2].xyz );
	verts[2].xyz[0] += 32;
	verts[2].xyz[1] += 32;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[3].xyz );
	verts[3].xyz[0] += 32;
	verts[3].xyz[1] -= 32;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.wakeMarkShader, 4, verts );
}



static int	timestamp;


/*
===============
CG_AddRefEntityWithPowerups

Adds a piece with modifications or duplications for powerups
Also called by CG_Missile for quad rockets, but nobody can tell...
===============
*/
void CG_AddRefEntityWithPowerups( refEntity_t *ent, int powerups, int eFlags, qboolean borg )
{
	if ( eFlags & EF_ITEMPLACEHOLDER )			// Hologram Decoy
	{
		float f1, f2;

		// We used EF_ITEMPLACEHOLDER flag to indicate that this 'player' model
		// is actually a holographic decoy.  Now there is a chance that the
		// decoy will flicker a bit because of ordering with alpha shaders...

		// The lowest the alpha goes is 4.0-2.5-1.0=0.5.
		f1 = 4.0 + 2.5*sin(52.423 + cg.time/205.243);
		f2 = sin(14.232 + cg.time/63.572);

		f1 = f1+f2;
		if (f1 > 1.0)
		{	// Just draw him solid.
			if ( cg.snap->ps.persistant[PERS_CLASS] == PC_TECH )
			{//technicians can see decoys as grids
				ent->customShader = cgs.media.rezOutShader;
				ent->shaderRGBA[0] =
				ent->shaderRGBA[1] =
				ent->shaderRGBA[2] = 128;
			}

			trap_R_AddRefEntityToScene( ent );
		}
		else
		{	// Draw him faded.
			if (f1 > 0.8)
			{	// Don't have alphas over 0.8, it just looks bad.
				f1=0.8;
			}
			else if (f1 < 0.1)
			{
				f1=0.1;
			}

			ent->renderfx |= RF_FORCE_ENT_ALPHA;	// Override the skin shader info and use this alpha value.
			ent->shaderRGBA[3] = 255.0*f1;
			trap_R_AddRefEntityToScene( ent );
			ent->renderfx &= ~RF_FORCE_ENT_ALPHA;
			ent->shaderRGBA[3] = 255;

			// ...with a static shader.
			ent->customShader = cgs.media.holoDecoyShader;
			ent->shaderRGBA[0] =
			ent->shaderRGBA[1] =
			ent->shaderRGBA[2] = 255.0*(1.0-f1);	// More solid as the player fades out...
			trap_R_AddRefEntityToScene(ent);
		}

		return;
	}
	if ((eFlags & EF_DEAD) && (timestamp > cg.time))
	{	// Dead.  timestamp holds the time of death.
		float alpha;
		int a;

		// First draw the entity itself.
		alpha = (timestamp - cg.time)/2500.0;
		ent->renderfx |= RF_FORCE_ENT_ALPHA;
		a = alpha * 255.0;
		if (a <= 0)
			a=1;
		ent->shaderRGBA[3] = a;
		trap_R_AddRefEntityToScene( ent );
		ent->renderfx &= ~RF_FORCE_ENT_ALPHA;
		ent->shaderRGBA[3] = 255;

		// Now draw the static shader over it.
		// Alpha in over half the time, out over half.
		alpha = sin(M_PI*alpha);
		a = alpha * 255.0;
		if (a <= 0)
			a=1;
		ent->customShader = cgs.media.rezOutShader;
		ent->shaderRGBA[0] =
		ent->shaderRGBA[1] =
		ent->shaderRGBA[2] = a;
		trap_R_AddRefEntityToScene( ent );
		ent->shaderRGBA[0] =
		ent->shaderRGBA[1] =
		ent->shaderRGBA[2] = 255;
	}
	else if ( powerups & ( 1 << PW_INVIS ) )
	{
		if ( cg.snap->ps.persistant[PERS_CLASS] == PC_TECH )
		{//technicians can see cloaked people
			trap_R_AddRefEntityToScene( ent );
		}
		ent->customShader = cgs.media.invisShader;
		trap_R_AddRefEntityToScene( ent );
	}
	else if (powerups & (1<<PW_DISINTEGRATE))
	{
		int dtime;

		dtime = cg.time-timeParam;
		if (dtime < 1000)
		{
			ent->renderfx |= RF_FORCE_ENT_ALPHA;
			ent->shaderRGBA[3] = 255 - (dtime)*0.25;
			trap_R_AddRefEntityToScene( ent );
			ent->renderfx &= ~RF_FORCE_ENT_ALPHA;
			ent->shaderRGBA[3] = 255;
		}

		if (dtime < 2000)
		{
			ent->customShader = cgs.media.disruptorShader;
			ent->shaderTime = timeParam / 1000.0f;
			trap_R_AddRefEntityToScene( ent );
		}
	}
	else if (powerups & (1<<PW_EXPLODE))
	{
		int dtime;

		dtime = cg.time-timeParam;

		if (dtime < 300)
		{
			ent->renderfx |= RF_FORCE_ENT_ALPHA;
			ent->shaderRGBA[3] = (int)(255.0 - (dtime / 300.0) * 254.0);
			trap_R_AddRefEntityToScene( ent );
			ent->renderfx &= ~RF_FORCE_ENT_ALPHA;
			ent->shaderRGBA[3] = 255;
		}

		if (dtime < 500)
		{
			ent->customShader = cgs.media.explodeShellShader;
			ent->renderfx |= RF_CAP_FRAMES;
			ent->shaderTime = timeParam / 1000.0f;
			trap_R_AddRefEntityToScene( ent );
			ent->renderfx &= ~RF_CAP_FRAMES;
		}
	}
	else if (powerups & (1<<PW_GHOST))
	{
		ent->renderfx |= RF_FORCE_ENT_ALPHA;
		ent->shaderRGBA[3] = 100 + 50*sin(cg.time/200.0);
		trap_R_AddRefEntityToScene( ent );
		ent->renderfx &= ~RF_FORCE_ENT_ALPHA;
		ent->shaderRGBA[3] = 255;
	}
	else
	{
		trap_R_AddRefEntityToScene( ent );

		// Quad should JUST be on the weapon now, sparky.
/*		if ( powerups & ( 1 << PW_QUAD ) )
		{
			if (team == TEAM_RED)
				ent->customShader = cgs.media.redQuadShader;
			else
				ent->customShader = cgs.media.quadShader;
			trap_R_AddRefEntityToScene( ent );
		}
*/


		if ( powerups & ( 1 << PW_REGEN ) ) {
			if ( ( ( cg.time / 100 ) % 10 ) == 1 ) {
				ent->customShader = cgs.media.regenShader;
				trap_R_AddRefEntityToScene( ent );
			}
		}
		if ( powerups & ( 1 << PW_BORG_ADAPT ))
		{
			ent->customShader = cgs.media.borgFullBodyShieldShader;
			trap_R_AddRefEntityToScene( ent );
			return;
		}
		if ( powerups & ( 1 << PW_BATTLESUIT ))
		{
				ent->customShader = cgs.media.battleSuitShader;
			trap_R_AddRefEntityToScene( ent );
			return;
		}
		if (powerups & (1 << PW_OUCH))
		{
			ent->customShader = cgs.media.holoOuchShader;
			// set rgb to 1 of 16 values from 0 to 255. don't use random so that the three
			//parts of the player model as well as the gun will all look the same
			ent->shaderRGBA[0] =
			ent->shaderRGBA[1] =
			ent->shaderRGBA[2] = ((cg.time % 17)*0.0625)*255.0;//irandom(0,255);
			trap_R_AddRefEntityToScene(ent);
		}

		if (powerups & (1<<PW_ARCWELD_DISINT))
		{
			int dtime;

			dtime = cg.time-timeParam;

			if (dtime < irandom(0,4000))
			{
				// Add an electrical shell, faded out over the first three seconds.
				ent->customShader = cgs.media.electricBodyShader;
//				ent->shaderTime = timeParam / 1000.0f;
				ent->shaderRGBA[0] =
				ent->shaderRGBA[1] =
				ent->shaderRGBA[2] = (int)(1.0 + ((4000.0 - dtime) / 4000.0f * 254.0f ));
				ent->shaderRGBA[3] = 255;
				trap_R_AddRefEntityToScene( ent );

				if ( random() > 0.95f )
				{
					// Play a zap sound to go it.
					trap_S_StartSound (ent->origin, entNum, CHAN_AUTO, cg_weapons[WP_DREADNOUGHT].altHitSound);
				}
			}
		}
	}
}

#define MAX_SHIELD_TIME	2500.0
#define MIN_SHIELD_TIME	1750.0


void CG_PlayerShieldHit(int entitynum, vec3_t dir, int amount)
{
	centity_t *cent;
	int	time;

	if (entitynum<0 || entitynum >= MAX_CLIENTS)
	{
		return;
	}

	cent = &cg_entities[entitynum];

	if (amount > 100)
	{
		time = cg.time + MAX_SHIELD_TIME;		// 2 sec.
	}
	else
	{
		time = cg.time + 500 + amount*20;
	}

	if (time > cent->damageTime)
	{
		cent->damageTime = time;
		VectorScale(dir, -1, dir);
		vectoangles(dir, cent->damageAngles);
	}
}


void CG_DrawPlayerShield(centity_t *cent, vec3_t origin)
{
	refEntity_t ent;
	int			alpha;
	float		scale;

	// Don't draw the shield when the player is dead.
	if (cent->currentState.eFlags & EF_DEAD)
	{
		return;
	}

	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( origin, ent.origin );
	ent.origin[2] += 10.0;
	AnglesToAxis( cent->damageAngles, ent.axis );

	alpha = 255.0 * ((cent->damageTime - cg.time) / MIN_SHIELD_TIME) + irandom(0, 16);
	if (alpha>255)
		alpha=255;

	// Make it bigger, but tighter if more solid
	scale = 1.8 - ((float)alpha*(0.4/255.0));		// Range from 1.4 to 1.8
	VectorScale( ent.axis[0], scale, ent.axis[0] );
	VectorScale( ent.axis[1], scale, ent.axis[1] );
	VectorScale( ent.axis[2], scale, ent.axis[2] );

	ent.hModel = cgs.media.explosionModel;
	ent.customShader = cgs.media.halfShieldShader;
	ent.shaderRGBA[0] = alpha;
	ent.shaderRGBA[1] = alpha;
	ent.shaderRGBA[2] = alpha;
	ent.shaderRGBA[3] = 255;
	trap_R_AddRefEntityToScene( &ent );
}


void CG_PlayerHitFX(centity_t *cent)
{
	centity_t *curent;

	// only do the below fx if the cent in question is...uh...me, and it's first person.
	if (cent->currentState.clientNum != cg.predictedPlayerState.clientNum || cg.renderingThirdPerson)
	{
		// Get the NON-PREDICTED player entity, because the predicted one doesn't have the damage info on it.
		curent = &cg_entities[cent->currentState.number];

		if (curent->damageTime > cg.time)
		{
			CG_DrawPlayerShield(curent, cent->lerpOrigin);
		}

		return;
	}
}

//------------------------------------
void CG_BorgEyebeam( centity_t *cent, const refEntity_t *parent )
{
	qboolean	large = qfalse;
	vec3_t		beamOrg, beamEnd;
	trace_t		trace;
	refEntity_t	temp;

	CG_PositionEntityOnTag( &temp, parent, parent->hModel, "tag_ear");

	if ( VectorCompare( temp.origin, parent->origin ))
	{
		// Vectors must be the same so the tag_ear wasn't found
		return;
	}
	//Note the above will also prevent the beam from being drawn in first person view if you don't have a tag_ear

	// well, we are in thirdperson or whatnot, so we should just render from a bolt-on ( tag_ear )
	if ( cent->currentState.clientNum != cg.predictedPlayerState.clientNum
					|| cg.renderingThirdPerson
					|| cg.snap->ps.pm_type == PM_INTERMISSION )
	{
		VectorCopy( temp.origin, beamOrg );
		VectorMA( beamOrg, 1024, temp.axis[0], beamEnd );//forward to end
	}
	else
	{
		vec3_t axis[3];

		// stupid offset hack
		AnglesToAxis( cent->lerpAngles, axis );
		VectorMA( cent->lerpOrigin, 26, axis[2], beamOrg );//up
		VectorMA( beamOrg,		   -26, axis[1], beamOrg );//right
//		VectorMA( beamOrg,		  0.2f, axis[0], beamOrg );//forward
		VectorMA( beamOrg,		  1024, axis[0], beamEnd );//forward to end
		large = qtrue; // render a fatter line
	}

	trap_CM_BoxTrace( &trace, beamOrg, beamEnd, NULL, NULL, 0, MASK_SHOT );
	VectorCopy( trace.endpos, beamEnd );
	VectorMA( beamOrg, 0.5, parent->axis[0], beamOrg );//forward

	FX_BorgEyeBeam( beamOrg, beamEnd, trace.plane.normal, large );
}

/*
===============
CG_Player
===============
*/
void CG_Player( centity_t *cent ) {
	clientInfo_t	*ci;
	refEntity_t		legs;
	refEntity_t		torso;
	refEntity_t		head;
	int				clientNum;
	int				renderfx;
	qboolean		shadow, borg = qfalse;
	float			shadowPlane;

	// the client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity");
	}
	ci = &cgs.clientinfo[ clientNum ];

	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !ci->infoValid ) {
		return;
	}

	borg = ( ci->pClass == PC_BORG ? qtrue : qfalse );

	// If I'm a borg and there is another borg teleporting, at least allow me to see a trail, then return
	if ( cg.snap->ps.persistant[PERS_CLASS]	== PC_BORG && borg && ( cent->currentState.eFlags & EF_NODRAW ))
	{
		FX_BorgTeleportTrails( cent->lerpOrigin );
		return;
	}

	if (cent->currentState.eFlags & EF_NODRAW)
	{	// Don't draw anymore...
		return;
	}

	memset( &legs, 0, sizeof(legs) );
	memset( &torso, 0, sizeof(torso) );
	memset( &head, 0, sizeof(head) );

	// get the rotation information
	CG_PlayerAngles( cent, legs.axis, torso.axis, head.axis );

	// get the animation state (after rotation, to allow feet shuffle)
	CG_PlayerAnimation( cent, &legs.oldframe, &legs.frame, &legs.backlerp,
		&torso.oldframe, &torso.frame, &torso.backlerp );

	// add powerups floating behind the player
	CG_PlayerPowerups( cent );

	// add the talk baloon or disconnect icon (not in intermission)
	if ( !cg.intermissionStarted )
	{
		CG_PlayerSprites( cent );
	}

	// add the shadow
	if ( !(cent->currentState.eFlags & EF_ITEMPLACEHOLDER) )
	{
		shadow = CG_PlayerShadow( cent, &shadowPlane );
	}
	else
	{	//  - unless we are a hologram...
		shadow=qfalse;
		shadowPlane=0;
	}

	// add a water splash if partially in and out of water
	CG_PlayerSplash( cent );

	// get the player model information
	renderfx = 0;
	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson ) {
		renderfx = RF_THIRD_PERSON;			// only draw in mirrors
	}
	if ( cg_shadows.integer == 3 && shadow ) {
		renderfx |= RF_SHADOW_PLANE;
	}
	renderfx |= RF_LIGHTING_ORIGIN;			// use the same origin for all

	// if we've been hit, display proper fullscreen fx
	CG_PlayerHitFX(cent);

	// If we are dead, set up the correct fx.
	if (cent->currentState.eFlags & EF_DEAD)
	{
		if (cent->deathTime==0)
			cent->deathTime = cg.time;		// Set Death Time so you can see yourself

		if (cent->currentState.time > cg.time)
		{	// Fading out.
			timestamp=cent->currentState.time;
			shadow = qfalse;
			shadowPlane = 0;
		}
		else
		{
			timestamp = 0;
		}
	}
	else
		cent->deathTime = 0;


	//
	// add the legs
	//
	legs.hModel = ci->legsModel;
	legs.customSkin = ci->legsSkin;

	VectorCopy( cent->lerpOrigin, legs.origin );

	VectorCopy( cent->lerpOrigin, legs.lightingOrigin );
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	VectorCopy (legs.origin, legs.oldorigin);	// don't positionally lerp at all

	// Setup the param, in case it is needed.
	timeParam = cent->deathTime;
	entNum = cent->currentState.number;
	CG_AddRefEntityWithPowerups( &legs, cent->currentState.powerups, cent->currentState.eFlags, borg );

	// if the model failed, allow the default nullmodel to be displayed
	if (!legs.hModel) {
		return;
	}

	//
	// add the torso
	//
	torso.hModel = ci->torsoModel;
	if (!torso.hModel) {
		return;
	}

	torso.customSkin = ci->torsoSkin;

	VectorCopy( cent->lerpOrigin, torso.lightingOrigin );

	CG_PositionRotatedEntityOnTag( &torso, &legs, ci->legsModel, "tag_torso");

	torso.shadowPlane = shadowPlane;
	torso.renderfx = renderfx;

	CG_AddRefEntityWithPowerups( &torso, cent->currentState.powerups, cent->currentState.eFlags, borg );

	//
	// add the head
	//
	head.hModel = ci->headModel;
	if (!head.hModel) {
		return;
	}
	head.customSkin = ci->headSkin;

	VectorCopy( cent->lerpOrigin, head.lightingOrigin );

	CG_PositionRotatedEntityOnTag( &head, &torso, ci->torsoModel, "tag_head" );

	head.shadowPlane = shadowPlane;
	head.renderfx = renderfx;

	CG_AddRefEntityWithPowerups( &head, cent->currentState.powerups, cent->currentState.eFlags, borg );

	if ( borg && cgs.pModAssimilation )
	{
		CG_BorgEyebeam( cent, &head );
	}

	//
	// add the gun / barrel / flash
	//
	CG_AddPlayerWeapon( &torso, NULL, cent );
}


//=====================================================================

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity( centity_t *cent ) {
	cent->errorTime = -99999;		// guarantee no error decay added
	cent->extrapolated = qfalse;

	CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.legs, cent->currentState.legsAnim );
	CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.torso, cent->currentState.torsoAnim );

	BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, cent->lerpOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, cg.time, cent->lerpAngles );

	VectorCopy( cent->lerpOrigin, cent->rawOrigin );
	VectorCopy( cent->lerpAngles, cent->rawAngles );

	memset( &cent->pe.legs, 0, sizeof( cent->pe.legs ) );
	cent->pe.legs.yawAngle = cent->rawAngles[YAW];
	cent->pe.legs.yawing = qfalse;
	cent->pe.legs.pitchAngle = 0;
	cent->pe.legs.pitching = qfalse;

	memset( &cent->pe.torso, 0, sizeof( cent->pe.legs ) );
	cent->pe.torso.yawAngle = cent->rawAngles[YAW];
	cent->pe.torso.yawing = qfalse;
	cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
	cent->pe.torso.pitching = qfalse;

	if ( cg_debugPosition.integer ) {
		CG_Printf("%i ResetPlayerEntity yaw=%i\n", cent->currentState.number, cent->pe.torso.yawAngle );
	}
}

/************* IGNORE STUFF *************/

/*
===============
CG_FindIgnore

Find an ignore entry.
===============
*/

char *CG_FindIgnore(char *ignores, char *findentry, int substring)
{
	char *ignored = ignores, *end;
	int foundcount = 0, iglen = strlen(findentry);

	if ( !*findentry ) {
		// shouldn't happen
		return NULL;
	}

	while(ignored = Q_strstr(ignored, findentry))
	{
		// find the end of the entry
		for(end = ignored + 1; *end && *(end - 1) != IGNORE_SEP; end++);

		if(substring)
		{
			// we found parts of the string in an ignore.

			// now get the start of the entry, not the substring.
			while(ignores != ignored && *(ignored-1) != IGNORE_SEP)
			{
				ignored--;
			}

			foundcount++;

			if(foundcount >= substring)
				break;

			ignored = end;
		}
		else
		{
			// Make sure that this is not a substring
			if((ignored != ignores && *(ignored-1) != IGNORE_SEP) ||
				(ignored[iglen] != '\0' && ignored[iglen] != IGNORE_SEP))
			{
				ignored = end;
				continue;
			}

			break;
		}
	}

	return ignored;
}



/*
===============
CG_IsIgnored

Checks whether a certain nick is ignored.
===============
*/

qboolean CG_IsIgnored(char *testnick)
{
	char ignores[MAX_IGNORE_LENGTH];
	char tnick[sizeof(cgs.clientinfo[0].name)];

	Q_strncpyz(tnick, testnick, sizeof(tnick));
	//Q_StripColor(tnick);

	trap_Cvar_VariableStringBuffer(IGNORE_CVARNAME, ignores, sizeof(ignores));

	if(CG_FindIgnore(ignores, tnick, 0))
		return qtrue;

	return qfalse;
}

/*
===============
CG_AddIgnore

Add a playername to cg_ignoredPlayers cvar
===============
*/

qboolean CG_AddIgnore(char *newignore)
{
	char curignored[MAX_IGNORE_LENGTH];
	int newiglen = strlen(newignore);

	trap_Cvar_VariableStringBuffer(IGNORE_CVARNAME, curignored, sizeof(curignored));

	if(CG_FindIgnore(curignored, newignore, 0))
		return qfalse;		// Already in list.

	if(strlen(curignored) + strlen(newignore) + 1 >= sizeof(curignored))
	{
		CG_Printf("Warning: ignored players list is full!\n");
		return qfalse;
	}

	Q_strcat(curignored, sizeof(curignored), newignore);
	Q_strcat(curignored, sizeof(curignored), IGNORE_SEP2);

	trap_Cvar_Set(IGNORE_CVARNAME, curignored);
	return qtrue;
}

/*
===============
CG_DelIgnore

Remove a playername from the cg_ignoredPlayers cvar
===============
*/

void CG_DelIgnore(char *delignore, qboolean substring)
{
	char curignored[MAX_IGNORE_LENGTH];
	char *ignptr, *start;
	int restlen;
	qboolean waslast = qfalse;

	trap_Cvar_VariableStringBuffer(IGNORE_CVARNAME, curignored, sizeof(curignored));

	while((start = CG_FindIgnore(curignored, delignore, substring ? 1 : 0)))
	{
		ignptr = start;
		for(; *ignptr && *(ignptr) != IGNORE_SEP; ignptr++);

		if(!ignptr)
			waslast = qtrue;
		else
			*ignptr = '\0';

		CG_Printf("Unignoring player %s\n", start);

		if(waslast)
			break;

		ignptr++;
		restlen = strlen(ignptr);
		memmove(start, ignptr, restlen+1);
	}

	trap_Cvar_Set(IGNORE_CVARNAME, curignored);
}
