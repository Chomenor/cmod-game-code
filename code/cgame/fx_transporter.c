
#include "cg_local.h"
#include "fx_local.h"

/*
-------------------------
TransporterParticle
-------------------------
*/

qboolean TransporterParticle( localEntity_t *le)
{
	vec3_t		org, velocity	=	{ 0, 0, 68 };
	vec3_t		accel = { 0, 0, -12 };
	float		scale, dscale;
	qhandle_t	shader;

	VectorCopy( le->refEntity.origin, org );
	org[2] += 0;//38;

	shader = ( le->data.spawner.dir[0] == 0 ) ? cgs.media.trans1Shader : cgs.media.trans2Shader;
	scale  = ( le->data.spawner.dir[0] == 0 ) ? 2.0 : 4.0;
	dscale  = ( le->data.spawner.dir[0] == 0 ) ? 4.0 : 24.0;

	le->data.spawner.dir[0]++;

	FX_AddSprite(	org,
					velocity,
					qfalse,
					scale,
					dscale,
					1.0f,
					0.0f,
					0,
					0.0f,
					450.0f,
					shader );
	
	VectorScale( velocity, -1, velocity );
	VectorScale( accel, -1, accel );

	FX_AddSprite(	org,
					velocity,
					qfalse,
					scale,
					dscale,
					1.0f,
					0.0f,
					0,
					0.0f,
					450.0f,
					shader );

	return qtrue;
}

/*
-------------------------
TransporterPad
-------------------------
*/

qboolean TransporterPad( localEntity_t *le)
{
	vec3_t		org;
	vec3_t		up = {0,0,1};
	float		scale, dscale;
	qhandle_t	shader;

	VectorCopy( le->refEntity.origin, org );
	org[2] -= 3;

	shader = cgs.media.trans1Shader;
	scale  = 20.0;
	dscale  = 2.0;

	FX_AddQuad(		org,
					up,
					scale,
					dscale,
					1.0f,
					0.0f,
					0,
					950.0f,
					shader );
	return qtrue;
}

/*
-------------------------
FX_Transporter
-------------------------
*/

void FX_Transporter( vec3_t origin )
{
	vec3_t up = {0,0,1};

	FX_AddSpawner( origin, up, NULL, NULL, qfalse, 0, 0, 200, TransporterParticle, 0 );
//	trap_S_StartSound( origin, NULL, CHAN_AUTO, cgs.media.teleInSound );
}

/*
-------------------------
FX_TransporterPad
-------------------------
*/

void FX_TransporterPad( vec3_t origin )
{
	vec3_t up = {0,0,1};

	FX_AddSpawner( origin, up, NULL, NULL, qfalse, 1000, 0, 0, TransporterPad, 0 );
}

