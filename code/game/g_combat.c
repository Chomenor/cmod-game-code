// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_combat.c

#include "g_local.h"

/*
============
AddScore

Adds score to both the client and his team
============
*/
void AddScore( gentity_t *ent, int score ) {
	if ( !ent )
	{
		return;
	}
	if ( !ent->client ) {
		return;
	}
	// no scoring during pre-match warmup
	if ( level.warmupTime ) {
		return;
	}
	if ( modfn.AdjustGeneralConstant( GC_DISABLE_ADDSCORE, 0 ) ) {
		return;
	}

	ent->client->ps.persistant[PERS_SCORE] += score;

	if ( g_gametype.integer == GT_TEAM ) {
		// this isn't capture score
		level.teamScores[ent->client->ps.persistant[PERS_TEAM]] += score;
	}

	CalculateRanks();
}

/*
============
(ModFN) CanItemBeDropped

Check if item can be tossed on death/disconnect.
============
*/
qboolean ModFNDefault_CanItemBeDropped( gitem_t *item, int clientNum ) {
	if ( item->giType == IT_POWERUP && g_gametype.integer == GT_TEAM ) {
		// no powerup drops in THM mode
		return qfalse;
	}

	if ( item->giType == IT_HOLDABLE ) {
		// no holdable drops in unmodded game
		return qfalse;
	}

	if ( item->giType == IT_WEAPON && item->giTag == WP_PHASER ) {
		// no phaser drops in unmodded game
		return qfalse;
	}

	return qtrue;
}

/*
=================
TossClientItems

Toss the weapon and powerups for the killed player
=================
*/
void TossClientItems( gentity_t *self ) {
	gitem_t		*item;
	int			weapon;
	float		angle;
	int			i;
	gentity_t	*drop;

	// drop the weapon if not a phaser
	weapon = self->s.weapon;

	// make a special check to see if they are changing to a new
	// weapon that isn't the mg or gauntlet.  Without this, a client
	// can pick up a weapon, be killed, and not drop the weapon because
	// their weapon change hasn't completed yet and they are still holding the MG.
	if ( weapon == WP_PHASER )
	{
		if ( self->client->ps.weaponstate == WEAPON_DROPPING ) {
			weapon = self->client->pers.cmd.weapon;
		}
		if ( !( self->client->ps.stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
			weapon = WP_NONE;
		}
	}

	if ( weapon >= WP_PHASER && self->client->ps.ammo[ weapon ] ) {
		// find the item type for this weapon
		item = BG_FindItemForWeapon( weapon );

		// spawn the item
		if ( modfn.CanItemBeDropped( item, self - g_entities ) ) {
			Drop_Item( self, item, 0 );
		}
	}

	// drop all the powerups if not in teamplay
	angle = 45;
	for ( i = 1 ; i < PW_NUM_POWERUPS ; i++ ) {
		if ( self->client->ps.powerups[ i ] > level.time ) {
			item = BG_FindItemForPowerup( i );
			if ( !item || !modfn.CanItemBeDropped( item, self - g_entities ) ) {
				continue;
			}
			drop = Drop_Item( self, item, angle );
			// decide how many seconds it has left
			drop->count = ( self->client->ps.powerups[ i ] - level.time ) / 1000;
			if ( drop->count < 1 ) {
				drop->count = 1;
			}
			angle += 45;
		}
	}

	// holdable drops are normally disabled in CanItemBeDropped, but can be enabled by mods
	for ( i = 1 ; i < HI_NUM_HOLDABLE ; i++ ) {
		item = BG_FindItemForHoldable( i );
		if ( self->client->ps.stats[STAT_HOLDABLE_ITEM] == item - bg_itemlist &&
				modfn.CanItemBeDropped( item, self - g_entities ) ) {
			Drop_Item( self, item, angle );
			angle += 45;
		}
	}
}


/*
==================
LookAtKiller
==================
*/
void LookAtKiller( gentity_t *self, gentity_t *inflictor, gentity_t *attacker ) {
	vec3_t		dir;

	if ( attacker && attacker != self ) {
		VectorSubtract (attacker->s.pos.trBase, self->s.pos.trBase, dir);
	} else if ( inflictor && inflictor != self ) {
		VectorSubtract (inflictor->s.pos.trBase, self->s.pos.trBase, dir);
	} else {
		self->client->ps.stats[STAT_DEAD_YAW] = self->s.angles[YAW];
		return;
	}

	self->client->ps.stats[STAT_DEAD_YAW] = vectoyaw ( dir );
}

/*
==================
GibEntity
==================
*/
static void GibEntity( gentity_t *self, int killer ) {
	// Start Disintegration
	G_AddEvent( self, EV_EXPLODESHELL, killer );
	self->takedamage = qfalse;
	self->s.eType = ET_INVISIBLE;
	self->r.contents = 0;
}

/*
==================
body_die
==================
*/
void body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	if ( self->health > GIB_HEALTH ) {
		return;
	}
	GibEntity( self, 0 );
}


// these are just for logging, the client prints its own messages
char	*modNames[MOD_MAX] = {
	"MOD_UNKNOWN",

	"MOD_WATER",
	"MOD_SLIME",
	"MOD_LAVA",
	"MOD_CRUSH",
	"MOD_TELEFRAG",
	"MOD_FALLING",
	"MOD_SUICIDE",
	"MOD_TARGET_LASER",
	"MOD_TRIGGER_HURT",

	// Trek weapons
	"MOD_PHASER",
	"MOD_PHASER_ALT",
	"MOD_CRIFLE",
	"MOD_CRIFLE_SPLASH",
	"MOD_CRIFLE_ALT",
	"MOD_CRIFLE_ALT_SPLASH",
	"MOD_IMOD",
	"MOD_IMOD_ALT",
	"MOD_SCAVENGER",
	"MOD_SCAVENGER_ALT",
	"MOD_SCAVENGER_ALT_SPLASH",
	"MOD_STASIS",
	"MOD_STASIS_ALT",
	"MOD_GRENADE",
	"MOD_GRENADE_ALT",
	"MOD_GRENADE_SPLASH",
	"MOD_GRENADE_ALT_SPLASH",
	"MOD_TETRYON",
	"MOD_TETRYON_ALT",
	"MOD_DREADNOUGHT",
	"MOD_DREADNOUGHT_ALT",
	"MOD_QUANTUM",
	"MOD_QUANTUM_SPLASH",
	"MOD_QUANTUM_ALT",
	"MOD_QUANTUM_ALT_SPLASH",

	"MOD_DETPACK",
	"MOD_SEEKER",

//expansion pack
	"MOD_KNOCKOUT",
	"MOD_ASSIMILATE",
	"MOD_BORG",
	"MOD_BORG_ALT",

	"MOD_RESPAWN",
	"MOD_EXPLOSION",
};//must be kept up to date with bg_public, meansOfDeath_t

/*
==================
(ModFN) PlayerDeathDiscardDetpack

Remove any placed detpack owned by a player who was just killed.
==================
*/
void ModFNDefault_PlayerDeathDiscardDetpack( int clientNum ) {
	gentity_t *self = &g_entities[clientNum];
	gentity_t *detpack = NULL;
	const char *classname = BG_FindClassnameForHoldable(HI_DETPACK);

	if (classname)
	{
		while ((detpack = G_Find (detpack, FOFS(classname), classname)) != NULL)
		{
			if (detpack->parent == self)
			{
				detpack->think = DetonateDetpack;		// Detonate next think.
				detpack->nextthink = level.time;
			}
		}
	}
}

/*
==================
(ModFN) PlayerDeathEffect

Play some weapon-specific effects when player is killed.
==================
*/
void ModFNDefault_PlayerDeathEffect( gentity_t *self, gentity_t *inflictor, gentity_t *attacker,
		int contents, int killer, int meansOfDeath ) {
	// check if we are in a NODROP Zone and died from a TRIGGER HURT
	//  if so, we assume that this resulted from a fall to a "bottomless pit" and
	//  treat it differently...
	if ( ( contents & CONTENTS_NODROP ) && meansOfDeath == MOD_TRIGGER_HURT && killer == ENTITYNUM_WORLD ) {
		self->takedamage = qfalse;
		return;
	}

	switch(meansOfDeath)
	{
	case MOD_CRIFLE_ALT:
		G_AddEvent( self, EV_DISINTEGRATION, killer );
		self->client->ps.powerups[PW_DISINTEGRATE] = level.time + 100000;
		VectorClear( self->client->ps.velocity );
		self->takedamage = qfalse;
		self->r.contents = 0;
		break;
	case MOD_QUANTUM_ALT:
		G_AddEvent( self, EV_DISINTEGRATION2, killer );
		self->client->ps.powerups[PW_EXPLODE] = level.time + 100000;
		VectorClear( self->client->ps.velocity );
		self->takedamage = qfalse;
		self->r.contents = 0;
		break;
	case MOD_SCAVENGER_ALT:
	case MOD_SCAVENGER_ALT_SPLASH:
	case MOD_GRENADE:
	case MOD_GRENADE_ALT:
	case MOD_GRENADE_SPLASH:
	case MOD_GRENADE_ALT_SPLASH:
	case MOD_QUANTUM:
	case MOD_QUANTUM_SPLASH:
	case MOD_QUANTUM_ALT_SPLASH:
	case MOD_DETPACK:
		G_AddEvent( self, EV_EXPLODESHELL, killer );
		self->client->ps.powerups[PW_EXPLODE] = level.time + 100000;
		VectorClear( self->client->ps.velocity );
		self->takedamage = qfalse;
		self->r.contents = 0;
		break;
	case MOD_DREADNOUGHT:
	case MOD_DREADNOUGHT_ALT:
		G_AddEvent( self, EV_ARCWELD_DISINT, killer);
		self->client->ps.powerups[PW_ARCWELD_DISINT] = level.time + 100000;
		VectorClear( self->client->ps.velocity );
		self->takedamage = qfalse;
		self->r.contents = 0;
		break;

	default:
		G_AddEvent( self, irandom(EV_DEATH1, EV_DEATH3), killer );
		break;
	}
}

/*
==================
player_die
==================
*/
void player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	gentity_t	*ent;
	int			anim;
	int			contents;
	int			killer;
	char		*killerName, *obit;
	static		int deathNum;
	int			awardPoints = 0;	// Amount of points to give/take from attacker (or self if attacker is non-client).

	if ( self->client->ps.pm_type == PM_DEAD ) {
		return;
	}

	if ( level.intermissiontime ) {
		return;
	}

	self->client->ps.pm_type = PM_DEAD;
	if ( self->health > 0 ) {
		self->health = 0;
	}
	//need to copy health here because pm_type was getting reset to PM_NORMAL if ClientThink_real was called before the STAT_HEALTH was updated
	self->client->ps.stats[STAT_HEALTH] = self->health;

	contents = trap_PointContents( self->r.currentOrigin, -1 );

	// if already dropped a detpack, blow it up
	modfn.PlayerDeathDiscardDetpack( self - g_entities );

	if ( attacker ) {
		killer = attacker->s.number;
		if ( attacker->client ) {
			killerName = attacker->client->pers.netname;
		} else {
			killerName = "<non-client>";
		}
	} else {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( killer < 0 || killer >= MAX_CLIENTS ) {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( meansOfDeath < 0 || meansOfDeath >= sizeof( modNames ) / sizeof( modNames[0] ) ) {
		obit = "<bad obituary>";
	} else {
		obit = modNames[meansOfDeath];
	}

	G_LogPrintf("Kill: %i %i %i: %s killed %s by %s\n",
		killer, self->s.number, meansOfDeath, killerName,
		self->client->pers.netname, obit );

	G_LogWeaponKill(killer, meansOfDeath);
	G_LogWeaponDeath(self->s.number, self->s.weapon);
	if (attacker && attacker->client && attacker->inuse)
	{
		G_LogWeaponFrag(killer, self->s.number);
	}

	if ( meansOfDeath != MOD_RESPAWN )
	{
		// broadcast the death event to everyone
		ent = G_TempEntity( self->r.currentOrigin, EV_OBITUARY );
		ent->s.eventParm = meansOfDeath;
		ent->s.otherEntityNum = self->s.number;
		ent->s.otherEntityNum2 = killer;
		ent->r.svFlags = SVF_BROADCAST;	// send to everyone
	}

	self->enemy = attacker;

	self->client->ps.persistant[PERS_KILLED]++;
	if (self == attacker)
	{
		self->client->pers.teamState.suicides++;
	}

	if (attacker && attacker->client)
	{
		if ( attacker == self || OnSameTeam (self, attacker ) )
		{
			if ( meansOfDeath != MOD_RESPAWN )
			{//just changing class
				awardPoints = -1;
			}
		}
		else
		{
			attacker->client->pers.teamState.frags++;
			awardPoints = 1;

			// check for two kills in a short amount of time
			// if this is close enough to the last kill, give a reward sound
			if ( level.time - attacker->client->lastKillTime < CARNAGE_REWARD_TIME ) {
				attacker->client->ps.persistant[PERS_REWARD_COUNT]++;
				attacker->client->ps.persistant[PERS_REWARD] = REWARD_EXCELLENT;
				attacker->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
				// add the sprite over the player's head
				attacker->client->ps.eFlags &= ~EF_AWARD_MASK;
				attacker->client->ps.eFlags |= EF_AWARD_EXCELLENT;
				attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;
			}

			// Check to see if the player is on a streak.
			attacker->client->streakCount++;
			// Only send awards on exact streak counts.
			switch(attacker->client->streakCount)
			{
			case STREAK_ACE:
				attacker->client->ps.persistant[PERS_REWARD_COUNT]++;
				attacker->client->ps.persistant[PERS_REWARD] = REWARD_STREAK;
				// add the sprite over the player's head
				attacker->client->ps.eFlags &= ~EF_AWARD_MASK;
				attacker->client->ps.eFlags |= EF_AWARD_ACE;
				attacker->client->rewardTime = level.time + REWARD_STREAK_SPRITE_TIME;
				break;
			case STREAK_EXPERT:
				attacker->client->ps.persistant[PERS_REWARD_COUNT]++;
				attacker->client->ps.persistant[PERS_REWARD] = REWARD_STREAK;
				// add the sprite over the player's head
				attacker->client->ps.eFlags &= ~EF_AWARD_MASK;
				attacker->client->ps.eFlags |= EF_AWARD_EXPERT;
				attacker->client->rewardTime = level.time + REWARD_STREAK_SPRITE_TIME;
				break;
			case STREAK_MASTER:
				attacker->client->ps.persistant[PERS_REWARD_COUNT]++;
				attacker->client->ps.persistant[PERS_REWARD] = REWARD_STREAK;
				// add the sprite over the player's head
				attacker->client->ps.eFlags &= ~EF_AWARD_MASK;
				attacker->client->ps.eFlags |= EF_AWARD_MASTER;
				attacker->client->rewardTime = level.time + REWARD_STREAK_SPRITE_TIME;
				break;
			case STREAK_CHAMPION:
				attacker->client->ps.persistant[PERS_REWARD_COUNT]++;
				attacker->client->ps.persistant[PERS_REWARD] = REWARD_STREAK;
				// add the sprite over the player's head
				attacker->client->ps.eFlags &= ~EF_AWARD_MASK;
				attacker->client->ps.eFlags |= EF_AWARD_CHAMPION;
				attacker->client->rewardTime = level.time + REWARD_STREAK_SPRITE_TIME;
				break;
			default:
				// Do nothing if not exact values.
				break;
			}

			// Check to see if the max streak should be raised.
			if (attacker->client->streakCount > attacker->client->ps.persistant[PERS_STREAK_COUNT])
			{
				attacker->client->ps.persistant[PERS_STREAK_COUNT] = attacker->client->streakCount;
			}

			if (!level.firstStrike)
			{	// There hasn't yet been a first strike.
				level.firstStrike = qtrue;
				attacker->client->ps.persistant[PERS_REWARD_COUNT]++;
				attacker->client->ps.persistant[PERS_REWARD] = REWARD_FIRST_STRIKE;
				// add the sprite over the player's head
				attacker->client->ps.eFlags &= ~EF_AWARD_MASK;
				attacker->client->ps.eFlags |= EF_AWARD_FIRSTSTRIKE;
				attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;
			}

			attacker->client->lastKillTime = level.time;
		}
	}
	else
	{
		if ( meansOfDeath != MOD_RESPAWN )
		{//not just changing class
			awardPoints = -1;
		}
	}

	// Add team bonuses
	Team_FragBonuses(self, inflictor, attacker);

	// if client is in a nodrop area, don't drop anything (but return CTF flags!)
	if ( !( contents & CONTENTS_NODROP ) && meansOfDeath != MOD_RESPAWN &&
			( meansOfDeath != MOD_SUICIDE || modfn.AdjustGeneralConstant( GC_TOSS_ITEMS_ON_SUICIDE, 0 ) ) )
	{
		TossClientItems( self );
	}
	else
	{//suiciding and respawning returns the flag
		if ( self->client->ps.powerups[PW_REDFLAG] )
		{
			Team_ReturnFlag(TEAM_RED);
		}
		else if ( self->client->ps.powerups[PW_BLUEFLAG] )
		{
			Team_ReturnFlag(TEAM_BLUE);
		}
	}

	// wait until the end of the server frame to send a score command update
	// this way if two players die at the same time, they both will receive the correct scores
	self->client->scoreUpdatePending = qtrue;

	self->takedamage = qtrue;	// can still be gibbed

	self->s.weapon = WP_NONE;
	self->s.powerups = 0;
	self->r.contents = CONTENTS_CORPSE;

	self->s.angles[0] = 0;
	self->s.angles[2] = 0;
	LookAtKiller (self, inflictor, attacker);

	VectorCopy( self->s.angles, self->client->ps.viewangles );

	self->s.loopSound = 0;

	self->r.maxs[2] = -8;

	// don't allow respawn until the death anim is done
	// g_forcerespawn may force spawning at some later time
	self->client->respawnKilledTime = level.time;

	// remove powerups
	memset( self->client->ps.powerups, 0, sizeof(self->client->ps.powerups) );

	// never gib in a nodrop
/*	if ( self->health <= GIB_HEALTH && !(contents & CONTENTS_NODROP) ) {
		// gib death
		GibEntity( self, killer );
	} else
*/

	// We always want to see the body for special animations, so make sure not to gib right away:
	if (meansOfDeath==MOD_CRIFLE_ALT ||
		meansOfDeath==MOD_DETPACK ||
		meansOfDeath==MOD_QUANTUM_ALT ||
		meansOfDeath==MOD_DREADNOUGHT_ALT)
	{	self->health=0;}

	switch ( deathNum ) {
	case 0:
		anim = BOTH_DEATH1;
		break;
	case 1:
		anim = BOTH_DEATH2;
		break;
	case 2:
	default:
		anim = BOTH_DEATH3;
		break;
	}
	self->client->ps.legsAnim =
		( ( self->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
	self->client->ps.torsoAnim =
		( ( self->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;

	self->die = body_die;

	modfn.PlayerDeathEffect( self, inflictor, attacker, contents, killer, meansOfDeath );

	// globally cycle through the different death animations
	deathNum = ( deathNum + 1 ) % 3;

	if ( meansOfDeath != MOD_RESPAWN ) {
		modfn.PostPlayerDie( self, inflictor, attacker, meansOfDeath, &awardPoints );
	}

	if ( awardPoints ) {
		if ( attacker && attacker->client ) {
			AddScore( attacker, awardPoints );
		} else {
			AddScore( self, awardPoints );
		}
	}

	CalculateRanks();

	trap_LinkEntity (self);
}


/*
================
CheckArmor
================
*/
int CheckArmor (gentity_t *ent, int damage, int dflags)
{
	gclient_t	*client;
	int			save;
	int			count;
	float		protectionFactor = ARMOR_PROTECTION;

	if (!damage)
		return 0;

	client = ent->client;

	if (!client)
		return 0;

	if (dflags & DAMAGE_NO_ARMOR)
		return 0;

	// armor
	if (dflags & DAMAGE_ARMOR_PIERCING)
	{
		protectionFactor = PIERCED_ARMOR_PROTECTION;
	}

	count = client->ps.stats[STAT_ARMOR];
	save = ceil( damage * protectionFactor );
	if (save >= count)
		save = count;

	if (!save)
		return 0;

	client->ps.stats[STAT_ARMOR] -= save;

	return save;
}


/*
============
(ModFN) CheckBorgAdapt

Returns whether borg adaptive shields have blocked damage.
Also sets PW_BORG_ADAPT to play effect on target if needed.
============
*/
qboolean ModFNDefault_CheckBorgAdapt( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	return qfalse;
}

/*
============
(ModFN) KnockbackMass

Returns mass value for knockback calculations.
============
*/
float ModFNDefault_KnockbackMass( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	return 200;
}

/*
============
(ModFN) ApplyKnockback

Apply knockback to players when taking damage.
============
*/
void ModFNDefault_ApplyKnockback( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod, float knockback ) {
	vec3_t	kvel;
	float	mass = modfn.KnockbackMass( targ, inflictor, attacker, dir, point, damage, dflags, mod );

	// flying targets get pushed around a lot more.
	if (targ->client->ps.powerups[PW_FLIGHT])
	{
		mass *= 0.375;
	}

	VectorScale (dir, g_knockback.value * (float)knockback / mass, kvel);
	VectorAdd (targ->client->ps.velocity, kvel, targ->client->ps.velocity);

	// set the timer so that the other client can't cancel
	// out the movement immediately
	if ( !targ->client->ps.pm_time ) {
		int		t;

		t = knockback * 2;
		if ( t < 50 ) {
			t = 50;
		}
		if ( t > 200 ) {
			t = 200;
		}
		targ->client->ps.pm_time = t;
		targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	}
}

/*
============
(ModFN) GetDamageMult

Returns effective g_dmgmult value.
Called separately for both damage and knockback calculation, differentiated by knockbackMode parameter.
============
*/
float ModFNDefault_GetDamageMult( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod, qboolean knockbackMode ) {
	return g_dmgmult.value;
}

/*
============
(ModFN) Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

inflictor, attacker, dir, and point can be NULL for environmental effects

dflags		these flags are used to control how G_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/
void ModFNDefault_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
				vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	gclient_t	*client = targ->client;
	int			take;
	int			asave;
	int			knockback;
	qboolean	bFriend = (targ && attacker)?OnSameTeam( targ, attacker ):qfalse;
	gentity_t	*evEnt;
	qboolean	adapted;

	if (!targ->takedamage) {
		return;
	}

	// the intermission has already been qualified for, so don't
	// allow any extra scoring
	if ( level.intermissionQueued ) {
		return;
	}

	if ( !inflictor ) {
		inflictor = &g_entities[ENTITYNUM_WORLD];
	}
	if ( !attacker ) {
		attacker = &g_entities[ENTITYNUM_WORLD];
	}

	// allow mods to change damage flags
	dflags = modfn.ModifyDamageFlags( targ, inflictor, attacker, dir, point, damage, dflags, mod );

	// shootable doors / buttons don't actually have any health
	if ( targ->s.eType == ET_MOVER && Q_stricmp("func_breakable", targ->classname) != 0 && Q_stricmp("misc_model_breakable", targ->classname) != 0)
	{
		if ( targ->use && targ->moverState == MOVER_POS1 )
		{
			targ->use( targ, inflictor, attacker );
		}
		return;
	}

	if ( dflags & DAMAGE_TRIGGERS_ONLY ) {
		return;
	}

	adapted = modfn.CheckBorgAdapt( targ, inflictor, attacker, dir, point, damage, dflags, mod );

	// multiply damage times dmgmult
	knockback = ( dflags & DAMAGE_NO_KNOCKBACK ) ? 0 :
			tonextint( damage * modfn.GetDamageMult( targ, inflictor, attacker, dir, point, damage, dflags, mod, qtrue ) );
	damage = tonextint( damage * modfn.GetDamageMult( targ, inflictor, attacker, dir, point, damage, dflags, mod, qfalse ) );

	// reduce damage by the attacker's handicap value
	// unless they are rocket jumping
	if ( attacker->client && attacker != targ ) {
		int handicap = modfn.EffectiveHandicap( attacker->client - level.clients, EH_DAMAGE );
		knockback = knockback * handicap / 100;
		damage = damage * handicap / 100;
	}

	if ( client )
	{
		if ( client->noclip ) {
			return;
		}

		// kef -- still gotta telefrag people
		if (MOD_TELEFRAG != mod)
		{
			if (targ->client->ps.introTime > level.time)
			{	// Can't be hurt when in holodeck intro.
				return;
			}

			if (targ->client->ps.powerups[PW_GHOST] >= level.time)
			{	// Can't be hurt when ghosted.
				return;
			}
		}
	}

	if ( !dir ) {
		dflags |= DAMAGE_NO_KNOCKBACK;
	} else {
		VectorNormalize(dir);
	}

	if ( knockback > 200 && !( dflags & DAMAGE_NO_KNOCKBACK_CAP ) ) {
		knockback = 200;
	}
	if ( targ->flags & FL_NO_KNOCKBACK ) {
		knockback = 0;
	}
	if ( dflags & DAMAGE_NO_KNOCKBACK ) {
		knockback = 0;
	}

	knockback = floor( knockback*g_dmgmult.value ) ;

	// figure momentum add, even if the damage won't be taken
	if ( knockback && targ->client )
	{
		//if it's non-radius damage knockback from a teammate, don't do it if the damage won't be taken
		if ( (dflags&DAMAGE_ALL_TEAMS) || (dflags&DAMAGE_RADIUS) || g_friendlyFire.integer || !attacker->client || !OnSameTeam (targ, attacker) )
		{
			modfn.ApplyKnockback( targ, inflictor, attacker, dir, point, damage, dflags, mod, knockback );
		}
	}

	// if TF_NO_FRIENDLY_FIRE is set, don't do damage to the target
	// if the attacker was on the same team
	// check for completely getting out of the damage
	if ( !(dflags & DAMAGE_NO_PROTECTION) ) {
		if ( !(dflags&DAMAGE_ALL_TEAMS) && mod != MOD_TELEFRAG && mod != MOD_DETPACK && targ != attacker && OnSameTeam (targ, attacker)  )
		{
			if ( attacker->client && targ->client )
			{//this only matters between clients
				if ( !g_friendlyFire.integer )
				{//friendly fire is not allowed
					return;
				}
			}
			else
			{//team damage between non-clients is never legal
				return;
			}
		}

		// check for godmode
		if ( targ->flags & FL_GODMODE ) {
			return;
		}
	}

	// kef -- still need to telefrag invulnerable people
	if ( MOD_TELEFRAG != mod )
	{
		// battlesuit protects from all damage...
		if ( client && ( client->ps.powerups[PW_BATTLESUIT] || adapted ))
		{	// EXCEPT DAMAGE_NO_INVULNERABILITY, like the IMOD
			if ( dflags & DAMAGE_NO_INVULNERABILITY )
			{	// Do only half damage if he has the battlesuit, and that's just because I'm in a good mood...
				damage *= 0.5;
			}
			else
			{
				G_AddEvent( targ, EV_POWERUP_BATTLESUIT, 0 );
				return;
			}
		}
	}

	// Disable damage if g_dmgmult is set to 0, rather than having all damages converted to 1.
	// Allows admins to disable all damage, similar to Gladiator mod.
	if ( g_dmgmult.value <= 0.0f && mod != MOD_TELEFRAG ) {
		return;
	}

	// always give half damage if hurting self
	// calculated after knockback, so rocket jumping works
	if ( targ == attacker && !( dflags & DAMAGE_NO_HALF_SELF ) ) {
		damage *= 0.5;
	}

	if ( damage < 1 ) {
		damage = 1;
	}
	take = damage;

	// save some from armor
	asave = CheckArmor (targ, take, dflags);
	take -= asave;

	if ( g_debugDamage.integer ) {
		G_Printf( "%i: client:%i health:%i damage:%i armor:%i\n", level.time, targ->s.number,
			targ->health, take, asave );
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if ( client && !( dflags & DAMAGE_LIMIT_EFFECTS ) ) {
		if ( attacker ) {
			client->ps.persistant[PERS_ATTACKER] = attacker->s.number;
		} else {
			client->ps.persistant[PERS_ATTACKER] = ENTITYNUM_WORLD;
		}
		client->damage_armor += asave;
		client->damage_blood += take;
		if ( dir ) {
			VectorCopy ( dir, client->damage_from );
			client->damage_fromWorld = qfalse;
		} else {
			VectorCopy ( targ->r.currentOrigin, client->damage_from );
			client->damage_fromWorld = qtrue;
		}
	}

	// See if it's the player hurting the emeny flag carrier
	Team_CheckHurtCarrier(targ, attacker);

	if (targ->client) {
		// set the last client who damaged the target
		targ->client->lasthurt_client = attacker->s.number;
		targ->client->lasthurt_mod = mod;

		// if target's shields (armor) took dmg and the dmg was armor-piercing, display the half-shields effect,
		//if non-armor-piercing display full shields
		if (asave && !( dflags & DAMAGE_LIMIT_EFFECTS ))
		{
			evEnt = G_TempEntity(targ->client->ps.origin, EV_SHIELD_HIT);
			VectorClear(evEnt->s.pos.trBase);	// save a bit of bandwidth
			evEnt->s.otherEntityNum = targ->s.number;
			evEnt->s.eventParm = DirToByte(dir);
			evEnt->s.time2=asave;

			if (attacker->client && !bFriend)
			{
				attacker->client->ps.persistant[PERS_SHIELDS] += asave;
			}
		}
	}

	// do the damage
	if (take && !( dflags & DAMAGE_NO_HEALTH_DAMAGE ))
	{
		// add to the attacker's hit counter
		if ( (MOD_TELEFRAG != mod) && attacker->client && targ != attacker && targ->health > 0 )
		{//don't telefrag since damage would wrap when sent as a short and the client would think it's a team dmg.
			if (bFriend)
			{
				attacker->client->ps.persistant[PERS_HITS] -= damage;
			}
			else if (targ->classname && strcmp(targ->classname, "holdable_shield") // no stupid hit noise when players shoot a shield -- dpk
					&& strcmp(targ->classname, "holdable_detpack")) // or the detpack either
			{
				attacker->client->ps.persistant[PERS_HITS] += damage;
			}

			if ( targ->client && ( targ->client->ps.eFlags & EF_TALK ) && modfn.AdjustGeneralConstant( GC_CHAT_HIT_WARNING, 0 ) ) {
				// Use alternative method to play chat warning, since cgame doesn't support normal team damage sound in ffa.
				// NOTE: Possibly could be done with a playerstate event to avoid entity limit risks?
				gentity_t *temp = G_TempEntity( vec3_origin, EV_GLOBAL_SOUND );
				temp->s.eventParm = G_SoundIndex( "sound/feedback/hit_teammate.wav" );
				temp->r.svFlags |= SVF_BROADCAST | SVF_SINGLECLIENT;
				temp->r.singleClient = attacker - g_entities;
			}
		}

		targ->health = targ->health - take;
		if ( targ->client )
		{
			targ->client->ps.stats[STAT_HEALTH] = targ->health;
			// kef -- pain effect
			// pjl -- if there was armor involved, the half-shield effect was shown in the above block.
			if (!asave)
			{
				targ->client->ps.powerups[PW_OUCH] = level.time + 500;
				// kef -- there absolutely MUST be a better way to do this. cleaner, at least.
				//display the full screen "ouch" effect to the player
			}
		}

		if ( targ->health <= 0 ) {
			if ( client )
				targ->flags |= FL_NO_KNOCKBACK;

			if (targ->health < -999)
				targ->health = -999;

			targ->enemy = attacker;
			targ->die (targ, inflictor, attacker, take, mod);
			return;
		} else if ( targ->pain ) {
			targ->pain (targ, attacker, take);
		}

		G_LogWeaponDamage(attacker->s.number, mod, take);
	}

}

/*
============
G_Damage

Wrapper to Damage modfn.
============
*/
void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
				vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	modfn.Damage( targ, inflictor, attacker, dir, point, damage, dflags, mod );
}


/*
============
CanDamage

Returns qtrue if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage (gentity_t *targ, vec3_t origin) {
	vec3_t	dest;
	trace_t	tr;
	vec3_t	midpoint;

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	VectorAdd (targ->r.absmin, targ->r.absmax, midpoint);
	VectorScale (midpoint, 0.5, midpoint);

	VectorCopy (midpoint, dest);
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;

	// this should probably check in the plane of projection,
	// rather than in world coordinate, and also include Z
	VectorCopy (midpoint, dest);
	dest[0] += 15.0;
	dest[1] += 15.0;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;


	return qfalse;
}


extern void tripwireThink ( gentity_t *ent );

/*
============
(ModFN) RadiusDamage
============
*/
qboolean ModFNDefault_RadiusDamage( vec3_t origin, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int dflags, int mod ) {
	float		points, dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	vec3_t		dir;
	int			i, e;
	qboolean	hitClient = qfalse;

	if ( radius < 1 ) {
		radius = 1;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;
		if ( ignore != NULL && ignore->parent != NULL && ent->parent == ignore->parent )
		{
			if ( ignore->think == tripwireThink && ent->think == tripwireThink )
			{//your own tripwires do not fire off other tripwires of yours.
				continue;
			}
		}

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( origin[i] < ent->r.absmin[i] ) {
				v[i] = ent->r.absmin[i] - origin[i];
			} else if ( origin[i] > ent->r.absmax[i] ) {
				v[i] = origin[i] - ent->r.absmax[i];
			} else {
				v[i] = 0;
			}
		}

		dist = VectorLength( v );
		if ( dist >= radius ) {
			continue;
		}

		points = damage * ( 1.0 - dist / radius );

		if( !CanDamage (ent, origin) ) {
			//no LOS to ent
			if ( !(dflags & DAMAGE_HALF_NOTLOS) ) {
				//not allowed to do damage without LOS
				continue;
			} else {
				//do 1/2 damage if no LOS but within rad
				points *= 0.5;
			}
		}

		if( LogAccuracyHit( ent, attacker ) ) {
			hitClient = qtrue;
		}
		VectorSubtract (ent->r.currentOrigin, origin, dir);
		// push the center of mass higher than the origin so players
		// get knocked into the air more
		dir[2] += 24;
		G_Damage (ent, NULL, attacker, dir, origin, (int)points, dflags|DAMAGE_RADIUS, mod);
	}

	return hitClient;
}

/*
============
G_RadiusDamage
============
*/
extern void tripwireThink ( gentity_t *ent );
qboolean G_RadiusDamage ( vec3_t origin, gentity_t *attacker, float damage, float radius,
					gentity_t *ignore, int dflags, int mod) {
	return modfn.RadiusDamage( origin, attacker, damage, radius, ignore, dflags, mod );
}
