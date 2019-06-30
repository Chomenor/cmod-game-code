// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_weapons.c -- events and effects dealing with weapons
#include "cg_local.h"
#include "fx_local.h"


/*
=================
CG_RegisterWeapon

The server says this item is used on this level
=================
*/

// kef -- sad? yep.
typedef struct wpnBarrelInfo_s
{
	weapon_t			giTag;
	int					numBarrels;
} wpnBarrelInfo_t;

wpnBarrelInfo_t wpnBarrelData[] =
{
	{WP_PHASER,				0},
	{WP_COMPRESSION_RIFLE,	0},
	{WP_IMOD,				0},
	{WP_SCAVENGER_RIFLE,	0},
	{WP_STASIS,				1},
	{WP_GRENADE_LAUNCHER,	2},
	{WP_TETRION_DISRUPTOR,	1},
	{WP_QUANTUM_BURST,		1},
	{WP_DREADNOUGHT,		4},
	{WP_VOYAGER_HYPO,		0},
	{WP_BORG_ASSIMILATOR,	0},
	{WP_BORG_WEAPON,		0},

	// make sure this is the last entry in this array, please
	{WP_NONE,				0},
};

void CG_RegisterWeapon( int weaponNum ) {
	weaponInfo_t	*weaponInfo;
	gitem_t			*item, *ammo;
	char			path[MAX_QPATH];
	vec3_t			mins, maxs;
	int				i;
	int				numBarrels = 0;
	wpnBarrelInfo_t	*barrelInfo = NULL;

	weaponInfo = &cg_weapons[weaponNum];

	if ( weaponNum == 0 ) {
		return;
	}

	if ( weaponInfo->registered ) {
		return;
	}

	memset( weaponInfo, 0, sizeof( *weaponInfo ) );
	weaponInfo->registered = qtrue;

	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( item->giType == IT_WEAPON && item->giTag == weaponNum ) {
			weaponInfo->item = item;
			break;
		}
	}
	if ( !item->classname ) {
		CG_Error( "Couldn't find weapon %i", weaponNum );
	}
	CG_RegisterItemVisuals( item - bg_itemlist );

	weaponInfo->weaponModel = trap_R_RegisterModel( item->world_model );

	// kef -- load in-view model
	weaponInfo->viewModel = trap_R_RegisterModel(item->view_model);

	// calc midpoint for rotation
	trap_R_ModelBounds( weaponInfo->weaponModel, mins, maxs );
	for ( i = 0 ; i < 3 ; i++ ) {
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * ( maxs[i] - mins[i] );
	}

	weaponInfo->weaponIcon = trap_R_RegisterShader( item->icon );

	for ( ammo = bg_itemlist + 1 ; ammo->classname ; ammo++ ) {
		if ( ammo->giType == IT_AMMO && ammo->giTag == weaponNum ) {
			break;
		}
	}
//	if ( ammo->classname && ammo->world_model ) {
//		weaponInfo->ammoModel = trap_R_RegisterModel( ammo->world_model );
//	}

	strcpy( path, item->view_model );
	COM_StripExtension( path, path );
	strcat( path, "_flash.md3" );
	weaponInfo->flashModel = trap_R_RegisterModel( path );

	for (barrelInfo = wpnBarrelData; barrelInfo->giTag != WP_NONE; barrelInfo++)
	{
		if (barrelInfo->giTag == weaponNum)
		{
			numBarrels = barrelInfo->numBarrels;
			break;
		}
	}
	for (i=0; i< numBarrels; i++) {
		Q_strncpyz( path, item->view_model, MAX_QPATH );
		COM_StripExtension( path, path );
		if (i)
		{
			strcat( path, va("_barrel%d.md3", i+1));
		}
		else
			strcat( path, "_barrel.md3" );
		weaponInfo->barrelModel[i] = trap_R_RegisterModel( path );
	}

	strcpy( path, item->view_model );
	COM_StripExtension( path, path );
	strcat( path, "_hand.md3" );
	weaponInfo->handsModel = trap_R_RegisterModel( path );

	if ( !weaponInfo->handsModel ) {
		weaponInfo->handsModel = trap_R_RegisterModel( "models/weapons2/prifle/prifle_hand.md3" );
	}

	switch ( weaponNum ) {
	case WP_PHASER:
		MAKERGB( weaponInfo->flashDlightColor, 0, 0, 0 );

		weaponInfo->firingSound = trap_S_RegisterSound( SOUND_DIR "phaser/phaserfiring.wav" );
		weaponInfo->altFiringSound = trap_S_RegisterSound( SOUND_DIR "phaser/altphaserfiring.wav" );
		weaponInfo->flashSound = trap_S_RegisterSound( SOUND_DIR "phaser/phaserstart.wav" );
		weaponInfo->altFlashSnd = trap_S_RegisterSound( SOUND_DIR "phaser/altphaserstart.wav" );
		weaponInfo->stopSound = trap_S_RegisterSound(SOUND_DIR "phaser/phaserstop.wav");
		weaponInfo->altStopSound = trap_S_RegisterSound(SOUND_DIR "phaser/altphaserstop.wav");

		cgs.media.phaserShader			= trap_R_RegisterShader( "gfx/misc/phaser" );
		cgs.media.phaserEmptyShader		= trap_R_RegisterShader( "gfx/misc/phaserempty" );

		cgs.media.phaserAltShader		= trap_R_RegisterShader("gfx/effects/whitelaser");	// "gfx/misc/phaser_alt" ); 

		cgs.media.phaserAltEmptyShader	= trap_R_RegisterShader( "gfx/misc/phaser_altempty" ); 
		cgs.media.phaserMuzzleEmptyShader= trap_R_RegisterShader( "models/weapons2/phaser/muzzle_empty" );

		break;

	case WP_DREADNOUGHT:
		MAKERGB( weaponInfo->flashDlightColor, 0.6, 0.6, 1 );
		MAKERGB( weaponInfo->missileDlightColor, 0.6, 0.6, 1 );
		weaponInfo->alt_missileDlight = 200;

		weaponInfo->alt_missileTrailFunc = FX_DreadnoughtProjectileThink;;

		weaponInfo->firingSound = trap_S_RegisterSound( SOUND_DIR "dreadnought/dn_firing.wav" );
		weaponInfo->flashSound = trap_S_RegisterSound( SOUND_DIR "dreadnought/dn_start.wav" );
		weaponInfo->altFlashSnd = trap_S_RegisterSound( SOUND_DIR "dreadnought/dn_altfire.wav" );
		weaponInfo->stopSound = trap_S_RegisterSound(SOUND_DIR "dreadnought/dn_stop.wav");
		weaponInfo->alt_missileSound = trap_S_RegisterSound(SOUND_DIR "dreadnought/dn_altmissile.wav");
		weaponInfo->altHitSound = trap_S_RegisterSound(SOUND_DIR "dreadnought/dn_althit.wav");

		cgs.media.dnBoltShader				= trap_R_RegisterShader( "gfx/misc/dnBolt" );
		cgs.media.stasisRingShader			= trap_R_RegisterShader( "gfx/misc/stasis_ring" );
		break;

	case WP_STASIS:
		weaponInfo->missileTrailFunc = FX_StasisProjectileThink;
		weaponInfo->missileDlight = 200;
		MAKERGB( weaponInfo->missileDlightColor, 0.6, 0.6, 1 );
		MAKERGB( weaponInfo->flashDlightColor, 0.6, 0.6, 1 );

		weaponInfo->flashSound = trap_S_RegisterSound( SOUND_DIR "stasis/fire.wav" );
		weaponInfo->altFlashSnd = trap_S_RegisterSound( SOUND_DIR "stasis/alt_fire.wav" );
		weaponInfo->mainHitSound = trap_S_RegisterSound(SOUND_DIR "stasis/hit_wall.wav");
	
		cgs.media.stasisRingShader			= trap_R_RegisterShader( "gfx/misc/stasis_ring" );
		cgs.media.stasisAltShader			= trap_R_RegisterShader( "gfx/misc/stasis_altfire" );
		cgs.media.altIMOD2Shader			= trap_R_RegisterShader( "gfx/misc/IMOD2alt" );
		cgs.media.dnBoltShader				= trap_R_RegisterShader( "gfx/misc/dnBolt" );
		break;

	case WP_GRENADE_LAUNCHER:
		weaponInfo->missileModel = trap_R_RegisterModel( "models/weapons2/launcher/projectile.md3" );
		weaponInfo->alt_missileModel = trap_R_RegisterModel( "models/weapons2/launcher/projectile2.md3" );

		weaponInfo->missileTrailFunc = FX_GrenadeThink;

		MAKERGB( weaponInfo->flashDlightColor, 0.6, 0.6, 1 );
		weaponInfo->flashSound = trap_S_RegisterSound( SOUND_DIR "glauncher/fire.wav" );
		weaponInfo->altFlashSnd = trap_S_RegisterSound( SOUND_DIR "glauncher/alt_fire.wav" );
		weaponInfo->altHitSound = trap_S_RegisterSound( SOUND_DIR "glauncher/beep.wav" );
		cgs.media.grenadeAltStickSound = trap_S_RegisterSound(SOUND_DIR "glauncher/alt_stick.wav");
		cgs.media.grenadeBounceSound1 = trap_S_RegisterSound(SOUND_DIR "glauncher/bounce1.wav");
		cgs.media.grenadeBounceSound2 = trap_S_RegisterSound(SOUND_DIR "glauncher/bounce2.wav");
		cgs.media.grenadeExplodeSound = trap_S_RegisterSound(SOUND_DIR "glauncher/explode.wav");

		cgs.media.orangeTrailShader			= trap_R_RegisterShader( "gfx/misc/orangetrail" );
		cgs.media.compressionMarkShader		= trap_R_RegisterShader( "gfx/damage/burnmark1" );
		if ( cgs.pModSpecialties || cg_buildScript.integer )
		{//tripwire shader
			cgs.media.whiteLaserShader		= trap_R_RegisterShader( "gfx/effects/whitelaser" );
			cgs.media.borgEyeFlareShader	= trap_R_RegisterShader( "gfx/misc/borgeyeflare" );
		}
		break;

	case WP_SCAVENGER_RIFLE:
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.6, 0.6 );

		weaponInfo->flashSound = trap_S_RegisterSound( SOUND_DIR "scavenger/fire.wav" );
		weaponInfo->altFlashSnd = trap_S_RegisterSound( SOUND_DIR "scavenger/alt_fire.wav" );
		weaponInfo->mainHitSound = trap_S_RegisterSound(SOUND_DIR "scavenger/hit_wall.wav");
		weaponInfo->altHitSound = trap_S_RegisterSound(SOUND_DIR "scavenger/alt_explode.wav");
		weaponInfo->missileTrailFunc = FX_ScavengerProjectileThink;
		weaponInfo->alt_missileTrailFunc = FX_ScavengerAltFireThink;
//		weaponInfo->wiTrailTime = 100;
//		weaponInfo->trailRadius = 8;
		cgs.media.tetrionFlareShader		= trap_R_RegisterShader( "gfx/misc/tet1" );
		cgs.media.tetrionTrail2Shader		= trap_R_RegisterShader( "gfx/misc/trail2" );
		cgs.media.redFlareShader			= trap_R_RegisterShader( "gfx/misc/red_flare" );

		cgs.media.scavengerAltShader		= trap_R_RegisterShader( "gfx/misc/scavaltfire" );
		cgs.media.scavExplosionFastShader	= trap_R_RegisterShader( "scavExplosionFast" );
		cgs.media.scavExplosionSlowShader	= trap_R_RegisterShader( "scavExplosionSlow" );
		cgs.media.compressionMarkShader		= trap_R_RegisterShader( "gfx/damage/burnmark1" );
		break;

	case WP_QUANTUM_BURST:
		MAKERGB( weaponInfo->flashDlightColor, 0.6, 0.6, 1 );	//Bluish

		weaponInfo->missileTrailFunc = FX_QuantumThink;
		weaponInfo->alt_missileTrailFunc = FX_QuantumAltThink;
		
		weaponInfo->missileDlight = 75;
		weaponInfo->alt_missileDlight = 100;
		MAKERGB( weaponInfo->missileDlightColor, 1.0, 1.0, 0.5);	//yellowish

		weaponInfo->flashSound = trap_S_RegisterSound( SOUND_DIR "quantum/fire.wav" );
		weaponInfo->altFlashSnd = trap_S_RegisterSound( SOUND_DIR "quantum/alt_fire.wav" );

		weaponInfo->mainHitSound = trap_S_RegisterSound( SOUND_DIR "quantum/hit_wall.wav" );;		
		weaponInfo->altHitSound = trap_S_RegisterSound( SOUND_DIR "quantum/alt_hit_wall.wav" );;		

		cgs.media.whiteRingShader			= trap_R_RegisterShader( "gfx/misc/whitering" );
		cgs.media.orangeRingShader			= trap_R_RegisterShader( "gfx/misc/orangering" );
		cgs.media.quantumExplosionShader	= trap_R_RegisterShader( "quantumExplosion" );
		cgs.media.quantumFlashShader		= trap_R_RegisterShader( "yellowflash" );
		cgs.media.bigBoomShader				= trap_R_RegisterShader( "gfx/misc/bigboom" );
		cgs.media.orangeTrailShader			= trap_R_RegisterShader( "gfx/misc/orangetrail" );
		cgs.media.compressionMarkShader		= trap_R_RegisterShader( "gfx/damage/burnmark1" );
		cgs.media.orangeTrailShader			= trap_R_RegisterShader( "gfx/misc/orangetrail" );
		cgs.media.quantumRingShader	    	= trap_R_RegisterShader( "gfx/misc/detpack3" );
		cgs.media.quantumBoom		    	= trap_S_RegisterSound ( SOUND_DIR "explosions/explode5.wav" );
		break;

	case WP_IMOD:
		MAKERGB( weaponInfo->flashDlightColor, 0.6, 0.6, 1 );

		weaponInfo->flashSound = trap_S_RegisterSound( SOUND_DIR "IMOD/fire.wav" );
		weaponInfo->altFlashSnd = trap_S_RegisterSound( SOUND_DIR "IMOD/alt_fire.wav" );

		cgs.media.IMODShader			= trap_R_RegisterShader( "gfx/misc/IMOD" );
		cgs.media.IMOD2Shader			= trap_R_RegisterShader( "gfx/misc/IMOD2" );
		cgs.media.altIMODShader			= trap_R_RegisterShader( "gfx/misc/IMODalt" );
		cgs.media.altIMOD2Shader		= trap_R_RegisterShader( "gfx/misc/IMOD2alt" );
		cgs.media.imodExplosionShader	= trap_R_RegisterShader( "imodExplosion" );
		break;

	case WP_COMPRESSION_RIFLE:
		MAKERGB( weaponInfo->flashDlightColor, 0.16, 0.16, 1 );

		weaponInfo->flashSound = trap_S_RegisterSound( SOUND_DIR "prifle/fire.wav" );
		weaponInfo->altFlashSnd = trap_S_RegisterSound( SOUND_DIR "prifle/alt_fire_new.wav" );

		weaponInfo->mainHitSound = trap_S_RegisterSound( SOUND_DIR "prifle/impact.wav" );;		
		
		cgs.media.prifleImpactShader	= trap_R_RegisterShader( "gfx/effects/prifle_hit" );
		cgs.media.compressionAltBeamShader	= trap_R_RegisterShader( "gfx/effects/prifle_altbeam" );
		cgs.media.compressionAltBlastShader	= trap_R_RegisterShader( "gfx/effects/prifle_altblast" );
		cgs.media.compressionMarkShader		= trap_R_RegisterShader( "gfx/damage/burnmark1" );
		break;

	case WP_TETRION_DISRUPTOR:
		MAKERGB( weaponInfo->flashDlightColor, 0.6, 0.6, 1 );
		
		weaponInfo->flashSound = trap_S_RegisterSound( SOUND_DIR "tetrion/fire.wav" );
		weaponInfo->altFlashSnd = trap_S_RegisterSound( SOUND_DIR "tetrion/alt_fire.wav" );
		cgs.media.tetrionRicochetSound1 = trap_S_RegisterSound(SOUND_DIR "tetrion/ricochet1.wav");
		cgs.media.tetrionRicochetSound2 = trap_S_RegisterSound(SOUND_DIR "tetrion/ricochet2.wav");
		cgs.media.tetrionRicochetSound3 = trap_S_RegisterSound(SOUND_DIR "tetrion/ricochet3.wav");

		weaponInfo->missileTrailFunc = FX_TetrionProjectileThink;
		weaponInfo->alt_missileTrailFunc = FX_TetrionProjectileThink;

		cgs.media.greenBurstShader			= trap_R_RegisterShader( "gfx/misc/greenburst" );
		cgs.media.greenTrailShader			= trap_R_RegisterShader( "gfx/misc/greentrail" );
		cgs.media.tetrionTrail2Shader		= trap_R_RegisterShader( "gfx/misc/trail2" );
		cgs.media.tetrionFlareShader		= trap_R_RegisterShader( "gfx/misc/tet1" );
		cgs.media.borgFlareShader			= trap_R_RegisterShader( "gfx/misc/borgflare" );
		cgs.media.bulletmarksShader			= trap_R_RegisterShader( "textures/decals/bulletmark4" );
		break;

	case WP_VOYAGER_HYPO:
		weaponInfo->flashSound = weaponInfo->altFlashSnd = trap_S_RegisterSound( "sound/items/jetpuffmed.wav" );
		break;

	case WP_BORG_ASSIMILATOR:
		cgs.media.borgBeamInSound	= trap_S_RegisterSound("sound/enemies/borg/borgbeam.wav" );
		weaponInfo->flashSound		= trap_S_RegisterSound( "sound/enemies/borg/borgass.wav" );
		weaponInfo->firingSound		= trap_S_RegisterSound( "sound/enemies/borg/borgassloop.wav" );
		weaponInfo->altFlashSnd		= trap_S_RegisterSound( "sound/enemies/borg/borgass.wav" );
		weaponInfo->altFiringSound		= trap_S_RegisterSound( "sound/enemies/borg/borgassloop.wav" );
		cgs.media.borgFlareShader	= trap_R_RegisterShader( "gfx/misc/borgflare" );
		break;

	case WP_BORG_WEAPON:
		cgs.media.borgBeamInSound	= trap_S_RegisterSound("sound/enemies/borg/borgbeam.wav");
		// FIXME: Temp stuff
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.6, 0.6 );
		weaponInfo->flashSound				= trap_S_RegisterSound( "sound/weapons/borg/borgshoot.wav" );
		weaponInfo->mainHitSound			= trap_S_RegisterSound( "sound/weapons/borg/borghit.wav" );
		weaponInfo->altFlashSnd				= trap_S_RegisterSound( "sound/enemies/borg/borgtaser.wav" );
		weaponInfo->missileTrailFunc		= FX_BorgProjectileThink;
		cgs.media.tetrionFlareShader		= trap_R_RegisterShader( "gfx/misc/tet1" );
		cgs.media.tetrionTrail2Shader		= trap_R_RegisterShader( "gfx/misc/trail2" );
		cgs.media.borgFlareShader			= trap_R_RegisterShader( "gfx/misc/borgflare" );
		cgs.media.redFlareShader			= trap_R_RegisterShader( "gfx/misc/red_flare" );
		for ( i = 0; i < 4; i++ ) {
			cgs.media.borgLightningShaders[i] = trap_R_RegisterShader( va( "gfx/misc/blightning%i", i+1 ) );
		}

		cgs.media.compressionMarkShader		= trap_R_RegisterShader( "gfx/damage/burnmark1" );
		break;

	 default:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 1 );
		weaponInfo->flashSound = trap_S_RegisterSound( SOUND_DIR "prifle/fire.wav" );
		break;
	}
}

/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals( int itemNum ) {
	itemInfo_t		*itemInfo;
	gitem_t			*item;

	itemInfo = &cg_items[ itemNum ];
	if ( itemInfo->registered ) {
		return;
	}

	item = &bg_itemlist[ itemNum ];

	memset( itemInfo, 0, sizeof( &itemInfo ) );
	itemInfo->registered = qtrue;

	itemInfo->model = trap_R_RegisterModel( item->world_model );

	itemInfo->icon = trap_R_RegisterShader( item->icon );

	if ( item->giType == IT_WEAPON ) {
		CG_RegisterWeapon( item->giTag );
	}

	// since the seeker uses the scavenger rifes sounds, we must precache the scavenger rifle stuff if we hit the item seeker
	if ( item->giTag == PW_SEEKER)
	{
		CG_RegisterWeapon( WP_SCAVENGER_RIFLE );
	}

	// hang onto the handles for holdable items in case they're useable (e.g. detpack)
/*	if (IT_HOLDABLE == item->giType)
	{
		// sanity check
		if ( (item->giTag < HI_NUM_HOLDABLE) && (item->giTag > 0) ) // use "> 0" cuz first slot should be empty
		{
			if (item->world_model[1])
			{
				cgs.useableModels[item->giTag] = trap_R_RegisterModel( item->useablemodel );
			}
			else
			{
				cgs.useableModels[item->giTag] = itemInfo->model];
			}
		}
	}
*/
}


/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

/*
=================
CG_MapTorsoToWeaponFrame

=================
*/
static int CG_MapTorsoToWeaponFrame( clientInfo_t *ci, int frame ) {

	// change weapon
	if ( frame >= ci->animations[TORSO_DROP].firstFrame 
		&& frame < ci->animations[TORSO_DROP].firstFrame + 9 ) {
		return frame - ci->animations[TORSO_DROP].firstFrame + 6;
	}

	// stand attack
	if ( frame >= ci->animations[TORSO_ATTACK].firstFrame 
		&& frame < ci->animations[TORSO_ATTACK].firstFrame + 6 ) {
		return 1 + frame - ci->animations[TORSO_ATTACK].firstFrame;
	}

	// stand attack 2
	if ( frame >= ci->animations[TORSO_ATTACK2].firstFrame 
		&& frame < ci->animations[TORSO_ATTACK2].firstFrame + 6 ) {
		return 1 + frame - ci->animations[TORSO_ATTACK2].firstFrame;
	}
	
	return 0;
}


/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition( vec3_t origin, vec3_t angles ) {
	float	scale;
	int		delta;
	float	fracsin;

	VectorCopy( cg.refdef.vieworg, origin );
	VectorCopy( cg.refdefViewAngles, angles );

	// on odd legs, invert some angles
	if ( cg.bobcycle & 1 ) {
		scale = -cg.xyspeed;
	} else {
		scale = cg.xyspeed;
	}

	// gun angles from bobbing
	angles[ROLL] += scale * cg.bobfracsin * 0.005;
	angles[YAW] += scale * cg.bobfracsin * 0.01;
	angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005;

	// drop the weapon when landing
	delta = cg.time - cg.landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		origin[2] += cg.landChange*0.25 * delta / LAND_DEFLECT_TIME;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		origin[2] += cg.landChange*0.25 * 
			(LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
	}

#if 0
	// drop the weapon when stair climbing
	delta = cg.time - cg.stepTime;
	if ( delta < STEP_TIME/2 ) {
		origin[2] -= cg.stepChange*0.25 * delta / (STEP_TIME/2);
	} else if ( delta < STEP_TIME ) {
		origin[2] -= cg.stepChange*0.25 * (STEP_TIME - delta) / (STEP_TIME/2);
	}
#endif

	// idle drift
	scale = cg.xyspeed + 40;
	fracsin = sin( cg.time * 0.001 );
	angles[ROLL] += scale * fracsin * 0.01;
	angles[YAW] += scale * fracsin * 0.01;
	angles[PITCH] += scale * fracsin * 0.01;
}


/*
===============
CG_LightningBolt

Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
The cent should be the non-predicted cent if it is from the player,
so the endpoint will reflect the simulated strike (lagging the predicted
angle)
===============
*/

#define RANGE_BEAM (2048.0)
#define BEAM_VARIATION	6

void CG_LightningBolt( centity_t *cent, vec3_t origin ) 
{
	trace_t		trace;
//	gentity_t	*traceEnt;
	vec3_t		startpos, endpos, forward;
	qboolean	spark = qfalse, impact = qtrue;
	int i;

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) 
	{
		return;		// Don't draw a phaser during an intermission you crezzy mon!
	}

	//Must be a durational weapon
	if ( cent->currentState.clientNum == cg.snap->ps.clientNum
		&& !cg.renderingThirdPerson ) {
		// different checks for first person view
		if ( ( cg.snap->ps.weapon == WP_DREADNOUGHT && !( cg.snap->ps.eFlags & EF_ALT_FIRING )) 
				|| cg.snap->ps.weapon == WP_PHASER)
		{	/*continue*/	}
		else
			return;
	} else {
		if ( ( cent->currentState.weapon == WP_DREADNOUGHT && !( cent->currentState.eFlags & EF_ALT_FIRING )) 
				|| cent->currentState.weapon == WP_PHASER)
		{	/*continue*/	}
		else
			return;
	}
	
	// Find the impact point of the beam
	if ( cent->currentState.clientNum == cg.snap->ps.clientNum
		&& !cg.renderingThirdPerson ) {
		// take origin from view
/*		
		VectorCopy( cg.refdef.vieworg, origin );
		VectorMA( origin, -8, cg.refdef.viewaxis[2], origin );
		VectorMA( origin, 8, cg.refdef.viewaxis[0], origin );
		VectorMA( origin, -2, cg.refdef.viewaxis[1], origin );
*/
		VectorCopy( cg.refdef.viewaxis[0], forward );
		VectorCopy( cg.refdef.vieworg, startpos);
	} 
	else 
	{
		// take origin from entity
		AngleVectors( cent->lerpAngles, forward, NULL, NULL );
		VectorCopy( origin, startpos);

		// Check first from the center to the muzzle.
		CG_Trace(&trace, cent->lerpOrigin, vec3_origin, vec3_origin, origin, cent->currentState.number, MASK_SHOT);
		if (trace.fraction < 1.0)
		{	// We hit something here...  Stomp the muzzle back to the eye...
			VectorCopy(cent->lerpOrigin, startpos);
			startpos[2] += cg.snap->ps.viewheight;
		}
	}

	VectorMA( startpos, RANGE_BEAM, forward, endpos );

	// Add a subtle variation to the beam weapon's endpoint
	for (i = 0; i < 3; i ++ )
	{
		endpos[i] += crandom() * BEAM_VARIATION;
	}

	CG_Trace( &trace, startpos, vec3_origin, vec3_origin, endpos, cent->currentState.number, MASK_SHOT );

//	traceEnt = &g_entities[ trace.entityNum ];

	// Make sparking be a bit less frame-rate dependent..also never add sparking when we hit a surface with a NOIMPACT flag
	if (!(trace.surfaceFlags & SURF_NOIMPACT))
	{
		spark = qtrue;
	}

	// Don't draw certain kinds of impacts when it hits a player and such..or when we hit a surface with a NOIMPACT flag
	if ( cg_entities[trace.entityNum].currentState.eType == ET_PLAYER || (trace.surfaceFlags & SURF_NOIMPACT) )
	{
		impact = qfalse;
	}
	
	// Add in the effect
	switch ( cent->currentState.weapon )
	{
	case WP_PHASER:
		if (cg.snap->ps.rechargeTime == 0)
		{
			if (  cent->currentState.eFlags & EF_ALT_FIRING )
				FX_PhaserAltFire( origin, trace.endpos, trace.plane.normal, spark, impact, cent->pe.empty );
			else
				FX_PhaserFire( origin, trace.endpos, trace.plane.normal, spark, impact, cent->pe.empty );
		}
		break;

	case WP_DREADNOUGHT:
		if (!(cent->currentState.eFlags & EF_ALT_FIRING))
		{
			vec3_t org;

			// Move the beam back a bit to help cover up the poly edges on the fire beam
			VectorMA( origin, -4, forward, org );
			FX_DreadnoughtFire( org, trace.endpos, trace.plane.normal, spark, impact );
		}
		break;
	}
}


/*
======================
CG_MachinegunSpinAngle
======================
*/
#define		SPIN_SPEED	0.9
#define		COAST_TIME	1000
static float	CG_MachinegunSpinAngle( centity_t *cent ) {
	int		delta;
	float	angle;
	float	speed;

	delta = cg.time - cent->pe.barrelTime;
	if ( cent->pe.barrelSpinning ) {
		angle = cent->pe.barrelAngle + delta * SPIN_SPEED;
	} else {
		if ( delta > COAST_TIME ) {
			delta = COAST_TIME;
		}

		speed = 0.5 * ( SPIN_SPEED + (float)( COAST_TIME - delta ) / COAST_TIME );
		angle = cent->pe.barrelAngle + delta * speed;
	}

	if ( cent->pe.barrelSpinning == !(cent->currentState.eFlags & EF_FIRING) ) {
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleMod( angle );
		cent->pe.barrelSpinning = !!(cent->currentState.eFlags & EF_FIRING);
	}

	return angle;
}


/*
========================
CG_AddWeaponWithPowerups
========================
*/

static void CG_AddWeaponWithPowerups( refEntity_t *gun, int powerups ) 
{
	// add powerup effects
	if ( powerups & ( 1 << PW_INVIS ) ) {
		if ( cg.snap->ps.persistant[PERS_CLASS] == PC_TECH )
		{//technicians can see cloaked people
			trap_R_AddRefEntityToScene( gun );
		}
		gun->customShader = cgs.media.invisShader;
		trap_R_AddRefEntityToScene( gun );
	} else {
		trap_R_AddRefEntityToScene( gun );

		if ( powerups & ( 1 << PW_BATTLESUIT ) ) {
			gun->customShader = cgs.media.battleWeaponShader;
			trap_R_AddRefEntityToScene( gun );
		}
		if ( powerups & ( 1 << PW_BORG_ADAPT ))
		{
			gun->customShader = cgs.media.borgFullBodyShieldShader;
			trap_R_AddRefEntityToScene( gun );
		}

		if ( powerups & ( 1 << PW_QUAD ) ) {
			gun->customShader = cgs.media.quadWeaponShader;
			trap_R_AddRefEntityToScene( gun );
		}
		if (powerups & (1 << PW_OUCH))
		{
			gun->customShader = cgs.media.holoOuchShader;
			// set rgb to 1 of 16 values from 0 to 255. don't use random so that the three
			//parts of the player model as well as the gun will all look the same
			gun->shaderRGBA[0] = 
			gun->shaderRGBA[1] = 
			gun->shaderRGBA[2] = ((cg.time % 17)*0.0625)*255;//irandom(0,255);
			trap_R_AddRefEntityToScene(gun);
		}
	}
}


/*
=============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
=============
*/
void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent ) {
	refEntity_t	gun;
	refEntity_t	barrel;
	refEntity_t	flash;
	vec3_t		angles;
	weapon_t	weaponNum;
	weaponInfo_t	*weapon;
	centity_t	*nonPredictedCent;
	int				i = 0, numBarrels = 0;
	wpnBarrelInfo_t	*barrelInfo = NULL;

	weaponNum = cent->currentState.weapon;

	CG_RegisterWeapon( weaponNum );
	weapon = &cg_weapons[weaponNum];

	// add the weapon
	memset( &gun, 0, sizeof( gun ) );
	VectorCopy( parent->lightingOrigin, gun.lightingOrigin );
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx = parent->renderfx;

	// set custom shading for railgun refire rate
	if ( ps ) {
		if ( cg.predictedPlayerState.weapon == WP_IMOD 
			&& cg.predictedPlayerState.weaponstate == WEAPON_FIRING ) {
			float	f;

			f = (float)cg.predictedPlayerState.weaponTime / 1500;
			gun.shaderRGBA[1] = 0;
			gun.shaderRGBA[0] = 
			gun.shaderRGBA[2] = 255 * ( 1.0 - f );
		} else {
			gun.shaderRGBA[0] = 255;
			gun.shaderRGBA[1] = 255;
			gun.shaderRGBA[2] = 255;
			gun.shaderRGBA[3] = 255;
		}
	}

	if (ps)
	{
		gun.hModel = weapon->viewModel;
	}
	else
	{
		gun.hModel = weapon->weaponModel;
	}

	if (!gun.hModel) {
		return;
	}

	if ( !ps ) {
		// add weapon stop sound
		if ( !( cent->currentState.eFlags & EF_FIRING ) && cent->pe.lightningFiring &&
			cg.predictedPlayerState.ammo[cg.predictedPlayerState.weapon] )
		{
			if (weapon->stopSound)
			{
				trap_S_StartSound( cent->lerpOrigin, cent->currentState.number, CHAN_WEAPON, weapon->stopSound );
			}
		}
		cent->pe.lightningFiring = qfalse;
		if ( cent->currentState.eFlags & EF_ALT_FIRING )
		{
			// hark, I smell hackery afoot
			if ((weaponNum == WP_PHASER) && !(cg.predictedPlayerState.ammo[WP_PHASER]))
			{
				trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.phaserEmptySound );
				cent->pe.lightningFiring = qtrue;
			}
			else if ( weapon->altFiringSound )
			{
				trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->altFiringSound );
				cent->pe.lightningFiring = qtrue;
			}
		}
		else if ( cent->currentState.eFlags & EF_FIRING )
		{
			if ((weaponNum == WP_PHASER) && !(cg.predictedPlayerState.ammo[WP_PHASER]))
			{
				trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.phaserEmptySound );
				cent->pe.lightningFiring = qtrue;
			}
			else if ( weapon->firingSound )
			{
				trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound );
				cent->pe.lightningFiring = qtrue;
			}
		}
	}

	CG_PositionEntityOnTag( &gun, parent, parent->hModel, "tag_weapon");

	CG_AddWeaponWithPowerups( &gun, cent->currentState.powerups );

	// add the spinning barrel
	//
	// 
	for (barrelInfo = wpnBarrelData; barrelInfo->giTag != WP_NONE; barrelInfo++)
	{
		if (barrelInfo->giTag == weaponNum)
		{
			numBarrels = barrelInfo->numBarrels;
			break;
		}
	}

	// don't add barrels to world model...only viewmodels
	if (ps)
	{
		for (i = 0; i < numBarrels; i++)
		{
			memset( &barrel, 0, sizeof( barrel ) );
			VectorCopy( parent->lightingOrigin, barrel.lightingOrigin );
			barrel.shadowPlane = parent->shadowPlane;
			barrel.renderfx = parent->renderfx;

			barrel.hModel = weapon->barrelModel[i];
			angles[YAW] = 0;
			angles[PITCH] = 0;
			if ( weaponNum == WP_TETRION_DISRUPTOR) {
				angles[ROLL] = CG_MachinegunSpinAngle( cent );
			} else {
				angles[ROLL] = 0;//CG_MachinegunSpinAngle( cent );
			}
			AnglesToAxis( angles, barrel.axis );

			if (!i) {
				CG_PositionRotatedEntityOnTag( &barrel, parent, weapon->handsModel, "tag_barrel" );
			} else {
				CG_PositionRotatedEntityOnTag( &barrel, parent, weapon->handsModel, va("tag_barrel%d",i+1) );
			}

			CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups );
		}
	}

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.clientNum];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on teh single player podiums), so
	// go ahead and use the cent
	if( ( nonPredictedCent - cg_entities ) != cent->currentState.clientNum ) {
		nonPredictedCent = cent;
	}

	// add the flash
	if ( (	weaponNum == WP_PHASER ||
			weaponNum == WP_DREADNOUGHT)
		&& ( nonPredictedCent->currentState.eFlags & EF_FIRING ) ) 
	{
		// continuous flash
	} 
	else 
	{
		// impulse flash
		if ( cg.time - cent->muzzleFlashTime > MUZZLE_FLASH_TIME) {
			return;
		}
	}

	memset( &flash, 0, sizeof( flash ) );
	VectorCopy( parent->lightingOrigin, flash.lightingOrigin );
	flash.shadowPlane = parent->shadowPlane;
	flash.renderfx = parent->renderfx;

	flash.hModel = weapon->flashModel;
	if (!flash.hModel) {
		return;
	}
	angles[YAW] = 0;
	angles[PITCH] = 0;
	angles[ROLL] = crandom() * 10;
	AnglesToAxis( angles, flash.axis );

	if (cent->pe.empty)
	{	// Make muzzle flash wussy when empty.
		flash.customShader = cgs.media.phaserMuzzleEmptyShader;
	}

	if (ps)
	{	// Rendering inside the head...
		CG_PositionRotatedEntityOnTag( &flash, &gun, weapon->viewModel, "tag_flash");
	}
	else
	{	// Rendering outside the head...
		CG_PositionRotatedEntityOnTag( &flash, &gun, weapon->weaponModel, "tag_flash");
	}
	trap_R_AddRefEntityToScene( &flash );

	if ( ps || cg.renderingThirdPerson || cent->currentState.number != cg.predictedPlayerState.clientNum ) 
	{
		// add phaser/dreadnought
		// grrr nonPredictedCent doesn't have the proper empty setting
		nonPredictedCent->pe.empty = cent->pe.empty;
		CG_LightningBolt( nonPredictedCent, flash.origin );

		// make a dlight for the flash
		if ( weapon->flashDlightColor[0] || weapon->flashDlightColor[1] || weapon->flashDlightColor[2] ) {
			trap_R_AddLightToScene( flash.origin, 200 + (rand()&31), weapon->flashDlightColor[0],
				weapon->flashDlightColor[1], weapon->flashDlightColor[2] );
		}
	}
}

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
void CG_AddViewWeapon( playerState_t *ps ) {
	refEntity_t	hand;
	centity_t	*cent;
	clientInfo_t	*ci;
	float		fovOffset;
	vec3_t		angles;
	weaponInfo_t	*weapon;

	if ( ps->persistant[PERS_TEAM] == TEAM_SPECTATOR || (ps->eFlags&EF_ELIMINATED) ) {
		return;
	}

	if ( ps->pm_type == PM_INTERMISSION ) {
		return;
	}

	// no gun if in third person view
	if ( cg.renderingThirdPerson ) {
		return;
	}

	// allow the gun to be completely removed
	if ( !cg_drawGun.integer ) {
		vec3_t		origin;

		if ( cg.predictedPlayerState.eFlags & EF_FIRING )
		{
			// special hack for phaser/dreadnought...
			VectorCopy( cg.refdef.vieworg, origin );
			VectorMA( origin, -8, cg.refdef.viewaxis[2], origin );
			CG_LightningBolt( &cg_entities[ps->clientNum], origin );
		}
		return;
	}

	// don't draw if testing a gun model
	if ( cg.testGun ) {
		return;
	}

	// drop gun lower at higher fov
	if ( cg_fov.integer > 80 ) {
		fovOffset = -0.2 * ( cg_fov.integer - 80 );
	} else {
		fovOffset = 0;
	}

	cent = &cg.predictedPlayerEntity;	// &cg_entities[cg.snap->ps.clientNum];
	CG_RegisterWeapon( ps->weapon );
	weapon = &cg_weapons[ ps->weapon ];

	memset (&hand, 0, sizeof(hand));

	// set up gun position
	CG_CalculateWeaponPosition( hand.origin, angles );

	VectorMA( hand.origin, cg_gun_x.value, cg.refdef.viewaxis[0], hand.origin );
	VectorMA( hand.origin, cg_gun_y.value, cg.refdef.viewaxis[1], hand.origin );
	VectorMA( hand.origin, (cg_gun_z.value+fovOffset), cg.refdef.viewaxis[2], hand.origin );

	AnglesToAxis( angles, hand.axis );

	// map torso animations to weapon animations
	if ( cg_gun_frame.integer ) {
		// development tool
		hand.frame = hand.oldframe = cg_gun_frame.integer;
		hand.backlerp = 0;
	} else {
		// get clientinfo for animation map
		ci = &cgs.clientinfo[ cent->currentState.clientNum ];
		hand.frame = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.frame );
		hand.oldframe = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.oldFrame );
		hand.backlerp = cent->pe.torso.backlerp;
	}

	hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;

	// add everything onto the hand
	CG_AddPlayerWeapon( &hand, ps, &cg.predictedPlayerEntity );
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/

void static CG_RegisterWeaponIcon( int weaponNum ) {
	weaponInfo_t	*weaponInfo;
	gitem_t			*item;

	weaponInfo = &cg_weapons[weaponNum];

	if ( weaponNum == 0 ) {
		return;
	}

	if ( weaponInfo->registered ) {
		return;
	}

	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( item->giType == IT_WEAPON && item->giTag == weaponNum ) {
			weaponInfo->item = item;
			break;
		}
	}
	if ( !item->classname ) {
		CG_Error( "Couldn't find weapon %i", weaponNum );
	}

	weaponInfo->weaponIcon = trap_R_RegisterShader( item->icon );
}

/*
===================
CG_DrawWeaponSelect
===================
*/
void CG_DrawWeaponSelect( void ) {
	int		i;
	int		bits;
	int		count;
	int		x, y, w;
	char	*name;
	float	*color;

	// don't display if dead
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	color = CG_FadeColor( cg.weaponSelectTime, WEAPON_SELECT_TIME );
	if ( !color ) {
		return;
	}
	trap_R_SetColor( color );

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	// count the number of weapons owned
	bits = cg.snap->ps.stats[ STAT_WEAPONS ];
	count = 0;
	for ( i = 1 ; i < 16 ; i++ ) {
		if ( bits & ( 1 << i ) ) {
			count++;
		}
	}

//	x = 320 - count * 20;
//	y = 432;

	x = 364 - (count * 20);
	y = 432;

	// Left end cap
	trap_R_SetColor(colorTable[CT_LTPURPLE2]);
	CG_DrawPic( x - 16, y - 4, 16, 50, cgs.media.weaponcap1);
	trap_R_SetColor(NULL);	

	for ( i = 1 ; i < 16 ; i++ ) {
		if ( !( bits & ( 1 << i ) ) ) {
			continue;
		}

		CG_RegisterWeaponIcon( i );	//short version

		// draw selection marker
		if ( i == cg.weaponSelect ) 
		{
			trap_R_SetColor(colorTable[CT_LTPURPLE1]);
		}
		else
		{
			trap_R_SetColor(colorTable[CT_DKPURPLE1]);
		}

		CG_DrawPic( x-4,y-4,38, 38, cgs.media.weaponbox);

		// draw weapon icon
		trap_R_SetColor(colorTable[CT_WHITE]);
		CG_DrawPic( x, y, 32, 32, cg_weapons[i].weaponIcon );

		// draw selection marker
		if ( i == cg.weaponSelect ) {
			CG_DrawPic( x-4, y-4, 40, 40, cgs.media.selectShader );
		}

		// no ammo cross on top
		if ( !cg.snap->ps.ammo[ i ] ) {
			CG_DrawPic( x, y, 32, 32, cgs.media.noammoShader );
		}

		x += 40;
	}

	// Right end cap
	trap_R_SetColor(colorTable[CT_LTPURPLE2]);
	CG_DrawPic( x - 20 + 18, y - 4, 16, 50, cgs.media.weaponcap2);
	trap_R_SetColor(NULL);	

	// draw the selected name
	if ( cg_weapons[ cg.weaponSelect ].item ) {
		name = cg_weapons[ cg.weaponSelect ].item->pickup_name;
		if ( name ) {
//			w = CG_DrawStrlen( name ) * BIGCHAR_WIDTH;
			w= UI_ProportionalStringWidth(name,UI_SMALLFONT);
			x = ( SCREEN_WIDTH - w ) / 2;
//			CG_DrawBigStringColor(x, y - 22, name, color);
			UI_DrawProportionalString(x, y - 22, name, UI_SMALLFONT,color);

		}
	}

	trap_R_SetColor( NULL );
}


/*
===============
CG_WeaponSelectable
===============
*/
static qboolean CG_WeaponSelectable( int i ) {
	if ( !cg.snap->ps.ammo[i] ) {
		return qfalse;
	}
	if ( ! (cg.snap->ps.stats[ STAT_WEAPONS ] & ( 1 << i ) ) ) {
		return qfalse;
	}

	return qtrue;
}

extern int altAmmoUsage[];
/*
{
	0,				//WP_NONE,
	2,				//WP_PHASER,				
	10,				//WP_COMPRESSION_RIFLE,	
	3,				//WP_IMOD,				
	5,				//WP_SCAVENGER_RIFLE,		
	1,				//WP_STASIS,				
	1,				//WP_GRENADE_LAUNCHER,	
	2,				//WP_TETRION_DISRUPTOR,	
	2,				//WP_QUANTUM_BURST,		
	5				//WP_DREADNOUGHT,
	20,				//WP_VOYAGER_HYPO,
	##,				//WP_BORG_ASSIMILATOR,
	##,				//WP_BORG_WEAPON,

};
*/

/*
===============
CG_WeaponAltSelectable
===============
*/
static qboolean CG_WeaponAltSelectable( int i ) {
	if ( cg.snap->ps.ammo[i] < altAmmoUsage[cg.snap->ps.weapon]) {
		return qfalse;
	}
	if ( ! (cg.snap->ps.stats[ STAT_WEAPONS ] & ( 1 << i ) ) ) {
		return qfalse;
	}

	return qtrue;
}


/*
===============
CG_NextWeapon_f
===============
*/
void CG_NextWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i = 0 ; i < 16 ; i++ ) {
		cg.weaponSelect++;
		if ( cg.weaponSelect == 16 ) {
			cg.weaponSelect = 0;
		}
		if ( CG_WeaponSelectable( cg.weaponSelect ) ) {
			break;
		}
	}
	if ( i == 16 ) {
		cg.weaponSelect = original;
	}
}

/*
===============
CG_PrevWeapon_f
===============
*/
void CG_PrevWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i = 0 ; i < 16 ; i++ ) {
		cg.weaponSelect--;
		if ( cg.weaponSelect == -1 ) {
			cg.weaponSelect = 15;
		}
		if ( CG_WeaponSelectable( cg.weaponSelect ) ) {
			break;
		}
	}
	if ( i == 16 ) {
		cg.weaponSelect = original;
	}
}

/*
===============
CG_Weapon_f
===============
*/
void CG_Weapon_f( void ) {
	int		num;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	num = atoi( CG_Argv( 1 ) );

	if ( num < 1 || num > 15 ) {
		return;
	}

	cg.weaponSelectTime = cg.time;

	if ( cg.snap->ps.persistant[PERS_CLASS] == PC_BORG )
	{//re-map them to proper weapon number
		if ( num == 1 )
		{
			num = WP_BORG_ASSIMILATOR;
		}
		else if ( num == 2 )
		{
			num = WP_BORG_WEAPON;
		}
	}
	if ( cg.snap->ps.persistant[PERS_CLASS] == PC_MEDIC )
	{//re-map them to proper weapon number
		if ( num == 2 )
		{
			num = WP_VOYAGER_HYPO;
		}
	}
	if ( ! ( cg.snap->ps.stats[STAT_WEAPONS] & ( 1 << num ) ) ) {
		return;		// don't have the weapon
	}

	cg.weaponSelect = num;
}

/*
===================
CG_OutOfAmmoChange

The current weapon has just run out of ammo
===================
*/
void CG_OutOfAmmoChange( qboolean altfire ) {
	int		i;

	cg.weaponSelectTime = cg.time;

	for ( i = 15 ; i > 0 ; i-- )
	{
		if (altfire)
		{
			if ( CG_WeaponAltSelectable( i ) )
			{
				cg.weaponSelect = i;
				break;
			}
		}
		else
		{
			if ( CG_WeaponSelectable( i ) )
			{
				cg.weaponSelect = i;
				break;
			}
		}
	}
}



/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event
================
*/
void CG_FireWeapon( centity_t *cent, qboolean alt_fire ) {
	entityState_t *ent;
	weaponInfo_t	*weap;

	ent = &cent->currentState;
	if ( ent->weapon == WP_NONE ) {
		return;
	}
	if ( ent->weapon >= WP_NUM_WEAPONS ) {
		CG_Error( "CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS" );
		return;
	}
	weap = &cg_weapons[ ent->weapon ];

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleFlashTime = cg.time;

	// lightning gun only does this this on initial press
	if (	ent->weapon == WP_PHASER ||
			ent->weapon == WP_DREADNOUGHT)
	{
		if ( cent->pe.lightningFiring ) {
			return;
		}
	}

	// play quad sound if needed
	if ( cent->currentState.powerups & ( 1 << PW_QUAD ) ) {
		trap_S_StartSound (NULL, cent->currentState.number, CHAN_ITEM, cgs.media.quadSound );
	}

	// play a sound
	if (alt_fire)
	{
		if ( weap->altFlashSnd )
		{
			trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->altFlashSnd );
		}
	}
	else
	{
		if ( weap->flashSound )
		{
			trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->flashSound );
		}
	}
}

/*
================
CG_FireSeeker

Caused by an EV_FIRE_WEAPON event
================
*/
void CG_FireSeeker( centity_t *cent )
{
	entityState_t *ent;
	weaponInfo_t	*weap;

	ent = &cent->currentState;
	weap = &cg_weapons[ WP_SCAVENGER_RIFLE ];

	trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->flashSound );
}

/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
void CG_MissileHitWall( centity_t *cent, int weapon, vec3_t origin, vec3_t dir ) 
{
	qhandle_t		mod;
	qhandle_t		mark;
	qhandle_t		shader;
	sfxHandle_t		sfx;
	float			radius;
	float			light;
	vec3_t			lightColor;
	localEntity_t	*le;
	qboolean		isSprite;
	int				duration;
	qboolean		alphaFade;
//	weaponInfo_t	*weaponInfo = &cg_weapons[weapon];

	mark = 0;
	radius = 32;
	sfx = 0;
	mod = 0;
	shader = 0;
	light = 0;
	lightColor[0] = 1;
	lightColor[1] = 1;
	lightColor[2] = 0;

	// set defaults
	isSprite = qfalse;
	duration = 600;

	switch ( weapon ) {
	default:
	case WP_PHASER:
		// no explosion at LG impact, it is added with the beam
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;	
	case WP_DREADNOUGHT:
		// no explosion at LG impact, it is added with the beam
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;
	case WP_GRENADE_LAUNCHER:
		FX_GrenadeExplode( origin, dir );
		return;
		break;
	case WP_STASIS:
		FX_StasisWeaponHitWall( origin, dir, cent->currentState.time2 );
		return;
		break;
	case WP_IMOD:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.imodExplosionShader;
		mark = cgs.media.energyMarkShader;
		radius = 24;
		break;
	case WP_COMPRESSION_RIFLE:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.imodExplosionShader;
		mark = cgs.media.energyMarkShader;
		radius = 24;
		break;
	case WP_TETRION_DISRUPTOR:
		FX_TetrionAltHitWall( origin, dir );
		return;
		break;
	case WP_SCAVENGER_RIFLE:
		if (cent->currentState.eFlags & EF_ALT_FIRING)
		{
			FX_ScavengerAltExplode( origin, dir );
		}
		else
		{
			FX_ScavengerWeaponHitWall( origin, dir, qfalse /*Not shot by NPC*/ );
		}
		return;
		break;
	case WP_BORG_WEAPON:
		if ( !( cent->currentState.eFlags & EF_ALT_FIRING ))
		{
			FX_BorgWeaponHitWall( origin, dir );
		}
		return;
		break;

	case WP_QUANTUM_BURST:
		if ( cent->currentState.eFlags & EF_ALT_FIRING )
		{
			FX_QuantumAltHitWall( origin, dir );
		}
		else
		{
			FX_QuantumHitWall( origin, dir );
		}
		return;
		break;
	}

	if ( sfx ) {
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx );
	}

	//
	// create the explosion
	//
	if ( mod ) {
		le = CG_MakeExplosion( origin, dir, 
							   mod,	shader,
							   duration, isSprite );
		le->light = light;
		VectorCopy( lightColor, le->lightColor );
	}

	//
	// impact mark
	//
	alphaFade = (mark == cgs.media.energyMarkShader);	// plasma fades alpha, all others fade color
	CG_ImpactMark( mark, origin, dir, random()*360, 1,1,1,1, alphaFade, radius, qfalse );
}


/*
=================
CG_MissileHitPlayer
=================
*/
void CG_MissileHitPlayer( centity_t *cent, int weapon, vec3_t origin, vec3_t dir)
{
	if (cent)
	{	// Showing blood is a no-no.

//		CG_Bleed( origin, cent->currentState.otherEntityNum );
	}

	CG_MissileHitWall( cent, weapon, origin, dir );
}


/*
=================
CG_BounceEffect

Caused by an EV_BOUNCE | EV_BOUNCE_HALF event
=================
*/

// big fixme. none of these sounds should be registered at runtime
void CG_BounceEffect( centity_t *cent, int weapon, vec3_t origin, vec3_t normal )
{
	switch( weapon )
	{
	case WP_GRENADE_LAUNCHER:
		if ( rand() & 1 ) {
			trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, trap_S_RegisterSound(SOUND_DIR "glauncher/bounce1.wav") );
		} else {
			trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, trap_S_RegisterSound(SOUND_DIR "glauncher/bounce2.wav") );
		}
		break;

	case WP_TETRION_DISRUPTOR:
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, trap_S_RegisterSound ( va(SOUND_DIR "tetrion/ricochet%d.wav", irandom(1, 3)) ) );
		FX_TetrionRicochet( origin, normal );	
		break;

	default:
		if ( rand() & 1 ) {
			trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, trap_S_RegisterSound(SOUND_DIR "glauncher/bounce1.wav") );
		} else {
			trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, trap_S_RegisterSound(SOUND_DIR "glauncher/bounce2.wav") );
		}
		break;
	}
}




/*
============================================================================

BULLETS

============================================================================
*/



/*
======================
CG_CalcMuzzlePoint
======================
*/
/*
static qboolean	CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle ) {
	vec3_t		forward;
	centity_t	*cent;
	int			anim;

	if ( entityNum == cg.snap->ps.clientNum ) {
		VectorCopy( cg.snap->ps.origin, muzzle );
		muzzle[2] += cg.snap->ps.viewheight;
		AngleVectors( cg.snap->ps.viewangles, forward, NULL, NULL );
		VectorMA( muzzle, 14, forward, muzzle );
		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if ( !cent->currentValid ) {
		return qfalse;
	}

	VectorCopy( cent->currentState.pos.trBase, muzzle );

	AngleVectors( cent->currentState.apos.trBase, forward, NULL, NULL );
	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR ) {
		muzzle[2] += CROUCH_VIEWHEIGHT;
	} else {
		muzzle[2] += DEFAULT_VIEWHEIGHT;
	}

	VectorMA( muzzle, 14, forward, muzzle );

	return qtrue;

}
*/



/*
================
CG_SurfaceExplosion

Adds an explosion to a surface
================
*/

#define NUM_SPARKS		12
#define NUM_PUFFS		1
#define NUM_EXPLOSIONS	4

void CG_SurfaceExplosion( vec3_t origin, vec3_t normal, float radius, float shake_speed, qboolean smoke )
{
	localEntity_t	*le;
	vec3_t			direction, new_org;
	vec3_t			sprayvel, velocity		= { 0, 0, 0 };
	vec3_t			temp_org, temp_vel;
	float			scale, dscale;
	int				i, numSparks;

	//Sparks

	numSparks = 32 + (random() * 16.0f);
	
	//VectorSet( normal, 0, 0, 1 );

	for ( i = 0; i < numSparks; i++ )
	{	
		scale = 0.25f + (random() * 2.0f);
		dscale = -scale*0.5;

		FXE_Spray( normal, 500, 150, 1.0f, sprayvel);

		FX_AddTrail( origin,
								sprayvel,
								qtrue,
								32.0f,
								-64.0f,
								scale,
								-scale,
								1.0f,
								0.0f,
								0.25f,
								4000.0f,
								cgs.media.sparkShader);
	}

	//Smoke

	//Move this out a little from the impact surface
	VectorMA( origin, 4, normal, new_org );
	VectorSet( velocity, 0.0f, 0.0f, 16.0f );

	for ( i = 0; i < 4; i++ )
	{
		VectorSet( temp_org, new_org[0] + (crandom() * 16.0f), new_org[1] + (crandom() * 16.0f), new_org[2] + (random() * 4.0f) );
		VectorSet( temp_vel, velocity[0] + (crandom() * 8.0f), velocity[1] + (crandom() * 8.0f), velocity[2] + (crandom() * 8.0f) );

		FX_AddSprite(	temp_org,
						temp_vel, 
						qfalse, 
						96.0f + (random() * 32.0f), 
						16.0f, 
						1.0f, 
						0.0f,
						20.0f + (crandom() * 90.0f),
						0.5f,
						2000.0f, 
						cgs.media.smokeShader);
	}

	//Core of the explosion

	//Orient the explosions to face the camera
	VectorSubtract( cg.refdef.vieworg, origin, direction );
	VectorNormalize( direction );

	//Tag the last one with a light
	le = CG_MakeExplosion2( origin, direction, cgs.media.explosionModel, 5, cgs.media.surfaceExplosionShader, 
							500, qfalse, radius * 0.02f + (random() * 0.3f), LEF_NONE);
	le->light = 150;
	VectorSet( le->lightColor, 0.9f, 0.8f, 0.5f );

	for ( i = 0; i < NUM_EXPLOSIONS-1; i ++)
	{
		VectorSet( new_org, (origin[0] + (32 + (crandom() * 8))*crandom()), (origin[1] + (32 + (crandom() * 8))*crandom()), (origin[2] + (32 + (crandom() * 8))*crandom()) );
		le = CG_MakeExplosion2( new_org, direction, cgs.media.explosionModel, 5, cgs.media.surfaceExplosionShader, 
								300 + (rand() & 99), qfalse, radius * 0.05f + (crandom() *0.3f), LEF_NONE);
	}

	//Shake the camera
	CG_ExplosionEffects( origin, shake_speed, 350 );

}
