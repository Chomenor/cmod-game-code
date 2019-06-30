#include "cg_local.h"
#include "fx_local.h"

/*
-------------------------
FX_DreadnoughtHitWall
-------------------------
*/
void FX_DreadnoughtHitWall( vec3_t origin, vec3_t normal, qboolean spark )
{
	float	scale = 1.0f + ( random() * 1.0f );
	int		num, i;
	localEntity_t	*le = NULL;
	vec3_t	vel;
//	weaponInfo_t	*weaponInfo = &cg_weapons[WP_DREADNOUGHT];

//	trap_S_StartSound(origin, ENTITYNUM_WORLD, CHAN_AUTO, weaponInfo->altHitSound);

	le = FX_AddQuad( origin, normal, 32.0 + random() * 48, 0, 0.5, 0.5, 0, 100, cgs.media.purpleParticleShader );
	if (le)
	{
		le->leFlags |= LEF_ONE_FRAME;
	}
	le = FX_AddQuad( origin, normal, 24.0 + random() * 32, 0, 0.6, 0.6, 0, 100, cgs.media.ltblueParticleShader );
	if (le)
	{
		le->leFlags |= LEF_ONE_FRAME;
	}

	CG_ImpactMark( cgs.media.scavMarkShader, origin, normal, random()*360, 1,1,1,0.2, qfalse, 
					random() * 4 + 8, qfalse );

	if ( spark )
	{
		trap_R_AddLightToScene( origin, 75 + (rand()&31), 1.0, 0.8, 1.0 );

		// Drop some sparks
		num = (int)(random() * 2) + 2;

		for ( i = 0; i < num; i++ )
		{
			scale = 0.6f + random();
			if ( rand() & 1 )
				FXE_Spray( normal, 70, 80, 0.9f, vel);
			else
				FXE_Spray( normal, 80, 200, 0.5f, vel);

			FX_AddTrail( origin, vel, qfalse, 8.0f + random() * 8, -48.0f,
									scale, -scale, 1.0f, 0.8f, 0.4f, 600.0f, cgs.media.spark2Shader );
		}
	}
}

/*
-------------------------
FX_DreadnoughtFire
-------------------------
*/
void FX_DreadnoughtFire( vec3_t origin, vec3_t end, vec3_t normal, qboolean spark, qboolean impact )
{
//	localEntity_t	*le = NULL;
	float	scale = 1.0f + ( random() * 1.0f );
	refEntity_t		beam;

		// Draw beams first.
	memset( &beam, 0, sizeof( beam ) );
	VectorCopy( origin, beam.origin);
	VectorCopy( end, beam.oldorigin );
	beam.reType = RT_LINE;
	beam.customShader = cgs.media.dnBoltShader;
	AxisClear( beam.axis );
	beam.shaderRGBA[0] = 0xff*0.2;
	beam.shaderRGBA[1] = 0xff*0.2;
	beam.shaderRGBA[2] = 0xff*0.2;
	beam.shaderRGBA[3] = 0xff;
	beam.data.line.stscale = 2.0;
	beam.data.line.width = scale*6;
	trap_R_AddRefEntityToScene( &beam );

	// Add second core beam
	memset( &beam, 0, sizeof( beam ) );
	VectorCopy( origin, beam.origin);
	VectorCopy( end, beam.oldorigin );
	beam.reType = RT_LINE;
	beam.customShader = cgs.media.dnBoltShader;
	AxisClear( beam.axis );
	beam.shaderRGBA[0] = 0xff*0.8;
	beam.shaderRGBA[1] = 0xff*0.8;
	beam.shaderRGBA[2] = 0xff*0.8;
	beam.shaderRGBA[3] = 0xff;
	beam.data.line.stscale = 1.0;
	beam.data.line.width = scale*4.5;
	trap_R_AddRefEntityToScene( &beam );

	if (spark)
	{
		// Add first electrical bolt
		memset( &beam, 0, sizeof( beam ) );
		VectorCopy( origin, beam.origin);
		VectorCopy( end, beam.oldorigin );
		beam.reType = RT_ELECTRICITY;
		beam.customShader = cgs.media.dnBoltShader;
		AxisClear( beam.axis );
		beam.shaderRGBA[0] = 0xff*0.8;
		beam.shaderRGBA[1] = 0xff*0.8;
		beam.shaderRGBA[2] = 0xff*0.8;
		beam.shaderRGBA[3] = 0xff;
		beam.data.electricity.stscale = 1.0;
		beam.data.electricity.width = scale*0.5;
		beam.data.electricity.deviation = 0.2;
		trap_R_AddRefEntityToScene( &beam );
	}

	// Add next electrical bolt
	memset( &beam, 0, sizeof( beam ) );
	VectorCopy( origin, beam.origin);
	VectorCopy( end, beam.oldorigin );
	beam.reType = RT_ELECTRICITY;
	beam.customShader = cgs.media.dnBoltShader;
	AxisClear( beam.axis );
	beam.shaderRGBA[0] = 0xff*0.8;
	beam.shaderRGBA[1] = 0xff*0.8;
	beam.shaderRGBA[2] = 0xff*0.8;
	beam.shaderRGBA[3] = 0xff;
	beam.data.electricity.stscale = 1.0;
	beam.data.electricity.width = scale*0.75;
	beam.data.electricity.deviation = 0.12;
	trap_R_AddRefEntityToScene( &beam );
	
/*

	le = FX_AddLine( origin, end, 2.0f, scale * 6, 0.0f, 0.2f, 0.2f, 100, cgs.media.dnBoltShader );
	if (le)
	{
		le->leFlags |= LEF_ONE_FRAME;
	}

	le = FX_AddLine( origin, end, 1.0f, scale * 4.5, 0.0f, 0.8f, 0.8f, 100, cgs.media.dnBoltShader );
	if (le)
	{
		le->leFlags |= LEF_ONE_FRAME;
	}
	
	if ( spark )
	{
		le = FX_AddElectricity( origin, end, 1.0f, scale * 0.5, 0, 0.8, 0.8, 100, cgs.media.dnBoltShader, 0.2 );
		if (le)
		{
			le->leFlags |= LEF_ONE_FRAME;
		}
	}
	le = FX_AddElectricity( origin, end, 1.0f, scale * 0.75, 0, 0.8, 0.8, 100, cgs.media.dnBoltShader, 0.12 );
	if (le)
	{
		le->leFlags |= LEF_ONE_FRAME;
	}
*/	
	// Add a subtle screen shake
	CG_ExplosionEffects( origin, 1.0f, 15 );

	if (impact)
	{
		FX_DreadnoughtHitWall( end, normal, spark );
	}
}

/*
-------------------------
FX_DreadnoughtProjectileThink

Freaky random lightning burst
-------------------------
*/
#define FX_DN_ALT_THINK_TIME	100

void FX_DreadnoughtProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
//	refEntity_t		beam;
	float scale;
	
	scale = flrandom(10.0, 15.0);

	// If this is a new thinking time, draw some starting stuff...
	if (cent->miscTime < cg.time)
	{
		trace_t trace;
		vec3_t fwd, right, boltdir, boltend, mins={-2,-2,-2}, maxs={2,2,2};
		float len;
		localEntity_t *le;
		int	playSound = 1;//(irandom(0,1) == 0)?1:0;

		cent->miscTime = cg.time + FX_DN_ALT_THINK_TIME;

		VectorSubtract(cent->currentState.origin, cent->currentState.origin2, fwd);

		// Throw a sprite from the start to the end over the next 
		VectorScale(fwd, 1000.0*(1.0/FX_DN_ALT_THINK_TIME), boltdir);
		le = FX_AddSprite(cent->currentState.origin2, boltdir, qfalse, scale*8, -scale*2, 1.0, 1.0, 0, 0, FX_DN_ALT_THINK_TIME, cgs.media.blueParticleShader);
		le->light = 200;
		le->refEntity.renderfx |= RF_NOSHADOW;
		VectorSet( le->lightColor, 0.5f, 0.8f, 1.0f );
		
		len = VectorNormalize(fwd);
		
		// Illegal if org and org2 are the same.
		if (len<=0)
			return;

		// Draw a bolt from the old position to the new.
		FX_AddLine( cent->currentState.origin2, cent->currentState.origin, 1.0, scale*4, -scale*4, 0.75, 0.0, FX_DN_ALT_THINK_TIME*2, cgs.media.dnBoltShader);

		// ALSO draw an electricity bolt from the old position to the new.
		FX_AddElectricity( cent->currentState.origin2,  cent->currentState.origin, 0.2f, scale, -scale, 1.0, 0.5, FX_DN_ALT_THINK_TIME*2, cgs.media.dnBoltShader, 0.5 );

		// And a bright new sprite at the current locale.
		FX_AddSprite(cent->currentState.origin, NULL, qfalse, scale*2, scale*4, 1.0, 1.0, 0, 0, FX_DN_ALT_THINK_TIME, cgs.media.blueParticleShader);

		// Put a sprite in the old position, fading away.
		FX_AddSprite(cent->currentState.origin2, NULL, qfalse, scale*5, -scale*5, 1.0, 1.0, 0, 0, FX_DN_ALT_THINK_TIME*2, cgs.media.blueParticleShader);

		// Shoot rays out (roughly) to the sides to connect with walls or whatever...
		// PerpendicularVector(right, fwd);
		right[0] = fwd[1];
		right[1] = -fwd[0];
		right[2] = -fwd[2];

		// Right vector
		// The boltdir uses a random offset to the perp vector.
		boltdir[0] = right[0] + flrandom(-0.25, 0.25);
		boltdir[1] = right[1] + flrandom(-0.25, 0.25);
		boltdir[2] = right[2] + flrandom(-1.0, 1.0);

		// Shoot a vector off to the side and trace till we hit a wall.
		VectorMA(cent->currentState.origin, 256, boltdir, boltend);
		CG_Trace( &trace, cent->currentState.origin, mins, maxs, boltend, cent->currentState.number, MASK_SOLID );
	
		if (trace.fraction < 1.0)
		{
			VectorCopy(trace.endpos, boltend);
			FX_AddElectricity( cent->currentState.origin, boltend, 0.2f, scale, -scale, 1.0, 0.5, FX_DN_ALT_THINK_TIME*2, cgs.media.dnBoltShader, 0.5 );
			// Put a sprite at the endpoint that stays.
			FX_AddQuad(trace.endpos, trace.plane.normal, scale, -scale*0.5, 1.0, 0.5, 0.0, FX_DN_ALT_THINK_TIME*2, cgs.media.blueParticleShader);
			if (playSound)
			{
				if (irandom(0,1))
				{
					weaponInfo_t	*weaponInfo = &cg_weapons[WP_DREADNOUGHT];
					trap_S_StartSound(trace.endpos, ENTITYNUM_WORLD, CHAN_AUTO, weaponInfo->alt_missileSound);
					playSound = 0;
				}
			}
		}

		// Left vector
		// The boltdir uses a random offset to the perp vector.
		boltdir[0] = -right[0] + flrandom(-0.25, 0.25);
		boltdir[1] = -right[1] + flrandom(-0.25, 0.25);
		boltdir[2] = -right[2] + flrandom(-1.0, 1.0);

		// Shoot a vector off to the side and trace till we hit a wall.
		VectorMA(cent->currentState.origin, 256, boltdir, boltend);
		CG_Trace( &trace, cent->currentState.origin, mins, maxs, boltend, cent->currentState.number, MASK_SOLID );
	
		if (trace.fraction < 1.0)
		{
			VectorCopy(trace.endpos, boltend);
			FX_AddElectricity( cent->currentState.origin, boltend, 0.2f, scale, -scale, 1.0, 0.5, FX_DN_ALT_THINK_TIME*2, cgs.media.dnBoltShader, 0.5 );
			// Put a sprite at the endpoint that stays.
			FX_AddQuad(trace.endpos, trace.plane.normal, scale, -scale*0.5, 1.0, 0.5, 0.0, FX_DN_ALT_THINK_TIME*2, cgs.media.blueParticleShader);
			if (playSound)
			{
				weaponInfo_t	*weaponInfo = &cg_weapons[WP_DREADNOUGHT];
				trap_S_StartSound(trace.endpos, ENTITYNUM_WORLD, CHAN_AUTO, weaponInfo->alt_missileSound);
				playSound = 0;
			}
		}
	}
}

/*
-------------------------
FX_DreadnoughtShotMiss

Alt-fire, miss effect
-------------------------
*/
void FX_DreadnoughtShotMiss( vec3_t end, vec3_t dir )
{
	vec3_t	org;
	weaponInfo_t	*weaponInfo = &cg_weapons[WP_DREADNOUGHT];

	trap_S_StartSound(end, ENTITYNUM_WORLD, CHAN_AUTO, weaponInfo->altHitSound);

	// Move me away from the wall a bit so that I don't z-buffer into it
	VectorMA( end, 0.5, dir, org );

	// Expanding rings
	FX_AddQuad( org, dir, 1, 24, 0.8, 0.2, random() * 360, 400, cgs.media.stasisRingShader );
	FX_AddQuad( org, dir, 1, 60, 0.8, 0.2, random() * 360, 300, cgs.media.stasisRingShader );
	// Impact effect
	FX_AddQuad( org, dir, 7, 35, 1.0, 0.0, random() * 360, 500, cgs.media.blueParticleShader );
	FX_AddQuad( org, dir, 5, 25, 1.0, 0.0, random() * 360, 420, cgs.media.ltblueParticleShader );

	CG_ImpactMark( cgs.media.scavMarkShader, org, dir, random()*360, 1,1,1,0.6, qfalse, 
				8 + random() * 2, qfalse );

	FX_AddSprite( end, NULL, qfalse, flrandom(40,60), -50, 1.0, 0.0, random() * 360, 0, 500, cgs.media.blueParticleShader );
}


