/*
* Model Groups
* 
* This module provides functions for determining the race of player models.
*/

#define MOD_NAME ModModelGroups

#include "mods/g_mod_local.h"

#define MAX_SKINS_FOR_RACE	128
#define MAX_GROUP_MEMBERS	1024

#define MAX_GROUP_FILE_SIZE	5000

typedef struct {
	char *name;
	char *text;
} group_list_t;

static struct {
	group_list_t group_list[MAX_GROUP_MEMBERS];
	int group_count;
	char races[256];
} *MOD_STATE;

/*
================
ModModelGroups_InitGroupsList
================
*/
static void ModModelGroups_InitGroupsList(void)
{
	char	filename[MAX_QPATH];
	char	dirlist[2048];
	int		i;
	char*	dirptr;
	char*	race_list;
	int		numdirs;
	int		dirlen;

	// search through each and every skin we can find
	numdirs = trap_FS_GetFileList("models/players2", "/", dirlist, 2048 );
	dirptr  = dirlist;
	for (i=0; i<numdirs ; i++,dirptr+=dirlen+1)
	{
		dirlen = strlen(dirptr);

		if (dirlen && dirptr[dirlen-1]=='/')
		{
			dirptr[dirlen-1]='\0';
		}

		if (!strcmp(dirptr,".") || !strcmp(dirptr,".."))
		{
			continue;
		}

		if (MOD_STATE->group_count == MAX_GROUP_MEMBERS)
		{
			G_DedPrintf("Number of possible models larger than group array - limiting to first %d models\n", MAX_GROUP_MEMBERS);
			break;
		}
		// work out racename to
		Com_sprintf(filename, sizeof(filename), "models/players2/%s/groups.cfg", dirptr);
		race_list = BG_RegisterRace(filename);

		MOD_STATE->group_list[MOD_STATE->group_count].name = G_ModUtils_AllocateString( dirptr );
		MOD_STATE->group_list[MOD_STATE->group_count++].text = G_ModUtils_AllocateString( race_list );
	}

	G_DedPrintf( "Loaded races for %i player models\n", MOD_STATE->group_count );
}

/*
================
ModModelGroups_Shared_SearchGroupList

For a given model name, return a comma-separated list of races it belongs to.
================
*/
char *ModModelGroups_Shared_SearchGroupList(const char *name)
{
	char	*text_p = NULL, *slash = NULL;
	char	text[MAX_GROUP_FILE_SIZE];
	int		i;
	char	mod_name[200];

	EF_ERR_ASSERT( MOD_STATE );

	memset (MOD_STATE->races, 0, sizeof(MOD_STATE->races));
	memset (text, 0, sizeof(text));

	// check to see if there is a '/' in the name
	Q_strncpyz(mod_name, name, sizeof(mod_name));
	slash = strstr( mod_name, "/" );
	if ( slash != NULL )
	{//drop the slash and everything after it for the purpose of finding the model name in th group
		*slash = 0;
	}

	// find the name in the group list
	for (i=0; i<MOD_STATE->group_count; i++)
	{
		if (!Q_stricmp(mod_name, MOD_STATE->group_list[i].name))
		{
			text_p = MOD_STATE->group_list[i].text;
			break;
		}
	}

	// did we find this group in the list?
	if (i == MOD_STATE->group_count)
	{
		Com_sprintf(MOD_STATE->races, sizeof(MOD_STATE->races), "unknown");
	}
	else
	{
		Com_sprintf(MOD_STATE->races, sizeof(MOD_STATE->races), text_p);
	}
	return MOD_STATE->races;

}

/*
===========
ModModelGroups_Shared_ListContainsRace

Compare a list of races with an incoming race name.
Used to decide if in a CTF game where a race is specified for a given team if a skin is actually already legal.
===========
*/
qboolean ModModelGroups_Shared_ListContainsRace(const char *race_list, const char *race)
{
	char current_race_name[125];
	const char *s = race_list;
	const char *max_place = race_list + strlen(race_list);
	const char *marker;

	memset(current_race_name, 0, sizeof(current_race_name));
	// look through the list till it's empty
	while (s < max_place)
	{
		marker = s;
		// figure out from where we are where the next ',' or 0 is
		while (*s != ',' && *s != 0)
		{
			s++;
		}

		// copy just that name
		Q_strncpyz(current_race_name, marker, (s-marker)+1);

		// avoid the comma or increment us past the end of the string so we fail the main while loop
		s++;

		// compare and see if this race is the same as the one we want
		if (!Q_stricmp(current_race_name, race))
		{
			return qtrue;
		}
	}
	return qfalse;
}

/*
===========
ModModelGroups_Shared_RandomModelForRace

Given a race name, go find all the models that use it, and randomly select one.
===========
*/
void ModModelGroups_Shared_RandomModelForRace( const char *race, char *model, unsigned int size ) {
	char	skinsForRace[MAX_SKINS_FOR_RACE][128];
	int		howManySkins = 0;
	int		i;
	int		temp;

	EF_ERR_ASSERT( MOD_STATE );

	memset(skinsForRace, 0, sizeof(skinsForRace));

	// search through each and every skin we can find
	for (i=0; i<MOD_STATE->group_count && howManySkins < MAX_SKINS_FOR_RACE; i++)
	{

		// if this models race list contains the race we want, then add it to the list
		if (ModModelGroups_Shared_ListContainsRace(MOD_STATE->group_list[i].text, race))
		{
			Q_strncpyz( skinsForRace[howManySkins++], MOD_STATE->group_list[i].name , 128 );
		}
	}

	// set model to a random one
	if (howManySkins)
	{
		temp = rand() % howManySkins;
		SetSkinForModel( skinsForRace[temp], "default", model, size );
	}
	else
	{
		model[0] = 0;
	}
}

/*
================
ModModelGroups_Init
================
*/
void ModModelGroups_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		ModModelGroups_InitGroupsList();
	}
}
