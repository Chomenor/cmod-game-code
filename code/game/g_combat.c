// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_combat.c

#include "g_local.h"

extern int	actionHeroClientNum;
extern int	borgQueenClientNum;
extern int	numKilled;
extern void G_RandomActionHero( int ignoreClientNum );
extern void SetClass( gentity_t *ent, char *s, char *teamName );


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
	ent->client->ps.persistant[PERS_SCORE] += score;
	//don't add score to team score during elimination
	if (g_gametype.integer == GT_TEAM && g_pModElimination.integer == 0 )
	{//this isn't capture score
		level.teamScores[ ent->client->ps.persistant[PERS_TEAM] ] += score;
	}
	CalculateRanks( qfalse );
}

/*
============
SetScore

============
*/
void SetScore( gentity_t *ent, int score ) {
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
	ent->client->ps.persistant[PERS_SCORE] = score;
	CalculateRanks( qfalse );
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

	if ( g_pModDisintegration.integer == 0 && g_pModSpecialties.integer == 0 && self->client->sess.sessionClass != PC_BORG )
	{//not in playerclass game mode and not in disintegration mode (okay to drop weap)
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

		if ( weapon > WP_PHASER && self->client->ps.ammo[ weapon ] ) {
			// find the item type for this weapon
			item = BG_FindItemForWeapon( weapon );

			// spawn the item
			Drop_Item( self, item, 0 );
		}
	}

	// drop all the powerups if not in teamplay
	if ( g_gametype.integer != GT_TEAM ) {
		angle = 45;
		for ( i = 1 ; i < PW_NUM_POWERUPS ; i++ ) {
			if ( self->client->ps.powerups[ i ] > level.time ) {
				item = BG_FindItemForPowerup( i );
				if ( !item ) {
					continue;
				}
				if ( g_pModSpecialties.integer != 0 || self->client->sess.sessionClass == PC_BORG )
				{//in playerclass game mode
					if ( item->giType != IT_TEAM )
					{//only drop the flag
						continue;
					}
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
	}
}


/*
==================
LookAtKiller
==================
*/
void LookAtKiller( gentity_t *self, gentity_t *inflictor, gentity_t *attacker ) {
	vec3_t		dir;
	vec3_t		angles;

	if ( attacker && attacker != self ) {
		VectorSubtract (attacker->s.pos.trBase, self->s.pos.trBase, dir);
	} else if ( inflictor && inflictor != self ) {
		VectorSubtract (inflictor->s.pos.trBase, self->s.pos.trBase, dir);
	} else {
		self->client->ps.stats[STAT_DEAD_YAW] = self->s.angles[YAW];
		return;
	}

	self->client->ps.stats[STAT_DEAD_YAW] = vectoyaw ( dir );

	angles[YAW] = vectoyaw ( dir );
	angles[PITCH] = 0; 
	angles[ROLL] = 0;
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
	"MOD_SEEKER"

//expansion pack
	"MOD_KNOCKOUT",
	"MOD_ASSIMILATE",
	"MOD_BORG",
	"MOD_BORG_ALT",

	"MOD_RESPAWN",
	"MOD_EXPLOSION",
};//must be kept up to date with bg_public, meansOfDeath_t

extern void DetonateDetpack(gentity_t *ent);

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
	int			i;
	char		*killerName, *obit;
	gentity_t	*detpack = NULL;
	char		*classname = NULL;
	int			BottomlessPitDeath;
	static		int deathNum;



	if ( self->client->ps.pm_type == PM_DEAD ) {
		return;
	}

	if ( level.intermissiontime ) {
		return;
	}

	self->client->ps.pm_type = PM_DEAD;
	//need to copy health here because pm_type was getting reset to PM_NORMAL if ClientThink_real was called before the STAT_HEALTH was updated
	self->client->ps.stats[STAT_HEALTH] = self->health;

	// check if we are in a NODROP Zone and died from a TRIGGER HURT
	//  if so, we assume that this resulted from a fall to a "bottomless pit" and 
	//  treat it differently...  
	//
	//  Any problems with other places in the code?
	//
	BottomlessPitDeath = 0;	// initialize

	contents = trap_PointContents( self->r.currentOrigin, -1 );
	if ( ( contents & CONTENTS_NODROP ) && (meansOfDeath == MOD_TRIGGER_HURT) )
	{
		BottomlessPitDeath = 1;
	//	G_AddEvent( EV_FALL_FAR );
	//	G_AddEvent( self, EV_FALL_FAR, 0 );
//		self->s.eFlags |= EF_NODRAW;

	}

	// if this dead guy had already dropped the first stage of a transporter, remove that transporter
/*
	classname = BG_FindClassnameForHoldable(HI_TRANSPORTER);
	if (classname)
	{
		gentity_t	*trans = NULL;
		while ((trans = G_Find (trans, FOFS(classname), classname)) != NULL)
		{
			if (trans->parent == self)
			{
				G_FreeEntity(trans);
			}
		}
	}
*/	
	// similarly, if El Corpso here has already dropped a detpack, blow it up
	classname = BG_FindClassnameForHoldable(HI_DETPACK);
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
		obit = modNames[ meansOfDeath ];
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
				if ( self->s.number == borgQueenClientNum )
				{
					numKilled++;
					AddScore( attacker, -500 );
				}
				else
				{
					AddScore( attacker, -1 );
				}
			}
		} 
		else 
		{
			attacker->client->pers.teamState.frags++;
			if ( self->s.number == borgQueenClientNum && attacker )
			{
				numKilled++;
				if ( attacker->client )
				{//killed by opponent
					AddScore( attacker, 500 );//500 bonus
				}
			}
			else if ( self->s.number == actionHeroClientNum && attacker )
			{
				if ( attacker->client )
				{//killed by opponent
					AddScore( attacker, 5 );//5 bonus
				}
			}
			else
			{
				AddScore( attacker, 1 );
			}

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
			if ( self->s.number == borgQueenClientNum )
			{
				numKilled++;
				AddScore( self, -500 );
			}
			else
			{
				AddScore( self, -1 );
			}
		}
	}

	// Add team bonuses
	Team_FragBonuses(self, inflictor, attacker);

	// if client is in a nodrop area, don't drop anything (but return CTF flags!)
	if ( !( contents & CONTENTS_NODROP ) && self->client->sess.sessionClass != PC_ACTIONHERO && meansOfDeath != MOD_SUICIDE && meansOfDeath != MOD_RESPAWN ) 
	{//action hero doesn't drop stuff
		//don't drop stuff in specialty mode
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

	Cmd_Score_f( self );		// show scores
	// send updated scores to any clients that are following this one,
	// or they would get stale scoreboards
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		gclient_t	*client;

		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		if ( client->sess.spectatorClient == self->s.number ) {
			Cmd_Score_f( g_entities + i );
		}
	}

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
	self->client->respawnTime = level.time + 1700;

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
	if ( (BottomlessPitDeath==1) && (killer == ENTITYNUM_WORLD))
	{
		//G_AddEvent( self, EV_FALL_FAR, killer ); ?? Need to play falling SF now, or 
		// use designer trigger??
		//FIXME: need *some* kind of death anim!
	}
	else
	{
		// normal death

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

		// the body can still be gibbed
		self->die = body_die;

	}
	// globally cycle through the different death animations
	deathNum = ( deathNum + 1 ) % 3;

	trap_LinkEntity (self);

	if ( g_pModAssimilation.integer != 0 )
	{
		if ( meansOfDeath == MOD_ASSIMILATE )
		{//Go to Borg team if killed by assimilation
			if ( attacker && attacker->client && attacker->client->sess.sessionTeam != self->client->sess.sessionTeam )
			{
				/*
				if ( !numKilled )
				{
					trap_SendServerCommand( -1, "cp \"Assimilation Has Begun!\"" );
				}
				*/
				numKilled++;
				self->client->mod = meansOfDeath;
				AddScore( attacker, 9 );//+ the 1 above = 10 points for an assimilation
			}
		}
	}
	if ( g_pModActionHero.integer != 0 )
	{
		if ( self->client && self->s.number == actionHeroClientNum )
		{
			//Make me no longer a hero... *sniff*...
			self->client->ps.persistant[PERS_CLASS] = self->client->sess.sessionClass = PC_NOCLASS;
			ClientUserinfoChanged( self->s.number );

			if ( attacker && attacker->client && attacker != self )
			{//killer of action hero becomes action hero
				actionHeroClientNum = attacker->s.number;
			}
			else
			{//other kind of hero death picks a random action hero
				G_RandomActionHero( actionHeroClientNum );
			}
			//respawn the new hero
			//FIXME: or just give them full health and all the goodies?
			respawn( &g_entities[actionHeroClientNum] );
		}
	}
	if ( g_pModElimination.integer != 0 && meansOfDeath != MOD_RESPAWN && meansOfDeath != MOD_KNOCKOUT )
	{//in elimination, you get scored by when you died, but knockout just respawns you, not kill you
		/*
		if ( !numKilled )
		{
			trap_SendServerCommand( -1, "cp \"Elimination Has Begun!\"" );
		}
		*/
		numKilled++;
		self->r.svFlags |= SVF_ELIMINATED;
		switch ( self->client->sess.sessionTeam )
		{
		case TEAM_RED:
			level.teamScores[TEAM_BLUE]++;
			break;
		case TEAM_BLUE:
			level.teamScores[TEAM_RED]++;
			break;
		}
		//Now increment score for everyone else
		if ( g_gametype.integer < GT_TEAM )
		{
			for ( i = 0; i < MAX_CLIENTS; i++ )
			{
				if ( &g_entities[i] != NULL && &g_entities[i].client != NULL && g_entities[i].inuse )
				{
					if ( g_entities[i].client->sess.sessionTeam != TEAM_SPECTATOR && g_entities[i].health > 0 && !(g_entities[i].client->ps.eFlags&EF_ELIMINATED) )
					{
						SetScore( &g_entities[i], numKilled );
					}
				}
			}
		}
		CalculateRanks( qfalse );
	}
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
//	else if (dflags & DAMAGE_SUPER_ARMOR_PIERCING)
//	{
//		protectionFactor = SUPER_PIERCED_ARMOR_PROTECTION;
//	}

	count = client->ps.stats[STAT_ARMOR];
	save = ceil( damage * protectionFactor );
	if (save >= count)
		save = count;

	if (!save)
		return 0;

	client->ps.stats[STAT_ARMOR] -= save;

	return save;
}

#define	BORG_ADAPT_NUM_HITS 10
qboolean G_CheckBorgAdaptation( gentity_t *targ, int mod )
{
	static int	borgAdaptHits[WP_NUM_WEAPONS];
	int	weapon = 0;

	if ( !targ->client )
	{
		return qfalse;
	}

	if ( targ->client->sess.sessionClass != PC_BORG )
	{
		return qfalse;
	}

	switch( mod )
	{
//other kinds of damage
	case MOD_UNKNOWN:
	case MOD_WATER:
	case MOD_SLIME:
	case MOD_LAVA:
	case MOD_CRUSH:
	case MOD_TELEFRAG:
	case MOD_FALLING:
	case MOD_SUICIDE:
	case MOD_RESPAWN:
	case MOD_TARGET_LASER:
	case MOD_TRIGGER_HURT:
	case MOD_DETPACK:
	case MOD_MAX:
	case MOD_KNOCKOUT:
	case MOD_IMOD:
	case MOD_IMOD_ALT:
	case MOD_EXPLOSION:
		return qfalse;
		break;
// Trek weapons
	case MOD_PHASER:
	case MOD_PHASER_ALT:
		weapon = WP_PHASER;
		break;
	case MOD_CRIFLE:
	case MOD_CRIFLE_SPLASH:
	case MOD_CRIFLE_ALT:
	case MOD_CRIFLE_ALT_SPLASH:
		weapon = WP_COMPRESSION_RIFLE;
		break;
	case MOD_SCAVENGER:
	case MOD_SCAVENGER_ALT:
	case MOD_SCAVENGER_ALT_SPLASH:
	case MOD_SEEKER:
		weapon = WP_SCAVENGER_RIFLE;
		break;
	case MOD_STASIS:
	case MOD_STASIS_ALT:
		weapon = WP_STASIS;
		break;
	case MOD_GRENADE:
	case MOD_GRENADE_ALT:
	case MOD_GRENADE_SPLASH:
	case MOD_GRENADE_ALT_SPLASH:
		weapon = WP_GRENADE_LAUNCHER;
		break;
	case MOD_TETRION:
	case MOD_TETRION_ALT:
		weapon = WP_TETRION_DISRUPTOR;
		break;
	case MOD_DREADNOUGHT:
	case MOD_DREADNOUGHT_ALT:
		weapon = WP_DREADNOUGHT;
		break;
	case MOD_QUANTUM:
	case MOD_QUANTUM_SPLASH:
	case MOD_QUANTUM_ALT:
	case MOD_QUANTUM_ALT_SPLASH:
		weapon = WP_QUANTUM_BURST;
		break;
	case MOD_ASSIMILATE:
	case MOD_BORG:
	case MOD_BORG_ALT:
		return qtrue;
		break;
	}

	borgAdaptHits[weapon]++;
	if ( borgAdaptHits[weapon] > BORG_ADAPT_NUM_HITS )//FIXME: different count per weapon?
	{//we have adapted to this weapon
		return qtrue;
	}
	return qfalse;
}

/*
============
T_Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

inflictor, attacker, dir, and point can be NULL for environmental effects

dflags		these flags are used to control how T_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/

void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
			   vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	gclient_t	*client;
	int			take;
	int			save;
	int			asave;
	int			knockback;
	qboolean	bFriend = (targ && attacker)?OnSameTeam( targ, attacker ):qfalse;
	gentity_t	*evEnt;

	if (!targ->takedamage) {
		return;
	}

	// the intermission has allready been qualified for, so don't
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

	// shootable doors / buttons don't actually have any health
	if ( targ->s.eType == ET_MOVER && Q_stricmp("func_breakable", targ->classname) != 0 && Q_stricmp("misc_model_breakable", targ->classname) != 0) 
	{
		if ( targ->use && targ->moverState == MOVER_POS1 ) 
		{
			targ->use( targ, inflictor, attacker );
		}
		return;
	}

	if ( G_CheckBorgAdaptation( targ, mod ) )
	{
		//flag targ for adaptation effect
		targ->client->ps.powerups[PW_BORG_ADAPT] = level.time + 250;
		//targ->client->ps.powerups[PW_BATTLESUIT] += 200;
		//do no damage
		//return;
	}

	// multiply damage times dmgmult
	damage *= g_dmgmult.value;

	// reduce damage by the attacker's handicap value
	// unless they are rocket jumping
	if ( attacker->client && attacker != targ ) {
		damage = damage * attacker->client->ps.stats[STAT_MAX_HEALTH] / 100;
	}

	client = targ->client;

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

	knockback = damage;
	if ( knockback > 200 ) {
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
			vec3_t	kvel;
			float	mass;

			// flying targets get pushed around a lot more.
			switch ( targ->client->sess.sessionClass )
			{
			case PC_INFILTRATOR:
				mass = 100;
				break;
			case PC_HEAVY:
				mass = 400;
				break;
			default:
				mass = 200;
				break;
			}

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
		if ( client && ( client->ps.powerups[PW_BATTLESUIT] || client->ps.powerups[PW_BORG_ADAPT] ))
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

	// always give half damage if hurting self
	// calculated after knockback, so rocket jumping works
	if ( targ == attacker) {
		damage *= 0.5;
	}

	if ( damage < 1 ) {
		damage = 1;
	}
	take = damage;
	save = 0;

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
	if ( client ) {
		if ( attacker ) {
			client->ps.persistant[PERS_ATTACKER] = attacker->s.number;
		} else {
			client->ps.persistant[PERS_ATTACKER] = ENTITYNUM_WORLD;
		}
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;
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
		if (asave)
		{
			evEnt = G_TempEntity(vec3_origin, EV_SHIELD_HIT);
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
	if (take)
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


/*
============
G_RadiusDamage
============
*/
extern void tripwireThink ( gentity_t *ent );
qboolean G_RadiusDamage ( vec3_t origin, gentity_t *attacker, float damage, float radius,
					 gentity_t *ignore, int dflags, int mod) {
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
