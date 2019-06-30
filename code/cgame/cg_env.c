//This file contains environmental effects for the designers

#include "cg_local.h"
#include "fx_local.h"

// these flags should be synchronized with the spawnflags in g_fx.c for fx_bolt
#define BOLT_SPARKS		(1<<0)
#define BOLT_BORG		(1<<1)


qboolean SparkThink( localEntity_t *le)
{
	vec3_t	dir, direction, start, end;
	vec3_t	velocity;
	float	scale = 0, alpha = 0;
	int		numSparks = 0, i = 0, j = 0;
	sfxHandle_t snd = cgs.media.envSparkSound1;

	switch(irandom(1, 3))
	{
	case 1:
		snd = cgs.media.envSparkSound1;
		break;
	case 2:
		snd = cgs.media.envSparkSound2;
		break;
	case 3:
		snd = cgs.media.envSparkSound3;
		break;
	}
	trap_S_StartSound (le->refEntity.origin, ENTITYNUM_WORLD, CHAN_BODY, snd );

	VectorCopy(le->data.spawner.dir, dir);

	AngleVectors( dir, dir, NULL, NULL );
	for ( j = 0; j < 3; j ++ )
		direction[j] = dir[j] + (0.25f * crandom());

	VectorNormalize( direction );

	//cgi_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgi_S_RegisterSound( va( "sound/world/ric%d.wav", (rand() & 2)+1) ) );

	numSparks = 8 + (random() * 4.0f);
	
	scale = 0.2f + (random() *0.4);
	VectorMA( le->refEntity.origin, 24.0f + (crandom() * 4.0f), dir, end );

	//One long spark
	FX_AddLine( le->refEntity.origin,
				end,
				1.0f,
				scale,
				0.0f,
				1.0f,
				0.25f,
				125.0f,
				cgs.media.sparkShader );
	
	for ( i = 0; i < numSparks; i++ )
	{	
		scale = 0.2f + (random() *0.4);

		for ( j = 0; j < 3; j ++ )
			direction[j] = dir[j] + (0.25f * crandom());
		
		VectorNormalize(direction);

		VectorMA( le->refEntity.origin, 0.0f + ( random() * 2.0f ), direction, start );
		VectorMA( start, 2.0f + ( random() * 16.0f ), direction, end );

		FX_AddLine( start,
					end,
					1.0f,
					scale,
					0.0f,
					1.0f,
					0.25f,
					125.0f,
					cgs.media.sparkShader );
	}

	if ( rand() & 1 )
	{
		numSparks = 1 + (random() * 2.0f);
		for ( i = 0; i < numSparks; i++ )
		{	
			scale = 0.5f + (random() * 0.5f);

			VectorScale( direction, 250, velocity );

			FX_AddTrail(	start,
							velocity,
							qtrue,
							8.0f,
							-32.0f,
							scale,
							-scale,
							1.0f,
							0.5f,
							0.25f,
							700.0f,
							cgs.media.sparkShader);

		}
	}

	VectorMA( le->refEntity.origin, 1, dir, direction );

	scale = 6.0f + (random() * 8.0f);
	alpha = 0.1 + (random() * 0.4f);

	VectorSet( velocity, 0, 0, 8 );

	FX_AddSprite(	direction, 
					velocity, 
					qfalse, 
					scale,
					scale,
					alpha,
					0.0f,
					random()*45.0f,
					0.0f,
					1000.0f,
					cgs.media.steamShader );

	return qtrue;
}

/*
======================
CG_Spark

Creates a spark effect
======================
*/

void CG_Spark( vec3_t origin, vec3_t normal, int delay)
{
	// give it a lifetime of 10 seconds because the refresh thinktime in g_fx.c is 10 seconds
	FX_AddSpawner( origin, normal, NULL, NULL, qfalse, delay, 1.5, 10000, SparkThink, 100 );

}



qboolean SteamThink( localEntity_t *le )
{
	float	speed = 200;
	vec3_t	direction;
	vec3_t	velocity		= { 0, 0, 128 };
	float	scale, dscale;

	//FIXME: Whole lotta randoms...

	VectorCopy( le->data.spawner.dir, direction );
	AngleVectors( direction, direction, NULL, NULL );
	VectorNormalize(direction);

	direction[0] += (direction[0] * crandom() * le->data.spawner.variance);
	direction[1] += (direction[0] * crandom() * le->data.spawner.variance);
	direction[2] += (direction[0] * crandom() * le->data.spawner.variance);


	VectorScale( direction, speed, velocity );

	scale = 4.0f + (random());
	dscale = scale * 4.0;

	FX_AddSprite(	le->refEntity.origin,
					velocity, 
					qfalse, 
					scale, 
					dscale, 
					1.0f, 
					0.0f,
					random() * 360,
					0.25f,
					300,//(len / speed) * 1000, 
					cgs.media.steamShader );

	return qtrue;
}

/*
======================
CG_Steam

Creates a steam effect
======================
*/

void CG_Steam( vec3_t position, vec3_t dir )
{
	// give it a lifetime of 10 seconds because the refresh thinktime in g_fx.c is 10 seconds
	FX_AddSpawner( position, dir, NULL, NULL, qfalse, 0, 0.15, 10000, SteamThink, 100 );
}

/*
======================
CG_Bolt

Creates a electricity bolt effect
======================
*/
#define DATA_EFFECTS	0
#define DATA_CHAOS		1
#define DATA_RADIUS		2

//-----------------------------
void BoltSparkSpew( vec3_t origin, vec3_t normal )
{
	float	scale = 1.0f + ( random() * 1.0f );
	int		num = 0, i = 0;
	vec3_t	vel;

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


qboolean BoltFireback( localEntity_t *le)
{
//localEntity_t *FX_AddElectricity( vec3_t origin, vec3_t origin2, float stScale, float scale, float dscale, 
//									float startalpha, float endalpha, float killTime, qhandle_t shader, float deviation );							
	float killTime = (0 == le->data.spawner.delay)?10000:200;

	FX_AddElectricity(le->refEntity.origin, le->data.spawner.dir, 0.2f, 15.0, -15.0, 1.0, 0.5, killTime,
		cgs.media.bolt2Shader, le->data.spawner.variance );
	// is this spawner on a random delay?
	if (le->data.spawner.data1)
	{
		le->data.spawner.delay = flrandom(0,5000);
	}
	return qtrue;
}

//-----------------------------
qboolean BorgBoltFireback( localEntity_t *le)
{
	float killTime = (0 == le->data.spawner.delay)?10000:200;

	FX_AddElectricity(le->refEntity.origin, le->data.spawner.dir, 0.2f, 15.0, -5.0, 1.0, 0.5, killTime,
		cgs.media.borgLightningShaders[1], le->data.spawner.variance );
	// is this spawner on a random delay?
	if (le->data.spawner.data1)
	{
		le->data.spawner.delay = flrandom(0,5000);
	}
	return qtrue;
}

//-----------------------------
qboolean BoltFirebackSparks( localEntity_t *le)
{
	vec3_t	dir;
	float killTime = (0 == le->data.spawner.delay)?10000:200;

	VectorSubtract(le->refEntity.origin, le->data.spawner.dir, dir);
	VectorNormalize(dir);
	FX_AddElectricity(le->refEntity.origin, le->data.spawner.dir, 0.2f, 15.0, -15.0, 1.0, 0.5, killTime,
		cgs.media.bolt2Shader, le->data.spawner.variance );
	BoltSparkSpew(le->data.spawner.dir, dir);
	// is this spawner on a random delay?
	if (le->data.spawner.data1)
	{
		le->data.spawner.delay = flrandom(0,5000);
	}
	return qtrue;
}

//-----------------------------
qboolean BorgBoltFirebackSparks( localEntity_t *le)
{
	vec3_t	dir;
	float killTime = (0 == le->data.spawner.delay)?10000:200;

	VectorSubtract(le->refEntity.origin, le->data.spawner.dir, dir);
	VectorNormalize(dir);
	FX_AddElectricity(le->refEntity.origin, le->data.spawner.dir, 0.2f, 15.0, -15.0, 1.0, 0.5, killTime,
		cgs.media.borgLightningShaders[0], le->data.spawner.variance );
	BoltSparkSpew(le->data.spawner.dir, dir);
	// is this spawner on a random delay?
	if (le->data.spawner.data1)
	{
		le->data.spawner.delay = flrandom(0,5000);
	}
	return qtrue;
}

//-----------------------------
void CG_Bolt( centity_t *cent )
{
	localEntity_t *le = NULL;
	qboolean	bSparks = cent->currentState.eventParm & BOLT_SPARKS;
	qboolean	bBorg = cent->currentState.eventParm & BOLT_BORG;
	float		radius = cent->currentState.angles2[0], chaos = cent->currentState.angles2[1];
	float		delay = cent->currentState.time2 * 1000; // the value given by the designer is in seconds
	qboolean	bRandom = qfalse;

	if (delay < 0)
	{
		// random
		delay = flrandom(0.1, 5000.0);
		bRandom = qtrue;
	}
	if (delay > 10000)
	{
		delay = 10000;
	}

	if ( bBorg )
	{
		if (bSparks)
		{
			le = FX_AddSpawner( cent->lerpOrigin, cent->currentState.origin2, NULL, NULL, qfalse, delay,
				chaos, 10000, BorgBoltFirebackSparks, radius );
		}
		else
		{
			le = FX_AddSpawner( cent->lerpOrigin, cent->currentState.origin2, NULL, NULL, qfalse, delay,
				chaos, 10000, BorgBoltFireback, radius );
		}
	}
	else
	{
		if (bSparks)
		{
			le = FX_AddSpawner( cent->lerpOrigin, cent->currentState.origin2, NULL, NULL, qfalse, delay,
				chaos, 10000, BoltFirebackSparks, radius );
		}
		else
		{
			le = FX_AddSpawner( cent->lerpOrigin, cent->currentState.origin2, NULL, NULL, qfalse, delay,
				chaos, 10000, BoltFireback, radius );
		}
	}
	if (bRandom)
	{
		le->data.spawner.data1 = 1;
	}
}

void CG_TransporterPad(vec3_t origin)
{
	FX_TransporterPad(origin);
}

/*
===========================
Drip

Create timed drip effect
===========================
*/

qboolean DripCallback( localEntity_t *le )
{
	localEntity_t *trail = NULL;
	qhandle_t	shader = 0;

	switch (le->data.spawner.data1)
	{
	case 1:
		shader = cgs.media.oilDropShader;
		break;
	case 2:
		shader = cgs.media.greenDropShader;
		break;
	case 0:
	default:
		shader = cgs.media.waterDropShader;
		break;
	}
	trail = FX_AddTrail(le->refEntity.origin, le->pos.trDelta, qfalse, 4, -2, 1, 0, 0.8, 0.4, 0.0,
		300, shader);
	trail->leFlags |= LEF_ONE_FRAME;

	return qtrue;
}

//------------------------------------------------------------------------------
qboolean DripSplash( localEntity_t *le )
{
	float	scale = 1.0f + ( random() * 1.0f );
	int		num = 0, i = 0;
	vec3_t	vel, normal, origin;
	qhandle_t	shader = 0;

	switch (le->data.spawner.data1)
	{
	case 1:
		shader = cgs.media.oilDropShader;
		break;
	case 2:
		shader = cgs.media.greenDropShader;
		break;
	case 0:
	default:
		shader = cgs.media.waterDropShader;
		break;
	}

	VectorCopy(le->data.spawner.dir, normal);
	VectorCopy(le->refEntity.origin, origin);

	// splashing water droplets. which, I'm fairly certain, is an alternative band from Europe.
	num = (int)(random() * 2) + 6;

	for ( i = 0; i < num; i++ )
	{
		scale = 0.6f + random();
		if ( rand() & 1 )
			FXE_Spray( normal, 110, 80, 0.9f, vel);
		else
			FXE_Spray( normal, 150, 150, 0.5f, vel);

		FX_AddTrail( origin, vel, qtrue, 4.0f, 0.0f,
								scale, -scale, 1.0f, 0.8f, 0.4f, 200.0f, shader );
	}

	switch( rand() & 3 )
	{
	case 1:
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_BODY, cgs.media.waterDropSound1 );
		break;
	case 2:
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_BODY, cgs.media.waterDropSound2 );
		break;
	default:
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_BODY, cgs.media.waterDropSound3 );
		break;
	}
	return qtrue;

}

qboolean JackTheDripper( localEntity_t *le )
{
	trace_t		trace;
	vec3_t		vel, down = {0,0,-1}, end, origin, new_origin;
	float		time, dis, diameter = 1.0;
	qhandle_t	shader = 0;
	int			maxDripsPerLifetime = 200; // given a 10 second lifetime
	int			desiredDrips = 1 + (int)(le->data.spawner.variance * (maxDripsPerLifetime-1)); // range of (1...max)
	float		percentLife = 1.0f - (le->endTime - cg.time)*le->lifeRate;
	localEntity_t *splash = NULL;

	switch (le->data.spawner.data1)
	{
	case 1:
		shader = cgs.media.oilDropShader;
		break;
	case 2:
		shader = cgs.media.greenDropShader;
		break;
	case 0:
	default:
		shader = cgs.media.waterDropShader;
		break;
	}

	// do we need to add a drip to maintain our drips-per-second rate?
	while ( (int)(flrandom(percentLife-0.05,percentLife+0.05)*desiredDrips) > le->data.spawner.data2)
	{
		VectorCopy(le->refEntity.origin, origin);

		// the more drips per second, spread them out from our origin point
		fxRandCircumferencePos(origin, down, 10*le->data.spawner.variance, new_origin);

		// Ideally, zero should be used for vel...so just use something sufficiently close
		VectorSet( vel, 0, 0, -0.00001 );

		// Find out where it will hit
		VectorMA( new_origin, 1024, down, end );
		CG_Trace( &trace, new_origin, NULL, NULL, end, 0, MASK_SHOT );
		if ( trace.fraction < 1.0 )
		{
			VectorSubtract( trace.endpos, new_origin, end );
			dis = VectorNormalize( end );

			time = sqrt( 2*dis / DEFAULT_GRAVITY ) * 1000;	// Calculate how long the thing will take to travel that distance

			// Falling drop
			splash = FX_AddParticle( new_origin, vel, qtrue, diameter, 0.0, 0.8, 0.8, 0.0, 0.0, time, shader, DripCallback );
			splash->data.spawner.data1 = le->data.spawner.data1;

			splash = FX_AddSpawner(trace.endpos, trace.plane.normal, vel, NULL, qfalse, time, 0, time + 200, DripSplash, 10);
			splash->data.spawner.data1 = le->data.spawner.data1;
		}
		else
		// Falling a long way so just send one that will fall for 2 secs, but don't spawn a splash
		{
			FX_AddParticle( new_origin, vel, qtrue, diameter, 0.0, 0.8, 0.8, 0.0, 0.0, 2000, shader, 0/*NULL*/ );
		}
		//increase our number-of-drips counter
		le->data.spawner.data2++;
	}
	return qtrue;
}

//------------------------------------------------------------------------------
void CG_Drip(centity_t *cent)
{
	vec3_t down = {0,0,-1};
	localEntity_t *le = NULL;

	// clamp variance to [0...1]
	if (cent->currentState.angles2[0] < 0)
	{
		cent->currentState.angles2[0] = 0;
	}
	else if (cent->currentState.angles2[0] > 1)
	{
		cent->currentState.angles2[0] = 1;
	}
	// cent->currentState.angles2[0] is the degree of drippiness
	// cent->currentState.time2 is the type of drip (water, oil, etc.)
	le = FX_AddSpawner( cent->lerpOrigin, down, NULL, NULL, qfalse, 0,
		cent->currentState.angles2[0], 10000, JackTheDripper, cent->currentState.time2 );
	//init our number-of-drips counter
	le->data.spawner.data2 = 0;
}

//------------------------------------------------------------------------------
void CG_Chunks( vec3_t origin, vec3_t dir, float scale, material_type_t type )
{
	int				i, j, k;
	int				numChunks;
	float			baseScale = 1.0f, dist, radius;
	vec3_t			v;
	sfxHandle_t		snd = 0;
	localEntity_t	*le;
	refEntity_t		*re;

	if ( type == MT_NONE )
		return;

	if ( type >= NUM_CHUNK_TYPES )
	{
		CG_Printf( "^6Chunk has invalid material %d!\n", type);
		type = MT_METAL;	//something legal please
	}

	switch( type )
	{
	case MT_GLASS:
	case MT_GLASS_METAL:
		snd = cgs.media.glassChunkSound;
		break;

	case MT_METAL:
	case MT_STONE:
	case MT_WOOD:
	default:
		snd = cgs.media.metalChunkSound;
		break;
	}

	trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_BODY, snd );

	numChunks = irandom( 8, 12 );

	// LOD num chunks
	VectorSubtract( cg.snap->ps.origin, origin, v );
	dist = VectorLength( v );

	if ( dist > 512 )
	{
		numChunks *= 512.0 / dist;		// 1/2 at 1024, 1/4 at 2048, etc.
	}

	// attempt to scale the size of the chunks based on the size of the brush
	radius = baseScale + ( ( scale - 128 ) / 128.0f );

	for ( i = 0; i < numChunks; i++ )
	{
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		le->leType = LE_FRAGMENT;
		le->endTime = cg.time + 2000;

		VectorCopy( origin, re->origin );

		for ( j = 0; j < 3; j++ )
		{
			re->origin[j] += crandom() * 12;
		}
		VectorCopy( re->origin, le->pos.trBase );

		//Velocity
		VectorSet( v, crandom(), crandom(), crandom() );
		VectorAdd( v, dir, v );
		VectorScale( v, flrandom( 100, 350 ), le->pos.trDelta );

		//Angular Velocity
		VectorSet( le->angles.trBase, crandom() * 360, crandom() * 360, crandom() * 360 );
		VectorSet( le->angles.trDelta, crandom() * 90, crandom() * 90, crandom() * 90 );

		AxisCopy( axisDefault, re->axis );

		le->data.fragment.radius = flrandom( radius * 0.7f, radius * 1.3f );

		re->nonNormalizedAxes = qtrue;

		if ( type == MT_GLASS_METAL )
		{
			if ( rand() & 1 )
			{
				re->hModel = cgs.media.chunkModels[MT_METAL][irandom(0,5)];
			}
			else
			{
				re->hModel = cgs.media.chunkModels[MT_GLASS][irandom(0,5)];
			}
		}
		else
		{
			re->hModel = cgs.media.chunkModels[type][irandom(0,5)];
		}
		
		le->pos.trType = TR_GRAVITY;
		le->pos.trTime = cg.time;
		le->angles.trType = TR_INTERPOLATE;
		le->angles.trTime = cg.time;
		le->bounceFactor = 0.2f + random() * 0.2f;
		le->leFlags |= LEF_TUMBLE;

		re->shaderRGBA[0] = re->shaderRGBA[1] = re->shaderRGBA[2] = re->shaderRGBA[3] = 255;

		// Make sure that we have the desired start size set
		for( k = 0; k < 3; k++)
		{
			VectorScale( le->refEntity.axis[k], le->data.fragment.radius, le->refEntity.axis[k] );
		}
	}
}
