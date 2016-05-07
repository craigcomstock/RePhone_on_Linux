/* 
 * TODO
 *
 * - change sam renderer to operate in chunks so that we don't use so much
 *   memory at once as well as allow us to interrupt sam.
 *
 * - improve audio quality
 */

#include <string.h>

#include "reciter.h"
#include "sam.h"
#include "debug.h"

#include "lua.h"
#include "lauxlib.h"

#include "vmtimer.h"
#include "vmmemory.h"
#include "vmlog.h"

#include "vmaudio_stream_play.h"

int debug = 0; // for sam :(
#define __LINKIT_RELEASE__

VM_AUDIO_HANDLE sam_handle = 0;
VMUINT sam_size;
VMINT sam_offset = 0;

VM_TIMER_ID_PRECISE sam_timer_id;

int sam_stop()
{
	vm_log_debug("sam_stop()\n");

	if (sam_handle != 0)
	{
		vm_log_debug("sam_stop(), sam_handle != 0 so finish, stop and close audio stream\n");
		vm_audio_stream_play_finish(sam_handle);
		vm_audio_stream_play_stop(sam_handle);
		vm_audio_stream_play_close(sam_handle);
		sam_handle = 0;
	}

	if (sam_timer_id > 0)
	{
		vm_log_debug("sam_stop(), sam_timer_id > 0 so delete the timer\n");
		vm_timer_delete_precise(sam_timer_id);
		sam_timer_id = 0;
	}

	return 1;
}

static void sam_put_data(void* buffer)
{
	vm_log_debug("sam_put_data(%p), sam_offset=%d, sam_size=%d\n", buffer, sam_offset, sam_size);

	VMINT ret;
	VMUINT8* data = NULL;
	VMUINT readed;
	VMUINT used_size;
	vm_audio_stream_play_buffer_status_t status;
	vm_audio_stream_play_start_parameters_t param;

	if (sam_offset >= sam_size) 
	{
		vm_log_debug("sam_offset=%d >= sam_size=%d, so stop and return\n", sam_offset, sam_size);
		sam_stop();
		return;
	}

	vm_audio_stream_play_get_buffer_status(sam_handle, &status);
	vm_log_debug("free_buffer_size=%d\n", status.free_buffer_size);

	int chunk_size = 4800;
	if (status.free_buffer_size > chunk_size)
	{
		if ((sam_size - sam_offset) < chunk_size)
		{
			used_size = sam_size - sam_offset;
		} else {
			used_size = status.free_buffer_size;
		}
	}
	else
	{
		vm_log_debug("free_buffer_size(%d) > chunk_size(%d) so return\n", status.free_buffer_size, chunk_size);
		return;
	}

	vm_log_debug("used_size=%d\n", used_size);

	data = (VMUINT8*)vm_calloc(used_size);
	memcpy(data, buffer + sam_offset, used_size);
	readed = used_size; // not needed but kept for similarity to file version

	vm_log_debug("copied %d bytes\n", readed);

	ret = vm_audio_stream_play_put_data(sam_handle, data, readed, &used_size);
	vm_log_debug("vm_audio_stream_play_put_data => %d, used_size=%d\n", ret, used_size);

	if (VM_IS_SUCCEEDED(ret))
	{
		sam_offset += used_size;
	}

	vm_free(data);
	param.start_time = 0;
	param.audio_path = VM_AUDIO_DEVICE_SPEAKER2;
	param.volume = VM_AUDIO_VOLUME_6; // TODO use global set value
	ret = vm_audio_stream_play_start(sam_handle, &param);
	vm_log_debug("vm_audio_stream_play_start=>%d\n", ret);

	if (sam_offset == sam_size)
	{
		vm_log_debug("sam_offset(%d) == sam_size(%d) so stop\n", sam_offset, sam_size);
		sam_stop();
	}
}


void sam_audio_cb (VM_AUDIO_HANDLE handle, VM_AUDIO_RESULT result, void* buffer)
{
	vm_log_debug("sam_audio_cb, result=%d\n", result);

	switch (result)
	{
		case VM_AUDIO_DATA_REQUEST:
			vm_log_debug("VM_AUDIO_DATA_REQUEST event\n");
			sam_put_data(buffer);
			break;
		default:
			break;
	}
}

void sam_put_data_timer_cb (VMINT tid, void* buffer)
{
	sam_put_data(buffer);
}

int sam_say(lua_State *L)
{
	vm_audio_stream_play_config_t audio_cfg;
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

	sam_offset = 0;
	sam_size = GetBufferLength();
	vm_log_debug("GetBufferLength()=%d\n", sam_size);

	sam_size /= 7; // why?!?!?
	vm_log_debug("translated to sam_size=%d\n", sam_size);

	char *buffer = GetBuffer();

	VMINT res;
	audio_cfg.is_stereo = 0;
	audio_cfg.bit_per_sample = 8;
	audio_cfg.sample_frequency = VM_AUDIO_SAMPLE_FREQUENCY_22050;
	audio_cfg.codec_type = VM_AUDIO_CODEC_PCM;

	res = vm_audio_stream_play_open(&sam_handle, &audio_cfg, sam_audio_cb, buffer);
	vm_log_debug("vm_audio_stream_play_open=>%d, sam_as_handle=%d\n", res, sam_handle);

	if (VM_IS_SUCCEEDED(res))
	{
		sam_timer_id = vm_timer_create_precise(1000, sam_put_data_timer_cb, buffer);
		vm_log_debug("vm_timer_create_precise=>%d\n", sam_timer_id);
	} else {
		vm_log_debug("vm_audio_stream_play_open failed\n");
		return res;
	} 


	return 1;
}

#undef MIN_OPT_LEVEL
#define MIN_OPT_LEVEL 0 // ???? TODO
#include "lrodefs.h"

const LUA_REG_TYPE sam_map[] =
{
	{LSTRKEY("say"), LFUNCVAL(sam_say)},
	{LSTRKEY("stop"), LFUNCVAL(sam_stop)},
	{LNILKEY, LNILVAL}
};

LUALIB_API int luaopen_sam(lua_State *L)
{
	luaL_register(L, "sam", sam_map);
	return 1;
}
