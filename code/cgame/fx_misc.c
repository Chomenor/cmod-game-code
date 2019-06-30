#include "cg_local.h"
#include "fx_local.h"


/*
-------------------------
FX_Disruptor
-------------------------
*/
void DisruptorShards(vec3_t org)
{
	vec3_t normal, end;

	// Pick a random endpoint
	VectorSet( normal, crandom(), crandom(), crandom() );
	VectorNormalize( normal );

	end[0] = org[0] + ( normal[0] * ( 48 + crandom() * 16 ));
	end[1] = org[1] + ( normal[1] * ( 48 + crandom() * 16 ));
	end[2] = org[2] + ( normal[2] * ( 64 + crandom() * 24 ));

	// Draw a light shard, use a couple of different kinds so it doesn't look too homogeneous
	if( rand() & 1 )
	{
		FX_AddLine( org, end, 1.0, random() * 0.5 + 0.5, 12.0, random() * 0.1 + 0.1, 0.0, 200 + random() * 350, cgs.media.orangeParticleShader );
	}
	else
	{
		FX_AddLine( org, end, 1.0, random() * 0.5 + 0.5, 12.0, random() * 0.1 + 0.1, 0.0, 200 + random() * 350, cgs.media.yellowParticleShader );
	}
}

qboolean MakeDisruptorShard( localEntity_t *le )
{
	DisruptorShards(le->refEntity.origin);
	return(qtrue);
}

// Effect used when scav beams in--this wouldn't work well for a scav on the ground if they were to beam out
void FX_Disruptor( vec3_t org, float length )
{//FIXME: make it move with owner?
	vec3_t org1, org2, normal={0,0,1};
	int t;

	VectorMA( org, 48, normal, org1 );
	VectorMA( org, -48, normal, org2 );

	// This is the core
	FX_AddLine( org1, org2, 1.0, 0.1, 48.0, 1.0, 0.0, length, cgs.media.dkorangeParticleShader );

	// Spawn a bunch to get the effect going
	for (t=0; t < 12; t++ )
	{
		DisruptorShards( org);
	}

	// Keep spawning the light shards for a while.
	FX_AddSpawner( org, normal, NULL, NULL, qfalse, 20, 10, length*0.75, MakeDisruptorShard, 0);
}




void FX_EnergyGibs(vec3_t origin )
{
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			dir;
	int				i, j, k;
	int				chunkModel=0;
	float			baseScale = 0.7f, dist;
	int				numChunks;

	numChunks = irandom( 10, 15 );

	VectorSubtract(cg.snap->ps.origin, origin, dir);
	dist = VectorLength(dir);
	if (dist > 512)
	{
		numChunks *= 512.0/dist;		// 1/2 at 1024, 1/4 at 2048, etc.
	}

	for ( i = 0; i < numChunks; i++ )
	{
		chunkModel = cgs.media.chunkModels[MT_METAL][irandom(0,5)];

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
		VectorSet( dir, crandom(), crandom(), crandom() );
		VectorScale( dir, flrandom( 300, 500 ), le->pos.trDelta );

		//Angular Velocity
		VectorSet( le->angles.trBase, crandom() * 360, crandom() * 360, crandom() * 360 );
		VectorSet( le->angles.trDelta, crandom() * 90, crandom() * 90, crandom() * 90 );

		AxisCopy( axisDefault, re->axis );

		le->data.fragment.radius = flrandom(baseScale * 0.4f, baseScale * 0.8f );

		re->nonNormalizedAxes = qtrue;
		re->hModel = chunkModel;
		re->renderfx |= RF_CAP_FRAMES;
		re->customShader = cgs.media.quantumDisruptorShader;
		re->shaderTime = cg.time/1000.0f;
		
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
			VectorScale(le->refEntity.axis[k], le->data.fragment.radius, le->refEntity.axis[k]);
		}
	}
}



void FX_ExplodeBits( vec3_t org)
{
	float width, length;
	vec3_t vel, pos;
	int i;

	FX_EnergyGibs(org);

	for (i = 0; i < 32; i++)
	{
		VectorSet(vel, flrandom(-320,320), flrandom(-320,320), flrandom(-100,320));
		VectorCopy(org, pos);
		pos[2] += flrandom(-8, 8);
		length = flrandom(10,20);
		width = flrandom(2.0,4.0);
		FX_AddTrail( pos, vel, qtrue, length, -length, width, -width, 
						1.0f, 1.0f, 0.5f, 1000.0f,  cgs.media.orangeTrailShader);
	}
}
