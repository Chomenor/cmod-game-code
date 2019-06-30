#include "cg_local.h"
#include "fx_local.h"



//
// detpack
//

#define NUM_RING_SHADERS 6

qboolean DetpackAftereffect(localEntity_t *le)
{
	localEntity_t *cyl = NULL;
	qhandle_t	shader = cgs.media.phaserShader;
	qhandle_t	slowRingShaders[NUM_RING_SHADERS];
	float		percentLife = 1.0 - (le->endTime - cg.time)*le->lifeRate;
	float		alpha = 0.6 - (0.6*percentLife*percentLife);
	// data for shell
	float		shellLife = percentLife + .2;
	float		height1 = 20 + (percentLife * 150);
	float		height2 =(50*percentLife);
	float		scale1 = 40 + (percentLife * 1500);
	float		scale2 = 20 + (percentLife * 1200);
	// data for flat energy rings
	float		ringLife = percentLife + .5;
	float		scale3 = 200 + (percentLife * 3400);
	float		scale4 = 100 + (percentLife * 3000);
	float		scale5 = 20 + (percentLife * 1000);
	float		scale6 = 10 + (percentLife * 200);
	float		ringAlpha = 0.6 - (0.6*ringLife*ringLife);
	vec3_t		up = {0,0,1},origin1;

	
	slowRingShaders[0] = cgs.media.testDetpackRingShader1;
	slowRingShaders[1] = cgs.media.testDetpackRingShader2;
	slowRingShaders[2] = cgs.media.testDetpackRingShader3;
	slowRingShaders[3] = cgs.media.testDetpackRingShader4;
	slowRingShaders[4] = cgs.media.testDetpackRingShader5;
	slowRingShaders[5] = cgs.media.testDetpackRingShader6;

	// slower, inner ring
	VectorCopy(le->refEntity.origin, origin1);
	if (NUM_RING_SHADERS == le->data.spawner.data1)
	{
		le->data.spawner.data1 = 0;
	}
	else if (le->data.spawner.data1 < 0)
	{
		le->data.spawner.data1 = 0;
	}
	shader = slowRingShaders[le->data.spawner.data1++];
	// fast, outer ring
	cyl = FX_AddCylinder(	origin1, 
							up,
							0.1,// height,
							0,// dheight,
							scale5,// scale,
							0,// dscale,
							scale6,// scale2,
							0,// dscale2,
							ringAlpha,// startalpha, 
							0.0,// endalpha, 
							500,// killTime, 
							shader,
							15);// bias );
	cyl->leFlags |= LEF_ONE_FRAME;

	if (shellLife <= 1.0f)
	{
		origin1[2] += height2;
		shader = cgs.media.phaserShader;
		cyl = FX_AddCylinder(	origin1, 
								up,
								height1,// height,
								0,// dheight,
								scale1,// scale,
								0,// dscale,
								scale2,// scale2,
								0,// dscale2,
								alpha,// startalpha, 
								0.0,// endalpha, 
								500,// killTime, 
								shader,
								15);// bias );
		cyl->leFlags |= LEF_ONE_FRAME;

		cyl = FX_AddCylinder(	le->refEntity.origin, 
								up,
								height2,	// height,
								0,			// dheight,
								scale1,		// scale,
								0,			// dscale,
								scale1,		// scale2,
								0,			// dscale2,
								alpha,		// startalpha, 
								0.0,		// endalpha, 
								500,		// killTime, 
								shader,
								15);		// bias );
		cyl->leFlags |= LEF_ONE_FRAME;
	}		
	// flat energy wave thingy
	if (ringLife <= 1.0f)
	{
		shader = cgs.media.testDetpackShader3;
		VectorCopy(le->refEntity.origin, origin1);
		// fast, outer ring
		
		cyl = FX_AddCylinder(	origin1, 
								up,
								0.1,// height,
								0,// dheight,
								scale3,// scale,
								0,// dscale,
								scale4,// scale2,
								0,// dscale2,
								ringAlpha,// startalpha, 
								0.0,// endalpha, 
								500,// killTime, 
								shader,
								15);// bias );
		cyl->leFlags |= LEF_ONE_FRAME;
	}
	return qtrue;
}


void FX_Detpack(vec3_t origin)
{
	localEntity_t	*le;
	vec3_t			direction, org, vel, norm = {0,0,1};
	int i;

	VectorCopy( norm, direction);

	// Add an explosion and tag a light to it
	le = CG_MakeExplosion2( origin, direction, cgs.media.nukeModel, 5, (qhandle_t)NULL, 250, qfalse, 100.0f, LEF_FADE_RGB);
	le->light = 300;
	le->refEntity.renderfx |= RF_NOSHADOW;

	VectorSet( le->lightColor, 1.0f, 0.6f, 0.2f );

	// Ground ring
//	FX_AddQuad( origin, norm, 5, 150, 1.0, 0.0, random() * 360, 600, cgs.media.bigShockShader );
	// Flare
	VectorMA( origin, 12, direction, org );
	FX_AddSprite( org, NULL, qfalse, 160.0, -160.0, 1.0, 0.0, 0.0, 0.0, 500, cgs.media.sunnyFlareShader );//, FXF_NON_LINEAR_FADE );

	for (i = 0; i < 12; i++)
	{
		float width, length;
		FXE_Spray( norm, 470, 325, 0.5f, vel);
		length = 50.0 + random() * 12;
		width = 1.5 + random() * 2;
		FX_AddTrail( origin, vel, qtrue, length, -length, width, -width, 
						1.0f, 1.0f, 0.5f, 1000.0f,  cgs.media.orangeTrailShader);
	}

//	trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.detpackExplodeSound );	

	// Smoke and impact
	CG_ImpactMark( cgs.media.compressionMarkShader, origin, norm, random()*360, 1,1,1,1.0, qfalse, 
				random() * 16 + 48, qfalse );

	// mondo explosion shock wave cloud thing
	le = FX_AddSpawner( origin, norm, NULL, NULL, qfalse, 0,
							 0, 1500, DetpackAftereffect, 10 );
	le->data.spawner.data1 = 0;

	// shake absolutely _everyone_
	CG_ExplosionEffects(origin, 5.0f, 8092);
}


//
// portable shield
//


void FX_DrawPortableShield(centity_t *cent)
{
	int				xaxis, height, posWidth, negWidth, team;
	vec3_t			start, end, normal;
	float			halfHeight;
	localEntity_t	*le;
	qhandle_t		shader;

	if (cent->currentState.eFlags & EF_NODRAW)
	{
		return;
	}

	// decode the data stored in time2
	xaxis = ((cent->currentState.time2 >> 24) & 1);
	height = ((cent->currentState.time2 >> 16) & 255);
	posWidth = ((cent->currentState.time2 >> 8) & 255);
	negWidth = (cent->currentState.time2 & 255);

	team = (cent->currentState.otherEntityNum2);
	halfHeight = (float)height * .5;

	VectorCopy(cent->lerpOrigin, start);
	VectorCopy(cent->lerpOrigin, end);
	start[2] += halfHeight;
	end[2] += halfHeight;

	VectorClear(normal);
	if (xaxis) // drawing along x-axis
	{
		start[0] -= negWidth;
		end[0] += posWidth;
		normal[1] = 1;
	}
	else
	{
		start[1] -= negWidth;
		end[1] += posWidth;
		normal[0] = 1;
	}
	// draw a rectangle o' shieldness

	if (team == TEAM_RED)
	{
		if (cent->currentState.eFlags & EF_ITEMPLACEHOLDER)
		{	// Damaged.
			shader = cgs.media.shieldDamageShaderRed;
		}
		else
		{
			shader = cgs.media.shieldActivateShaderRed;
		}
	}
	else
	{
		if (cent->currentState.eFlags & EF_ITEMPLACEHOLDER)
		{	// Damaged.
			shader = cgs.media.shieldDamageShaderBlue;
		}
		else
		{
			shader = cgs.media.shieldActivateShaderBlue;
		}
	}

	le = FX_AddOrientedLine(start, end, normal, 1.0f, height, 0.0f, 1.0f, 1.0f, 50.0, shader);
//	le->leFlags |= LEF_ONE_FRAME;
}


