#include <string.h>

#include "reciter.h"
#include "sam.h"
#include "debug.h"

#include "lua.h"
#include "lauxlib.h"

#include "vmaudio.h"
#include "vmaudio_stream_play.h"
#include "vmtimer.h"
#include "vmmemory.h"

VMUINT g_buffer_size;
VMINT g_offset = 0;
VMINT g_flag = 0;
VMINT g_codec = VM_AUDIO_CODEC_MP3;
char* g_buffer;
VM_AUDIO_HANDLE g_handle = 0;
VM_TIMER_ID_PRECISE g_timer_id;

int debug = 1; // for sam :(

static void audio_put_data()
{
	VMINT ret;
	VMUINT8* data = NULL;
	VMUINT readed;
	VMUINT used_size;
	vm_audio_stream_play_buffer_status_t status;
	vm_audio_stream_play_start_parameters_t param;

	/* stop the timer if the pointer reaches end of buffer */
	if (g_offset >= g_buffer_size)
	{
		return;
	}

	/* get stream player buffer status */
	vm_audio_stream_play_get_buffer_status(g_handle, &status);

	fprintf(stderr, "free_buffer_size=%d, g_buffer_size=%d, g_offset=%d\n", status.free_buffer_size, g_buffer_size, g_offset);
	if(status.free_buffer_size > 4800)
	{
		if ((g_buffer_size - g_offset) < 4800)
			used_size = g_buffer_size - g_offset;
		else
			used_size = 4800;
	}
	else
	{
		fprintf(stderr, "free_buffer_size less than 4800 so return\n");
		return;
	}

	data = (VMUINT8*)vm_calloc(used_size);
	VMUINT i;
	fprintf(stderr, "before copying from buffer to data, g_offset=%d, g_buffer_size=%d\n", g_offset, g_buffer_size);

	for( i=0; i<used_size; i++)
	{
		if (g_offset + i > g_buffer_size) {
			readed = i;
			break;
		}
		data[i] = g_buffer[g_offset + i];
	}

	ret = vm_audio_stream_play_put_data(g_handle, data, readed, &used_size);
	fprintf(stderr, "play_put_data readed=%d, used_size=%d, ret=%d\n", readed, used_size, ret);
	if(VM_OK == ret)
		g_offset += used_size;

	vm_free(data);

	{
		param.start_time = 0;
		param.audio_path = VM_AUDIO_DEVICE_SPEAKER2;
		param.volume = VM_AUDIO_VOLUME_3; // CRAIG TODO CONFIG
		ret = vm_audio_stream_play_start(g_handle, &param);
		fprintf(stderr, "vm_audio_stream_play_start => %d\n", ret);
	}

	if(g_offset == g_buffer_size)
	{
		vm_audio_stream_play_finish(g_handle);
	}
}

void audio_callback(VM_AUDIO_HANDLE handle, VM_AUDIO_RESULT result, void* user_data)
{
	fprintf(stderr, "audio_callback, result=%d\n", result);

	switch (result)
	{
		case VM_AUDIO_DATA_REQUEST:
			fprintf(stderr, "VM_AUDIO_DATA_REQUEST\n");
			audio_put_data();
			break;
		default:
			break;
	}
}

VMINT audio_open()
{
	vm_audio_stream_play_config_t audio_cfg;
//	audio_cfg.codec_type = VM_AUDIO_CODEC_AAC;
	audio_cfg.codec_type = g_codec;
	audio_cfg.sample_frequency = VM_AUDIO_SAMPLE_FREQUENCY_22050;
//	VM_AUDIO_FORMAT_WAV;
//	VM_AUDIO_CODEC_WAV;

	VMINT res;
	if (VM_OK != (res = vm_audio_stream_play_open(&g_handle, &audio_cfg, audio_callback, NULL)))
	{
		fprintf(stderr, "vm_audio_stream_play_open error: %d\n", res);
	}
	return res;
}

void audio_put_data_timer_cb (VMINT tid, void* user_data)
{
	audio_put_data();
}

void OutputSound()
{
	fprintf(stderr, "OutputSound() start\n");

	g_buffer_size = GetBufferLength();
	fprintf(stderr, "sam GetBufferLength()=%d\n", g_buffer_size);

//	bufferpos /= 50;

//	char *buffer = GetBuffer();

	g_buffer = GetBuffer();

	g_offset = 0;
	if (VM_OK != audio_open())
	{
		fprintf(stderr, "audio_open failed\n");
		return;
	}
	g_timer_id = vm_timer_create_precise(1000, audio_put_data_timer_cb, NULL);
	audio_put_data();

	fprintf(stderr, "OutputSound() end\n");
	
}

int sam_say(lua_State *L)
{
	char* text = luaL_checkstring(L, 1);
	g_codec = luaL_checkint(L, 2);

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
