//Compression rifle weapon effects

#include "cg_local.h"
#include "fx_local.h"

qboolean AltCompressionAftereffect(localEntity_t *le)
{
	localEntity_t *cyl = NULL;
	qhandle_t	shader = cgs.media.compressionAltBlastShader;
	float		percentLife = 1.0 - (le->endTime - cg.time)*le->lifeRate;
	float		alpha = 0.6 - (0.6*percentLife);
	float		length = 20;
	vec3_t		vec2, dir2;

	cyl = FX_AddCylinder(	le->refEntity.origin, 
							le->data.spawner.dir,
							length,// height,
							0,// dheight,
							10,//10+(30*(1-percentLife)),// scale,
							210,// dscale,
							10+(30*percentLife),// scale2,
							210,// dscale2,
							alpha,// startalpha, 
							0.0,// endalpha, 
							500,// killTime, 
							shader,
							15);// bias );
	cyl->leFlags |= LEF_ONE_FRAME;

	VectorMA(le->refEntity.origin, length*2.0, le->data.spawner.dir, vec2);
	VectorScale(le->data.spawner.dir, -1.0, dir2);
	cyl = FX_AddCylinder(	vec2, 
							dir2,
							length,// height,
							0,// dheight,
							10,//10+(30*(1-percentLife)),// scale,
							210,// dscale,
							10+(30*percentLife),// scale2,
							210,// dscale2,
							alpha,// startalpha, 
							0.0,// endalpha, 
							500,// killTime, 
							shader,
							15);// bias );
	cyl->leFlags |= LEF_ONE_FRAME;

	return qtrue;
}

/*
-------------------------
FX_CompressionShot
-------------------------
*/
#define MAXRANGE_CRIFLE		8192
void FX_CompressionShot( vec3_t start, vec3_t dir )
{
	localEntity_t	*le;
	vec3_t			end;
	trace_t			trace;
	qboolean		render_impact = qtrue;
	centity_t		*traceEnt = NULL;
	int			clientNum = -1;

	VectorMA(start, MAXRANGE_CRIFLE, dir, end);
	CG_Trace( &trace, start, NULL, NULL, end, 0, MASK_SHOT );

	// draw the beam
	le = FX_AddLine(start, trace.endpos, 1.0, 2.0, 0.0, 1.0, 1.0, 100.0, cgs.media.sparkShader);
	le->leFlags |= LEF_ONE_FRAME;

	// draw an impact at the endpoint of the trace
	// If the beam hits a skybox, etc. it would look foolish to add in an explosion
	if ( trace.surfaceFlags & SURF_NOIMPACT ) 
	{
		render_impact = qfalse;
	}
	if ( render_impact )
	{
		traceEnt = &cg_entities[trace.entityNum];
		clientNum = traceEnt->currentState.clientNum;
		if ( (trace.entityNum != ENTITYNUM_WORLD) && (clientNum >= 0 || clientNum < MAX_CLIENTS) )
		{
			FX_CompressionHit(trace.endpos);
		} 
		else 
		{
			FX_CompressionExplosion(start, trace.endpos, trace.plane.normal, qfalse);
		}
	}
}
/*
-------------------------
FX_CompressionShot
-------------------------
*/
void FX_CompressionAltShot( vec3_t start, vec3_t dir )
{
	vec3_t			end, vel = {0,0,0};
	trace_t			trace;
	qboolean		render_impact = qtrue;
	centity_t		*traceEnt = NULL;
	int			clientNum = -1;

	VectorMA(start, MAXRANGE_CRIFLE, dir, end);
	CG_Trace( &trace, start, NULL, NULL, end, cg_entities[cg.predictedPlayerState.clientNum].currentState.number, MASK_SHOT );

	// draw the beam
	FX_AddLine( start, trace.endpos, 1.0f, 3.0f, 0.0f, 1.0f, 0.0f, 350/*125.0f*/, cgs.media.sparkShader );
	FX_AddLine( start, trace.endpos, 1.0f, 6.0f, 20.0f, 0.6f, 0.0f, 800/*175.0f*/, cgs.media.phaserShader);//compressionAltBeamShader );

	FX_AddSpawner( start, dir, vel, NULL, qfalse, 0,
							 0, 500, AltCompressionAftereffect, 10 );

	// draw an impact at the endpoint of the trace
	// If the beam hits a skybox, etc. it would look foolish to add in an explosion
	if ( trace.surfaceFlags & SURF_NOIMPACT ) 
	{
		render_impact = qfalse;
	}
	if ( render_impact )
	{
		traceEnt = &cg_entities[trace.entityNum];
		clientNum = traceEnt->currentState.clientNum;
		if ( (trace.entityNum != ENTITYNUM_WORLD) && (clientNum >= 0 || clientNum < MAX_CLIENTS) )
		{
			FX_CompressionHit(trace.endpos);
		} 
		else 
		{
			FX_CompressionExplosion(start, trace.endpos, trace.plane.normal, qtrue);
		}
	}
}

/*
-------------------------
FX_CompressionExplosion
-------------------------
*/

void FX_CompressionExplosion( vec3_t start, vec3_t origin, vec3_t normal, qboolean altfire )
{
	localEntity_t	*le;
	vec3_t			dir;
	vec3_t			velocity;  //, shot_dir;
	float			scale, dscale;
	int				i, j, numSparks;
	weaponInfo_t	*weaponInfo = &cg_weapons[WP_COMPRESSION_RIFLE];

	//Sparks
	numSparks = 4 + (random() * 4.0f);
	
	if (altfire)
	{
		numSparks *= 1.5f;
	}
	for ( i = 0; i < numSparks; i++ )
	{	
		scale = 0.25f + (random() * 1.0f);
		dscale = -scale;

				//Randomize the direction
		for (j = 0; j < 3; j ++ )
		{
			dir[j] = normal[j] + (0.75 * crandom());
		}

		VectorNormalize(dir);

		//set the speed
		VectorScale( dir, 200 + (50 * crandom()), velocity);

		le = FX_AddTrail( origin,
								velocity,
								qtrue,
								4.0f,
								-4.0f,
								scale,
								-scale,
								1.0f,
								1.0f,
								0.5f,
								1000.0f,
								cgs.media.sparkShader);

//		FXE_Spray( normal, 200, 50, 0.4f, le);
	}

	VectorMA( origin, 8, normal, dir );
	VectorSet(velocity, 0, 0, 8);
/*
	FX_AddSprite(	dir, 
					velocity, 
					qfalse, 
					(altfire?50.0f:32.0f),
					16.0f,
					1.0f,
					0.0f,
					random()*45.0f,
					0.0f,
					(altfire?1300.0f:1000.0f),
					cgs.media.steamShader );
*/
	//Orient the explosions to face the camera
	VectorSubtract( cg.refdef.vieworg, origin, dir );
	VectorNormalize( dir );

	if (!altfire)
	{
		le = CG_MakeExplosion2( origin, dir, cgs.media.explosionModel, 5, cgs.media.electricalExplosionSlowShader, 
								475, qfalse, 1.2f + ( crandom() * 0.3f), LEF_NONE);
		le->light = 150;
		le->refEntity.renderfx |= RF_NOSHADOW;
		VectorSet( le->lightColor, 0.8f, 0.8f, 1.0f );

		CG_ImpactMark( cgs.media.compressionMarkShader, origin, normal, random()*360, 1,1,1,1, qfalse, 12, qfalse );

		//Shake the camera
		CG_ExplosionEffects( origin, 1, 200 );
	}
	else
	{
		le = CG_MakeExplosion2( origin, dir, cgs.media.explosionModel, 5, cgs.media.electricalExplosionSlowShader, 
								500, qfalse, 2.2f + ( crandom() * 0.4f), LEF_NONE);
		le->light = 200;
		le->refEntity.renderfx |= RF_NOSHADOW;
		VectorSet( le->lightColor, 0.8f, 0.8f, 1.0f );

		CG_ImpactMark( cgs.media.compressionMarkShader, origin, normal, random()*360, 1,1,1,1, qfalse, 28, qfalse );

		//Shake the camera
		CG_ExplosionEffects( origin, 2, 240 );
	}

	// nice explosion sound at the point of impact
	trap_S_StartSound(origin, ENTITYNUM_WORLD, CHAN_AUTO, weaponInfo->mainHitSound);
}

/*
-------------------------
FX_CompressionHit
-------------------------
*/

void FX_CompressionHit( vec3_t origin )
{
	FX_AddSprite( origin, 
					NULL,
					qfalse,
					32.0f,
					-32.0f,
					1.0f,
					1.0f,
					random()*360,
					0.0f,
					250.0f,
					cgs.media.prifleImpactShader );

	//FIXME: Play an impact sound with a body
//	cgi_S_StartSound (origin, NULL, 0, cgi_S_RegisterSound ("sound/weapons/prifle/fire.wav") );
}
