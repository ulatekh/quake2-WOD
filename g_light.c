#include "g_local.h"
#include <stdlib.h>
#include <ctype.h>

cvar_t *sv_autodark;

static char kNormalLight[2] = "m";

char g_LightLevel[200];

static void
SetLightLevel (char *pszLevel)
{
	int i;

	for (i = CS_LIGHTS + 0; i <= CS_LIGHTS + 11; i++)
		gi.configstring (i, pszLevel);

	strcpy (g_LightLevel, pszLevel);
}

void
G_LightLevels (void)
{
	int autodarkLen;

	// If autodark is on, and it's time to adjust the light level, do so.
	autodarkLen = strlen (sv_autodark->string);
	if (autodarkLen > 0 && timelimit->value)
	{
		int levelIndex;
		char cLevel;
		char newLightLevel[sizeof (g_LightLevel)];

		// Determine the current light level.
		levelIndex = level.time * autodarkLen
			/ (timelimit->value * 60.0);
		if (levelIndex >= autodarkLen)
			levelIndex = autodarkLen - 1;
		cLevel = sv_autodark->string[levelIndex];

		// If it's a lowercase letter, that's the light level.
		if (islower (cLevel))
		{
			newLightLevel[0] = cLevel;
			newLightLevel[1] = '\0';
		}

		// If it's an uppercase character, it's part of a pattern.  Extract
		// the pattern.
		else
		{
			char *here;
			char *there;

			// Find the beginning of the pattern.
			here = sv_autodark->string + levelIndex;
			while (here > sv_autodark->string && isupper (here[-1]))
				here--;

			// Copy until the end of the pattern is found, converting to
			// lowercase as we go.
			there = newLightLevel;
			while (here <= sv_autodark->string + autodarkLen
				 && there < newLightLevel + sizeof (newLightLevel) - 1
				 && isupper (*here))
			{
				*there = tolower (*here);
				here++;
				there++;
			}

			// Terminate the string.
			*there = '\0';
		}

		// If that's different than the current light level, change it.
		if (strcmp (g_LightLevel, newLightLevel) != 0)
			SetLightLevel (newLightLevel);
	}

	// If autodark has been turned off, restore the light level.
	else if (strcmp (g_LightLevel, kNormalLight) != 0)
		SetLightLevel (kNormalLight);
}
