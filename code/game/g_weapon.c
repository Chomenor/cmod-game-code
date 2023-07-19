// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_weapon.c
// perform the server side effects of a weapon firing

#include "g_local.h"

static	float	s_quadFactor;
static	vec3_t	forward, right, up;
static	vec3_t	muzzle;

#define DMG_VAR			( modfn.AdjustWeaponConstant( WC_USE_RANDOM_DAMAGE, 1 ) ? flrandom(0.8,1.2) : 1.0f )

#define WEAPON_TRACE( results, start, mins, maxs, end, passEntityNum, contentmask ) \
		modfn.TrapTrace( results, start, mins, maxs, end, passEntityNum, contentmask, MOD_TRACE_WEAPON )

// Random functions which can be replicated on the client if weapon prediction is enabled
#define PREDICTABLE_RANDOM( clientNum ) BG_PREDICTABLE_RANDOM( modfn.WeaponPredictableRNG( clientNum ) )
#define PREDICTABLE_CRANDOM( clientNum ) BG_PREDICTABLE_CRANDOM( modfn.WeaponPredictableRNG( clientNum ) )
#define PREDICTABLE_IRANDOM( clientNum, min, max ) BG_PREDICTABLE_IRANDOM( modfn.WeaponPredictableRNG( clientNum ), min, max )

// Weapon damages are located up here for easy access...
// Phaser
#define	PHASER_DAMAGE				modfn.AdjustWeaponConstant( WC_PHASER_DAMAGE, 6 )
#define PHASER_ALT_RADIUS			modfn.AdjustWeaponConstant( WC_PHASER_ALT_RADIUS, 12 )

// Compression Rifle
#define	CRIFLE_DAMAGE				modfn.AdjustWeaponConstant( WC_CRIFLE_DAMAGE, 16 )
#define CRIFLE_MAIN_SPLASH_RADIUS	modfn.AdjustWeaponConstant( WC_CRIFLE_MAIN_SPLASH_RADIUS, 64 )
#define CRIFLE_MAIN_SPLASH_DMG		modfn.AdjustWeaponConstant( WC_CRIFLE_MAIN_SPLASH_DMG, 16 )
#define CRIFLE_ALTDAMAGE			modfn.AdjustWeaponConstant( WC_CRIFLE_ALTDAMAGE, 85 )
#define CRIFLE_ALT_SPLASH_RADIUS	modfn.AdjustWeaponConstant( WC_CRIFLE_ALT_SPLASH_RADIUS, 32 )
#define CRIFLE_ALT_SPLASH_DMG		modfn.AdjustWeaponConstant( WC_CRIFLE_ALT_SPLASH_DMG, 10 )

// Imod
#define	IMOD_DAMAGE					modfn.AdjustWeaponConstant( WC_IMOD_DAMAGE, 20 )
#define	IMOD_ALT_DAMAGE				modfn.AdjustWeaponConstant( WC_IMOD_ALT_DAMAGE, 48 )

// Scavenger Rifle
#define SCAV_DAMAGE					modfn.AdjustWeaponConstant( WC_SCAV_DAMAGE, 8 )
#define SCAV_ALT_DAMAGE				modfn.AdjustWeaponConstant( WC_SCAV_ALT_DAMAGE, 60 )
#define SCAV_ALT_SPLASH_RAD			modfn.AdjustWeaponConstant( WC_SCAV_ALT_SPLASH_RAD, 100 )
#define SCAV_ALT_SPLASH_DAM			modfn.AdjustWeaponConstant( WC_SCAV_ALT_SPLASH_DAM, 60 )

// Stasis Weapon
#define STASIS_DAMAGE				modfn.AdjustWeaponConstant( WC_STASIS_DAMAGE, 9 )
#define STASIS_ALT_DAMAGE			modfn.AdjustWeaponConstant( WC_STASIS_ALT_DAMAGE, 24 )
#define STASIS_ALT_DAMAGE2			modfn.AdjustWeaponConstant( WC_STASIS_ALT_DAMAGE2, 12 )

// Grenade Launcher
#define GRENADE_DAMAGE				modfn.AdjustWeaponConstant( WC_GRENADE_DAMAGE, 75 )
#define GRENADE_SPLASH_RAD			modfn.AdjustWeaponConstant( WC_GRENADE_SPLASH_RAD, 160 )
#define GRENADE_SPLASH_DAM			modfn.AdjustWeaponConstant( WC_GRENADE_SPLASH_DAM, 75 )
#define GRENADE_ALT_DAMAGE			64		// appears effectively unused (mines only do splash damage, see g_missile.c)

// Tetrion Disruptor
#define TETRION_DAMAGE				modfn.AdjustWeaponConstant( WC_TETRION_DAMAGE, 4 )
#define TETRION_ALT_DAMAGE			modfn.AdjustWeaponConstant( WC_TETRION_ALT_DAMAGE, 17 )

// Quantum Burst
#define QUANTUM_DAMAGE				modfn.AdjustWeaponConstant( WC_QUANTUM_DAMAGE, 140 )
#define QUANTUM_SPLASH_DAM			modfn.AdjustWeaponConstant( WC_QUANTUM_SPLASH_DAM, 140 )
#define QUANTUM_SPLASH_RAD			modfn.AdjustWeaponConstant( WC_QUANTUM_SPLASH_RAD, 160 )
#define QUANTUM_ALT_DAMAGE			modfn.AdjustWeaponConstant( WC_QUANTUM_ALT_DAMAGE, 140 )
#define QUANTUM_ALT_SPLASH_DAM		modfn.AdjustWeaponConstant( WC_QUANTUM_ALT_SPLASH_DAM, 140 )
#define QUANTUM_ALT_SPLASH_RAD		modfn.AdjustWeaponConstant( WC_QUANTUM_ALT_SPLASH_RAD, 80 )

// Dreadnought Weapon
#define DREADNOUGHT_DAMAGE			modfn.AdjustWeaponConstant( WC_DREADNOUGHT_DAMAGE, 8 )	// Note that there's two traces, so we can do up to DOUBLE damage...
#define DREADNOUGHT_WIDTH			modfn.AdjustWeaponConstant( WC_DREADNOUGHT_WIDTH, 9 )
#define DREADNOUGHT_ALTDAMAGE		modfn.AdjustWeaponConstant( WC_DREADNOUGHT_ALTDAMAGE, 35 )

// Borg Weapon
#define BORG_PROJ_DAMAGE			modfn.AdjustWeaponConstant( WC_BORG_PROJ_DAMAGE, 20 )
#define BORG_TASER_DAMAGE			modfn.AdjustWeaponConstant( WC_BORG_TASER_DAMAGE, 15 )

/*
======================
(ModFN) WeaponPredictableRNG
======================
*/
unsigned int ModFNDefault_WeaponPredictableRNG( int clientNum ) {
	return (unsigned int)rand();
}

/*
======================
SnapVectorTowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating
into a wall.
======================
*/
void SnapVectorTowards( vec3_t v, vec3_t to ) {
	int		i;

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( to[i] <= v[i] ) {
			v[i] = (int)v[i];
		} else {
			v[i] = (int)v[i] + 1;
		}
	}
}




/*
----------------------------------------------
	PLAYER WEAPONS
----------------------------------------------


----------------------------------------------
	PHASER
----------------------------------------------
*/

#define MAXRANGE_PHASER			2048		// This is the same as the range MAX_BEAM_RANGE	2048
#define NUM_PHASER_TRACES 3

#define BEAM_VARIATION		6

#define PHASER_POINT_BLANK			96
#define PHASER_POINT_BLANK_FRAC		((float)PHASER_POINT_BLANK / (float)MAXRANGE_PHASER)


//---------------------------------------------------------
void WP_FirePhaser( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	trace_t		tr;
	vec3_t		end;
	gentity_t	*traceEnt;
	int			trEnts[NUM_PHASER_TRACES], i = 0;
	float		trEntFraction[NUM_PHASER_TRACES];
	int			damage = 0;

	VectorMA (muzzle, MAXRANGE_PHASER, forward, end);
	// Add a subtle variation to the beam weapon's endpoint
	for (i = 0; i < 3; i ++ )
	{
		end[i] += crandom() * BEAM_VARIATION;
	}

	for (i = 0; i < NUM_PHASER_TRACES; i++)
	{
		trEnts[i] = -1;
		trEntFraction[i] = 0.0;
	}
	// Find out who we've hit
//	gi.trace ( &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);
	WEAPON_TRACE (&tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );
	if (tr.entityNum != (MAX_GENTITIES-1))
	{
		trEnts[0] = tr.entityNum;
		trEntFraction[0] = tr.fraction;
	}
	if ( alt_fire && ent->client->ps.ammo[WP_PHASER])
	{	// Use the ending point of the thin trace to do two more traces, one on either side, for actual damaging effect.
		vec3_t	vUp = {0,0,1}, vRight, start2, end2;
		float	halfBeamWidth = PHASER_ALT_RADIUS;

		CrossProduct(forward, vUp, vRight);
		VectorNormalize(vRight);
		VectorMA(muzzle, halfBeamWidth, vRight, start2);
		VectorMA(end, halfBeamWidth, vRight, end2);
		VectorCopy(tr.endpos, end);
		WEAPON_TRACE (&tr, muzzle, NULL, NULL, end, ent->s.number, (CONTENTS_PLAYERCLIP|CONTENTS_BODY) );
		if (	(tr.entityNum != (MAX_GENTITIES-1)) &&
				(tr.entityNum != trEnts[0]) )
		{
			trEnts[1] = tr.entityNum;
			trEntFraction[1] = tr.fraction;
		}
		VectorMA(muzzle, -halfBeamWidth, vRight, start2);
		VectorMA(end, -halfBeamWidth, vRight, end2);
		WEAPON_TRACE (&tr, muzzle, NULL, NULL, end, ent->s.number, (CONTENTS_PLAYERCLIP|CONTENTS_BODY) );
		if (	(tr.entityNum != (MAX_GENTITIES-1)) &&
				(tr.entityNum != trEnts[0]) &&
				(tr.entityNum != trEnts[1]))
		{
			trEnts[2] = tr.entityNum;
			trEntFraction[2] = tr.fraction;
		}
	}

	for (i = 0; i < NUM_PHASER_TRACES; i++)
	{
		if (-1 == trEnts[i])
		{
			continue;
		}
		traceEnt = &g_entities[ trEnts[i] ];

		if ( traceEnt->takedamage)
		{
//			damage = (float)PHASER_DAMAGE*DMG_VAR*s_quadFactor;		// No variance on phaser
			damage = (float)PHASER_DAMAGE*s_quadFactor;

			if (trEntFraction[i] <= PHASER_POINT_BLANK_FRAC)
			{	// Point blank!  Do up to double damage.
				damage += damage * (1.0 - (trEntFraction[i]/PHASER_POINT_BLANK_FRAC));
			}
			else
			{	// Normal range
				damage -= (int)(trEntFraction[i]*5.0);
			}

			if (!ent->client->ps.ammo[WP_PHASER])
			{
				damage *= .35; // weak out-of-ammo phaser
			}

			if (damage > 0)
			{
				if ( alt_fire || ent->client->ps.ammo[WP_PHASER])		// If empty, don't armor pierce.
				{
					G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage,
								DAMAGE_NO_KNOCKBACK | DAMAGE_NOT_ARMOR_PIERCING, MOD_PHASER_ALT );
				}
				else
				{
					G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage,
								DAMAGE_NO_KNOCKBACK | DAMAGE_ARMOR_PIERCING, MOD_PHASER );
				}
			}
		}
	}
	if (damage)
	{
		// log hit
		if (ent->client)
		{
			ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
		}
	}
}

/*
----------------------------------------------
	COMPRESSION RIFLE
----------------------------------------------
*/

#define COMPRESSION_SPREAD	100
#define MAXRANGE_CRIFLE		8192

//---------------------------------------------------------
void WP_FireCompressionRifle ( gentity_t *ent, qboolean alt_fire, qboolean imod_fx )
//---------------------------------------------------------
{
	gentity_t	*tent = 0;
	gentity_t	*traceEnt;
	int			damage = alt_fire ? (CRIFLE_ALTDAMAGE*DMG_VAR) : (CRIFLE_DAMAGE*DMG_VAR);
	int			splashDmg = alt_fire ? CRIFLE_ALT_SPLASH_DMG : CRIFLE_MAIN_SPLASH_DMG;
	int			splashRadius = alt_fire ? CRIFLE_ALT_SPLASH_RADIUS : CRIFLE_MAIN_SPLASH_RADIUS;
	trace_t		tr;
	vec3_t		end;
	float		r;
	float		u;
	qboolean	render_impact = qtrue;
	int			mod;
	int			dflags = DAMAGE_NOT_ARMOR_PIERCING;

	VectorMA (muzzle, MAXRANGE_CRIFLE, forward, end);

	// Make me 100% accurate
	if (	!alt_fire ||
			ent->client->ps.velocity[0] ||
			ent->client->ps.velocity[1] ||
			ent->client->ps.velocity[2] )
	{
		r = PREDICTABLE_CRANDOM( ent - g_entities )*COMPRESSION_SPREAD;
		u = PREDICTABLE_CRANDOM( ent - g_entities )*COMPRESSION_SPREAD;
		VectorMA (end, r, right, end);
		VectorMA (end, u, up, end);
	}

	WEAPON_TRACE (&tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );

	// If the beam hits a skybox, etc. it would look foolish to add in an explosion
	if ( tr.surfaceFlags & SURF_NOIMPACT )
	{
		render_impact = qfalse;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	if ( imod_fx ) {
		// Gladiator/instagib style imod shot
		tent = G_TempEntity( tr.endpos, EV_IMOD );
		VectorCopy( muzzle, tent->s.origin2 );
		SnapVector( tent->s.origin2 );
		SnapVector( tr.endpos );	// try to keep same behavior as rifle...
		
		// Add mod effects
		modfn.AddWeaponEffect( WE_IMOD_RIFLE, ent, &tr );
	} else {
		SnapVector(tr.endpos);		// Make this less networking-intensive.
		// Render a shot in any case.
		if ( alt_fire )
		{
			tent = G_TempEntity( muzzle, EV_COMPRESSION_RIFLE_ALT );
			tent->s.eFlags |= EF_ALT_FIRING;
		}
		else
		{
			tent = G_TempEntity( muzzle, EV_COMPRESSION_RIFLE );
		}
		// stash our firing direction in the message also
		VectorSubtract(end, muzzle, tent->s.origin2);
		VectorShort(tent->s.origin2);

		// Add mod effects
		if ( alt_fire ) {
			modfn.AddWeaponEffect( WE_RIFLE_ALT, ent, &tr );
		}
	}

	// Only add in impact stuff when told to do so
	if ( render_impact )
	{
		// send bullet impact
		if ( traceEnt->takedamage && traceEnt->client )
		{
		}
		else if ( splashDmg && splashRadius )
		{
			// Quad does not affect rifle splash unless enabled by mod
			float splashDmgAdjusted = splashDmg;
			if ( modfn.AdjustWeaponConstant( WC_CRIFLE_SPLASH_USE_QUADFACTOR, 0 ) ) {
				splashDmgAdjusted *= s_quadFactor;
			}

			if (alt_fire)
			{
				mod = MOD_CRIFLE_ALT_SPLASH;
			}
			else
			{
				mod = MOD_CRIFLE_SPLASH;
			}

			if( G_RadiusDamage( tr.endpos, ent, splashDmgAdjusted, splashRadius, NULL, 0, mod ) )
			{
				// Does a burst radius hit really count as a "hit"?  Not for compression rifles...
				//g_entities[ent->s.number].client->ps.persistant[PERS_ACCURACY_HITS]++;
			}
		}
	}
	if ( traceEnt->takedamage)
	{
		if (alt_fire)
		{
			mod = MOD_CRIFLE_ALT;
		}
		else
		{
			mod = MOD_CRIFLE;
		}
		G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage*s_quadFactor, dflags, mod );

		// give the shooter a reward sound if they have made two sniper hits in a row
		// check for "impressive" reward sound

		if (traceEnt->client)
		{
			if (((g_gametype.integer >= GT_TEAM) && (ent->client->ps.persistant[PERS_TEAM] != traceEnt->client->ps.persistant[PERS_TEAM]))
				|| (g_gametype.integer < GT_TEAM))
			{
				if (alt_fire && modfn.AdjustWeaponConstant( WC_IMPRESSIVE_REWARDS, 1 ))
				{
					ent->client->accurateCount++;
					if ( ent->client->accurateCount >= 2 )
					{
						ent->client->accurateCount -= 2;
						ent->client->ps.persistant[PERS_REWARD_COUNT]++;
						ent->client->ps.persistant[PERS_REWARD] = REWARD_IMPRESSIVE;
						ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
						// add the sprite over the player's head
						ent->client->ps.eFlags &= ~EF_AWARD_MASK;
						ent->client->ps.eFlags |= EF_AWARD_IMPRESSIVE;
						ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
					}
				}
			}
		}

		// log hit
		if (ent->client)
		{
			ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
		}
	}
	else
	{	// Misses reset the hit counter.
		ent->client->accurateCount = 0;
	}

	G_LogWeaponFire(ent->s.number, WP_COMPRESSION_RIFLE);
}


/*
----------------------------------------------
	IMOD
----------------------------------------------
*/

#define MAXRANGE_IMOD			4096
#define	MAX_RAIL_HITS	4

void IMODHit(qboolean alt_fire, gentity_t *traceEnt, gentity_t *ent, vec3_t d_dir, vec3_t endpos, vec3_t normal,
			qboolean render_impact)
{
	gentity_t	*tent = NULL;

	if ( alt_fire )
	{
		// send beam impact
		if ( traceEnt && traceEnt->takedamage )
		{
			G_Damage( traceEnt, ent, ent, d_dir, endpos, IMOD_ALT_DAMAGE*DMG_VAR*s_quadFactor,
						DAMAGE_NO_ARMOR | DAMAGE_NO_INVULNERABILITY, MOD_IMOD_ALT);	// Used to be DAMAGE_SUPER_ARMOR_PIERCING
			if (render_impact)
			{
				tent = G_TempEntity( endpos, EV_IMOD_ALTFIRE_HIT );
			}
			// log hit
			if (ent->client)
			{
				ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
			}
		}
	}
	else
	{
		// send beam impact
		if ( traceEnt && traceEnt->takedamage )
		{
			G_Damage( traceEnt, ent, ent, d_dir, endpos, IMOD_DAMAGE*DMG_VAR*s_quadFactor,
						DAMAGE_NO_ARMOR | DAMAGE_NO_INVULNERABILITY, MOD_IMOD);		// Used to be DAMAGE_SUPER_ARMOR_PIERCING
			if (render_impact)
			{
				tent = G_TempEntity( endpos, EV_IMOD_HIT );
			}
			// log hit
			if (ent->client)
			{
				ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
			}
		}
	}
	if (tent)
	{
		tent->s.eventParm = DirToByte( normal );
		tent->s.weapon = ent->s.weapon;
		VectorCopy( muzzle, tent->s.origin2 );
		SnapVector(tent->s.origin2);
	}
}

//---------------------------------------------------------
void WP_FireIMOD ( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	int			i = 0;
	int			hits = 0;
	int			unlinked = 0;
	gentity_t	*unlinkedEntities[MAX_RAIL_HITS];

	trace_t		tr;
	vec3_t		end, d_dir;
	gentity_t	*tent = 0;
	gentity_t	*traceEnt = NULL;
	qboolean	render_impact;

	if ( modfn.AdjustWeaponConstant( WC_USE_IMOD_RIFLE, 0 ) ) {
		WP_FireCompressionRifle( ent, qtrue, qtrue );
		return;
	}

	VectorMA (muzzle, MAXRANGE_IMOD, forward, end);

	// trace only against the solids, so the railgun will go through people
	do
	{
		WEAPON_TRACE (&tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );
		traceEnt = &g_entities[ tr.entityNum ];
		if ( tr.surfaceFlags & SURF_NOIMPACT )
		{	// If the beam hit the skybox, etc. it would look foolish to add in an explosion
			render_impact = qfalse;
		} else
		{
			render_impact = qtrue;
		}
		if ( traceEnt->takedamage )
		{
			if( LogAccuracyHit( traceEnt, ent ) )
			{
				hits++;
			}
			//For knockback - send them up in air
			VectorCopy(forward, d_dir);
			if(d_dir[2] < 0.30f)
			{
				d_dir[2] = 0.30f;
			}
			VectorNormalize(d_dir);
			// do the damage and send an impact effect
			IMODHit(alt_fire, traceEnt, ent, d_dir, tr.endpos, tr.plane.normal, qtrue);

			// give the shooter a reward sound if they have made two sniper hits in a row
			// check for "impressive" reward sound
			// Note that hitting two people in a line warrants an "impressive" as well...
			if (traceEnt->client)
			{
				if (((g_gametype.integer >= GT_TEAM) && (ent->client->ps.persistant[PERS_TEAM] != traceEnt->client->ps.persistant[PERS_TEAM]))
					|| (g_gametype.integer < GT_TEAM))
				{
					if (alt_fire && modfn.AdjustWeaponConstant( WC_IMPRESSIVE_REWARDS, 1 ))
					{
						ent->client->accurateCount++;
						if ( ent->client->accurateCount >= 2 )
						{
							ent->client->accurateCount -= 2;
							ent->client->ps.persistant[PERS_REWARD_COUNT]++;
							ent->client->ps.persistant[PERS_REWARD] = REWARD_IMPRESSIVE;
							ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
							// add the sprite over the player's head
							ent->client->ps.eFlags &= ~EF_AWARD_MASK;
							ent->client->ps.eFlags |= EF_AWARD_IMPRESSIVE;
							ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
						}
					}
				}
			}
		}
		else
		{	// send an impact effect
			IMODHit(alt_fire, traceEnt, ent, d_dir, tr.endpos, tr.plane.normal, render_impact);
		}
		if ( tr.contents & CONTENTS_SOLID )
		{
			modfn.AddWeaponEffect( WE_IMOD_END, ent, &tr );
			break;		// we hit something solid enough to stop the beam
		}
		// unlink this entity, so the next trace will go past it
		trap_UnlinkEntity( traceEnt );
		unlinkedEntities[unlinked] = traceEnt;
		unlinked++;
	} while ( unlinked < MAX_RAIL_HITS );

	// link back in any entities we unlinked
	for ( i = 0 ; i < unlinked ; i++ )
	{
		trap_LinkEntity( unlinkedEntities[i] );
	}

	// With endcapping on, the beam is actually longer than what the length parm in FX_AddLine for this
	//		effect specifies..so move it out so it doesn't clip into the gun so much
	if ( alt_fire )
		VectorMA (muzzle, 10, forward, muzzle);
	else
		VectorMA (muzzle, 2, forward, muzzle);

	// Create the events that will add in the necessary effects
	if ( alt_fire )
	{
		tent = G_TempEntity( tr.endpos, EV_IMOD_ALTFIRE );
	}
	else
	{
		tent = G_TempEntity( tr.endpos, EV_IMOD );
	}

	// Stash origins, etc. so the effect can have access to them
	VectorCopy( muzzle, tent->s.origin2 );
	SnapVector( tent->s.origin2 );			// save net bandwidth
	if ( render_impact )
	{
		tent->s.eventParm = DirToByte( tr.plane.normal );
		tent->s.weapon = ent->s.weapon;
	}

	G_LogWeaponFire(ent->s.number, WP_IMOD);
}

/*
----------------------------------------------
	SCAVENGER
----------------------------------------------
*/

#define SCAV_SPREAD			0.5
#define SCAV_VELOCITY		modfn.AdjustWeaponConstant( WC_SCAV_VELOCITY, 1500 )
#define SCAV_SIZE			3

#define SCAV_ALT_VELOCITY	1000
#define SCAV_ALT_UP_VELOCITY	100
#define SCAV_ALT_SIZE		6

//---------------------------------------------------------
void FireScavengerBullet( gentity_t *ent, vec3_t start, vec3_t dir )
//---------------------------------------------------------
{
	gentity_t	*bolt;

	bolt = G_Spawn();

	bolt->classname = "scav_proj";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_FreeEntity;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_SCAVENGER_RIFLE;
	bolt->r.ownerNum = ent->s.number;
	bolt->parent = ent;

//fixme  - remove
	{
		// Flags effect as being the full beefy version for the player
		bolt->count = 0;
	}

	bolt->damage = SCAV_DAMAGE*DMG_VAR*s_quadFactor;
	bolt->splashDamage = 0;
	bolt->splashRadius = 0;
	bolt->methodOfDeath = MOD_SCAVENGER;
	bolt->clipmask = MASK_SHOT;

	// Set the size of the missile up
	VectorSet(bolt->r.maxs, SCAV_SIZE, SCAV_SIZE, SCAV_SIZE);
	VectorSet(bolt->r.mins, -SCAV_SIZE, -SCAV_SIZE, -SCAV_SIZE);

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - 10;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	SnapVector( bolt->s.pos.trBase );			// save net bandwidth
	VectorScale( dir, SCAV_VELOCITY, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy( start, bolt->r.currentOrigin);

	modfn.PostFireProjectile( bolt );
}

// Alt-fire...
//---------------------------------------------------------
void FireScavengerGrenade( gentity_t *ent, vec3_t start, vec3_t dir )
//---------------------------------------------------------
{
	gentity_t	*grenade;

	grenade = G_Spawn();

	grenade->classname = "scav_grenade";
	grenade->nextthink = level.time + 9000;
	grenade->think = G_FreeEntity;
	grenade->s.eType = ET_ALT_MISSILE;
	grenade->s.weapon = WP_SCAVENGER_RIFLE;
	grenade->r.ownerNum = ent->s.number;
	grenade->parent = ent;
	grenade->damage = SCAV_ALT_DAMAGE*DMG_VAR*s_quadFactor;
	grenade->splashDamage = SCAV_ALT_SPLASH_DAM*s_quadFactor;
	grenade->splashRadius = SCAV_ALT_SPLASH_RAD;//*s_quadFactor;
	grenade->methodOfDeath = MOD_SCAVENGER_ALT;
	grenade->splashMethodOfDeath = MOD_SCAVENGER_ALT_SPLASH;
	grenade->clipmask = MASK_SHOT;
	grenade->s.eFlags |= EF_ALT_FIRING;

	// Set the size of the missile up
	VectorSet(grenade->r.maxs, SCAV_ALT_SIZE, SCAV_ALT_SIZE, SCAV_ALT_SIZE);
	VectorSet(grenade->r.mins, -SCAV_ALT_SIZE, -SCAV_ALT_SIZE, -SCAV_ALT_SIZE);

	grenade->s.pos.trType = TR_GRAVITY;
	grenade->s.pos.trTime = level.time;		// move a bit on the very first frame

	VectorCopy( start, grenade->s.pos.trBase );
	SnapVector( grenade->s.pos.trBase );			// save net bandwidth
	VectorScale( dir, PREDICTABLE_RANDOM( ent - g_entities ) * 100 + SCAV_ALT_VELOCITY, grenade->s.pos.trDelta );

	// Add a tad of upwards velocity to the shot.
	grenade->s.pos.trDelta[2] += SCAV_ALT_UP_VELOCITY;

	// Add in half the player's velocity to the shot.
	VectorMA(grenade->s.pos.trDelta, 0.5, ent->s.pos.trDelta, grenade->s.pos.trDelta);

	SnapVector( grenade->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, grenade->r.currentOrigin);
	VectorCopy (start, grenade->pos1);
	VectorCopy (start, grenade->s.origin2);
	SnapVector( grenade->s.origin2 );			// save net bandwidth

	modfn.PostFireProjectile( grenade );
}

//---------------------------------------------------------
void WP_FireScavenger( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	vec3_t		dir, angles, temp_ang, temp_org;
	vec3_t		start;
	float		offset;

	VectorCopy( forward, dir );
	VectorCopy( muzzle, start );

	if ( alt_fire )
	{
		FireScavengerGrenade( ent, start, dir );
	}
	else
	{
		vectoangles( dir, angles );
		VectorSet( temp_ang, angles[0] + (PREDICTABLE_CRANDOM( ent - g_entities ) * SCAV_SPREAD),
				angles[1] + (PREDICTABLE_CRANDOM( ent - g_entities ) * SCAV_SPREAD), angles[2] );
		AngleVectors( temp_ang, dir, NULL, NULL );

		// try to make the shot alternate between barrels
		offset = PREDICTABLE_IRANDOM(ent - g_entities, 0, 1) * 2 + 1;

		// FIXME:  These offsets really don't work like they should
		VectorMA( start, offset, right, temp_org );
		VectorMA( temp_org, offset, up, temp_org );
		FireScavengerBullet( ent, temp_org, dir );
	}

	G_LogWeaponFire(ent->s.number, WP_SCAVENGER_RIFLE);
}

/*
----------------------------------------------
	STASIS
----------------------------------------------
*/

#define STASIS_VELOCITY		modfn.AdjustWeaponConstant( WC_STASIS_VELOCITY, 1100 )

// #define STASIS_SPREAD		5.0		//2.5	//1.8	// Keep the spread relatively small so that you can get multiple projectile impacts when a badie is close
#define STASIS_SPREAD		0.085f		// Roughly equivalent to sin(5 deg).

#define STASIS_MAIN_MISSILE_BIG		4
#define STASIS_MAIN_MISSILE_SMALL	2

#define STASIS_ALT_RIGHT_OFS	0.10
#define STASIS_ALT_UP_OFS		0.02
#define STASIS_ALT_MUZZLE_OFS	1

#define MAXRANGE_ALT_STASIS		4096

//---------------------------------------------------------
void FireStasisMissile( gentity_t *ent, vec3_t origin, vec3_t dir, int size )
//---------------------------------------------------------
{
	gentity_t	*bolt;
	int			boltsize;

	bolt = G_Spawn();
	bolt->classname = "stasis_projectile";


	bolt->nextthink = level.time + 10000;
	bolt->think = G_FreeEntity;

	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_STASIS;
	bolt->r.ownerNum = ent->s.number;
	bolt->parent = ent;
	bolt->damage = size * STASIS_DAMAGE*DMG_VAR*s_quadFactor;
	bolt->splashDamage = 0;
	bolt->splashRadius = 0;
	bolt->methodOfDeath = MOD_STASIS;
	bolt->clipmask = MASK_SHOT;

	// Set the size of the missile up
	boltsize=3*size;
	VectorSet(bolt->r.maxs, boltsize, boltsize, boltsize);
	boltsize=-boltsize;
	VectorSet(bolt->r.mins, boltsize, boltsize, boltsize);

//	bolt->trigger_formation = qfalse;		// don't draw tail on first frame

	// There are going to be a couple of different sized projectiles, so store 'em here
	bolt->count = size;
	// kef -- need to keep the size in something that'll reach the cgame side
	bolt->s.time2 = size;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;
	VectorCopy( origin, bolt->s.pos.trBase );
	SnapVector( bolt->s.pos.trBase );			// save net bandwidth

	VectorScale( dir, STASIS_VELOCITY + ( 50 * size ), bolt->s.pos.trDelta );

	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (origin, bolt->r.currentOrigin);
	// Used by trails
	VectorCopy (origin, bolt->pos1 );
	VectorCopy (origin, bolt->pos2 );
	// kef -- need to keep the origin in something that'll reach the cgame side
	VectorCopy(origin, bolt->s.angles2);
	SnapVector( bolt->s.angles2 );			// save net bandwidth

	modfn.PostFireProjectile( bolt );
}

//---------------------------------------------------------
void WP_FireStasisMain( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t	dir;

	// Fire forward
	FireStasisMissile( ent, muzzle, forward, STASIS_MAIN_MISSILE_BIG );

	// Fire slightly to the left
	VectorMA(forward, STASIS_SPREAD, right, dir);
	VectorNormalize(dir);
	FireStasisMissile( ent, muzzle, dir, STASIS_MAIN_MISSILE_SMALL );

	// Fire slightly to the right.
	VectorMA(forward, -STASIS_SPREAD, right, dir);
	VectorNormalize(dir);
	FireStasisMissile( ent, muzzle, dir, STASIS_MAIN_MISSILE_SMALL );
}


void DoSmallStasisBeam(gentity_t *ent, vec3_t start, vec3_t dir)
{
	vec3_t end;
	trace_t tr;
	gentity_t *traceEnt;

	VectorMA(start, MAXRANGE_ALT_STASIS, dir, end);
	WEAPON_TRACE(&tr, start, NULL, NULL, end, ent->s.number, MASK_SHOT);

	traceEnt = &g_entities[ tr.entityNum ];

	if ( traceEnt->takedamage)
	{
		//For knockback - send them up in air
		if ( dir[2] < 0.20f )
		{
			dir[2] = 0.20f;
		}

		VectorNormalize( dir );

		G_Damage(traceEnt, ent, ent, dir, tr.endpos, STASIS_ALT_DAMAGE2*DMG_VAR*s_quadFactor, DAMAGE_ARMOR_PIERCING, MOD_STASIS_ALT );
		// log hit
		if (ent->client)
		{
			ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
		}
	}
}


//---------------------------------------------------------
void WP_FireStasisAlt( gentity_t *ent )
//---------------------------------------------------------
{
	trace_t		tr;
	vec3_t		end, d_dir, d_right, d_up={0,0,1}, start;
	gentity_t	*tent;
	gentity_t	*traceEnt;

	// Find the main impact point
	VectorMA (muzzle, MAXRANGE_ALT_STASIS, forward, end);
	WEAPON_TRACE ( &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);

	traceEnt = &g_entities[ tr.entityNum ];

	// Why am I doing this when I've got a right and up already?  Well, because this is how it is calc'ed on the client side.
	CrossProduct(forward, d_up, d_right);
	VectorNormalize(d_right);				// "right" is scaled by the sin of the angle between fwd & up...  Ditch that.
	CrossProduct(d_right, forward, d_up);	// Change the "fake up" (0,0,1) to a "real up" (perpendicular to the forward vector).
	// VectorNormalize(d_up);				// If I cared about how the vertical variance looked when pointing up or down, I'd normalize this.

	// Fire a shot up and to the right.
	VectorMA(forward, STASIS_ALT_RIGHT_OFS, d_right, d_dir);
	VectorMA(d_dir, STASIS_ALT_UP_OFS, d_up, d_dir);
	VectorMA(muzzle, STASIS_ALT_MUZZLE_OFS, d_right, start);
	DoSmallStasisBeam(ent, start, d_dir);

	// Fire a shot up and to the left.
	VectorMA(forward, -STASIS_ALT_RIGHT_OFS, d_right, d_dir);
	VectorMA(d_dir, STASIS_ALT_UP_OFS, d_up, d_dir);
	VectorMA(muzzle, -STASIS_ALT_MUZZLE_OFS, d_right, start);
	DoSmallStasisBeam(ent, start, d_dir);

	// Fire a shot a bit down and to the right.
	VectorMA(forward, 2.0*STASIS_ALT_RIGHT_OFS, d_right, d_dir);
	VectorMA(d_dir, -0.5*STASIS_ALT_UP_OFS, d_up, d_dir);
	VectorMA(muzzle, 2.0*STASIS_ALT_MUZZLE_OFS, d_right, start);
	DoSmallStasisBeam(ent, start, d_dir);

	// Fire a shot up and to the left.
	VectorMA(forward, -2.0*STASIS_ALT_RIGHT_OFS, d_right, d_dir);
	VectorMA(d_dir, -0.5*STASIS_ALT_UP_OFS, d_up, d_dir);
	VectorMA(muzzle, -2.0*STASIS_ALT_MUZZLE_OFS, d_right, start);
	DoSmallStasisBeam(ent, start, d_dir);

	// Main beam
	tent = G_TempEntity( tr.endpos, EV_STASIS );
	tent->s.angles[YAW] = (int)(ent->client->ps.viewangles[YAW]);

	if ( traceEnt->takedamage)
	{
		//For knockback - send them up in air
		VectorCopy( forward, d_dir );
		if ( d_dir[2] < 0.30f )
		{
			d_dir[2] = 0.30f;
		}

		VectorNormalize( d_dir );

		G_Damage( traceEnt, ent, ent, d_dir, tr.endpos, STASIS_ALT_DAMAGE*DMG_VAR*s_quadFactor,
					DAMAGE_ARMOR_PIERCING, MOD_STASIS_ALT );
		// log hit
		if (ent->client)
		{
			ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
		}
	}

	// Stash origins, etc. so that the effects can have access to them
	VectorCopy( muzzle, tent->s.origin2 );
	SnapVector(tent->s.origin2);
}

//---------------------------------------------------------
void WP_FireStasis( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	// This was moved out of the FireWeapon switch statement below to keep things more consistent
	if ( alt_fire )
	{
		WP_FireStasisAlt( ent );
	}
	else
	{
		WP_FireStasisMain( ent );
	}

	G_LogWeaponFire(ent->s.number, WP_STASIS);
}

/*
----------------------------------------------
	GRENADE LAUNCHER
----------------------------------------------
*/

#define GRENADE_VELOCITY		1000
#define GRENADE_TIME			2000
#define GRENADE_SIZE			4
#define GRENADE_ALT_VELOCITY	1200
#define GRENADE_ALT_TIME		2500

#define SHRAPNEL_DAMAGE			30
#define SHRAPNEL_DISTANCE		4096
#define SHRAPNEL_BITS			6
#define SHRAPNEL_RANDOM			3
#define SHRAPNEL_SPREAD			0.75

//---------------------------------------------------------
void grenadeExplode( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t		pos;
	gentity_t	*tent;

	VectorSet( pos, ent->r.currentOrigin[0], ent->r.currentOrigin[1], ent->r.currentOrigin[2] + 8 );

	tent = G_TempEntity( pos, EV_GRENADE_EXPLODE );

	// add mod effects
	modfn.AddWeaponEffect( WE_GRENADE_PRIMARY, ent, NULL );

	// splash damage (doesn't apply to person directly hit)
	if ( ent->splashDamage ) {
		G_RadiusDamage( pos, ent->parent, ent->splashDamage, ent->splashRadius,
			NULL, 0, ent->splashMethodOfDeath );
	}
	G_FreeEntity( ent );
}

//---------------------------------------------------------
void grenadeSpewShrapnel( gentity_t *ent )
//---------------------------------------------------------
{
	gentity_t	*tent = NULL;

	tent = G_TempEntity( ent->r.currentOrigin, EV_GRENADE_SHRAPNEL_EXPLODE );
	tent->s.eventParm = DirToByte(ent->pos1);

	// add mod effects
	modfn.AddWeaponEffect( WE_GRENADE_ALT, ent, NULL );

	// just do radius dmg for altfire
	if( G_RadiusDamage( ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius,
		ent, 0, ent->splashMethodOfDeath ) )
	{
		// log hit
		if (ent->client)
		{
			ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
		}
	}

	G_FreeEntity(ent);
}

//---------------------------------------------------------
void WP_FireGrenade( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	gentity_t	*grenade;
	gentity_t	*tripwire = NULL;
	vec3_t		dir, start;
	int			tripcount = 0;
	int			foundTripWires[MAX_GENTITIES] = {ENTITYNUM_NONE};
	int			tripcount_org;
	int			lowestTimeStamp;
	int			removeMe;
	int			i;

	VectorCopy( forward, dir );
	VectorCopy( muzzle, start );

	grenade = G_Spawn();

	// kef -- make sure count is 0 so it won't get its bounciness removed like the tetrion projectile
	grenade->count = 0;

	if ( alt_fire )
	{
		if ( ent->client && modfn.AdjustWeaponConstant( WC_USE_TRIPMINES, 0 ) )
		{
			//limit to 10 placed at any one time
			//see how many there are now
			while ( (tripwire = G_Find( tripwire, FOFS(classname), "tripwire" )) != NULL )
			{
				if ( tripwire->parent != ent )
				{
					continue;
				}
				foundTripWires[tripcount++] = tripwire->s.number;
			}
			//now remove first ones we find until there are only 9 left
			tripwire = NULL;
			tripcount_org = tripcount;
			lowestTimeStamp = level.time;
			while ( tripcount > 9 )
			{
				removeMe = -1;
				for ( i = 0; i < tripcount_org; i++ )
				{
					if ( foundTripWires[i] == ENTITYNUM_NONE )
					{
						continue;
					}
					tripwire = &g_entities[foundTripWires[i]];
					if ( tripwire && tripwire->timestamp < lowestTimeStamp )
					{
						removeMe = i;
						lowestTimeStamp = tripwire->timestamp;
					}
				}
				if ( removeMe != -1 )
				{
					//remove it... or blow it?
					if ( &g_entities[foundTripWires[removeMe]] == NULL )
					{
						break;
					}
					else
					{
						G_FreeEntity( &g_entities[foundTripWires[removeMe]] );
					}
					foundTripWires[removeMe] = ENTITYNUM_NONE;
					tripcount--;
				}
				else
				{
					break;
				}
			}
			//now make the new one
			grenade->classname = "tripwire";
			grenade->splashDamage = GRENADE_SPLASH_DAM*s_quadFactor;
			grenade->splashRadius = GRENADE_SPLASH_RAD;
			grenade->s.pos.trType = TR_LINEAR;
			grenade->nextthink = level.time + 1000; // How long 'til she blows
			grenade->count = 1;//tell it it's a tripwire for when it sticks
			grenade->timestamp = level.time;//remember when we placed it
			grenade->s.otherEntityNum2 = ent->client->sess.sessionTeam;
		}
		else
		{
			grenade->classname = "grenade_alt_projectile";
			grenade->splashDamage = GRENADE_SPLASH_DAM*s_quadFactor;
			grenade->splashRadius = GRENADE_SPLASH_RAD;//*s_quadFactor;
			grenade->s.pos.trType = TR_GRAVITY;
			grenade->nextthink = level.time + GRENADE_ALT_TIME; // How long 'til she blows
		}
		grenade->think = grenadeSpewShrapnel;
		grenade->s.eFlags |= EF_MISSILE_STICK;
		VectorScale( dir, 1000/*GRENADE_ALT_VELOCITY*/, grenade->s.pos.trDelta );

		grenade->damage = GRENADE_ALT_DAMAGE*DMG_VAR*s_quadFactor;
		grenade->methodOfDeath = MOD_GRENADE_ALT;
		grenade->splashMethodOfDeath = MOD_GRENADE_ALT_SPLASH;
		grenade->s.eType = ET_ALT_MISSILE;
	}
	else
	{
		grenade->classname = "grenade_projectile";
		grenade->nextthink = level.time + GRENADE_TIME; // How long 'til she blows
		grenade->think = grenadeExplode;
		grenade->s.eFlags |= EF_BOUNCE_HALF;
		VectorScale( dir, GRENADE_VELOCITY, grenade->s.pos.trDelta );
		grenade->s.pos.trType = TR_GRAVITY;

		grenade->damage = GRENADE_DAMAGE*DMG_VAR*s_quadFactor;
		grenade->splashDamage = GRENADE_SPLASH_DAM*s_quadFactor;
		grenade->splashRadius = GRENADE_SPLASH_RAD;//*s_quadFactor;
		grenade->methodOfDeath = MOD_GRENADE;
		grenade->splashMethodOfDeath = MOD_GRENADE_SPLASH;
		grenade->s.eType = ET_MISSILE;
	}

	grenade->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	grenade->s.weapon = WP_GRENADE_LAUNCHER;
	grenade->r.ownerNum = ent->s.number;
	grenade->parent = ent;

	VectorSet(grenade->r.mins, -GRENADE_SIZE, -GRENADE_SIZE, -GRENADE_SIZE);
	VectorSet(grenade->r.maxs, GRENADE_SIZE, GRENADE_SIZE, GRENADE_SIZE);

	grenade->clipmask = MASK_SHOT;

	grenade->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, grenade->s.pos.trBase );
	SnapVector( grenade->s.pos.trBase );			// save net bandwidth

	SnapVector( grenade->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, grenade->r.currentOrigin);

	VectorCopy( start, grenade->pos2 );

	G_LogWeaponFire(ent->s.number, WP_GRENADE_LAUNCHER);

	modfn.PostFireProjectile( grenade );
}


/*
----------------------------------------------
	TETRION
----------------------------------------------
*/

#define TETRION_SPREAD			275
#define NUM_TETRION_SHOTS		3

#define TETRION_ALT_VELOCITY	1500
#define TETRION_ALT_SIZE		6

#define MAXRANGE_TETRION		4096

typedef struct tetrionHit_s
{
	gentity_t	*ent;
	vec3_t		pos;
} tetrionHit_t;

// provide the center of the circle, a normal out from it (normalized, please), and the radius.
//out will then become a random position on the radius of the circle.
void fxRandCircumferencePos(vec3_t center, vec3_t normal, float radius, vec3_t out)
{
	float		rnd = flrandom(0, 2*M_PI);
	float		s = sin(rnd);
	float		c = cos(rnd);
	vec3_t		vTemp, radialX, radialY;

	vTemp[0]=0;
	vTemp[1]=0;
	vTemp[2]=-1;
	CrossProduct(normal, vTemp, radialX);
	CrossProduct(normal, radialX, radialY);
	VectorScale(radialX, radius, radialX);
	VectorScale(radialY, radius, radialY);
	VectorMA(center, s, radialX, out);
	VectorMA(out, c, radialY, out);
}
#define NUM_TETRION_BULLETS		3
//---------------------------------------------------------
void FireTetrionBullet( gentity_t *ent, vec3_t start, vec3_t dir )
//---------------------------------------------------------
{
	gentity_t	*tent = NULL;
	trace_t		tr;
	vec3_t		end, up = {0,0,1}, right;		// , zero = {0,0,0};
	int			i = 0, j = 0;
	qboolean		bHitAlready = qfalse;
	int				numHits = 0;
	tetrionHit_t	hitEnts[NUM_TETRION_BULLETS];


	vec3_t	new_start, radial, start2, spreadFwd;
	float	firingRadius = 6, minDeviation = 0.95, maxDeviation = 1.1;

	for (i = 0; i < NUM_TETRION_BULLETS; i++)
	{
		hitEnts[i].ent = NULL;
	}
	// create our message-entity with the firing position for an origin
	tent = G_TempEntity(muzzle, EV_TETRION);
	// stash our firing direction in the message also
	VectorCopy(forward, tent->s.angles2);
	VectorShort(tent->s.angles2);

	// now do the damage. we're going to fake three separate bullets here by doing three
	//traces and splitting two or three hits worth of dmg between whatever the traces hit.
	CrossProduct(forward, up, right);
	CrossProduct(right, forward, up); // get a "real" up

	for (i = 0; i < NUM_TETRION_BULLETS; i++)
	{
		// determine new firing position
		fxRandCircumferencePos(start, forward, firingRadius, new_start);
		VectorSubtract(new_start, start, radial);
		VectorMA(start, 10, forward, start2);
		VectorMA(start2, flrandom(minDeviation, maxDeviation), radial, start2);
		VectorSubtract(start2, new_start, spreadFwd);
		VectorNormalize(spreadFwd);
		// determine new end position for this bullet. give the endpoint some spread, too.
		VectorMA(new_start, MAXRANGE_TETRION, spreadFwd, end);
		WEAPON_TRACE ( &tr, new_start, NULL, NULL, end, ent->s.number, MASK_SHOT);
		if ((tr.entityNum < MAX_GENTITIES) && (tr.entityNum != ENTITYNUM_WORLD))
		{
			bHitAlready = qfalse;
			for (j = 0; j < numHits; j++)
			{
				if (hitEnts[j].ent == &g_entities[ tr.entityNum ])
				{
					// already hit this ent
					bHitAlready = qtrue;
					break;
				}
			}
			if (!bHitAlready)
			{
				hitEnts[numHits].ent = &g_entities[ tr.entityNum ];
				VectorCopy(tr.endpos, hitEnts[numHits].pos);
				numHits++;
			}
		}
	}
	if (numHits)
	{
		// determine damage (2 or 3 bullets)
		float totalDmg = (float)(irandom(2,3)*TETRION_DAMAGE*s_quadFactor), dmgPerHit = 0;

		dmgPerHit = (int)(totalDmg/(float)numHits);
		for (i = 0; i < numHits; i++)
		{
			if (hitEnts[i].ent->takedamage)
			{
				G_Damage( hitEnts[i].ent, ent, ent, forward, hitEnts[i].pos, dmgPerHit,
							DAMAGE_ARMOR_PIERCING|DAMAGE_NO_KNOCKBACK, MOD_TETRION );
			}
			// log hit
			if (ent->client)
			{
				ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
			}
		}
	}
}

//---------------------------------------------------------
void FireTetrionProjectile( gentity_t *ent, vec3_t start, vec3_t dir )
//---------------------------------------------------------
{// Projectile that bounces off surfaces but does not have gravity
	gentity_t	*bolt;

	bolt = G_Spawn();
	bolt->classname = "tetrion_projectile";
	bolt->nextthink = level.time + 2000;
	bolt->think = G_FreeEntity;

	bolt->s.eType = ET_ALT_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.eFlags |= EF_BOUNCE;
	bolt->count = random() > 0.75f ? 3 : 4;		// Number of bounces

	VectorSet(bolt->r.mins, -TETRION_ALT_SIZE, -TETRION_ALT_SIZE, -TETRION_ALT_SIZE);
	VectorSet(bolt->r.maxs, TETRION_ALT_SIZE, TETRION_ALT_SIZE, TETRION_ALT_SIZE);

	bolt->s.weapon = WP_TETRION_DISRUPTOR;
	bolt->r.ownerNum = ent->s.number;
	bolt->parent = ent;
	bolt->damage = TETRION_ALT_DAMAGE*DMG_VAR*s_quadFactor;
	bolt->splashDamage = 0;
	bolt->splashRadius = 0;
	bolt->methodOfDeath = MOD_TETRION_ALT;
	bolt->clipmask = MASK_SHOT;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	SnapVector( bolt->s.pos.trBase );			// save net bandwidth
	VectorScale( dir, TETRION_ALT_VELOCITY, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	modfn.PostFireProjectile( bolt );
}

//---------------------------------------------------------
void WP_FireTetrionDisruptor( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	vec3_t	dir;
	vec3_t	start;

	VectorCopy( forward, dir );
	VectorCopy( muzzle, start );
	VectorNormalize (dir);

	if ( alt_fire )
	{
		FireTetrionProjectile( ent, start, dir );
	}
	else
	{
		FireTetrionBullet( ent, start, dir );
	}

	G_LogWeaponFire(ent->s.number, WP_TETRION_DISRUPTOR);
}


/*
----------------------------------------------
	QUANTUM BURST
----------------------------------------------
*/

#define QUANTUM_VELOCITY	1100
#define QUANTUM_SIZE		3

#define QUANTUM_ALT_VELOCITY	550
#define QUANTUM_ALT_THINK_TIME	300
#define QUANTUM_ALT_SEARCH_TIME	100
#define QUANTUM_ALT_SEARCH_DIST	4096

//---------------------------------------------------------
void FireQuantumBurst( gentity_t *ent, vec3_t start, vec3_t dir )
//---------------------------------------------------------
{
	gentity_t	*bolt;

	bolt = G_Spawn();
	bolt->classname = "quantum_projectile";

	bolt->nextthink = level.time + 6000;
	bolt->think = G_FreeEntity;

	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_QUANTUM_BURST;
	bolt->r.ownerNum = ent->s.number;
	bolt->parent = ent;

	bolt->damage = QUANTUM_DAMAGE*DMG_VAR*s_quadFactor;
	bolt->splashDamage = QUANTUM_SPLASH_DAM*s_quadFactor;
	bolt->splashRadius = QUANTUM_SPLASH_RAD;//*s_quadFactor;

	bolt->methodOfDeath = MOD_QUANTUM;
	bolt->splashMethodOfDeath = MOD_QUANTUM_SPLASH;
	bolt->clipmask = MASK_SHOT;

	VectorSet(bolt->r.mins, -QUANTUM_SIZE, -QUANTUM_SIZE, -QUANTUM_SIZE);
	VectorSet(bolt->r.maxs, QUANTUM_SIZE, QUANTUM_SIZE, QUANTUM_SIZE);

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	SnapVector( bolt->s.pos.trBase );			// save net bandwidth

	VectorScale( dir, QUANTUM_VELOCITY, bolt->s.pos.trDelta );

	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);
	VectorCopy (start, bolt->pos1);

	modfn.PostFireProjectile( bolt );
}


qboolean SearchTarget(gentity_t *ent, vec3_t start, vec3_t end)
{
	trace_t tr;
	gentity_t *traceEnt;
	vec3_t fwd;

	WEAPON_TRACE (&tr, start, NULL, NULL, end, ent->s.number, MASK_SHOT );
	traceEnt = &g_entities[ tr.entityNum ];

	// Don't find teleporting borg in Assimilation mode
	if ( traceEnt->client
				&& traceEnt->client->sess.sessionClass == PC_BORG
				&& traceEnt->s.eFlags == EF_NODRAW )
	{
		return qfalse;
	}

	if (traceEnt->takedamage && traceEnt->client && !OnSameTeam(traceEnt, &g_entities[ent->r.ownerNum]))
	{
		ent->target_ent = traceEnt;
		VectorSubtract(ent->target_ent->r.currentOrigin, ent->r.currentOrigin, fwd);
		VectorNormalize(fwd);
		VectorScale(fwd, QUANTUM_ALT_VELOCITY, ent->s.pos.trDelta);
		VectorCopy(fwd, ent->movedir);
		SnapVector(ent->s.pos.trDelta);			// save net bandwidth
		VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
		ent->s.pos.trTime = level.time;
		ent->nextthink = level.time + QUANTUM_ALT_THINK_TIME;
		return qtrue;
	}
	return qfalse;
}


void WP_QuantumAltThink(gentity_t *ent)
{
	vec3_t start, newdir, targetdir, up={0,0,1}, right, search;
	float dot, dot2;

	ent->health--;
	if (ent->health<=0)
	{
		G_FreeEntity(ent);
		return;
	}

	if (ent->target_ent)
	{	// Already have a target, start homing.
		if (ent->health <= 0 || !ent->inuse ||
				(ent->target_ent->client
					&& ent->target_ent->client->sess.sessionClass == PC_BORG
					&& ent->target_ent->s.eFlags == EF_NODRAW))
		{	// No longer target this
			ent->target_ent = NULL;
			ent->nextthink = level.time + 1000;
			ent->health -= 5;
			return;
		}
		VectorSubtract(ent->target_ent->r.currentOrigin, ent->r.currentOrigin, targetdir);
		VectorNormalize(targetdir);

		// Now the rocket can't do a 180 in space, so we'll limit the turn to about 45 degrees.
		dot = DotProduct(targetdir, ent->movedir);
		// a dot of 1.0 means right-on-target.
		if (dot < 0.0)
		{	// Go in the direction opposite, start a 180.
			CrossProduct(ent->movedir, up, right);
			dot2 = DotProduct(targetdir, right);
			if (dot2 > 0)
			{	// Turn 45 degrees right.
				VectorAdd(ent->movedir, right, newdir);
			}
			else
			{	// Turn 45 degrees left.
				VectorSubtract(ent->movedir, right, newdir);
			}
			// Yeah we've adjusted horizontally, but let's split the difference vertically, so we kinda try to move towards it.
			newdir[2] = (targetdir[2] + ent->movedir[2]) * 0.5;
			VectorNormalize(newdir);
		}
		else if (dot < 0.7)
		{	// Need about one correcting turn.  Generate by meeting the target direction "halfway".
			// Note, this is less than a 45 degree turn, but it is sufficient.  We do this because the rocket may have to go UP.
			VectorAdd(ent->movedir, targetdir, newdir);
			VectorNormalize(newdir);
		}
		else
		{	// else adjust to right on target.
			VectorCopy(targetdir, newdir);
		}

		VectorScale(newdir, QUANTUM_ALT_VELOCITY, ent->s.pos.trDelta);
		VectorCopy(newdir, ent->movedir);
		SnapVector(ent->s.pos.trDelta);			// save net bandwidth
		VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
		SnapVector(ent->s.pos.trBase);
		ent->s.pos.trTime = level.time;

		// Home at a reduced frequency.
		ent->nextthink = level.time + QUANTUM_ALT_THINK_TIME;	// Nothing at all spectacular happened, continue.
	}
	else
	{	// Search in front of the missile for targets.
		VectorCopy(ent->r.currentOrigin, start);
		CrossProduct(ent->movedir, up, right);

		// Search straight ahead.
		VectorMA(start, QUANTUM_ALT_SEARCH_DIST, ent->movedir, search);

		// Add some small randomness to the search Z height, to give a bit of variation to where we are searching.
		search[2] += flrandom(-QUANTUM_ALT_SEARCH_DIST*0.075, QUANTUM_ALT_SEARCH_DIST*0.075);

		if (SearchTarget(ent, start, search))
			return;

		// Search to the right.
		VectorMA(search, QUANTUM_ALT_SEARCH_DIST*0.1, right, search);
		if (SearchTarget(ent, start, search))
			return;

		// Search to the left.
		VectorMA(search, -QUANTUM_ALT_SEARCH_DIST*0.2, right, search);
		if (SearchTarget(ent, start, search))
			return;

		// Search at a higher rate than correction.
		ent->nextthink = level.time + QUANTUM_ALT_SEARCH_TIME;	// Nothing at all spectacular happened, continue.

	}
	return;
}

//---------------------------------------------------------
void FireQuantumBurstAlt( gentity_t *ent, vec3_t start, vec3_t dir )
//---------------------------------------------------------
{
	gentity_t	*bolt;

	bolt = G_Spawn();
	bolt->classname = "quantum_alt_projectile";

	bolt->nextthink = level.time + 100;
	bolt->think = WP_QuantumAltThink;
	bolt->health = 25;		// 10 seconds.

	bolt->s.eType = ET_ALT_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_QUANTUM_BURST;
	bolt->r.ownerNum = ent->s.number;
	bolt->parent = ent;
	bolt->s.eFlags |= EF_ALT_FIRING;

	bolt->damage = QUANTUM_ALT_DAMAGE*DMG_VAR*s_quadFactor;
	bolt->splashDamage = QUANTUM_ALT_SPLASH_DAM*s_quadFactor;
	bolt->splashRadius = QUANTUM_ALT_SPLASH_RAD;//*s_quadFactor;

	bolt->methodOfDeath = MOD_QUANTUM_ALT;
	bolt->splashMethodOfDeath = MOD_QUANTUM_ALT_SPLASH;
	bolt->clipmask = MASK_SHOT;

	VectorSet(bolt->r.mins, -QUANTUM_SIZE, -QUANTUM_SIZE, -QUANTUM_SIZE);
	VectorSet(bolt->r.maxs, QUANTUM_SIZE, QUANTUM_SIZE, QUANTUM_SIZE);

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	SnapVector(bolt->s.pos.trBase);

	VectorScale( dir, QUANTUM_ALT_VELOCITY, bolt->s.pos.trDelta );
	VectorCopy(dir, bolt->movedir);

	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	modfn.PostFireProjectile( bolt );
}

//---------------------------------------------------------
void WP_FireQuantumBurst( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	vec3_t	dir, start;

	VectorCopy( forward, dir );
	VectorCopy( muzzle, start );

	if ( alt_fire )
	{
		FireQuantumBurstAlt( ent, start, dir );
	}
	else
	{
		FireQuantumBurst( ent, start, dir );
	}

	G_LogWeaponFire(ent->s.number, WP_QUANTUM_BURST);
}


/*
----------------------------------------------
	DREADNOUGHT
----------------------------------------------
*/

#define MAXRANGE_DREADNOUGHT	2048

//---------------------------------------------------------
void WP_FireDreadnoughtBeam( gentity_t *ent )
//---------------------------------------------------------
{
	trace_t		tr;
	vec3_t		end;
	gentity_t	*traceEnt;
	vec3_t		start;
	qboolean	bHit = qfalse;

	// Trace once to the right...
	VectorMA( muzzle, DREADNOUGHT_WIDTH, right, start);
	VectorMA( start, MAXRANGE_DREADNOUGHT, forward, end );

	// Find out who we've hit
	WEAPON_TRACE( &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );
	traceEnt = &g_entities[ tr.entityNum ];

	if ( traceEnt->takedamage)
	{
		G_Damage( traceEnt, ent, ent, forward, tr.endpos, DREADNOUGHT_DAMAGE*s_quadFactor,
					DAMAGE_NOT_ARMOR_PIERCING, MOD_DREADNOUGHT);
		bHit = qtrue;
	}

	// Now trace once to the left...
	VectorMA( muzzle, -DREADNOUGHT_WIDTH, right, start);
	VectorMA( start, MAXRANGE_DREADNOUGHT, forward, end );

	// Find out who we've hit
	WEAPON_TRACE( &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );
	traceEnt = &g_entities[ tr.entityNum ];

	if ( traceEnt->takedamage)
	{
		G_Damage( traceEnt, ent, ent, forward, tr.endpos, DREADNOUGHT_DAMAGE*s_quadFactor,
					DAMAGE_NOT_ARMOR_PIERCING, MOD_DREADNOUGHT);
		bHit = qtrue;
	}
	if (bHit)
	{
		// log hit
		if (ent->client)
		{
			ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
		}
	}
}


//#define DN_SEARCH_DIST	512
//#define DN_SIDE_DIST	64
//#define DN_RAND_DEV		8

#define DN_SEARCH_DIST	modfn.AdjustWeaponConstant( WC_DN_SEARCH_DIST, 256 )
#define DN_SIDE_DIST	128
#define DN_RAND_DEV		16
#define DN_ALT_THINK_TIME	100
#define DN_ALT_SIZE		modfn.AdjustWeaponConstant( WC_DN_ALT_SIZE, 12 )
#define DN_MAX_MOVES	modfn.AdjustWeaponConstant( WC_DN_MAX_MOVES, 0 )
void DreadnoughtBurstThink(gentity_t *ent)
{
	vec3_t startpos, endpos, perp;
	trace_t tr;
	gentity_t *traceEnt, *tent;
	int source;
	vec3_t dest, mins={0,0,-1}, maxs={0,0,1};
	float dot;
	static qboolean recursion=qfalse;

	ent->nextthink2 = ent->nextthink2 + DN_ALT_THINK_TIME;
	if ( ent->nextthink2 <= level.time ) {
		ent->nextthink2 = level.time + DN_ALT_THINK_TIME;
	}
	ent->nextthink = ent->nextthink2;

	mins[0] = mins[1] = -DN_ALT_SIZE;
	maxs[0] = maxs[1] = DN_ALT_SIZE;

	VectorCopy(ent->s.origin, startpos);

	// Search in a 3-way arc in front of it.
	VectorMA(startpos, DN_SEARCH_DIST, ent->movedir, endpos);
	endpos[0] += flrandom(-DN_RAND_DEV, DN_RAND_DEV);
	endpos[1] += flrandom(-DN_RAND_DEV, DN_RAND_DEV);
	endpos[2] += flrandom(-DN_RAND_DEV*0.5, DN_RAND_DEV*0.5);

	// unlink this entity, so the next trace will go past it
	trap_UnlinkEntity(ent);

	// link back in any entities we unlinked
	if (ent->target_ent)
	{
		source = ent->target_ent->s.number;
	}
	else if (ent->r.ownerNum)
	{
		source = ent->r.ownerNum;
	}
	else
	{
		source = ent->s.number;
	}

	WEAPON_TRACE (&tr, startpos, mins, maxs, endpos, source, MASK_SHOT );
	traceEnt = &g_entities[ tr.entityNum ];
	// did we hit the holdable shield?
	if (traceEnt->classname && !Q_stricmp(traceEnt->classname, "holdable_shield"))
	{
		// Make sure you damage the surface first
		G_Damage( traceEnt, ent, &g_entities[ent->r.ownerNum], forward, tr.endpos, ent->damage,
					DAMAGE_NOT_ARMOR_PIERCING, MOD_DREADNOUGHT_ALT);

		VectorCopy(ent->s.origin, ent->s.origin2);
		VectorCopy(endpos, ent->s.origin);
		SnapVector(ent->s.origin);
		SnapVector(ent->s.origin2);

		// yes. We are done.
		ent->think = G_FreeEntity;

		tent = G_TempEntity( tr.endpos, EV_DREADNOUGHT_MISS );
		// Stash origins, etc. so that the effects can have access to them
		VectorCopy( startpos, tent->s.origin2 );
		SnapVector(tent->s.origin2);
		tent->s.eventParm = DirToByte( tr.plane.normal );

		return;
	}

	if (traceEnt->takedamage)
	{
		ent->target_ent = traceEnt;
		VectorCopy(ent->s.origin, ent->s.origin2);
		SnapVector(ent->s.origin);
		SnapVector(ent->s.origin2);
		VectorCopy(traceEnt->r.currentOrigin, ent->s.origin);
		trap_LinkEntity(ent);
		VectorNormalize(ent->movedir);
		G_Damage( traceEnt, ent, &g_entities[ent->r.ownerNum], forward, tr.endpos, ent->damage,
					DAMAGE_NOT_ARMOR_PIERCING, MOD_DREADNOUGHT_ALT);
		// log hit
		if (ent->client)
		{
			ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
		}
		return;
	}
	else
	{
		if (tr.fraction < 0.02)
		{	// Hit a wall...

			dot = DotProduct(ent->movedir, tr.plane.normal);
			if (dot < -0.6 || dot == 0.0)
			{	// Stop.
				G_FreeEntity( ent );

				tent = G_TempEntity( tr.endpos, EV_DREADNOUGHT_MISS );

				// Stash origins, etc. so that the effects can have access to them
				VectorCopy( startpos, tent->s.origin2 );
				SnapVector(tent->s.origin2);
				tent->s.eventParm = DirToByte( tr.plane.normal );

				return;
			}
			else
			{
				// Bounce off the surface just a little
				VectorMA(ent->movedir, -1.25*dot, tr.plane.normal, ent->movedir);
				VectorNormalize(ent->movedir);

				if (!recursion)
				{	// NOTE RECURSION HERE.
					int oldThink = ent->nextthink;
					recursion=qtrue;
					DreadnoughtBurstThink(ent);
					recursion=qfalse;
					ent->nextthink = oldThink;
					ent->nextthink2 = oldThink;
				}

				return;
			}

		}

		VectorCopy(tr.endpos, dest);
	}

	// Didn't hit anything forward.  Try some side vectors.

	// Get the perp vector (okay, only in 2D) to find a right or left (random)-pointing perpendicular vector to the facing.
	if (irandom(0,1))
	{	// Right vector
		perp[0] = ent->movedir[1];
		perp[1] = -ent->movedir[0];
		perp[2] = ent->movedir[2];
	}
	else
	{	// Left vector
		perp[0] = -ent->movedir[1];
		perp[1] = ent->movedir[0];
		perp[2] = ent->movedir[2];
	}

	// Search a random interval from the side arc
	VectorMA(endpos, DN_SIDE_DIST, perp, endpos);
	endpos[0] += flrandom(-DN_RAND_DEV, DN_RAND_DEV);
	endpos[1] += flrandom(-DN_RAND_DEV, DN_RAND_DEV);
	endpos[2] += flrandom(-DN_RAND_DEV*0.5, DN_RAND_DEV*0.5);

	WEAPON_TRACE (&tr, startpos, mins, maxs, endpos, source, MASK_SHOT );
	traceEnt = &g_entities[ tr.entityNum ];
	// did we hit the holdable shield?
	if (traceEnt->classname && !Q_stricmp(traceEnt->classname, "holdable_shield"))
	{
		// yes. We are done.
		VectorCopy(ent->s.origin, ent->s.origin2);
		VectorCopy(endpos, ent->s.origin);
		SnapVector(ent->s.origin);
		SnapVector(ent->s.origin2);

		ent->think = G_FreeEntity;
		tent = G_TempEntity( tr.endpos, EV_DREADNOUGHT_MISS );
		// Stash origins, etc. so that the effects can have access to them
		VectorCopy( startpos, tent->s.origin2 );
		SnapVector(tent->s.origin2);
		tent->s.eventParm = DirToByte( tr.plane.normal );

		return;
	}
	if (traceEnt->takedamage)
	{
		ent->target_ent = traceEnt;
		VectorCopy(ent->s.origin, ent->s.origin2);
		SnapVector(ent->s.origin);
		SnapVector(ent->s.origin2);
		VectorCopy(traceEnt->r.currentOrigin, ent->s.origin);
		trap_LinkEntity(ent);
		VectorNormalize(ent->movedir);
		G_Damage( traceEnt, ent, &g_entities[ent->r.ownerNum], forward, tr.endpos, ent->damage,
					DAMAGE_NOT_ARMOR_PIERCING, MOD_DREADNOUGHT_ALT);

		// log hit
		if (ent->client)
		{
			ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
		}
		// NOTE Send this as a hit effect once we have one...
		G_Sound( traceEnt, G_SoundIndex( SOUND_DIR "dreadnought/dn_althit.wav"));

		return;
	}

	// Search a random interval in the opposite direction
	VectorMA(endpos, -2.0*DN_SIDE_DIST, perp, endpos);
	endpos[0] += flrandom(-DN_RAND_DEV, DN_RAND_DEV);
	endpos[1] += flrandom(-DN_RAND_DEV, DN_RAND_DEV);
	endpos[2] += flrandom(-DN_RAND_DEV*0.5, DN_RAND_DEV*0.5);

	WEAPON_TRACE (&tr, startpos, mins, maxs, endpos, source, MASK_SHOT );
	traceEnt = &g_entities[ tr.entityNum ];
	// did we hit the holdable shield?
	if (traceEnt->classname && !Q_stricmp(traceEnt->classname, "holdable_shield"))
	{
		VectorCopy(ent->s.origin, ent->s.origin2);
		VectorCopy(endpos, ent->s.origin);
		SnapVector(ent->s.origin);
		SnapVector(ent->s.origin2);

		// yes. We are done.
		ent->think = G_FreeEntity;

		tent = G_TempEntity( tr.endpos, EV_DREADNOUGHT_MISS );
		// Stash origins, etc. so that the effects can have access to them
		VectorCopy( startpos, tent->s.origin2 );
		SnapVector(tent->s.origin2);
		tent->s.eventParm = DirToByte( tr.plane.normal );

		return;
	}
	if (traceEnt->takedamage)
	{
		ent->target_ent = traceEnt;
		VectorCopy(ent->s.origin, ent->s.origin2);
		SnapVector(ent->s.origin);
		SnapVector(ent->s.origin2);
		VectorCopy(traceEnt->r.currentOrigin, ent->s.origin);
		trap_LinkEntity(ent);
		VectorNormalize(ent->movedir);
		G_Damage( traceEnt, ent, &g_entities[ent->r.ownerNum], forward, tr.endpos, ent->damage,
					DAMAGE_NOT_ARMOR_PIERCING, MOD_DREADNOUGHT_ALT);
		// log hit
		if (ent->client)
		{
			ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
		}
		return;
	}

	// Check for end of range
	if ( ent->health > 0 ) {
		if ( !--ent->health ) {
			G_FreeEntity( ent );
			return;
		}
	}

	// We didn't find anything, so move the entity to the middle destination.
	ent->target_ent = NULL;
	VectorCopy(ent->s.origin, ent->s.origin2);
	VectorCopy(dest, ent->s.origin);
	VectorCopy(dest, ent->s.pos.trBase);
	SnapVector(ent->s.origin2);
	trap_LinkEntity(ent);

	return;
}


//---------------------------------------------------------
void WP_FireDreadnoughtBurst( gentity_t *ent )
//---------------------------------------------------------
{
	gentity_t	*bolt;
	vec3_t		dir, start;

	VectorCopy( forward, dir );
	VectorCopy( muzzle, start );

	bolt = G_Spawn();
	bolt->classname = "dn_projectile";

	bolt->nextthink = level.time + 200;
	bolt->think = DreadnoughtBurstThink;

	bolt->s.eType = ET_ALT_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_DREADNOUGHT;
	bolt->r.ownerNum = ent->s.number;
	bolt->parent = ent;
	bolt->damage = DREADNOUGHT_ALTDAMAGE*DMG_VAR*s_quadFactor;
	bolt->splashDamage = 0;
	bolt->splashRadius = 0;
	bolt->methodOfDeath = MOD_DREADNOUGHT_ALT;
	bolt->clipmask = MASK_SHOT;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;
	VectorCopy( start, bolt->s.pos.trBase );
	VectorCopy( start, bolt->s.origin);
	SnapVector(bolt->s.pos.trBase);
	SnapVector(bolt->s.origin);

	VectorCopy( forward, bolt->movedir);

	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy( start, bolt->r.currentOrigin );
	VectorCopy( start, bolt->pos1 );
	VectorCopy( start, bolt->pos2 );

	// number of think cycles before deleting projectile (0 = infinite)
	bolt->health = DN_MAX_MOVES;

	if ( !modfn.AdjustWeaponConstant( WC_WELDER_SKIP_INITIAL_THINK, 0 ) ) {
		DreadnoughtBurstThink(bolt);
	}

	if ( bolt->inuse ) {
		modfn.PostFireProjectile( bolt );
	}
}

//---------------------------------------------------------
void WP_FireDreadnought( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	// This was moved out of the FireWeapon switch statement below to keep things more consistent
	if ( alt_fire )
	{
		WP_FireDreadnoughtBurst( ent );
	}
	else
	{
		WP_FireDreadnoughtBeam( ent );
	}

	G_LogWeaponFire(ent->s.number, WP_DREADNOUGHT);
}


//======================================================================

#define BORG_PROJECTILE_SIZE	8
#define BORG_PROJ_VELOCITY		modfn.AdjustWeaponConstant( WC_BORG_PROJ_VELOCITY, 1000 )

void WP_FireBorgTaser( gentity_t *ent )
{
	trace_t		tr;
	vec3_t		end, d_dir;
	gentity_t	*tent = 0;
	gentity_t	*traceEnt = NULL;

	VectorMA (muzzle, MAXRANGE_IMOD, forward, end);

	WEAPON_TRACE (&tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );
	traceEnt = &g_entities[ tr.entityNum ];

	if ( traceEnt->takedamage )
	{
		LogAccuracyHit( traceEnt, ent );

		//For knockback - send them up in air
		VectorCopy( forward, d_dir );
		if ( d_dir[2] < 0.30f )
		{
			d_dir[2] = 0.30f;
		}
		VectorNormalize( d_dir );

		G_Damage( traceEnt, ent, ent, forward, tr.endpos, BORG_TASER_DAMAGE * s_quadFactor, 0, MOD_BORG_ALT );
	}

	VectorMA (muzzle, 2, forward, muzzle);
	tent = G_TempEntity( tr.endpos, EV_BORG_ALT_WEAPON );

	// Stash origins, etc. so the effect can have access to them
	VectorCopy( muzzle, tent->s.origin2 );
	SnapVector( tent->s.origin2 );			// save net bandwidth
	tent->s.eventParm = DirToByte( tr.plane.normal );
	tent->s.weapon = ent->s.weapon;
}


void WP_FireBorgProjectile( gentity_t *ent, vec3_t start )
//---------------------------------------------------------
{
	gentity_t	*bolt;

	bolt = G_Spawn();

	bolt->classname = "borg_proj";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_FreeEntity;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_BORG_WEAPON;
	bolt->r.ownerNum = ent->s.number;
	bolt->parent = ent;

	bolt->damage = BORG_PROJ_DAMAGE * DMG_VAR * s_quadFactor;
	bolt->splashDamage = 0;
	bolt->splashRadius = 0;
	bolt->methodOfDeath = MOD_BORG;
	bolt->clipmask = MASK_SHOT;

	// Set the size of the missile up
	VectorSet(bolt->r.maxs, BORG_PROJECTILE_SIZE, BORG_PROJECTILE_SIZE, BORG_PROJECTILE_SIZE);
	VectorScale( bolt->r.maxs, -1, bolt->r.mins );

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	SnapVector( bolt->s.pos.trBase );			// save net bandwidth
	VectorScale( forward, BORG_PROJ_VELOCITY, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy( start, bolt->r.currentOrigin);

	modfn.PostFireProjectile( bolt );
}

void WP_FireBorgWeapon( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	if ( alt_fire )
	{
		WP_FireBorgTaser( ent );
	}
	else
	{
		WP_FireBorgProjectile( ent, muzzle );
	}

	G_LogWeaponFire( ent->s.number, WP_BORG_WEAPON );
}


/*
===============
LogAccuracyHit
===============
*/
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker ) {
	if( !target->takedamage ) {
		return qfalse;
	}

	if ( target == attacker ) {
		return qfalse;
	}

	if( !target->client ) {
		return qfalse;
	}

	if( !attacker->client ) {
		return qfalse;
	}

	if( target->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return qfalse;
	}

	if ( OnSameTeam( target, attacker ) ) {
		return qfalse;
	}

	return qtrue;
}

#define MAX_FORWARD_TRACE	8192

void CorrectForwardVector(gentity_t *ent, vec3_t forward, vec3_t muzzlePoint, float projsize)
{
	trace_t		tr;
	vec3_t		end;
	vec3_t		eyepoint;
	vec3_t		mins, maxs;

	// Find the eyepoint.
	VectorCopy(ent->client->ps.origin, eyepoint);
	eyepoint[2] += ent->client->ps.viewheight;

	// First we must trace from the eyepoint to the muzzle point, to make sure that we have a legal muzzle point.
	if (projsize>0)
	{
		VectorSet(mins, -projsize, -projsize, -projsize);
		VectorSet(maxs, projsize, projsize, projsize);
		WEAPON_TRACE(&tr, eyepoint, mins, maxs, muzzlePoint, ent->s.number, MASK_SHOT);
	}
	else
	{
		WEAPON_TRACE(&tr, eyepoint, NULL, NULL, muzzlePoint, ent->s.number, MASK_SHOT);
	}

	if (tr.fraction < 1.0)
	{	// We hit something here...  Stomp the muzzlePoint back to the eye...
		VectorCopy(eyepoint, muzzlePoint);
		// Keep the forward vector where it is, 'cause straight forward from the eyeball is right where we want to be.
	}
	else
	{
		// figure out what our crosshairs are on...
		VectorMA(eyepoint, MAX_FORWARD_TRACE, forward, end);
		WEAPON_TRACE (&tr, eyepoint, NULL, NULL, end, ent->s.number, MASK_SHOT );

		// ...and have our new forward vector point at it
		VectorSubtract(tr.endpos, muzzlePoint, forward);
		VectorNormalize(forward);
	}
}

/*
===============
CalcMuzzlePoint

set muzzle location relative to pivoting eye
===============
*/

// Muzzle point table...
vec3_t WP_MuzzlePoint[WP_NUM_WEAPONS] =
{//	Fwd,	right,	up.
	{0,		0,		0	},	// WP_NONE,
	{29,	2,		-4	},	// WP_PHASER,
	{25,	7,		-10	},	// WP_COMPRESSION_RIFLE,
	{25,	4,		-5	},	// WP_IMOD,
	{10,	14,		-8	},	// WP_SCAVENGER_RIFLE,
	{25,	5,		-8	},	// WP_STASIS,
	{25,	5,		-10	},	// WP_GRENADE_LAUNCHER,
	{22,	4.5,	-8	},	// WP_TETRION_DISRUPTOR,
	{5,		6,		-6	},	// WP_QUANTUM_BURST,
	{27,	8,		-10	},	// WP_DREADNOUGHT,
	{29,	2,		-4	},	// WP_VOYAGER_HYPO,
	{29,	2,		-4	},	// WP_BORG_ASSIMILATOR
	{27,	8,		-10	},	// WP_BORG_WEAPON
};


float WP_ShotSize( weapon_t weapon ) {
	switch ( weapon ) {
	case WP_SCAVENGER_RIFLE:
		return SCAV_SIZE;
	case WP_STASIS:
		return STASIS_MAIN_MISSILE_BIG*3;
	case WP_GRENADE_LAUNCHER:
		return GRENADE_SIZE;
	case WP_QUANTUM_BURST:
		return QUANTUM_SIZE;
	case WP_BORG_WEAPON:
		return BORG_PROJECTILE_SIZE;
	}

	return 0;
}

float WP_ShotAltSize( weapon_t weapon ) {
	switch ( weapon ) {
		case WP_PHASER:
			return PHASER_ALT_RADIUS;
		case WP_SCAVENGER_RIFLE:
			return SCAV_ALT_SIZE;
		case WP_GRENADE_LAUNCHER:
			return GRENADE_SIZE;
		case WP_TETRION_DISRUPTOR:
			return TETRION_ALT_SIZE;
		case WP_QUANTUM_BURST:
			return QUANTUM_SIZE;
		case WP_DREADNOUGHT:
			return DN_ALT_SIZE;
	}

	return 0;
}



//---------------------------------------------------------
void CalcMuzzlePoint ( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint, float projsize)
//---------------------------------------------------------
{
	int weapontype;

	weapontype = ent->s.weapon;
	VectorCopy( ent->s.pos.trBase, muzzlePoint );

#if 1
	if (weapontype > WP_NONE && weapontype < WP_NUM_WEAPONS)
	{	// Use the table to generate the muzzlepoint;
		{	// Crouching.  Use the add-to-Z method to adjust vertically.
			VectorMA(muzzlePoint, WP_MuzzlePoint[weapontype][0], forward, muzzlePoint);
			VectorMA(muzzlePoint, WP_MuzzlePoint[weapontype][1], right, muzzlePoint);
			muzzlePoint[2] += ent->client->ps.viewheight + WP_MuzzlePoint[weapontype][2];
			// VectorMA(muzzlePoint, ent->client->ps.viewheight + WP_MuzzlePoint[weapontype][2], up, muzzlePoint);
		}
	}
#else	// Test code
	muzzlePoint[2] += ent->client->ps.viewheight;//By eyes
	muzzlePoint[2] += g_debugUp.value;
	VectorMA( muzzlePoint, g_debugForward.value, forward, muzzlePoint);
	VectorMA( muzzlePoint, g_debugRight.value, right, muzzlePoint);
#endif

	CorrectForwardVector(ent, forward, muzzlePoint, projsize);
	SnapVector( muzzlePoint );
}


//---------------------------------------------------------
/*
//---------------------------------------------------------
void WP_TricorderScan (gentity_t *ent, qboolean alt_fire)
//---------------------------------------------------------
{
	static	int	sound_debounce_time;

	if ( sound_debounce_time < level.time )
	{
		if ( alt_fire )
		{
			sound_debounce_time = level.time + Q_irand(500, 1500);
		}
		else
		{
			sound_debounce_time = level.time + 5000;
		}
	}
}
*/

void WP_SprayVoyagerHypo( gentity_t *ent, qboolean alt_fire )
{
	gentity_t	*tr_ent;
	trace_t		tr;
	vec3_t		mins, maxs, end;

	VectorMA( muzzle, 32, forward, end );

	VectorSet( maxs, 6, 6, 6 );
	VectorScale( maxs, -1, mins );

	WEAPON_TRACE ( &tr, muzzle, mins, maxs, end, ent->s.number, MASK_SHOT );

	if ( tr.entityNum >= ENTITYNUM_WORLD )
	{
		/*
		gentity_t	*t_ent;

		// Create the effect -- thought something was needed here, but apparently not.
		VectorMA( muzzle, 20, forward, muzzle );
		VectorMA( muzzle, 4, vright, muzzle );
		t_ent = G_TempEntity( muzzle, EV_HYPO_PUFF );
		t_ent->s.eventParm = qfalse;
		VectorCopy( forward, t_ent->pos1 );
		*/
		return;
	}

	tr_ent = &g_entities[tr.entityNum];

	if ( tr_ent && tr_ent->client && tr_ent->health > 0 )
	{
		if ( !alt_fire )
		{//heal
			if ( tr_ent->health < tr_ent->client->ps.stats[STAT_MAX_HEALTH] )
			{
				tr_ent->health++;
			}
		}
		else
		{//knockout - FIXME: check for g_friendlyfire?
			tr_ent->health = 0;
			player_die( tr_ent, ent, ent, 100, MOD_KNOCKOUT );
			G_LogWeaponFire( ent->s.number, WP_VOYAGER_HYPO );
		}
	}
}

void WP_Assimilate( gentity_t *ent, qboolean alt_fire )
{//MCG: hacked this in for now for testing the rules
	gentity_t	*tr_ent;
	trace_t		tr;
	vec3_t		end;
	float		range;

	if ( modfn.IsBorgQueen( ent - g_entities ) )
	{
		range = 32;
	}
	else
	{
		range = 64;
	}
	VectorMA( muzzle, range, forward, end );

	WEAPON_TRACE ( &tr, muzzle, vec3_origin, vec3_origin, end, ent->s.number, MASK_SHOT );

	if ( tr.entityNum >= ENTITYNUM_WORLD )
	{
		return;
	}

	tr_ent = &g_entities[tr.entityNum];

	if ( tr_ent && tr_ent->client && tr_ent->health > 0 && ent->client &&
			( ent->client->sess.sessionTeam != tr_ent->client->sess.sessionTeam ||
			modfn.AdjustWeaponConstant( WC_ASSIM_NO_STRICT_TEAM_CHECK, 0 ) ) )
	{
		if ( modfn.IsBorgQueen( ent - g_entities ) )
		{//Borg queen assimilates with one hit
			tr_ent->health = 0;
			player_die( tr_ent, ent, ent, 100, MOD_ASSIMILATE );
			//G_Damage( tr_ent, ent, ent, forward, tr.endpos, 50, DAMAGE_NO_ARMOR|DAMAGE_NO_INVULNERABILITY, MOD_ASSIMILATE );
		}
		else
		{
			G_Damage( tr_ent, ent, ent, forward, tr.endpos, 10, DAMAGE_NO_ARMOR|DAMAGE_NO_INVULNERABILITY, MOD_ASSIMILATE );
		}
	}
}

/*
===============
G_SetQuadFactor

Called ahead of weapon firing functions to set global s_quadFactor value.
===============
*/
void G_SetQuadFactor( int clientNum ) {
	if ( G_AssertConnectedClient( clientNum ) && level.clients[clientNum].ps.powerups[PW_QUAD] ) {
		s_quadFactor = 3;
	} else {
		s_quadFactor = 1;
	}
}

/*
===============
FireWeapon
===============
*/

#define ACCURACY_TRACKING_DELAY			100		// in ms
#define NUM_FAST_WEAPONS				3

void FireWeapon( gentity_t *ent, qboolean alt_fire )
{
	float			projsize;
	int				i = 0;
	qboolean		bFastWeapon = qfalse;
	weapon_t		fastWeapons[NUM_FAST_WEAPONS] =
	{
		WP_PHASER,
		WP_TETRION_DISRUPTOR,
		WP_DREADNOUGHT
	};

	G_SetQuadFactor( ent - g_entities );

	// Unghost the player.
	modfn.SetClientGhosting( ent - g_entities, qfalse );

	// track shots taken for accuracy tracking
	for (i = 0; i < NUM_FAST_WEAPONS; i++)
	{
		if (fastWeapons[i] == ent->s.weapon)
		{
			bFastWeapon = qtrue;
			break;
		}
	}
	if (bFastWeapon && ((level.time - ent->client->pers.teamState.lastFireTime) < ACCURACY_TRACKING_DELAY))
	{
		// using a weapon with a high rate of fire so try to be a little more forgiving with
		//this whole accuracy thing
	}
	else
	{
		ent->client->ps.persistant[PERS_ACCURACY_SHOTS]++;
		ent->client->pers.teamState.lastFireTime = level.time;
	}

	// set aiming directions
	AngleVectors (ent->client->ps.viewangles, forward, right, up);

	if (alt_fire)
	{
		projsize = WP_ShotAltSize(ent->s.weapon);
	}
	else
	{
		projsize = WP_ShotSize(ent->s.weapon);
	}
	CalcMuzzlePoint ( ent, forward, right, up, muzzle, projsize);

	// fire the specific weapon
	switch( ent->s.weapon )
	{
	// Player weapons
	//-----------------
	case WP_PHASER:
		WP_FirePhaser( ent, alt_fire );
		break;
	case WP_COMPRESSION_RIFLE:
		WP_FireCompressionRifle( ent, alt_fire, qfalse );
		break;
	case WP_IMOD:
		WP_FireIMOD(ent, alt_fire);
		break;
	case WP_SCAVENGER_RIFLE:
		WP_FireScavenger( ent, alt_fire );
		break;
	case WP_STASIS:
		WP_FireStasis( ent, alt_fire );
		break;
	case WP_GRENADE_LAUNCHER:
		WP_FireGrenade( ent, alt_fire );
		break;
	case WP_TETRION_DISRUPTOR:
		WP_FireTetrionDisruptor( ent, alt_fire );
		break;
	case WP_DREADNOUGHT:
		WP_FireDreadnought( ent, alt_fire );
		break;
	case WP_QUANTUM_BURST:
		WP_FireQuantumBurst( ent, alt_fire );
		break;
/*
	case WP_TRICORDER:
		WP_TricorderScan( ent, alt_fire );
		break;
*/
	case WP_VOYAGER_HYPO:
		WP_SprayVoyagerHypo( ent, alt_fire );
		break;
	case WP_BORG_ASSIMILATOR:
		WP_Assimilate( ent, alt_fire );
		break;
	case WP_BORG_WEAPON:
		WP_FireBorgWeapon( ent, alt_fire );
		break;

	default:
// FIXME		G_Error( "Bad ent->s.weapon" );
		break;
	}
}

#define		SEEKER_RADIUS	500

qboolean SeekerAcquiresTarget ( gentity_t *ent, vec3_t pos )
{
	vec3_t		seekerPos;
	float		angle;
	int			entityList[MAX_GENTITIES]; // targets within inital radius
	int			visibleTargets[MAX_GENTITIES]; // final filtered target list
	int			numListedEntities;
	int			i, e;
	gentity_t	*target;
	vec3_t		mins, maxs;

//	if (!irandom(0,2)) for now, it'll shoot every second it finds a target
	{
		angle = level.time/100.0f;
		seekerPos[0] = ent->r.currentOrigin[0] + 18 * cos(angle);
		seekerPos[1] = ent->r.currentOrigin[1] + 18 * sin(angle);
		seekerPos[2] = ent->r.currentOrigin[2] + ent->client->ps.viewheight + 8 + (3*cos(level.time/150.0f));

		for ( i = 0 ; i < 3 ; i++ )
		{
			mins[i] = seekerPos[i] - SEEKER_RADIUS;
			maxs[i] = seekerPos[i] + SEEKER_RADIUS;
		}

		//	get potential targets within radius
		numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

		i = 0; // reset counter

		for ( e = 0 ; e < numListedEntities ; e++ )
		{
			target = &g_entities[entityList[ e ]];

			// seeker owner not a valid target
			if (target == ent)
			{
				continue;
			}

			// only players are valid targets
			if (!target->classname || strcmp(target->classname, "player"))
			{
				continue;
			}

			// teammates not valid targets
			if (OnSameTeam(ent, target))
			{
				continue;
			}

			// don't shoot at dead things
			if (target->health <= 0)
			{
				continue;
			}

			if( CanDamage (target, seekerPos) ) // visible target, so add it to the list
			{
				visibleTargets[i++] = entityList[e];
			}
		}

		if (i)
		{
			// ok, now we know there are i visible targets.  Pick one as the seeker's target
			ent->enemy = &g_entities[visibleTargets[irandom(0,i-1)]];
			VectorCopy(seekerPos, pos);
			return qtrue;
		}
	}

	return qfalse;
}

void FireSeeker( gentity_t *owner, gentity_t *target, vec3_t origin)
{
	vec3_t dir;

	VectorSubtract( target->r.currentOrigin, origin, dir);
	VectorNormalize(dir);

	// for now I'm just using the scavenger bullet.
	G_SetQuadFactor( owner - g_entities );
	FireScavengerBullet( owner, origin, dir );
}
