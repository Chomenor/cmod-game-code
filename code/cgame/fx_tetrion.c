#include "cg_local.h"
#include "fx_local.h"


/*
-------------------------
FX_TetrionProjectileThink
-------------------------
*/
void FX_TetrionProjectileThink( centity_t *cent, const struct weaponInfo_s *wi )
{
	vec3_t		forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0 )
		forward[2] = 1;

	FX_AddSprite(	cent->lerpOrigin, 
					NULL, qfalse,
					4.0f + random() * 16.0f, 0.0f,
					0.4f, 0.0f,
					random()*360, 0.0f,
					1.0f,
					cgs.media.greenBurstShader );
	FX_AddSprite(	cent->lerpOrigin,
					NULL, qfalse,
					16.0f + random() * 16.0f, 0.0f,
					0.6f, 0.0f,
					random()*360, 0.0f,
					1.0f,
					cgs.media.borgFlareShader );
	FX_AddTrail(	cent->lerpOrigin,
					forward, qfalse,
					64, 0,
					2.0f, 0,
					0.5f, 0,
					0,
					1,
					cgs.media.greenTrailShader );
}


/*
-------------------------
FX_TetrionShot
-------------------------
*/
#define MAXRANGE_TETRION		8192
void FX_TetrionShot( vec3_t start, vec3_t forward )
{
	trace_t	trace;
	vec3_t	end, dir, new_start, new_end, radial, start2, spreadFwd;
	float	off, len, i, numBullets = 3;
	float	firingRadius = 6, minDeviation = 0.95, maxDeviation = 1.1;
	qboolean	render_impact = qtrue;
	centity_t	*traceEnt = NULL;
	int			clientNum = -1;

	for (i = 0; i < numBullets; i++)
	{
		render_impact = qtrue;
		// determine new firing position 
		fxRandCircumferencePos(start, forward, firingRadius, new_start);
		VectorSubtract(new_start, start, radial);
		VectorMA(start, 10, forward, start2);
		VectorMA(start2, flrandom(minDeviation, maxDeviation), radial, start2);
		VectorSubtract(start2, new_start, spreadFwd);
		VectorNormalize(spreadFwd);
		// determine new end position for this bullet. give the endpoint some spread, too.
		VectorMA(new_start, MAXRANGE_TETRION, spreadFwd, end);
		CG_Trace( &trace, new_start, NULL, NULL, end, cg_entities[cg.predictedPlayerState.clientNum].currentState.number, MASK_SHOT );
		// Get the length of the whole shot
		VectorSubtract( trace.endpos, new_start, dir ); 
		len = VectorNormalize( dir );
		// Don't do tracers when it gets really short
		if ( len >= 64 )
		{
			// Move the end_point in a bit so the tracer doesn't always trace the full line length--this isn't strictly necessary, but it does
			//		add a bit of variance
			off = flrandom(0.7, 1.0);
			VectorMA( new_start, len * off, dir, new_end );

			// Draw the tracer
			FX_AddLine( new_end, new_start, 1.0f, 1.5f + random(), 0.0f, flrandom(0.3,0.6), 0.0,
				flrandom(300,500), cgs.media.borgFlareShader );
		}
		// put the impact effect where this tracer hits
		if (len >= 32)
		{
			// Rendering things like impacts when hitting a sky box would look bad, but you still want to see the tracer
			if ( trace.surfaceFlags & SURF_NOIMPACT ) 
			{
				render_impact = qfalse;
			}

			if (render_impact)
			{
				traceEnt = &cg_entities[trace.entityNum];
				clientNum = traceEnt->currentState.clientNum;
				if ( (trace.entityNum != ENTITYNUM_WORLD) && (clientNum >= 0 || clientNum < MAX_CLIENTS) )
				{
					// hit a player. let the shield/pain effects be the indicator for this
				}
				else
				{
					// hit something else
					FX_TetrionWeaponHitWall(trace.endpos, trace.plane.normal);
				}
			}
		}
	}
}

/*
-------------------------
FX_TetrionWeaponHitWall
-------------------------
*/
void FX_TetrionWeaponHitWall( vec3_t origin, vec3_t normal )
{
	vec3_t	vel, accel, org;
	int		i = 0, t = 0; 
	float	scale = random() * 2.5 + 1.5;

	CG_ImpactMark( cgs.media.bulletmarksShader, origin, normal, random()*360, 1,1,1,0.2, qfalse, 
					scale, qfalse );

	// Move out a hair to avoid z buffer nastiness
	VectorMA( origin, 0.5, normal, org );

	// Add a bit of variation every now and then
	if ( rand() & 1 )
	{
		FX_AddQuad( org, normal, 
					scale * 2, -4, 
					0.5, 0.5, 
					0, 
					175, 
					cgs.media.sunnyFlareShader );
	}

	FX_AddQuad( org, normal, 
				scale * 4, -8, 
				1.0, 1.0, 
				0, 
				175, 
				cgs.media.borgFlareShader );

	// Add some smoke puffs
	for ( i = 0; i < 2; i ++ )
	{
		for ( t = 0; t < 3; t++ )
		{
			vel[t] = normal[t] + crandom();
		}

		VectorScale( vel, 12 + random() * 12, vel );

		vel[2] += 16;

		VectorScale( vel, -0.25, accel );
		FX_AddSprite( origin, 
						vel, qfalse, 
						random() * 4 + 2, 12, 
						0.6 + random() * 0.4, 0.0, 
						random() * 180, 
						0.0, 
						random() * 200 + 300, 
						cgs.media.steamShader );
	}
}

/*
-------------------------
FX_TetrionRicochet
-------------------------
*/
void FX_TetrionRicochet( vec3_t origin, vec3_t normal )
{
	vec3_t org;

	// Move away from the wall a bit to help avoid z buffer clipping.
	VectorMA( origin, 0.5, normal, org );

	FX_AddQuad( org, normal,
				24, -24,
				1.0, 0.0,
				0,
				300,
				cgs.media.greenBurstShader );
	FX_AddQuad( org, normal,
				48, -48,
				0.5, 0.0,
				0,
				300,
				cgs.media.borgFlareShader );
}

/*
-------------------------
FX_TetrionAltHitWall
-------------------------
*/
void FX_TetrionAltHitWall( vec3_t origin, vec3_t normal )
{
	vec3_t	org;
	float	scale;

	scale = random() * 2.0 + 1.0;

	CG_ImpactMark( cgs.media.bulletmarksShader, origin, normal, random()*360, 1,1,1,0.2, qfalse, 
					scale, qfalse );

	// Move out a hair to avoid z buffer nastiness
	VectorMA( origin, 0.5, normal, org );

	FX_AddQuad( origin, normal,
				64, -96,
				1.0, 0.0,
				0,
				200,
				cgs.media.greenBurstShader );
	FX_AddQuad( origin, normal,
				128, -192,
				1.0, 0.0,
				0,
				200,
				cgs.media.borgFlareShader );

	// kef -- urp.fixme.
//	cgi_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cg_weapons[WP_TETRION_DISRUPTOR].altmissileHitSound );
}

/*
-------------------------
FX_TetrionAltHitPlayer
-------------------------
*/
void FX_TetrionAltHitPlayer( vec3_t origin, vec3_t normal )
{
}
