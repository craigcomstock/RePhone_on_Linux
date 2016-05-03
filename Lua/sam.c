#include <string.h>

#include "reciter.h"
#include "sam.h"
#include "debug.h"

#include "lua.h"
#include "lauxlib.h"

#include "vmtimer.h"
#include "vmmemory.h"

int debug = 1; // for sam :(

void OutputSound()
{
	fprintf(stderr, "OutputSound() start\n");

	VMUINT buffer_size = GetBufferLength();
	fprintf(stderr, "sam GetBufferLength()=%d\n", buffer_size);

//	bufferpos /= 50;

	char *buffer = GetBuffer();

	/*
	VMINT offset = 0;
	if (VM_OK != audio_open())
	{
		fprintf(stderr, "audio_open failed\n");
		return;
	}
	*/

	fprintf(stderr, "OutputSound() end\n");
	
}

int sam_say(lua_State *L)
{
	char* text = luaL_checkstring(L, 1);

	char input[512];

	strncpy(input, text, 512);

	strncat(input, "[", 512);
	if (!TextToPhonemes(input))
	{
		fprintf(stderr, "TextToPhonemes failed\n");
		return 1;
	}
	fprintf(stderr, "phonetic input: %s\n", input);

	SetInput(input);
	if (!SAMMain()) {
		fprintf(stderr, "SAMMain failed\n");
		return 1;
	}
	OutputSound();

	return 1;
}

#undef MIN_OPT_LEVEL
#define MIN_OPT_LEVEL 0 // ???? TODO
#include "lrodefs.h"

const LUA_REG_TYPE sam_map[] =
{
	{LSTRKEY("say"), LFUNCVAL(sam_say)},
	{LNILKEY, LNILVAL}
};

LUALIB_API int luaopen_sam(lua_State *L)
{
	luaL_register(L, "sam", sam_map);
	return 1;
}
