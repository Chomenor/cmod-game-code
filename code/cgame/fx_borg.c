#include "cg_local.h"
#include "fx_local.h"


#define BORG_SPIN 0.6f

//------------------------------------------------------------------------------
void FX_BorgProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	float	len;
	vec3_t	dir, end;

	FX_AddSprite( cent->lerpOrigin, NULL, qfalse, 
					8.0f + ( random() * 24.0f ), 0.0f, 
					1.0f, 1.0f, 
					random() * 360, 0.0f, 
					1, 
					cgs.media.borgFlareShader);

	// Energy glow
	FX_AddSprite( cent->lerpOrigin, NULL, qfalse, 
					18.0f + ( random() * 24.0f ), 0.0f, 
					0.2f, 0.1f, 
					random() * 360, 0.0f, 
					1, 
					cgs.media.borgFlareShader);

	VectorSet( dir, crandom(), crandom(), crandom() );
	VectorNormalize( dir );
	len = random() * 12.0f + 18.0f;
	VectorMA( cent->lerpOrigin, len, dir, end );
	FX_AddElectricity( cent->lerpOrigin, end, 0.2f, 0.6f, 0.0f, 0.3f, 0.0f, 5, cgs.media.borgLightningShaders[2], 1.0f );
}

/*
-------------------------
FX_BorgWeaponHitWall
-------------------------
*/
void FX_BorgWeaponHitWall( vec3_t origin, vec3_t normal )
{
	weaponInfo_t	*weaponInfo = &cg_weapons[WP_BORG_WEAPON];

	// Expanding shock ring
	FX_AddQuad( origin, normal, 
					0.5f, 6.4f, 
					0.8, 0.0, 
					random() * 360.0f, 
					200, 
					cgs.media.borgLightningShaders[0] );

	// Impact core
	FX_AddQuad( origin, normal, 
					16.0f + ( random() * 8.0f ), 3.2f, 
					0.6f, 0.0f, 
					cg.time * BORG_SPIN, 
					100, 
					cgs.media.borgLightningShaders[0] );

	//Sound
	trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, weaponInfo->mainHitSound);
	
	CG_ImpactMark( cgs.media.scavMarkShader, origin, normal, random()*360, 1,1,1,0.2, qfalse, random() + 5.5f, qfalse );
}

/*
-------------------------
FX_BorgTaser
-------------------------
*/

void FX_BorgTaser( vec3_t end, vec3_t start )
{
	float	len;
	vec3_t	dis;

	FX_AddSprite( end, NULL, qfalse, 9.0f, 0.0f, 1.0f, 0.0f, 30.0f, 0.0f, 250, cgs.media.borgLightningShaders[0] );
	FX_AddSprite( end, NULL, qfalse, 18.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 150, cgs.media.borgLightningShaders[0] );
	FX_AddSprite( start, NULL, qfalse, 12.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 100, cgs.media.borgLightningShaders[0] );
	FX_AddSprite( start, NULL, qfalse, 6.0f, 0.0f, 1.0f, 0.0f, 60.0f, 0.0f, 150, cgs.media.borgLightningShaders[0] );

	VectorSubtract( start, end, dis );
	len = VectorNormalize( dis );

	if ( len < 96 )
		len = 0.6f;
	else if ( len < 256 )
		len = 0.4f;
	else if ( len > 512 )
		len = 0.1f;
	else
		len = 0.2f;

	FX_AddLine( start, end, 1.0f, 5.0f, 0.0f, 1.0f, 0.0f, 150, cgs.media.borgLightningShaders[1] );
	FX_AddElectricity( start, end, 0.2f, 3.0f, 0.0f, 1.0f, 0.0f, 150, cgs.media.borgLightningShaders[2], len );
	FX_AddElectricity( start, end, 0.5f, 2.0f, 0.0f, 1.0f, 0.0f, 150, cgs.media.borgLightningShaders[3], len );
}

//------------------------------------------------
void FX_BorgEyeBeam( vec3_t start, vec3_t end, vec3_t normal, qboolean large )
{
	float	width, alpha;
	vec3_t	rgb = {1.0f,0.0f,0.0f};

	width = 0.5f + ( crandom() * 0.1 );
	if ( large )
		width *= 3.5;

	alpha = 0.4f + ( random() * 0.25 );

	FX_AddLine2( start, end, 1.0f, 
				width, 0.0f, width, 0.0f, 
				alpha, alpha, 
				rgb, rgb, 
				1.0f, 
				cgs.media.whiteLaserShader );

	FX_AddSprite( start, NULL, qfalse, 
				1.0f + (random() * 2.0f), 0.0f, 
				0.6f, 0.6f, 
				0.0f, 0.0f, 1.0f, 
				cgs.media.borgEyeFlareShader );

	FX_AddQuad( end, normal, 
				2.0f + (crandom() * 1.0f), 0.0f, 
				1.0f, 1.0f, 
				0.0f, 1.0f, 
				cgs.media.borgEyeFlareShader );
}

#define BORG_PARTICLE_RADIUS	32
//------------------------------------------------------------------------------------
void FX_BorgTeleportParticles( vec3_t origin, vec3_t dir )
{
	int		i;
	vec3_t	neworg, vel;

	for ( i = 0; i < 26; i++ )
	{
		VectorSet( neworg, 
				origin[0] + ( crandom() * ( BORG_PARTICLE_RADIUS * 0.5 ) ), 
				origin[1] + ( crandom() * ( BORG_PARTICLE_RADIUS * 0.5 ) ), 
				origin[2] + ( crandom() * 4.0f ) );
		VectorScale( dir, 32 + ( random() * 96 ), vel );

		FX_AddSprite( neworg, vel, qfalse, 1.0f + ( crandom() * 2.0f ), 0.0f, 1.0f, 0.0f, random() * 360, 0.0f, 1700, cgs.media.borgFlareShader );
	}
}

//-------------------------------------
void FX_BorgTeleport( vec3_t origin )
{
	vec3_t			org, org2, angles, dir;
	localEntity_t	*le;

	VectorSet( angles, 0, 0, 1 );
	VectorSet( org, origin[0], origin[1], origin[2] - 32 );
	FX_BorgTeleportParticles( origin, angles );

	VectorSet( angles, 0, 0, -1 );
	VectorSet( org2, origin[0], origin[1], origin[2] + 32 );
	FX_BorgTeleportParticles( origin, angles );

	VectorSubtract( org2, org, dir );
	VectorNormalize( dir );

	le = FX_AddCylinder( org, dir, 96.0f, 0.0f, 1.0f, 48.0f, 1.0f, 48.0f, 1.0f, 0.0f, 1500, cgs.media.borgFlareShader, 0.5 );
	le->refEntity.data.cylinder.wrap = qtrue;
	le->refEntity.data.cylinder.stscale = 24;

	trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.borgBeamInSound );
}

//-------------------------------------
void FX_BorgTeleportTrails( vec3_t origin )
{
	int		i;
	float	scale;
	vec3_t	org, ang, angvect;

	for ( i = 0; i < 2; i++ )
	{
		// position on the sphere
		if ( i == 0 )
		{
			ang[ROLL]	= 0;
			ang[PITCH]	= sin( cg.time * 0.002f ) * 360;
			ang[YAW]	= cg.time * 0.04f;
		}
		else
		{
			ang[ROLL]	= 0;
			ang[PITCH]	= sin( cg.time * 0.002f + 3.14159f ) * 360;
			ang[YAW]	= cg.time * 0.04f + 180.0f;
		}

		AngleVectors( ang, angvect, NULL, NULL);

		// Set the particle position
		org[0] = 12 * angvect[0] + origin[0];
		org[1] = 12 * angvect[1] + origin[1];
		org[2] = 32 * angvect[2] + origin[2];

		scale = random() * 4.0f + 4.0f;

		FX_AddSprite( org, NULL, qtrue, scale, -scale, 1.0f, 1.0f, 0.0f, 0.0f, 200.0f + random() * 200.0f, cgs.media.borgFlareShader );
	}
}
