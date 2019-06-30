#include "cg_local.h"
#include "fx_local.h"


#define SCAV_SPIN	0.3

void FX_ScavengerProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t		forward;
	qboolean	fired_from_NPC = qfalse;		// Always

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0 )
		forward[2] = 1;

	// The effect you'd see from first person looks horrible from third person..or when shot by an NPC,
	//		so we'll just tone down the effect so it's not so horrible. :)
	if ( fired_from_NPC )
	{
		// Energy glow
		FX_AddSprite( cent->lerpOrigin, 
						NULL, qfalse, 
						8.0f + random() * 8.0f, 0.0f, 
						0.7f, 0.0f, 
						random()*360, 0.0f, 
						1.0f, 
						cgs.media.tetrionFlareShader );

		// Spinning projectile core
		FX_AddSprite( cent->lerpOrigin, 
						NULL, qfalse, 
						8.0f + random() * 10.0f, 0.0f, 
						1.0f, 0.0f, 
						cg.time * SCAV_SPIN, 0.0f, 
						1.0f, 
						cgs.media.redFlareShader );

		// leave a cool tail
		FX_AddTrail( cent->lerpOrigin, 
						forward, qfalse, 
						16, 0, 
						1.0f, 0.0f, 
						0.8f, 0.0f, 
						0, 
						1, 
						cgs.media.tetrionTrail2Shader );
	}
	else
	{
		// Energy glow
		FX_AddSprite( cent->lerpOrigin, 
						NULL, qfalse, 
						16.0f + random() * 16.0f, 0.0f, 
						0.5f, 0.0f, 
						random()*360, 0.0f, 
						1.0f, 
						cgs.media.tetrionFlareShader );

		// Spinning projectile
		FX_AddSprite( cent->lerpOrigin, 
						NULL, qfalse, 
						4.0f + random() * 10.0f, 0.0f, 
						0.6f, 0.0f, 
						cg.time * SCAV_SPIN, 0.0f, 
						1.0f, 
						cgs.media.redFlareShader );

		// leave a cool tail
		FX_AddTrail( cent->lerpOrigin, 
						forward, qfalse, 
						64, 0, 
						1.4f, 0.0f, 
						0.6f, 0.0f, 
						0, 
						1, 
						cgs.media.tetrionTrail2Shader );
	}
}


/*
-------------------------
FX_ScavengerAltFireThink
-------------------------
*/
#define SCAV_TRAIL_SPACING 12

void FX_ScavengerAltFireThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t	diff;
	float	len;

	// Make a trail that's reasonably consistent and not so much based on frame rate.
	if (cent->thinkFlag)
	{
		VectorSubtract( cent->lerpOrigin, cent->beamEnd, diff );
	}
	else
	{
		VectorSubtract( cent->lerpOrigin, cent->currentState.origin2, diff );
	}

	len = VectorNormalize( diff );

	if ( len > SCAV_TRAIL_SPACING )
	{
		vec3_t		origin;
		int			i;
		float scale;

		for ( i = 0 ; i < len; i += SCAV_TRAIL_SPACING )
		{
			// Calc the right spot along the trail
			VectorMA( cent->lerpOrigin, -i, diff, origin );
			scale = 18.0f + (random()*5.0f);
			FX_AddSprite( origin, 
								NULL, qfalse, 
								scale, -8.75, 
								0.4f, 0.0f, 
								random() * 360, 0.0f, 
								250.0f, 
								cgs.media.scavengerAltShader );
		}

		// Stash the current position
		VectorCopy( cent->lerpOrigin, cent->beamEnd);
		cent->thinkFlag = 1;
	}

	// Glowing bit
	FX_AddSprite( cent->lerpOrigin, 
					NULL, qfalse, 
					24.0f + ( random() * 16.0f ), 0.0f, 
					1.0f, 0.0f, 
					0, 0.0f, 
					1.0f, 
					cgs.media.tetrionFlareShader );
}


/*
-------------------------
FX_ScavengerWeaponHitWall
-------------------------
*/
void FX_ScavengerWeaponHitWall( vec3_t origin, vec3_t normal, qboolean fired_by_NPC )
{
	weaponInfo_t	*weaponInfo = &cg_weapons[WP_SCAVENGER_RIFLE];

	// Tone down when shot by an NPC
	// FIXME:  is this really a good idea?
	if ( fired_by_NPC )
	{
		// Expanding shock ring
		FX_AddQuad( origin, normal, 
						0.5f, 6.4f, 
						0.8, 0.0, 
						random() * 360.0f, 
						200, 
						cgs.media.redRingShader );

		// Impact core
		FX_AddQuad( origin, normal, 
						12.0f + ( random() * 8.0f ), 3.2f, 
						0.6f, 0.0f, 
						cg.time * SCAV_SPIN, 
						100, 
						cgs.media.redFlareShader );
	}
	else
	{
		// Expanding shock ring
		FX_AddQuad( origin, normal, 
						8.0f, 12.8f, 
						1.0, 0.0, 
						random() * 360.0f, 
						200, 
						cgs.media.redRingShader );

		// Impact core
		FX_AddQuad( origin, normal, 
						24.0f + ( random() * 16.0f ), 6.4f, 
						0.8f, 0.0f, 
						cg.time * SCAV_SPIN, 
						100, 
						cgs.media.redFlareShader );
	}

	//Sound
	trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, weaponInfo->mainHitSound);
	
	CG_ImpactMark( cgs.media.scavMarkShader, origin, normal, random()*360, 1,1,1,0.2, qfalse, random() + 5.5f, qfalse );
}


/*
-------------------------
FX_ScavengerWeaponHitPlayer
-------------------------
*/
void FX_ScavengerWeaponHitPlayer( vec3_t origin, vec3_t normal, qboolean fired_by_NPC )
{
	if ( fired_by_NPC )
	{
		// Smaller expanding shock ring
		FX_AddQuad( origin, normal, 
						0.5f, 32.0f, 
						0.8, 0.0, 
						random() * 360.0f,
						125, 
						cgs.media.redRingShader );
	}
	else
	{
		// Expanding shock ring
		FX_AddQuad( origin, normal, 
						1.0f, 64.0f, 
						0.8, 0.0, 
						random() * 360.0f, 
						125, 
						cgs.media.redRingShader );
	}

	//Sound
//	cgi_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cg_weapons[WP_SCAVENGER_RIFLE].missileHitSound );
}



/*
-------------------------
FX_Scavenger_Alt_Explode
------------------------- 
*/
void FX_ScavengerAltExplode( vec3_t origin, vec3_t dir )
{
//	FXCylinder		*fx2;
	localEntity_t	*le;
	vec3_t			direction, org;
	int i;
	weaponInfo_t	*weaponInfo = &cg_weapons[WP_SCAVENGER_RIFLE];
	
	//Orient the explosions to face the camera
	VectorSubtract( cg.refdef.vieworg, origin, direction );
	VectorNormalize( direction );

	VectorMA( origin, 12, direction, org );
	// Add an explosion and tag a light to it
	le = CG_MakeExplosion2( org, direction, cgs.media.explosionModel, 5, cgs.media.scavExplosionSlowShader, 675, qfalse, 1.0f + (random()*0.5f), LEF_NONE);
	le->light = 150;
	le->refEntity.renderfx |= RF_NOSHADOW;
	VectorSet( le->lightColor, 1.0f, 0.6f, 0.6f );

	VectorSet( org, (org[0] + crandom() * 8), (org[1] + crandom() * 8), (org[2] + crandom() * 8) );
	CG_MakeExplosion2( org, direction, cgs.media.explosionModel, 5, cgs.media.scavExplosionFastShader, 375, qfalse, 0.7f + (random()*0.5f), LEF_NONE);

	//Sound
	trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, weaponInfo->altHitSound );	

	CG_ImpactMark( cgs.media.compressionMarkShader, origin, dir, random()*360, 1,1,1,0.2, qfalse, 
					random() * 6 + 20, qfalse );

	// Always orient horizontally
	VectorSet ( direction, 0,0,1 );

	le = FX_AddCylinder( origin, direction, 4, 0, 20, 210, 14, 140, 1.0, 0.0, 600, cgs.media.redRing2Shader, 1.5 );
	le->refEntity.data.cylinder.wrap = qtrue;
	le->refEntity.data.cylinder.stscale = 6;

	for (i = 0; i < 6; i++)
	{
		vec3_t velocity;

		FXE_Spray( dir, 300, 175, 0.8f, velocity);
		FX_AddTrail( origin, velocity, qtrue, 12.0f, -12.0f,
								2, -2, 1.0f, 1.0f, 0.2f, 1000.0f,  cgs.media.tetrionTrail2Shader);
	}
}
