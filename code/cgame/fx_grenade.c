#include "cg_local.h"
#include "fx_local.h"


/*
-------------------------
FX_GrenadeThink
-------------------------
*/ 

void FX_GrenadeThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	FX_AddSprite( cent->lerpOrigin, NULL, qfalse, 8.0f + random() * 32.0f, 0.0f, 0.75f, 0.75f, 0, 0.0f, 1, cgs.media.dkorangeParticleShader );
	if ( rand() & 1 )
		FX_AddSprite( cent->lerpOrigin, NULL, qfalse, 16.0f + random() * 32.0f, 0.0f, 0.6f, 0.6f, 0, 0.0f, 1, cgs.media.yellowParticleShader );
}

/*
-------------------------
FX_GrenadeHitWall
-------------------------
*/

void FX_GrenadeHitWall( vec3_t origin, vec3_t normal )
{
	trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.grenadeExplodeSound );	
	CG_SurfaceExplosion( origin, normal, 8, 1, qfalse );
}

/*
-------------------------
FX_GrenadeHitPlayer
-------------------------
*/

void FX_GrenadeHitPlayer( vec3_t origin, vec3_t normal )
{
	trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.grenadeExplodeSound );	
	CG_SurfaceExplosion( origin, normal, 8, 1, qfalse );
}

/*
-------------------------
FX_GrenadeExplode
-------------------------
*/

void FX_GrenadeExplode( vec3_t origin, vec3_t normal )
{
	localEntity_t	*le;
	vec3_t			direction, org, vel;
	int i;

	VectorSet( direction, 0,0,1 );

	// Add an explosion and tag a light to it
	le = CG_MakeExplosion2( origin, direction, cgs.media.nukeModel, 5, (qhandle_t)NULL, 250, qfalse, 25.0f, LEF_FADE_RGB);
	le->light = 150;
	le->refEntity.renderfx |= RF_NOSHADOW;

	VectorSet( le->lightColor, 1.0f, 0.6f, 0.2f );

	// Ground ring
	FX_AddQuad( origin, normal, 5, 100, 1.0, 0.0, random() * 360, 300, cgs.media.bigShockShader );
	// Flare
	VectorMA( origin, 12, direction, org );
	FX_AddSprite( org, NULL, qfalse, 160.0, -160.0, 1.0, 0.0, 0.0, 0.0, 200, cgs.media.sunnyFlareShader );//, FXF_NON_LINEAR_FADE );

	for (i = 0; i < 12; i++)
	{
		float width, length;
		FXE_Spray( normal, 470, 325, 0.5f, vel);
		length = 24.0 + random() * 12;
		width = 0.5 + random() * 2;
		FX_AddTrail( origin, vel, qtrue, length, -length, width, -width, 
						1.0f, 1.0f, 0.5f, 1000.0f,  cgs.media.orangeTrailShader);
	}

	trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.grenadeExplodeSound );	

	// Smoke and impact
//	FX_AddSpawner( origin, normal, NULL, NULL, 100, 25.0f, 2000.0f, (void *) CG_SmokeSpawn, NULL, 1024 );
	CG_ImpactMark( cgs.media.compressionMarkShader, origin, normal, random()*360, 1,1,1,1.0, qfalse, 
				random() * 16 + 48, qfalse );
}


void FX_GrenadeShrapnelExplode( vec3_t origin, vec3_t norm )
{
	localEntity_t	*le;
	vec3_t			direction, org, vel;
	int i;

	VectorCopy( norm, direction);

	// Add an explosion and tag a light to it
	le = CG_MakeExplosion2( origin, direction, cgs.media.nukeModel, 5, (qhandle_t)NULL, 250, qfalse, 25.0f, LEF_FADE_RGB);
	le->light = 150;
	le->refEntity.renderfx |= RF_NOSHADOW;

	VectorSet( le->lightColor, 1.0f, 0.6f, 0.2f );

	// Ground ring
	FX_AddQuad( origin, norm, 5, 100, 1.0, 0.0, random() * 360, 300, cgs.media.bigShockShader );
	// Flare
	VectorMA( origin, 12, direction, org );
	FX_AddSprite( org, NULL, qfalse, 160.0, -160.0, 1.0, 0.0, 0.0, 0.0, 200, cgs.media.sunnyFlareShader );//, FXF_NON_LINEAR_FADE );

	for (i = 0; i < 12; i++)
	{
		float width, length;
		FXE_Spray( norm, 470, 325, 0.5f, vel);
		length = 24.0 + random() * 12;
		width = 0.5 + random() * 2;
		FX_AddTrail( origin, vel, qtrue, length, -length, width, -width, 
						1.0f, 1.0f, 0.5f, 1000.0f,  cgs.media.orangeTrailShader);
	}

	trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.grenadeExplodeSound );	

	// Smoke and impact
	CG_ImpactMark( cgs.media.compressionMarkShader, origin, norm, random()*360, 1,1,1,1.0, qfalse, 
				random() * 16 + 48, qfalse );
}

qboolean GrenadeBeep(localEntity_t *le)
{
	weaponInfo_t	*weaponInfo = &cg_weapons[WP_GRENADE_LAUNCHER];

	trap_S_StartSound(le->refEntity.origin, ENTITYNUM_WORLD, CHAN_AUTO, weaponInfo->altHitSound);
	return qtrue;
}

#define FX_GRENADE_ALT_STICK_TIME		2500
void FX_GrenadeShrapnelBits( vec3_t start  )
{
	vec3_t	zero = {0, 0, 0};
	// check G_MissileStick() to make sure this killtime coincides with that nextthink
	FX_AddSpawner( start, zero, NULL, NULL, qfalse, 300,
							 0, FX_GRENADE_ALT_STICK_TIME, GrenadeBeep, 10 );
}



