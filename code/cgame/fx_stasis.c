#include "cg_local.h"
#include "fx_local.h"

void FX_StasisDischarge( vec3_t origin, vec3_t normal, int count, float dist_out, float dist_side );

#define FX_STASIS_ALT_RIGHT_OFS	0.10
#define FX_STASIS_ALT_UP_OFS	0.02
#define FX_STASIS_ALT_MUZZLE_OFS	1
#define FX_MAXRANGE_STASIS		4096

/*
-------------------------
FX_StasisShot

Alt-fire, beam that shrinks to its impact point
-------------------------
*/

void FX_SmallStasisBeam(centity_t *cent, vec3_t start, vec3_t dir)
{
	vec3_t end, org, vel = { 0,0,-4};
	trace_t tr;
	float r;
	int i, ct, t;

	VectorMA(start, FX_MAXRANGE_STASIS, dir, end);
	CG_Trace(&tr, start, NULL, NULL, end, cent->currentState.number, MASK_SHOT);

	// Beam
	FX_AddLine( start, tr.endpos, 1.0f, 3.0f, 4.0f, 0.8f, 0.0f, 400.0f, cgs.media.stasisAltShader);

	// Do a quick LOD for number of decay particles
	ct = tr.fraction * (FX_MAXRANGE_STASIS * 0.02);
	if ( ct < 12 )
		ct = 12;
	else if ( ct > 24 )
		ct = 24;

	for ( i = 0; i < ct; i++ )
	{
		r = random() * tr.fraction * (FX_MAXRANGE_STASIS * 0.5);
		VectorMA( start, r, dir, org );

		for ( t = 0; t < 3; t++ )
			org[t] += crandom();

		if ( rand() & 1 )
			FX_AddSprite( org, vel, qfalse, random() + 1.5, -3, 1.0, 1.0, 0.0, 0.0, 500, cgs.media.blueParticleShader);
		else
			FX_AddSprite( org, vel, qfalse, random() + 1.5, -3, 1.0, 1.0, 0.0, 0.0, 500, cgs.media.purpleParticleShader);	
	}

	// Impact graphic if needed.
	if (cg_entities[tr.entityNum].currentState.eType == ET_PLAYER)
	{	// Hit an entity.
		// Expanding rings
		FX_AddSprite( tr.endpos, NULL, qfalse, 1, 60, 0.8, 0.2, random() * 360, 0, 400, cgs.media.stasisRingShader );
		// Impact effect
//		FX_AddSprite( tr.endpos, NULL, qfalse, 7, 25, 1.0, 0.0, random() * 360, 0, 500, cgs.media.blueParticleShader );
		FX_AddSprite( tr.endpos, NULL, qfalse, 5, 18, 1.0, 0.0, random() * 360, 0, 420, cgs.media.ltblueParticleShader );
	}
	else if (!(tr.surfaceFlags & SURF_NOIMPACT) )
	{
		// Move me away from the wall a bit so that I don't z-buffer into it
		VectorMA( tr.endpos, 1.5, tr.plane.normal, end);

		// Expanding rings
		FX_AddQuad( end, tr.plane.normal, 1, 12, 0.8, 0.2, random() * 360, 400, cgs.media.stasisRingShader );
		FX_AddQuad( end, tr.plane.normal, 1, 30, 0.8, 0.2, random() * 360, 300, cgs.media.stasisRingShader );
		// Impact effect
		FX_AddQuad( end, tr.plane.normal, 4, 16, 1.0, 0.0, random() * 360, 500, cgs.media.blueParticleShader );
		FX_AddQuad( end, tr.plane.normal, 3, 12, 1.0, 0.0, random() * 360, 420, cgs.media.ltblueParticleShader );

		CG_ImpactMark( cgs.media.scavMarkShader, end, tr.plane.normal, random()*360, 1,1,1,0.6, qfalse, 
					5 + random() * 2, qfalse );
	}

	FX_AddSprite( tr.endpos, NULL, qfalse, flrandom(40,60), -50, 1.0, 0.0, random() * 360, 0, 500, cgs.media.blueParticleShader );

	// Pass the end position back to the calling function (yes, I know).
	VectorCopy(tr.endpos, dir);
}


// kef -- fixme. the altfire stuff really wants to use some endcap stuff and some other flags
void FX_StasisShot( centity_t *cent, vec3_t end, vec3_t start )
{
	trace_t tr;
	vec3_t	fwd, newdir, org, vel = { 0,0,-4}, newstart, end2;
	int		i, t, ct;
	float	len, r;
	vec3_t	fwd2, right, up;
	int		bolt1, bolt2;
	vec3_t	bolt1vec, bolt2vec;
	centity_t		*traceEnt = NULL;
	int			clientNum = -1;

	// Choose which bolt will have the electricity accent.
	bolt1 = irandom(0,2);
	bolt2 = irandom(0,4);

	VectorSubtract( end, start, fwd );
	len = VectorNormalize( fwd );

	// Beam
	FX_AddLine( end, start, 1.0f, 4.0f, 6.0f, 0.8f, 0.0f, 500.0f, cgs.media.stasisAltShader);

	// Do a quick LOD for number of decay particles
	ct = len * 0.03;
	if ( ct < 16 )
		ct = 16;
	else if ( ct > 32 )
		ct = 32;

	for ( i = 0; i < ct; i++ )
	{
		r = random() * len * 0.5;
		VectorMA( start, r, fwd, org );

		for ( t = 0; t < 3; t++ )
			org[t] += crandom();

		if ( rand() & 1 )
			FX_AddSprite( org, vel, qfalse, random() + 2, -4, 1.0, 1.0, 0.0, 0.0, 600, cgs.media.blueParticleShader);
		else
			FX_AddSprite( org, vel, qfalse, random() + 2, -4, 1.0, 1.0, 0.0, 0.0, 600, cgs.media.purpleParticleShader);	
	}
	VectorMA(start, FX_MAXRANGE_STASIS, fwd, end2);
	CG_Trace(&tr, start, NULL, NULL, end2, cent->currentState.number, MASK_SHOT);
	if (!( tr.surfaceFlags & SURF_NOIMPACT ))
	{
		traceEnt = &cg_entities[tr.entityNum];
		clientNum = traceEnt->currentState.clientNum;
		if ( (tr.entityNum != ENTITYNUM_WORLD) && (clientNum >= 0 || clientNum < MAX_CLIENTS) )
		{
			// hit a player
			FX_StasisShotImpact(tr.endpos, tr.plane.normal);
		}
		else
		{
			// hit the world
			FX_StasisShotMiss(tr.endpos, tr.plane.normal);
		}
	}
	// cap the impact end of the main beam to hide the nasty end of the line
	FX_AddSprite( tr.endpos, NULL, qfalse, flrandom(40,60), -50, 1.0, 0.0, random() * 360, 0, 500, cgs.media.blueParticleShader );

	if (bolt1==0)
	{
		VectorCopy(end, bolt1vec);
	}
	else if (bolt2==0)
	{
		VectorCopy(end, bolt2vec);
	}

	AngleVectors(cent->currentState.angles, fwd2, right, up);

//	CrossProduct(fwd, up, right);
//	VectorNormalize(right);			// "right" is scaled by the sin of the angle between fwd & up...  Ditch that.
//	CrossProduct(right, fwd, up);	// Change the "fake up" (0,0,1) to a "real up" (perpendicular to the forward vector).
	// VectorNormalize(up);			// If I cared about how the vertical variance looked when pointing up or down, I'd normalize this.

	// Fire a shot up and to the right.
	VectorMA(fwd, FX_STASIS_ALT_RIGHT_OFS, right, newdir);
	VectorMA(newdir, FX_STASIS_ALT_UP_OFS, up, newdir);
	VectorMA(start, FX_STASIS_ALT_MUZZLE_OFS, right, newstart);
	FX_SmallStasisBeam(cent, newstart, newdir);

	if (bolt1==1)
	{
		VectorCopy(newdir, bolt1vec);
	}
	else if (bolt2==1)
	{
		VectorCopy(newdir, bolt2vec);
	}

	// Fire a shot up and to the left.
	VectorMA(fwd, -FX_STASIS_ALT_RIGHT_OFS, right, newdir);
	VectorMA(newdir, FX_STASIS_ALT_UP_OFS, up, newdir);
	VectorMA(start, -FX_STASIS_ALT_MUZZLE_OFS, right, newstart);
	FX_SmallStasisBeam(cent, newstart, newdir);

	if (bolt1==2)
	{
		VectorCopy(newdir, bolt1vec);
	}
	else if (bolt2==2)
	{
		VectorCopy(newdir, bolt2vec);
	}

	// Fire a shot a bit down and to the right.
	VectorMA(fwd, 2.0*FX_STASIS_ALT_RIGHT_OFS, right, newdir);
	VectorMA(newdir, -0.5*FX_STASIS_ALT_UP_OFS, up, newdir);
	VectorMA(start, 2.0*FX_STASIS_ALT_MUZZLE_OFS, right, newstart);
	FX_SmallStasisBeam(cent, newstart, newdir);

	if (bolt1==3)
	{
		VectorCopy(newdir, bolt1vec);
	}
	else if (bolt2==3)
	{
		VectorCopy(newdir, bolt2vec);
	}

	// Fire a shot up and to the left.
	VectorMA(fwd, -2.0*FX_STASIS_ALT_RIGHT_OFS, right, newdir);
	VectorMA(newdir, -0.5*FX_STASIS_ALT_UP_OFS, up, newdir);
	VectorMA(start, -2.0*FX_STASIS_ALT_MUZZLE_OFS, right, newstart);
	FX_SmallStasisBeam(cent, newstart, newdir);

	if (bolt1==4)
	{
		VectorCopy(newdir, bolt1vec);
	}
	else if (bolt2==4)
	{
		VectorCopy(newdir, bolt2vec);
	}
		
	// Put a big gigant-mo sprite at the muzzle end so people can't see the crappy edges of the line
	FX_AddSprite( start, NULL, qfalse, random()*3 + 15, -20, 1.0, 0.5, 0.0, 0.0, 600, cgs.media.blueParticleShader);

	// Do an electrical arc to one of the impact points.
	FX_AddElectricity( start, bolt1vec, 0.2f, 15.0, -15.0, 1.0, 0.5, 100, cgs.media.dnBoltShader, 0.1 );

	if (bolt1!=bolt2)
	{
		// ALSO do an electrical arc to another point.
		FX_AddElectricity( bolt1vec, bolt2vec, 0.2f, 15.0, -15.0, 1.0, 0.5, flrandom(100,200), cgs.media.dnBoltShader, 0.5 );
	}
}

/*
-------------------------
FX_StasisShotImpact

Alt-fire, impact effect
-------------------------
*/
void FX_StasisShotImpact( vec3_t end, vec3_t dir )
{
	vec3_t	org;

	// Move me away from the wall a bit so that I don't z-buffer into it
	VectorMA( end, 1.5, dir, org );

	// Expanding rings
	FX_AddQuad( org, dir, 1, 80, 0.8, 0.2, random() * 360, 400, cgs.media.stasisRingShader );
	// Impact effect
	FX_AddQuad( org, dir, 7, 35, 1.0, 0.0, random() * 360, 500, cgs.media.blueParticleShader );
	FX_AddQuad( org, dir, 5, 25, 1.0, 0.0, random() * 360, 420, cgs.media.ltblueParticleShader );

//	CG_ImpactMark( cgs.media.scavMarkShader, org, dir, random()*360, 1,1,1,0.6, qfalse, 
//				8 + random() * 2, qfalse );

//	FX_StasisDischarge( org, dir, irandom( 2,4 ), 24 + random() * 12, 64 + random() * 48 );
}

/*
-------------------------
FX_StasisShotMiss

Alt-fire, miss effect
-------------------------
*/
void FX_StasisShotMiss( vec3_t end, vec3_t dir )
{
	vec3_t	org;

	// Move me away from the wall a bit so that I don't z-buffer into it
	VectorMA( end, 0.5, dir, org );

	// Expanding rings
	FX_AddQuad( org, dir, 1, 16, 0.8, 0.2, random() * 360, 400, cgs.media.stasisRingShader );
	FX_AddQuad( org, dir, 1, 40, 0.8, 0.2, random() * 360, 300, cgs.media.stasisRingShader );
	// Impact effect
	FX_AddQuad( org, dir, 5, 25, 1.0, 0.0, random() * 360, 500, cgs.media.blueParticleShader );
	FX_AddQuad( org, dir, 4, 17, 1.0, 0.0, random() * 360, 420, cgs.media.ltblueParticleShader );

	CG_ImpactMark( cgs.media.scavMarkShader, org, dir, random()*360, 1,1,1,0.6, qfalse, 
				6 + random() * 2, qfalse );

	FX_AddSprite( end, NULL, qfalse, flrandom(40,60), -50, 1.0, 0.0, random() * 360, 0, 500, cgs.media.blueParticleShader );

//	FX_StasisDischarge( org, dir, irandom( 2,4 ), 24 + random() * 12, 64 + random() * 48 );
}

/*
-------------------------
FX_StasisProjectileThink

Main fire, with crazy bits swirling around main projectile
-------------------------
*/
void FX_StasisProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	int			size = 0;
	vec3_t  forward, right, up;
	float	radius, temp;
	vec3_t	org;

	size = cent->currentState.time2;

	if ( size < 3 )
		FX_AddSprite( cent->lerpOrigin, NULL, qfalse, ( size * size ) * 5.0f + ( random() * size * 5.0f ), 0.0f, 1.0f, 0.0f, 0, 0.0f, 1, cgs.media.blueParticleShader );
	else
	{
		// Only do this extra crap if you're the big cheese

		// convert direction of travel into normalized forward vector
		if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0 ) {
			forward[2] = 1;
		}
		MakeNormalVectors( forward, right, up );

		// Main projectile
		FX_AddSprite( cent->lerpOrigin, NULL, qfalse, ( size * size ) * 4.5f + ( random() * size * 4.0f ), 0, 1.0, 1.0, 0.0, 0.0, 1, cgs.media.blueParticleShader );

		// Crazy polar coordinate plotting stuff--for particle swarm
		radius = 30 * sin( cg.time * 0.002 * 5 );
		temp = radius * cos( cg.time * 0.002 );
		VectorMA( cent->lerpOrigin, temp, right, org );
		temp = radius * sin( cg.time * 0.002 );
		VectorMA( org, temp, up, org );

		// Main bit
		FX_AddSprite( org, NULL, qfalse, random() * 8 + 4, 0, 1.0, 1.0, 0.0, 0.0, 1, cgs.media.purpleParticleShader );
		// Glowy bit
		FX_AddSprite( org, NULL, qfalse, random() * 8 + 24, 0, 0.3, 0.3, 0.0, 0.0, 1, cgs.media.purpleParticleShader );
		// Tail bit
		FX_AddLine( org, cent->rawAngles, 1.0, 2.0, -1.0, 0.8, 0.0, 300, cgs.media.altIMOD2Shader );
		VectorCopy( org, cent->rawAngles );

		radius = 30 * sin( cg.time * 0.002 * 3 );
		temp = radius * cos( cg.time * 0.002 + 3.141 );
		VectorMA( cent->lerpOrigin, temp, right, org );
		temp = radius * sin( cg.time * 0.002 + 3.141 );
		VectorMA( org, temp, up, org );

		FX_AddSprite( org, NULL, qfalse, random() * 8 + 4, 0, 1.0, 1.0, 0.0, 0.0, 1, cgs.media.purpleParticleShader );
		FX_AddSprite( org, NULL, qfalse, random() * 8 + 24, 0, 0.3, 0.3, 0.0, 0.0, 1, cgs.media.purpleParticleShader );
		FX_AddLine( org, cent->rawOrigin, 1.0, 2.0, -1.0, 0.8, 0.0, 300, cgs.media.altIMOD2Shader );
		VectorCopy( org, cent->rawOrigin );

		radius = 30 * sin( cg.time * 0.002 * 3.5 );
		temp = radius * cos( cg.time * 0.002 + 3.141 * 0.5);
		VectorMA( cent->lerpOrigin, temp, right, org );
		temp = radius * sin( cg.time * 0.002 + 3.141 * 0.5 );
		VectorMA( org, temp, up, org );

		FX_AddSprite( org, NULL, qfalse, random() * 8 + 4, 0, 1.0, 1.0, 0.0, 0.0, 1, cgs.media.purpleParticleShader );
		FX_AddSprite( org, NULL, qfalse, random() * 8 + 24, 0, 0.3, 0.3, 0.0, 0.0, 1, cgs.media.purpleParticleShader );

		radius = 30 * sin( cg.time * 0.002 * 4.5 );
		temp = radius * cos( cg.time * 0.002 + 3.141 * 1.5);
		VectorMA( cent->lerpOrigin, temp, right, org );
		temp = radius * sin( cg.time * 0.002 + 3.141 * 1.5 );
		VectorMA( org, temp, up, org );

		FX_AddSprite( org, NULL, qfalse, random() * 8 + 4, 0, 1.0, 1.0, 0.0, 0.0, 1, cgs.media.purpleParticleShader );
		FX_AddSprite( org, NULL, qfalse, random() * 8 + 24, 0, 0.3, 0.3, 0.0, 0.0, 1, cgs.media.purpleParticleShader );
	}
}

/*
-------------------------
FX_OrientedBolt

Creates new bolts for a while
-------------------------
*/

void FX_OrientedBolt( vec3_t start, vec3_t end, vec3_t dir )
{
	vec3_t	mid;

	VectorSubtract( end, start, mid );
	VectorScale( mid, 0.1f + (random() * 0.8), mid );
	VectorAdd( start, mid, mid );
	VectorMA(mid, 3.0f + (random() * 10.0f), dir, mid );

	FX_AddElectricity( mid, start, 0.5, 0.75 + random() * 0.75, 0.0, 1.0, 0.5, 300.0f + random() * 300, cgs.media.bolt2Shader, DEFAULT_DEVIATION);
	FX_AddElectricity( mid, end, 0.5, 0.75 + random() * 0.75, 1.0, 1.0, 0.5, 300.0f + random() * 300, cgs.media.bolt2Shader, DEFAULT_DEVIATION);
}

/*
-------------------------
FX_StasisDischarge

Fun "crawling" electricity ( credit goes to Josh for this one )
-------------------------
*/

void FX_StasisDischarge( vec3_t origin, vec3_t normal, int count, float dist_out, float dist_side )
{
	trace_t	trace;
	vec3_t	org, dir, dest;
	vec3_t	vr;
	int		i;
	int		discharge = dist_side;

	vectoangles( normal, dir );
	dir[ROLL] += random() * 360;

	for (i = 0;	i < count; i++)
	{
		//Move out a set distance
		VectorMA( origin, dist_out, normal, org );
		
		//Even out the hits
		dir[ROLL] += (360 / count) + (rand() & 31);
		AngleVectors( dir, NULL, vr, NULL );

		//Move to the side in a random direction
		discharge += (int)( crandom() * 8.0f );
		VectorMA( org, discharge, vr, org );

		//Trace back to find a surface
		VectorMA( org, -dist_out * 3, normal, dest );

		CG_Trace( &trace, org, NULL, NULL, dest, 0, MASK_SHOT );
		
		//No surface found, start over
		if (trace.fraction == 1) 
			continue;

		//Connect the two points with bolts
		FX_OrientedBolt( origin, trace.endpos, normal );
	}
}

/*
-------------------------
FX_StasisWeaponHitWall

Main fire impact
-------------------------
*/

#define NUM_DISCHARGES		2
#define	DISCHARGE_DIST		8
#define	DISCHARGE_SIDE_DIST	24

void FX_StasisWeaponHitWall( vec3_t origin, vec3_t dir, int size )
{
	vec3_t			vel, accel, hitpos;
	int				i, t;
	weaponInfo_t	*weaponInfo = &cg_weapons[WP_STASIS];

	// Generate "crawling" electricity		// eh, don't it doesn't look that great.
//	FX_StasisDischarge( origin, dir, NUM_DISCHARGES, DISCHARGE_DIST, DISCHARGE_SIDE_DIST );

	VectorMA(origin, size, dir, hitpos);

	// Set an oriented residual glow effect
	FX_AddQuad( hitpos, dir, size * size * 15.0f, -150.0f, 
				1.0f, 0.0f, 0, 300, cgs.media.blueParticleShader );

	CG_ImpactMark( cgs.media.scavMarkShader, origin, dir, random()*360, 1,1,1,0.6, qfalse, 
					size * 12 + 1, qfalse );

	FX_AddSprite( hitpos, NULL, qfalse, size * size * 15.0f, -150.0f, 
				1.0f, 0.0f, 360*random(), 0, 400, cgs.media.blueParticleShader );

	// Only play the impact sound and throw off the purple particles when it's the main projectile
	if ( size < 3 )
		return;

	for ( i = 0; i < 4; i++ )
	{
		for ( t = 0; t < 3; t++ )
			vel[t] = ( dir[t] + crandom() * 0.9 ) * ( random() * 100 + 250 );

		VectorScale( vel, -2.2, accel );
		FX_AddSprite( hitpos, vel, qfalse, random() * 8 + 8, 0, 1.0, 0.0, 0.0, 0.0, 200, cgs.media.purpleParticleShader );

	}
	trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, weaponInfo->mainHitSound );
}


