#include "cg_local.h"
#include "fx_local.h"

/*
-------------------------
FX_AltIMODBolt
-------------------------
*/

void FX_AltIMODBolt( vec3_t start, vec3_t end, vec3_t dir )
{
	vec3_t	control1, control2, control1_vel, control2_vel,
					control1_acceleration, control2_acceleration;
	vec3_t	direction, vr, vu;
	float	len;
	localEntity_t *le;
	
	MakeNormalVectors( dir, vr, vu );

	VectorSubtract( end, start, direction );
	len = VectorNormalize( direction );

	VectorMA(start, len*0.5f, direction, control1 );
	VectorMA(start, len*0.25f, direction, control2 );

	vectoangles( direction, control1_vel );
	control1_vel[ROLL] += crandom() * 360;
	AngleVectors( control1_vel, NULL, NULL, control1_vel );

	vectoangles( direction, control2_vel );
	control2_vel[ROLL] += crandom() * 360;
	AngleVectors( control2_vel, NULL, NULL, control2_vel );

	VectorScale(control1_vel, 12.0f + (140.0f * random()), control1_vel);
	VectorScale(control2_vel, -12.0f + (-140.0f * random()), control2_vel);

	VectorClear(control1_acceleration);
	VectorClear(control2_acceleration);
	le = FX_AddBezier( start, end, 
					control1, control2, control1_vel, control2_vel, control1_acceleration, control2_acceleration, 
					4.0f,				//scale
					1000.0f,			//killtime
					cgs.media.IMOD2Shader );
	le->alpha = 0.8;
	le->dalpha = -0.8;
}

/*
-------------------------
FX_IMODBolt2
-------------------------
*/

void FX_IMODBolt2( vec3_t start, vec3_t end, vec3_t dir )
{
	vec3_t			control1, control2, control1_velocity, control2_velocity,
					control1_acceleration, control2_acceleration;
	float			length = 0;
	vec3_t			vTemp;
	localEntity_t	*le;

	// initial position of control points
	VectorSubtract(end, start, vTemp);
	length = VectorNormalize(vTemp);
	VectorMA(start, 0.5 * length, vTemp, control1);				
	VectorMA(start, 0.25 * length, vTemp, control2);

	// initial velocity of control points
	vectoangles(vTemp, control1_velocity);
	control1_velocity[ROLL] += crandom() * 360;
	AngleVectors(control1_velocity, NULL, NULL, control1_velocity);

	vectoangles(vTemp, control2_velocity);
	control2_velocity[ROLL] += crandom() * 360;
	AngleVectors(control2_velocity, NULL, NULL, control2_velocity);

	VectorScale(control1_velocity, 12.0f + (140.0f * random()), control1_velocity);
	VectorScale(control2_velocity, -12.0f + (-140.0f * random()), control2_velocity);

	// constant acceleration of control points
	/*
	VectorScale(control1_velocity, -1.2, control1_acceleration);	
	for (i = 0; i < 3; i++)
	{
		control1_acceleration[i] += flrandom (-10, 10);
	}
	VectorScale(control2_velocity, -1.2, control2_acceleration);
	for (i = 0; i < 3; i++)
	{
		control2_acceleration[i] += flrandom (-10, 10);
	}
	*/
	VectorClear(control1_acceleration);
	VectorClear(control2_acceleration);

	le = FX_AddBezier(start, end, control1, control2, control1_velocity, control2_velocity, control1_acceleration,
						control2_acceleration, 4, 600, cgs.media.altIMOD2Shader);
	le->alpha = 0.6;
	le->dalpha = -0.6;
}


/*
-------------------------
FX_IMODShot
-------------------------
*/

#define MAXRANGE_IMOD			8192

void FX_IMODShot( vec3_t end, vec3_t start, vec3_t dir)
{
	vec3_t	ofs, end2;
	trace_t			trace;
	qboolean		render_impact = qtrue;

	VectorMA( end, 1, dir, ofs );

	FX_AddLine(start, end, 1.0, 8.0, -8.0, 1.0, 0.0, 350, cgs.media.altIMODShader);

	FX_IMODBolt2( start, end, dir);

	// cover up the start point of the beam
	FX_AddSprite( start, NULL, qfalse, irandom(8,12), -8, 1.0, 0.6, random()*360, 0.0, 400, cgs.media.purpleParticleShader);
	// where do we put an explosion?
	VectorMA(start, MAXRANGE_IMOD, cg.refdef.viewaxis[0], end2);
	CG_Trace( &trace, start, NULL, NULL, end2, 0, MASK_SHOT );

	if ( trace.surfaceFlags & SURF_NOIMPACT ) 
	{
		render_impact = qfalse;
	}
	if ( render_impact )
	{
		FX_IMODExplosion(end, trace.plane.normal);
	}
}

/*
-------------------------
FX_AltIMODShot
-------------------------
*/
#define FX_ALT_IMOD_HOLD		200
#define FX_ALT_IMOD_FLASHSIZE	16
qboolean IMODAltAftereffect(localEntity_t *le)
{
	localEntity_t *spr = NULL;
	qhandle_t	shader = cgs.media.blueParticleShader;

	//only want an initial sprite
	le->endTime = cg.time;

	spr = FX_AddSprite(	le->refEntity.origin,// origin
						NULL,				// velocity
						qfalse,				// gravity
						FX_ALT_IMOD_FLASHSIZE,	// scale
						-10,				// dscale
						0.8,				// startalpha
						0.0,				// endalpha
						0.0,				// roll
						0.0,				// elasticity
						700,				// killTime
						shader);			// shader

	return qtrue;
}

void FX_AltIMODShot( vec3_t end, vec3_t start, vec3_t dir)
{
	vec3_t		ofs, end2;
	int			i = 0;
	trace_t			trace;
	qboolean		render_impact = qtrue;

	VectorMA( end, 1, dir, ofs );

	FX_AddLine( start, end, 1.0f, 32.0f, -32.0f, 1.0f, 1.0f, 500.0f, cgs.media.IMODShader);

	for ( i = 0; i < 2; i++ )
		FX_AltIMODBolt( start, end, dir );

	// cover up the start point of the beam
	FX_AddSprite( start, NULL, qfalse, FX_ALT_IMOD_FLASHSIZE, 0, 1.0, 0.6, 0.0, 0.0, FX_ALT_IMOD_HOLD, cgs.media.blueParticleShader);
	FX_AddSpawner( start, dir, NULL, NULL, qfalse, FX_ALT_IMOD_HOLD,
							 0, FX_ALT_IMOD_HOLD+100, IMODAltAftereffect, 10 );
	// where do we put an explosion?
	VectorMA(start, MAXRANGE_IMOD, cg.refdef.viewaxis[0], end2);
	CG_Trace( &trace, start, NULL, NULL, end2, 0, MASK_SHOT );

	if ( trace.surfaceFlags & SURF_NOIMPACT ) 
	{
		render_impact = qfalse;
	}
	if ( render_impact )
	{
		FX_AltIMODExplosion(end, trace.plane.normal);
	}
}

/*
-------------------------
FX_IMODExplosion
-------------------------
*/

void FX_IMODExplosion( vec3_t origin, vec3_t normal )
{
	localEntity_t	*le;
	vec3_t			direction, vel;
	float			scale, dscale;
	int				i, numSparks;

	//Orient the explosions to face the camera
	VectorSubtract( cg.refdef.vieworg, origin, direction );
	VectorNormalize( direction );

	//Tag the last one with a light
	le = CG_MakeExplosion( origin, direction, cgs.media.explosionModel, cgs.media.imodExplosionShader, 400, qfalse);
	le->light = 75;
	VectorSet( le->lightColor, 1.0f, 0.8f, 0.5f );

	//Sparks
	numSparks = 3 + (rand() & 7);
	// kef -- fixme. what does vel do?
	VectorClear(vel);
	
	for ( i = 0; i < numSparks; i++ )
	{	
		scale = 1.0f + (random() * 0.5f);
		dscale = -scale*0.5;

		FX_AddTrail(			origin,
								NULL,
								qfalse,
								32.0f + (random() * 64.0f),
								-256.0f,
								scale,
								0.0f,
								1.0f,
								0.0f,
								0.25f,
								750.0f,
								cgs.media.purpleParticleShader );

		FXE_Spray( normal, 500, 250, 0.75f, /*256,*/vel );
	}

	CG_ImpactMark( cgs.media.IMODMarkShader, origin, normal, random()*360, 1,1,1,0.75, qfalse, 5, qfalse );
//	CG_ImpactMark( cgs.media.scavMarkShader, origin, normal, random()*360, 1,1,1,0.2, qfalse, random() + 1, qfalse );

	CG_ExplosionEffects( origin, 1.0f, 150 );
}

/*
-------------------------
FX_AltIMODExplosion
-------------------------
*/
void FX_AltIMODExplosion( vec3_t origin, vec3_t normal )
{
	localEntity_t	*le;
	vec3_t			direction, org, vel;
	float			scale, dscale;
	int				i, numSparks;

	//Orient the explosions to face the camera
	VectorSubtract( cg.refdef.vieworg, origin, direction );
	VectorNormalize( direction );

	//Tag the last one with a light
	le = CG_MakeExplosion( origin, direction, cgs.media.explosionModel, cgs.media.electricalExplosionSlowShader, 475, qfalse);
	le->light = 150;
	VectorSet( le->lightColor, 1.0f, 0.8f, 0.5f );

	for ( i = 0; i < 2; i ++)
	{
		VectorSet( org, origin[0] + 16 * random(), origin[1] + 16 * random(), origin[2] + 16 * random() );
		CG_MakeExplosion( org, direction, cgs.media.explosionModel, cgs.media.electricalExplosionFastShader, 
					250, qfalse);
	}

	//Sparks

	numSparks = 8 + (rand() & 7);
	
	// kef -- fixme. what does this vector do!?! waaaaaah!
	VectorClear(vel);

	for ( i = 0; i < numSparks; i++ )
	{	
		scale = 1.5f + (random() * 0.5f);
		dscale = -scale*0.5;

		FX_AddTrail(			 origin,
								NULL,
								qfalse,
								32.0f + (random() * 64.0f),
								-256.0f,
								scale,
								0.0f,
								1.0f,
								0.0f,
								0.25f,
								750.0f,
								cgs.media.spark2Shader );

		FXE_Spray( normal, 500, 250, 0.75f, /*256,*/ vel );
	}

	CG_ImpactMark( cgs.media.IMODMarkShader, origin, normal, random()*360, 1,1,1,1, qfalse, 8, qfalse );

	CG_ExplosionEffects( origin, 2.0f, 250 );
}
