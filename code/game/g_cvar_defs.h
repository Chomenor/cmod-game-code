/*
Format:
CVAR_DEF( vmcvar, name, default, flags, announce )
*/

// don't override the cheat state set by the system
CVAR_DEF( g_cheats, "sv_cheats", "", 0, qfalse )

// latched vars
CVAR_DEF( g_gametype, "g_gametype", "0", CVAR_SERVERINFO | CVAR_LATCH, qfalse )
CVAR_DEF( g_maxclients, "sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, qfalse )
CVAR_DEF( g_maxGameClients, "g_maxGameClients", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, qfalse )

// change anytime vars
CVAR_DEF( g_dmflags, "dmflags", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, qtrue )
CVAR_DEF( g_fraglimit, "fraglimit", "20", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, qtrue )
CVAR_DEF( g_timelimit, "timelimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, qtrue )
CVAR_DEF( g_timelimitWinningTeam, "timelimitWinningTeam", "", CVAR_NORESTART, qtrue )
CVAR_DEF( g_capturelimit, "capturelimit", "8", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, qtrue )

CVAR_DEF( g_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO, qfalse )

CVAR_DEF( g_friendlyFire, "g_friendlyFire", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, qtrue )

CVAR_DEF( g_teamAutoJoin, "g_teamAutoJoin", "0", CVAR_ARCHIVE, qfalse )
CVAR_DEF( g_teamForceBalance, "g_teamForceBalance", "1", CVAR_ARCHIVE, qfalse )

CVAR_DEF( g_intermissionTime, "g_intermissionTime", "20", CVAR_ARCHIVE, qtrue )
CVAR_DEF( g_warmup, "g_warmup", "20", CVAR_ARCHIVE, qtrue )
CVAR_DEF( g_doWarmup, "g_doWarmup", "0", CVAR_ARCHIVE, qtrue )
CVAR_DEF( g_log, "g_log", "games.log", CVAR_ARCHIVE, qfalse )
CVAR_DEF( g_logSync, "g_logSync", "0", CVAR_ARCHIVE, qfalse )

CVAR_DEF( g_password, "g_password", "", CVAR_USERINFO, qfalse )

CVAR_DEF( g_banIPs, "g_banIPs", "", CVAR_ARCHIVE, qfalse )
CVAR_DEF( g_filterBan, "g_filterBan", "1", CVAR_ARCHIVE, qfalse )

CVAR_DEF( g_needpass, "g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM, qfalse )

CVAR_DEF( g_dedicated, "dedicated", "0", 0, qfalse )

CVAR_DEF( g_speed, "g_speed", "250", CVAR_SERVERINFO | CVAR_ARCHIVE, qtrue )				// Quake 3 default was 320.
CVAR_DEF( g_gravity, "g_gravity", "800", CVAR_SERVERINFO | CVAR_ARCHIVE, qtrue )
CVAR_DEF( g_knockback, "g_knockback", "500", 0, qtrue )
CVAR_DEF( g_dmgmult, "g_dmgmult", "1", 0, qtrue )
CVAR_DEF( g_weaponRespawn, "g_weaponrespawn", "5", 0, qtrue )		// Quake 3 default (with 1 ammo weapons) was 5.
CVAR_DEF( g_teamWeaponRespawn, "g_teamWeaponRespawn", "30", 0, qtrue )	// Used instead of g_weaponrespawn in THM gametype.
CVAR_DEF( g_adaptRespawn, "g_adaptrespawn", "1", 0, qtrue )		// Make weapons respawn faster with a lot of players.
CVAR_DEF( g_forcerespawn, "g_forcerespawn", "0", 0, qtrue )		// Quake 3 default was 20. This is more "user friendly".
CVAR_DEF( g_inactivity, "g_inactivity", "0", 0, qtrue )
CVAR_DEF( g_debugMove, "g_debugMove", "0", 0, qfalse )
CVAR_DEF( g_debugDamage, "g_debugDamage", "0", 0, qfalse )
CVAR_DEF( g_debugAlloc, "g_debugAlloc", "0", 0, qfalse )
CVAR_DEF( g_motd, "g_motd", "", 0, qfalse )

CVAR_DEF( g_podiumDist, "g_podiumDist", "80", 0, qfalse )
CVAR_DEF( g_podiumDrop, "g_podiumDrop", "70", 0, qfalse )

#if 0
CVAR_DEF( g_debugForward, "g_debugForward", "0", 0, qfalse )
CVAR_DEF( g_debugRight, "g_debugRight", "0", 0, qfalse )
CVAR_DEF( g_debugUp, "g_debugUp", "0", 0, qfalse )
#endif

CVAR_DEF( g_language, "g_language", "", CVAR_ARCHIVE, qfalse )

CVAR_DEF( g_holoIntro, "g_holoIntro", "1", CVAR_ARCHIVE, qfalse )
CVAR_DEF( g_ghostRespawn, "g_ghostRespawn", "5", CVAR_ARCHIVE, qfalse )		// How long the player is ghosted, in seconds.

CVAR_DEF( bot_minplayers, "bot_minplayers", "0", CVAR_SERVERINFO, qfalse )
