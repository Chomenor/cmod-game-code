#include "g_local.h"
//#include "g_functions.h"
//#include "b_local.h"

//extern team_t TranslateTeamName( const char *name );
//extern	cvar_t	*g_spskill;

//client side shortcut hacks from cg_local.h
//extern void CG_FireLaser( vec3_t start, vec3_t end, vec3_t normal, vec4_t laserRGB, qboolean hit_ent );
//extern void CG_AimLaser( vec3_t start, vec3_t end, vec3_t normal );


#define	ARM_ANGLE_RANGE		60
#define	HEAD_ANGLE_RANGE	90
#define	TURR_FOFS	18.0f
#define	TURR_ROFS	0.0f
#define	TURR_UOFS	12.0f
#define	ARM_FOFS	0.0f
#define	ARM_ROFS	0.0f
#define	ARM_UOFS	0.0f
#define	FARM_FOFS	14.0f
#define	FARM_ROFS	0.0f
#define	FARM_UOFS	4.0f
#define	FTURR_FOFS	0.0f
#define	FTURR_ROFS	0.0f
#define	FTURR_UOFS	6.0f
#define	LARM_FOFS	2.0f
#define	LARM_ROFS	0.0f
#define	LARM_UOFS	-26.0f

// return 0 for null parameter, consistent with qvm behavior
#define ATOI_SAFE( x ) ( ( x ) ? atoi( x ) : 0 )

void turret_die ( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath )
{
	vec3_t	dir;
	gentity_t	*owner, *te;

	//Turn off the thinking of the base & use it's targets
	self->activator->think = 0/*NULL*/;
	self->activator->nextthink = -1;
	self->activator->use = 0/*NULL*/;
	if ( self->activator->target )
	{
		G_UseTargets( self->activator, attacker );
	}

	//Remove the arm
	if ( self->r.ownerNum >= 0 && self->r.ownerNum < ENTITYNUM_WORLD )
	{
		owner = &g_entities[self->r.ownerNum];
		G_FreeEntity( owner );
	}

	//clear my data
	self->die  = 0/*NULL*/;
	self->think = 0/*NULL*/;
	self->nextthink = -1;
	self->takedamage = qfalse;
	self->health = 0;

	//Throw some chunks
	//AngleVectors( self->activator->r.currentAngles, dir, NULL, NULL );
	//VectorNormalize( dir );
	//CG_Chunks( self->s.number, self->r.currentOrigin, dir, Q_flrand(150, 300), irandom(3, 7), self->material, -1, 1.0 );

	if ( self->splashDamage > 0 && self->splashRadius > 0 )
	{//FIXME: specify type of explosion?  (barrel, electrical, etc.)
		G_RadiusDamage( self->r.currentOrigin, attacker, self->splashDamage, self->splashRadius, self->activator, DAMAGE_RADIUS, MOD_EXPLOSION );

		te = G_TempEntity( self->r.currentOrigin, EV_MISSILE_MISS );
		VectorSet( dir, 0, 0, 1 );
		te->s.eventParm = DirToByte( dir );
		te->s.weapon = WP_GRENADE_LAUNCHER;

		//G_Sound(self->activator, G_SoundIndex("sound/weapons/explosions/explode11.wav"));
	}

	self->activator->s.modelindex = self->activator->s.modelindex2;

	G_FreeEntity( self );
}

#define FORGE_TURRET_DAMAGE			2
#define FORGE_TURRET_SPLASH_RAD		64
#define FORGE_TURRET_SPLASH_DAM		4
#define FORGE_TURRET_VELOCITY		500

void turret_fire ( gentity_t *ent, vec3_t start, vec3_t dir )
{
	gentity_t	*bolt;

	bolt = G_Spawn();
	bolt->classname = "red turret shot";

	bolt->nextthink = level.time + 10000;
	bolt->think = G_FreeEntity;

	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_STASIS;
	bolt->r.ownerNum = ent->s.number;
	bolt->parent = ent;
	bolt->damage = ent->damage;
	bolt->splashDamage = 0;
	bolt->splashRadius = 0;
	bolt->methodOfDeath = MOD_STASIS;
	bolt->clipmask = MASK_SHOT;

	// There are going to be a couple of different sized projectiles, so store 'em here
	// kef -- need to keep the size in something that'll reach the cgame side
	bolt->count = bolt->s.time2 = 2;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	SnapVector( bolt->s.pos.trBase );			// save net bandwidth
	VectorScale( dir, 1100, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy( start, bolt->r.currentOrigin );

	VectorCopy (start, bolt->r.currentOrigin);
	// Used by trails
	VectorCopy (start, bolt->pos1 );
	VectorCopy (start, bolt->pos2 );
	// kef -- need to keep the origin in something that'll reach the cgame side
	VectorCopy(start, bolt->s.angles2);
	SnapVector( bolt->s.angles2 );			// save net bandwidth
}

void fturret_fire ( gentity_t *ent, vec3_t start, vec3_t dir )
{
	gentity_t	*bolt;

	bolt = G_Spawn();

	bolt->classname = "red turret shot";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_FreeEntity;
	bolt->s.eType = ET_MISSILE;
	bolt->s.weapon = WP_SCAVENGER_RIFLE;
	bolt->r.ownerNum = ent->s.number;
	bolt->damage = ent->damage;
	bolt->methodOfDeath = MOD_SCAVENGER;	// ?
	bolt->clipmask = MASK_SHOT;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 1100, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );		// save net bandwidth
	VectorCopy( start, bolt->r.currentOrigin);
}

void turret_head_think (gentity_t *self)
{
	qboolean	fire_now = qfalse;

	if ( !(self->activator->spawnflags & 2) )
	{//because forge turret heads have no anims... sigh...
		//animate
		if ( self->activator->enemy || self->pain_debounce_time > level.time || self->s.frame )
		{
			self->s.frame++;
			if ( self->s.frame > 10 )
			{
				self->s.frame = 0;
			}

			if ( self->s.frame == 0 || self->s.frame == 4 )
			{
				fire_now = qtrue;
			}
		}
	}
	else
	{
		if ( self->fly_sound_debounce_time < level.time )
		{
			self->fly_sound_debounce_time = level.time + self->wait * 10;
			fire_now = qtrue;
		}
	}

	//Fire
	if ( fire_now && self->activator->enemy && self->last_move_time < level.time )
	{//Only fire if ready to
		vec3_t	forward, right, up, muzzleSpot;
		float	rOfs = 0;

		AngleVectors(self->r.currentAngles, forward, right, up);
		VectorMA( self->r.currentOrigin, 16, forward, muzzleSpot );
		VectorMA( self->r.currentOrigin, 8, up, muzzleSpot );
		if ( !(self->activator->spawnflags & 2) )
		{//DN turrets have offsets
			if ( self->s.frame == 0 )
			{//Fire left barrel
				rOfs = -6;
			}
			else if ( self->s.frame == 4 )
			{//Fire right barrel
				rOfs = 6;
			}
		}

		VectorMA( self->r.currentOrigin, rOfs, right, muzzleSpot );


		if ( ATOI_SAFE(self->team) == TEAM_RED )
		{
			//G_Sound(self, G_SoundIndex("sound/enemies/turret/ffire.wav"));
			fturret_fire( self, muzzleSpot, forward );
		}
		else
		{
			//G_Sound(self, G_SoundIndex("sound/enemies/turret/fire.wav"));
			turret_fire( self, muzzleSpot, forward );
		}
	}

	//next think
	self->nextthink = level.time + self->wait;
}

void bolt_head_to_arm( gentity_t *arm, gentity_t *head, float fwdOfs, float rtOfs, float upOfs )
{
	vec3_t	headOrg, forward, right, up;

	AngleVectors( arm->r.currentAngles, forward, right, up );
	VectorMA( arm->r.currentOrigin, fwdOfs, forward, headOrg );
	VectorMA( headOrg, rtOfs, right, headOrg );
	VectorMA( headOrg, upOfs, up, headOrg );
	G_SetOrigin( head, headOrg );
	head->r.currentAngles[1] = head->s.apos.trBase[1] = head->s.angles[1] = arm->r.currentAngles[1];
	trap_LinkEntity( head );
}

void bolt_arm_to_base( gentity_t *base, gentity_t *arm, float fwdOfs, float rtOfs, float upOfs )
{
	vec3_t	headOrg, forward, right, up;

	AngleVectors( base->r.currentAngles, forward, right, up );
	VectorMA( base->r.currentOrigin, fwdOfs, forward, headOrg );
	VectorMA( headOrg, rtOfs, right, headOrg );
	VectorMA( headOrg, upOfs, up, headOrg );
	G_SetOrigin( arm, headOrg );
	trap_LinkEntity( arm );
	VectorCopy( base->r.currentAngles, arm->s.apos.trBase );
}

void rebolt_turret( gentity_t *base )
{
	vec3_t	headOrg, forward, right, up;

	if ( !base->lastEnemy )
	{//no arm
		return;
	}

	if ( !base->lastEnemy->lastEnemy )
	{//no head
		return;
	}

	if ( base->spawnflags&2 )
	{
		bolt_arm_to_base( base, base->lastEnemy, FARM_FOFS, FARM_ROFS, FARM_UOFS );
		bolt_head_to_arm( base->lastEnemy, base->lastEnemy->lastEnemy, FTURR_FOFS, FTURR_ROFS, FTURR_UOFS );
	}
	else
	{
		//FIXME: maybe move these seperately so they interpolate?
		G_SetOrigin( base->lastEnemy, base->s.pos.trBase );
		trap_LinkEntity(base->lastEnemy);
		//VectorCopy( base->r.currentAngles, base->lastEnemy->s.apos.trBase );
		AngleVectors( base->lastEnemy->r.currentAngles, forward, right, up );
		VectorMA( base->lastEnemy->r.currentOrigin, TURR_FOFS, forward, headOrg );
		VectorMA( headOrg, TURR_ROFS, right, headOrg );
		VectorMA( headOrg, TURR_UOFS, up, headOrg );
		G_SetOrigin( base->lastEnemy->lastEnemy, headOrg );
		//base->lastEnemy->lastEnemy->r.currentAngles[1] = base->lastEnemy->lastEnemy->s.apos.trBase[1] = base->lastEnemy->lastEnemy->s.angles[1] = base->lastEnemy->r.currentAngles[1];
		trap_LinkEntity( base->lastEnemy->lastEnemy );
	}
}
/*
void turret_aim( gentity_t *self )

  aims arm and head at enemy or neutral position
*/
void turret_aim( gentity_t *self )
{
	vec3_t	enemyDir;
	vec3_t	desiredAngles;
	float	diffAngle, armAngleDiff, headAngleDiff;
	//qboolean	turned = qfalse;
	int		upTurn = 0;
	int		yawTurn = 0;

	if ( self->enemy )
	{//Aim at enemy
		VectorSubtract( self->enemy->r.currentOrigin, self->r.currentOrigin, enemyDir );
		vectoangles( enemyDir, desiredAngles );
	}
	else
	{//Return to front
		VectorCopy( self->r.currentAngles, desiredAngles );
	}

	//yaw-aim arm at enemy at speed
	//FIXME: noise when turning?
	diffAngle = AngleSubtract(desiredAngles[1], self->lastEnemy->r.currentAngles[1]);
	if ( diffAngle )
	{
		if ( fabs(diffAngle) < self->speed )
		{//Just set the angle
			self->lastEnemy->r.currentAngles[1] = desiredAngles[1];
			//turned = qtrue;
		}
		else
		{//Add the increment
			self->lastEnemy->r.currentAngles[1] += (diffAngle < 0) ? -self->speed : self->speed;
			//turned = qtrue;
		}
		yawTurn = (diffAngle > 0) ? 1 : -1;
	}
	//Cap the range
	armAngleDiff = AngleSubtract(self->r.currentAngles[1], self->lastEnemy->r.currentAngles[1]);
	if ( armAngleDiff > ARM_ANGLE_RANGE )
	{
		self->lastEnemy->r.currentAngles[1] = AngleNormalize360(self->r.currentAngles[1] - ARM_ANGLE_RANGE);
		//turned = qfalse;
	}
	else if ( armAngleDiff < -ARM_ANGLE_RANGE )
	{
		self->lastEnemy->r.currentAngles[1] = AngleNormalize360(self->r.currentAngles[1] + ARM_ANGLE_RANGE);
		//turned = qfalse;
	}
	VectorCopy( self->lastEnemy->r.currentAngles, self->lastEnemy->s.apos.trBase );

	//Now put the turret at the tip of the arm
	if ( self->spawnflags&2 )
	{
		bolt_head_to_arm( self->lastEnemy, self->lastEnemy->lastEnemy, FTURR_FOFS, FTURR_ROFS, FTURR_UOFS );
	}
	else
	{
		bolt_head_to_arm( self->lastEnemy, self->lastEnemy->lastEnemy, TURR_FOFS, TURR_ROFS, TURR_UOFS );
	}

	//pitch-aim head at enemy at speed
	//FIXME: noise when turning?
	if ( self->enemy )
	{
		VectorSubtract( self->enemy->r.currentOrigin, self->lastEnemy->lastEnemy->r.currentOrigin, enemyDir );
		vectoangles( enemyDir, desiredAngles );
	}
	/*//Not necc
	else
	{
		VectorCopy(self->r.currentAngles, desiredAngles);
	}
	*/
	diffAngle = AngleSubtract( desiredAngles[0], self->lastEnemy->lastEnemy->r.currentAngles[0] );
	if ( diffAngle )
	{
		if ( fabs(diffAngle) < self->speed )
		{//Just set the angle
			self->lastEnemy->lastEnemy->r.currentAngles[0] = desiredAngles[0];
			//turned = qtrue;
		}
		else
		{//Add the increment
			self->lastEnemy->lastEnemy->r.currentAngles[0] += (diffAngle < 0) ? -self->speed : self->speed;
			//turned = qtrue;
		}
		upTurn = (diffAngle > 0) ? 1 : -1;
	}
	//Cap the range
	headAngleDiff = AngleSubtract(self->r.currentAngles[0], self->lastEnemy->lastEnemy->r.currentAngles[0]);
	if ( headAngleDiff > HEAD_ANGLE_RANGE )
	{
		self->lastEnemy->lastEnemy->r.currentAngles[0] = AngleNormalize360(self->r.currentAngles[0] - HEAD_ANGLE_RANGE);
		//turned = qfalse;
	}
	else if ( headAngleDiff < -HEAD_ANGLE_RANGE )
	{
		self->lastEnemy->lastEnemy->r.currentAngles[0] = AngleNormalize360(self->lastEnemy->r.currentAngles[0] + HEAD_ANGLE_RANGE);
		//turned = qfalse;
	}
	VectorCopy( self->lastEnemy->lastEnemy->r.currentAngles, self->lastEnemy->lastEnemy->s.apos.trBase );

	//Play sound if turret changes direction
	//Pitch:
	/*
	if ( upTurn && upTurn != self->count )
	{//changed dir
		G_Sound(self->lastEnemy->lastEnemy, G_SoundIndex("sound/enemies/turret/move.wav"));
	}
	else if ( !upTurn && self->count )
	{//Just stopped
		G_Sound(self->lastEnemy->lastEnemy, G_SoundIndex("sound/enemies/turret/stop.wav"));
	}
	self->count = upTurn;
	*/
	//Yaw:
	if ( yawTurn && yawTurn != self->soundPos2 )
	{//changed dir
		G_Sound(self->lastEnemy, G_SoundIndex("sound/enemies/turret/move.wav"));
	}
	else if ( !yawTurn && self->soundPos2 )
	{//Just stopped
		G_Sound(self->lastEnemy, G_SoundIndex("sound/enemies/turret/stop.wav"));
	}
	self->soundPos2 = yawTurn;

	/*
	if ( turned )
	{
		G_Sound(self->lastEnemy, G_SoundIndex("sound/enemies/turret/move.wav"));
	}
	*/
}

void turret_turnoff (gentity_t *self)
{
	if ( self->enemy == NULL )
	{
		return;
	}
	//shut-down sound
	G_Sound(self, G_SoundIndex("sound/enemies/turret/shutdown.wav"));

	//make turret keep animating for 3 secs
	self->lastEnemy->lastEnemy->pain_debounce_time = level.time + 3000;

	//Clear enemy
	self->enemy = NULL;
}

void turret_base_think (gentity_t *self)
{
	vec3_t		enemyDir;
	float		enemyDist;

	self->nextthink = level.time + FRAMETIME;

	if ( self->spawnflags & 1 )
	{//not turned on
		turret_turnoff( self );
		turret_aim( self );
		//No target
		if ( self->lastEnemy && self->lastEnemy->lastEnemy )
		{
			self->lastEnemy->lastEnemy->flags |= FL_NOTARGET;
		}
		return;
	}
	else
	{//I'm all hot and bothered
		if ( self->lastEnemy && self->lastEnemy->lastEnemy )
		{
			self->lastEnemy->lastEnemy->flags &= ~FL_NOTARGET;
		}
	}

	if ( !self->enemy )
	{//Find one
		gentity_t	*entity_list[MAX_GENTITIES], *target;
		int			count, i;
		float		bestDist = self->random * self->random;

		if ( self->last_move_time > level.time )
		{//We're active and alert, had an enemy in the last 5 secs
			if ( self->pain_debounce_time < level.time )
			{
				G_Sound(self, G_SoundIndex("sound/enemies/turret/ping.wav"));
				self->pain_debounce_time = level.time + 1000;
			}
		}

		count = G_RadiusList( self->r.currentOrigin, self->random, self->lastEnemy->lastEnemy, qtrue, entity_list );
		for ( i = 0; i < count; i++ )
		{
			target = entity_list[i];
			if ( target == self )
			{
				continue;
			}

			if ( target->takedamage && target->health > 0 && !(target->flags & FL_NOTARGET) )
			{
				if ( !target->client && target->team && ATOI_SAFE(target->team) == ATOI_SAFE(self->team) )
				{//Something of ours we don't want to destroy
					continue;
				}
				if ( target->client && target->client->sess.sessionTeam == ATOI_SAFE(self->team) )
				{//A bot we don't want to shoot
					continue;
				}

				if ( trap_InPVS( self->lastEnemy->lastEnemy->r.currentOrigin, target->r.currentOrigin ) )
				{
					trace_t	tr;

					trap_Trace( &tr, self->lastEnemy->lastEnemy->r.currentOrigin, NULL, NULL, target->r.currentOrigin, self->lastEnemy->lastEnemy->s.number, MASK_SHOT );

					if ( !tr.allsolid && !tr.startsolid && (tr.fraction == 1.0 || tr.entityNum == target->s.number) )
					{//Only acquire if have a clear shot
						//Is it in range and closer than our best?
						VectorSubtract( target->r.currentOrigin, self->r.currentOrigin, enemyDir );
						enemyDist = VectorLengthSquared( enemyDir );
						if ( enemyDist < bestDist )//all things equal, keep current
						{
							if ( self->last_move_time < level.time )
							{//We haven't fired or acquired an enemy in the last 5 seconds
								//start-up sound
								G_Sound(self, G_SoundIndex("sound/enemies/turret/startup.wav"));
								//Wind up turrets for a second
								self->lastEnemy->lastEnemy->last_move_time = level.time + 1000;
							}
							self->enemy = target;
							bestDist = enemyDist;
						}
					}
				}
			}
		}
	}

	if ( self->enemy )
	{//Check if still in random
		if ( self->enemy->health <= 0 )
		{
			turret_turnoff( self );
			return;
		}

		VectorSubtract( self->enemy->r.currentOrigin, self->r.currentOrigin, enemyDir );
		enemyDist = VectorLengthSquared( enemyDir );
		if ( enemyDist > self->random*self->random )
		{
			turret_turnoff( self );
			return;
		}

		if ( !trap_InPVS( self->lastEnemy->lastEnemy->r.currentOrigin, self->enemy->r.currentOrigin ) )
		{
			turret_turnoff( self );
			return;
		}

		// Every now and again, check to see if we can even trace to the enemy
		if ( irandom( 0, 16 ) > 15 )
		{
			trace_t tr;

			trap_Trace( &tr, self->lastEnemy->lastEnemy->r.currentOrigin, NULL, NULL, self->enemy->r.currentOrigin, self->lastEnemy->lastEnemy->s.number, MASK_SHOT );
			if ( tr.allsolid || tr.startsolid || tr.fraction != 1.0 )
			{
				// Couldn't see our enemy
				turret_turnoff( self );
			}
		}
	}

	if ( self->enemy )
	{//Aim
		//Won't need to wind up turrets for a while
		self->last_move_time = level.time + 5000;
		turret_aim( self );
	}
	else if ( self->last_move_time < level.time )
	{
		//Move arm and head back to neutral angles
		turret_aim( self );
	}
}

void turret_base_use (gentity_t *self, gentity_t *other, gentity_t *activator)
{//Toggle on and off
	self->spawnflags = (self->spawnflags ^ 1);
}

/*QUAKED misc_turret (1 0 0) (-8 -8 -8) (8 8 8) START_OFF
Will aim and shoot at enemies

  START_OFF - Starts off

  random - How far away an enemy can be for it to pick it up (default 512)
  speed - How fast it turns (degrees per second, default 30)
  wait	- How fast it shoots (shots per second, default 4, can't be less)
  dmg	- How much damage each shot does (default 5)
  health - How much damage it can take before exploding (default 100)

  splashDamage - How much damage the explosion does
  splashRadius - The random of the explosion
  NOTE: If either of the above two are 0, it will not make an explosion

  targetname - Toggles it on/off
  target - What to use when destroyed

  "team" - This cannot take damage from members of this team and will not target members of this team (2 = blue, 1 = red)
*/
void SP_misc_turret (gentity_t *base)
{
	//We're the base, spawn the arm and head
	gentity_t *arm = G_Spawn();
	gentity_t *head = G_Spawn();
	vec3_t		fwd;

	//Base
	//Base does the looking for enemies and pointing the arm and head
	VectorCopy( base->s.angles, base->s.apos.trBase );
	VectorCopy( base->s.angles, base->r.currentAngles );
	AngleVectors( base->r.currentAngles, fwd, NULL, NULL );
	VectorMA( base->s.origin, -8, fwd, base->s.origin );
	G_SetOrigin(base, base->s.origin);
	trap_LinkEntity(base);
	if ( ATOI_SAFE( base->team ) == TEAM_RED )
	{//red model
		base->s.modelindex = G_ModelIndex("models/mapobjects/forge/turret.md3");
		base->s.modelindex2 = G_ModelIndex("models/mapobjects/forge/turret_d1.md3");
	}
	else
	{//blue model
		base->s.modelindex = G_ModelIndex("models/mapobjects/dn/gunturret_base.md3");
	}
	base->s.eType = ET_GENERAL;
	//anglespeed - how fast it can track the player, entered in degrees per second, so we divide by FRAMETIME/1000
	if ( !base->speed )
	{
		base->speed = 3.0f;
	}
	else
	{
		base->speed /= FRAMETIME/1000.0f;
	}
	//range
	if ( !base->random )
	{
		base->random = 512;
	}
	base->use = turret_base_use;
	base->think = turret_base_think;
	base->nextthink = level.time + FRAMETIME;

	//Arm
	//Does nothing, not solid, gets removed when head explodes
	if ( ATOI_SAFE( base->team ) == TEAM_RED )
	{
		bolt_arm_to_base( base, arm, FARM_FOFS, FARM_ROFS, FARM_UOFS );
		bolt_head_to_arm( arm, head, FTURR_FOFS, FTURR_ROFS, FTURR_UOFS );
	}
	else
	{
		bolt_arm_to_base( base, arm, ARM_FOFS, ARM_ROFS, ARM_UOFS );
		//G_SetOrigin( arm, base->s.origin );
		//trap_LinkEntity(arm);
		//VectorCopy( base->r.currentAngles, arm->s.apos.trBase );
		bolt_head_to_arm( arm, head, TURR_FOFS, TURR_ROFS, TURR_UOFS );
	}
	if ( ATOI_SAFE( base->team ) == TEAM_RED )
	{
		arm->s.modelindex = G_ModelIndex("models/mapobjects/forge/turret_neck.md3");
	}
	else
	{
		arm->s.modelindex = G_ModelIndex("models/mapobjects/dn/gunturret_arm.md3");
	}
	arm->team = base->team;

	//Head
	//Fires when enemy detected, animates, can be blown up
	VectorCopy( base->r.currentAngles, head->s.apos.trBase );
	if ( ATOI_SAFE( base->team ) == TEAM_RED )
	{
		head->s.modelindex = G_ModelIndex("models/mapobjects/forge/turret_head.md3");
	}
	else
	{
		head->s.modelindex = G_ModelIndex("models/mapobjects/dn/gunturret_head.md3");
	}
	head->team = base->team;
	head->s.eType = ET_GENERAL;
	VectorSet( head->r.mins, -8, -8, -16 );
	VectorSet( head->r.maxs, 8, 8, 16 );
	//FIXME: make an index into an external string table for localization
	//head->fullName = "Turret";
	trap_LinkEntity(head);

	//How much health head takes to explode
	if ( !base->health )
	{
		head->health = 100;
	}
	else
	{
		head->health = base->health;
	}
	base->health = 0;
	//How quickly to fire
	if ( !base->wait )
	{
		head->wait = 50;
	}
	else
	{
		head->wait = 100/(base->wait/2);
	}
	base->wait = 0;
	//splashDamage
	if ( !base->splashDamage )
	{
		head->splashDamage = 10;
	}
	else
	{
		head->splashDamage = base->splashDamage;
	}
	base->splashDamage = 0;
	//splashRadius
	if ( !base->splashRadius )
	{
		head->splashRadius = 25;
	}
	else
	{
		head->splashRadius = base->splashRadius;
	}
	base->splashRadius = 0;
	//dmg
	if ( !base->damage )
	{
		head->damage = 5;
	}
	else
	{
		head->damage = base->damage;
	}
	base->damage = 0;

	//Precache firing and explode sounds
	G_SoundIndex("sound/weapons/explosions/explode11.wav");
	G_SoundIndex("sound/enemies/turret/startup.wav");
	G_SoundIndex("sound/enemies/turret/shutdown.wav");
	G_SoundIndex("sound/enemies/turret/move.wav");
	G_SoundIndex("sound/enemies/turret/stop.wav");
	G_SoundIndex("sound/enemies/turret/ping.wav");
	if ( ATOI_SAFE( base->team ) == TEAM_RED )
	{
		G_SoundIndex("sound/enemies/turret/ffire.wav");
	}
	else
	{
		G_SoundIndex("sound/enemies/turret/fire.wav");
	}

	head->r.contents = CONTENTS_BODY;
	//head->max_health = head->health;
	head->takedamage = qtrue;
	head->die = turret_die;

	head->think = turret_head_think;
	head->nextthink = level.time + FRAMETIME;

	//head->material = MAT_METAL;
	//head->r.svFlags |= SVF_NO_TELEPORT|SVF_NONNPC_ENEMY|SVF_SELF_ANIMATING;

	//Link them up
	base->lastEnemy = arm;
	arm->lastEnemy = head;
	head->r.ownerNum = arm->s.number;
	arm->activator = head->activator = base;

	//register the weapons whose effects are being used
	if ( ATOI_SAFE( base->team ) == TEAM_RED )
	{
		//temp gfx and sounds
		RegisterItem( BG_FindItemForWeapon( WP_SCAVENGER_RIFLE ) );	//precache the weapon
	}
	else
	{
		//temp gfx and sounds
		RegisterItem( BG_FindItemForWeapon( WP_STASIS ) );	//precache the weapon
	}
}

/*
void laser_arm_fire (gentity_t *ent)
{
	vec3_t	start, end, fwd, rt, up;
	trace_t	trace;

	if ( ent->last_move_time < level.time && ent->alt_fire )
	{
		// If I'm firing the laser and it's time to quit....then quit!
		ent->alt_fire = qfalse;
//		ent->e_ThinkFunc = thinkF_NULL;
//		return;
	}

	ent->nextthink = level.time + FRAMETIME;

	// If a fool gets in the laser path, fry 'em
	AngleVectors( ent->r.currentAngles, fwd, rt, up );

	VectorMA( ent->r.currentOrigin, 20, fwd, start );
	//VectorMA( start, -6, rt, start );
	//VectorMA( start, -3, up, start );
	VectorMA( start, 4096, fwd, end );

	trap_Trace( &trace, start, NULL, NULL, end, -1, MASK_SHOT );//ignore

	// Only deal damage when in alt-fire mode
	if ( trace.fraction < 1.0 && ent->alt_fire )
	{
		if ( trace.entityNum < ENTITYNUM_WORLD )
		{
			gentity_t *hapless_victim = &g_entities[trace.entityNum];
			if ( hapless_victim && hapless_victim->takedamage && ent->damage )
			{
				G_Damage( hapless_victim, ent, ent->nextTrain->activator, fwd, trace.endpos, ent->damage, DAMAGE_IGNORE_TEAM, MOD_SURGICAL_LASER );
			}
		}
	}

	if ( ent->alt_fire )
	{
		CG_FireLaser( start, trace.endpos, trace.plane.normal, ent->nextTrain->startRGBA, qfalse );
	}
	else
	{
		CG_AimLaser( start, trace.endpos, trace.plane.normal );
	}
}

void laser_arm_use (gentity_t *self, gentity_t *other, gentity_t *activator)
{
	vec3_t	newAngles;

	self->activator = activator;
	switch( self->count )
	{
	case 0:
	default:
		//Fire
		//trap_Printf("FIRE!\n");
//		self->lastEnemy->lastEnemy->e_ThinkFunc = thinkF_laser_arm_fire;
//		self->lastEnemy->lastEnemy->nextthink = level.time + FRAMETIME;
		//For 3 seconds
		self->lastEnemy->lastEnemy->alt_fire = qtrue; // Let 'er rip!
		self->lastEnemy->lastEnemy->last_move_time = level.time + self->lastEnemy->lastEnemy->wait;
		G_Sound(self->lastEnemy->lastEnemy, G_SoundIndex("sound/enemies/l_arm/fire.wav"));
		break;
	case 1:
		//Yaw left
		//trap_Printf("LEFT...\n");
		VectorCopy( self->lastEnemy->r.currentAngles, newAngles );
		newAngles[1] += self->speed;
		VectorCopy( newAngles, self->lastEnemy->s.apos.trBase );
		bolt_head_to_arm( self->lastEnemy, self->lastEnemy->lastEnemy, LARM_FOFS, LARM_ROFS, LARM_UOFS );
		G_Sound( self->lastEnemy, G_SoundIndex( "sound/enemies/l_arm/move.wav" ) );
		break;
	case 2:
		//Yaw right
		//trap_Printf("RIGHT...\n");
		VectorCopy( self->lastEnemy->r.currentAngles, newAngles );
		newAngles[1] -= self->speed;
		VectorCopy( newAngles, self->lastEnemy->s.apos.trBase );
		bolt_head_to_arm( self->lastEnemy, self->lastEnemy->lastEnemy, LARM_FOFS, LARM_ROFS, LARM_UOFS );
		G_Sound( self->lastEnemy, G_SoundIndex( "sound/enemies/l_arm/move.wav" ) );
		break;
	case 3:
		//pitch up
		//trap_Printf("UP...\n");
		//FIXME: Clamp
		VectorCopy( self->lastEnemy->lastEnemy->r.currentAngles, newAngles );
		newAngles[0] -= self->speed;
		if ( newAngles[0] < -45 )
		{
			newAngles[0] = -45;
		}
		VectorCopy( newAngles, self->lastEnemy->lastEnemy->s.apos.trBase );
		G_Sound( self->lastEnemy->lastEnemy, G_SoundIndex( "sound/enemies/l_arm/move.wav" ) );
		break;
	case 4:
		//pitch down
		//trap_Printf("DOWN...\n");
		//FIXME: Clamp
		VectorCopy( self->lastEnemy->lastEnemy->r.currentAngles, newAngles );
		newAngles[0] += self->speed;
		if ( newAngles[0] > 90 )
		{
			newAngles[0] = 90;
		}
		VectorCopy( newAngles, self->lastEnemy->lastEnemy->s.apos.trBase );
		G_Sound( self->lastEnemy->lastEnemy, G_SoundIndex( "sound/enemies/l_arm/move.wav" ) );
		break;
	}
}
*/
/*QUAKED misc_laser_arm (1 0 0) (-8 -8 -8) (8 8 8)

What it does when used depends on it's "count" (can be set by a script)
	count:
		0 (default) - Fire in direction facing
		1 turn left
		2 turn right
		3 aim up
		4 aim down

  speed - How fast it turns (degrees per second, default 30)
  dmg	- How much damage the laser does 10 times a second (default 5 = 50 points per second)
  wait  - How long the beam lasts, in seconds (default is 3)

  targetname - to use it
  target - What thing for it to be pointing at to start with

  "startRGBA" - laser color, Red Green Blue Alpha, range 0 to 1 (default  1.0 0.85 0.15 0.75 = Yellow-Orange)
*/
/*
void laser_arm_start (gentity_t *base)
{
	vec3_t	armAngles;
	vec3_t	headAngles;

	base->e_ThinkFunc = thinkF_NULL;
	//We're the base, spawn the arm and head
	gentity_t *arm = G_Spawn();
	gentity_t *head = G_Spawn();

	VectorCopy( base->s.angles, armAngles );
	VectorCopy( base->s.angles, headAngles );
	if ( base->target && base->target[0] )
	{//Start out pointing at something
		gentity_t *targ = G_Find( NULL, FOFS(targetname), base->target );
		if ( !targ )
		{//couldn't find it!
			Com_Printf(S_COLOR_RED "ERROR : laser_arm can't find target %s!\n", base->target);
		}
		else
		{//point at it
			vec3_t	dir, angles;

			VectorSubtract(targ->r.currentOrigin, base->s.origin, dir );
			vectoangles( dir, angles );
			armAngles[1] = angles[1];
			headAngles[0] = angles[0];
			headAngles[1] = angles[1];
		}
	}

	//Base
	//Base does the looking for enemies and pointing the arm and head
	VectorCopy( base->s.angles, base->s.apos.trBase );
	//base->s.origin[2] += 4;
	G_SetOrigin(base, base->s.origin);
	trap_LinkEntity(base);
	//FIXME: need an actual model
	base->s.modelindex = G_ModelIndex("models/mapobjects/dn/laser_base.md3");
	base->s.eType = ET_GENERAL;
	G_SpawnVector4( "startRGBA", "1.0 0.85 0.15 0.75", (float *)&base->startRGBA );
	//anglespeed - how fast it can track the player, entered in degrees per second, so we divide by FRAMETIME/1000
	if ( !base->speed )
	{
		base->speed = 3.0f;
	}
	else
	{
		base->speed *= FRAMETIME/1000.0f;
	}
	base->e_UseFunc = useF_laser_arm_use;
	base->nextthink = level.time + FRAMETIME;

	//Arm
	//Does nothing, not solid, gets removed when head explodes
	G_SetOrigin( arm, base->s.origin );
	trap_LinkEntity(arm);
	VectorCopy( armAngles, arm->s.apos.trBase );
	bolt_head_to_arm( arm, head, LARM_FOFS, LARM_ROFS, LARM_UOFS );
	arm->s.modelindex = G_ModelIndex("models/mapobjects/dn/laser_arm.md3");

	//Head
	//Fires when enemy detected, animates, can be blown up
	//Need to normalize the headAngles pitch for the clamping later
	if ( headAngles[0] < -180 )
	{
		headAngles[0] += 360;
	}
	else if ( headAngles[0] > 180 )
	{
		headAngles[0] -= 360;
	}
	VectorCopy( headAngles, head->s.apos.trBase );
	head->s.modelindex = G_ModelIndex("models/mapobjects/dn/laser_head.md3");
	head->s.eType = ET_GENERAL;
//	head->r.svFlags |= SVF_BROADCAST;// Broadcast to all clients
	VectorSet( head->r.mins, -8, -8, -8 );
	VectorSet( head->r.maxs, 8, 8, 8 );
	head->r.contents = CONTENTS_BODY;
	//FIXME: make an index into an external string table for localization
	head->fullName = "Surgical Laser";
	trap_LinkEntity(head);

	//dmg
	if ( !base->damage )
	{
		head->damage = 5;
	}
	else
	{
		head->damage = base->damage;
	}
	base->damage = 0;
	//lifespan of beam
	if ( !base->wait )
	{
		head->wait = 3000;
	}
	else
	{
		head->wait = base->wait * 1000;
	}
	base->wait = 0;

	//Precache firing and explode sounds
	G_SoundIndex("sound/weapons/explosions/cargoexplode.wav");
	G_SoundIndex("sound/enemies/l_arm/fire.wav");
	G_SoundIndex("sound/enemies/l_arm/move.wav");

	//Link them up
	base->lastEnemy = arm;
	arm->lastEnemy = head;
	head->owner = arm;
	arm->nextTrain = head->nextTrain = base;

	// The head should always think, since it will be either firing a damage laser or just a target laser
	head->e_ThinkFunc = thinkF_laser_arm_fire;
	head->nextthink = level.time + FRAMETIME;
	head->alt_fire = qfalse; // Don't do damage until told to
}

void SP_laser_arm (gentity_t *base)
{
	base->e_ThinkFunc = thinkF_laser_arm_start;
	base->nextthink = level.time + FRAMETIME;
}
*/
