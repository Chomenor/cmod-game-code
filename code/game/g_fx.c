// This file contains game side effects that the designers can place throughout the maps

#include "g_local.h"

#define SPARK_STARTOFF		1

/*QUAKED fx_spark (0 0 1) (-8 -8 -8) (8 8 8) STARTOFF
Emits sparks at the specified point in the specified direction

  "target" - ( optional ) direction to aim the sparks in, otherwise, uses the angles set in the editor.
  "wait(2000)"	- interval between events (randomly twice as long) 
*/

//------------------------------------------
void spark_think( gentity_t *ent )
{
	G_AddEvent( ent, EV_FX_SPARK, 0 );
	ent->nextthink = level.time + 10000.0; // send a refresh message every 10 seconds
}

//------------------------------------------
void spark_link( gentity_t *ent )
{

	ent->think = spark_think;	
	ent->nextthink = level.time + 10000.0;

	ent->s.time2 = ent->wait;
	if ( ent->target )
	{
		// try to use the target to orient me.
		gentity_t	*target = NULL;
		vec3_t		dir;

		target = G_Find (target, FOFS(targetname), ent->target);

		if (!target)
		{
			Com_Printf("spark_link: target specified but not found: %s\n", ent->target );

			ent->think = 0;
			ent->nextthink = -1;

			return;
		}
		
		VectorSubtract( target->s.origin, ent->s.origin, dir );
		VectorNormalize( dir );
		vectoangles( dir, ent->r.currentAngles );
		VectorCopy( ent->r.currentAngles, ent->s.angles );
		VectorShort(ent->s.angles);
		VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );
		SnapVector(ent->s.apos.trBase);
	}
	G_AddEvent( ent, EV_FX_SPARK, 0 );
}

//------------------------------------------
void SP_fx_spark( gentity_t	*ent )
{
	if (!ent->wait)
	{
		ent->wait = 2000;
	}

	SnapVector(ent->s.origin);
	VectorCopy( ent->s.origin, ent->s.pos.trBase );

	// The thing that this is targetting may not be spawned in yet, so wait a bit to try and link to it
	ent->think = spark_link; 
	ent->nextthink = level.time + 2000;

	trap_LinkEntity( ent );
}


/*QUAKED fx_steam (0 0 1) (-8 -8 -8) (8 8 8) STARTOFF BURSTS 
Emits steam at the specified point in the specified direction. will point at a target if one is specified.

  right now, neither spawnflag does anything. just give it a direction.

  "targetname" - toggles on/off whenever used
  "damage" - damage to apply when caught in steam vent, default - zero damage (no damage). Don't add this unless you really have to.
*/

#define STEAM_STARTOFF		1
#define STEAM_BURSTS		2

//------------------------------------------
void steam_think( gentity_t *ent )
{
	G_AddEvent( ent, EV_FX_STEAM, 0 );
	ent->nextthink = level.time + 10000.0; // send a refresh message every 10 seconds
/*	if ( ent->spawnflags & STEAM_BURSTS )
	{
		ent->nextthink = level.time + 1000 + random() * 500;
	}
	else
	{
		ent->nextthink = level.time + 50;
	}

	// FIXME: This may be a bit weird for steam bursts
	// If a fool gets in the bolt path, zap 'em
	if ( ent->damage ) 
	{
		vec3_t	start, temp;
		trace_t	trace;

		VectorSubtract( ent->s.origin2, ent->r.currentOrigin, temp );
		VectorNormalize( temp );
		VectorMA( ent->r.currentOrigin, 1, temp, start );

		trap_Trace( &trace, start, NULL, NULL, ent->s.origin2, -1, MASK_SHOT );//ignore

		if ( trace.fraction < 1.0 )
		{
			if ( trace.entityNum < ENTITYNUM_WORLD )
			{
				gentity_t *victim = &g_entities[trace.entityNum];
				if ( victim && victim->takedamage )
				{
					G_Damage( victim, ent, ent->activator, temp, trace.endpos, ent->damage, 0, MOD_UNKNOWN );
				}
			}
		}
	}
*/
}

//------------------------------------------
void steam_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->count )
	{
		self->think = 0;
	}
	else
	{
		self->think = steam_think;
		self->nextthink = level.time + 100;
	}
	
	self->count = !self->count;
}

//------------------------------------------
void steam_link( gentity_t *ent )
{
	gentity_t	*target = NULL;
	vec3_t		dir;
	float		len;

	if (ent->target)
	{
		target = G_Find (target, FOFS(targetname), ent->target);
	}

	if (!target)
	{
		Com_Printf("steam_link: unable to find target %s\n", ent->target );

		ent->think = 0;
		ent->nextthink = -1;

		return;
	}

	VectorSubtract( target->s.origin, ent->s.origin, dir );
	len = VectorNormalize( dir );
	vectoangles( dir, ent->s.angles2 );
	VectorShort(ent->s.angles2);
	
	VectorCopy( target->s.origin, ent->s.origin2 );
	SnapVector(ent->s.origin2);

	// This is used as the toggle switch
	ent->count = !(ent->spawnflags & STEAM_STARTOFF);
	trap_LinkEntity( ent );

	// this actually creates the continuously-spawning steam jet
	G_AddEvent( ent, EV_FX_STEAM, 0 );
	ent->think = steam_think;
	ent->nextthink = level.time + 10000;
}

//------------------------------------------
void SP_fx_steam( gentity_t	*ent )
{
	SnapVector(ent->s.origin);
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	trap_LinkEntity( ent );

	// Try to apply defaults if nothing was set
	G_SpawnInt( "damage", "0", &ent->damage );

	ent->think = steam_link;
	ent->nextthink = level.time + 2000;
}

/*QUAKED fx_bolt (0 0 1) (-8 -8 -8) (8 8 8) SPARKS BORG TAPER SMOOTH
Emits blue ( or borg green ) electric bolts from the specified point to the specified point

  SPARKS - create impact sparks, probably best used for time delayed bolts
  BORG - Make the bolts green

  "wait" - seconds between bolts (0 is always on, default is 2.0, -1 for random number between 0 and 5), bolts are always on for 0.2 seconds
  "damage" - damage per server frame (default 0)
  "random" - bolt chaos (0.1 = too calm, 0.5 = default, 1.0 or higher = pretty wicked)
*/


#define BOLT_SPARKS		(1<<0)
#define BOLT_BORG		(1<<1)

//------------------------------------------
void bolt_think( gentity_t *ent )
{
	vec3_t	start, temp;
	trace_t	trace;

	G_AddEvent( ent, EV_FX_BOLT, ent->spawnflags );
	ent->s.time2 = ent->wait;
	ent->nextthink = level.time + 10000;//(ent->wait + crandom() * ent->wait * 0.25) * 1000;

	// If a fool gets in the bolt path, zap 'em
	if ( ent->damage ) 
	{
		VectorSubtract( ent->s.origin2, ent->r.currentOrigin, temp );
		VectorNormalize( temp );
		VectorMA( ent->r.currentOrigin, 1, temp, start );

		trap_Trace( &trace, start, NULL, NULL, ent->s.origin2, -1, MASK_SHOT );//ignore

		if ( trace.fraction < 1.0 )
		{
			if ( trace.entityNum < ENTITYNUM_WORLD )
			{
				gentity_t *victim = &g_entities[trace.entityNum];
				if ( victim && victim->takedamage )
				{
					G_Damage( victim, ent, ent->activator, temp, trace.endpos, ent->damage, 0, MOD_UNKNOWN );
				}
			}
		}
	}
	// net optimisations
	SnapVector(ent->s.origin2);
}

//------------------------------------------
void bolt_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->count )
	{
		self->think = 0;
	}
	else
	{
		self->think = bolt_think;
		self->nextthink = level.time + 200;
	}
	
	self->count = !self->count;
}

//------------------------------------------
void bolt_link( gentity_t *ent )
{
	gentity_t	*target = NULL;
	vec3_t		dir;
	float		len;

	if (ent->target)
	{
		target = G_Find (target, FOFS(targetname), ent->target);
	}

	if (!target)
	{
		Com_Printf("bolt_link: unable to find target %s\n", ent->target );

		ent->think = 0;
		ent->nextthink = -1;

		return;
	}

	VectorSubtract( target->s.origin, ent->s.origin, dir );
	len = VectorNormalize( dir );
	vectoangles( dir, ent->s.angles );
	
	VectorCopy( target->s.origin, ent->s.origin2 );
	SnapVector(ent->s.origin2);

	if ( ent->targetname )
	{
		ent->use = bolt_use;
	}

	G_AddEvent( ent, EV_FX_BOLT, ent->spawnflags );
	ent->s.time2 = ent->wait;
	ent->think = bolt_think;	
	ent->nextthink = level.time + 10000;
	trap_LinkEntity( ent );
}

//------------------------------------------
void SP_fx_bolt( gentity_t *ent )
{
	G_SpawnInt( "damage", "0", &ent->damage );
	G_SpawnFloat( "random", "0.5", &ent->random );
	G_SpawnFloat( "speed", "15.0", &ent->speed );

	// See if effect is supposed to be delayed
	G_SpawnFloat( "wait", "2.0", &ent->wait );

	SnapVector(ent->s.origin);
	VectorCopy( ent->s.origin, ent->s.pos.trBase );

	ent->s.angles2[0] = ent->speed;
	ent->s.angles2[1] = ent->random;

	if (ent->target)
	{
		ent->think = bolt_link;
		ent->nextthink = level.time + 100;
		return;
	}

	trap_LinkEntity( ent );
}


//--------------------------------------------------
/*QUAKED fx_transporter (0 0 1) (-8 -8 -8) (8 8 8)
Emits transporter pad effect at the specified point. just rest it flush on top of the pad. 

*/

void transporter_link( gentity_t *ent )
{
	G_AddEvent( ent, EV_FX_TRANSPORTER_PAD, 0 );
}

//------------------------------------------
void SP_fx_transporter(gentity_t *ent)
{
	SnapVector(ent->s.origin);
	VectorCopy( ent->s.origin, ent->s.pos.trBase );

	ent->think = transporter_link; 
	ent->nextthink = level.time + 2000;

	trap_LinkEntity( ent );
}



/*QUAKED fx_drip (0 0 1) (-8 -8 -8) (8 8 8) STARTOFF

  "damage" -- type of drips. 0 = water, 1 = oil, 2 = green
  "random" -- (0...1) degree of drippiness. 0 = one drip, 1 = Niagara Falls
*/

//------------------------------------------
void drip_think( gentity_t *ent )
{
	G_AddEvent( ent, EV_FX_DRIP, 0 );
	ent->nextthink = level.time + 10000; // send a refresh message every 10 seconds
}

//------------------------------------------
void SP_fx_drip( gentity_t	*ent )
{
	ent->s.time2 = ent->damage; 

	ent->s.angles2[0] = ent->random; 

	SnapVector(ent->s.origin);
	VectorCopy( ent->s.origin, ent->s.pos.trBase );

	ent->think = drip_think; 
	ent->nextthink = level.time + 1000;

	trap_LinkEntity( ent );
}

