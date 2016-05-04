#include <stdio.h>
#include "vmtype.h"
#include "vmsystem.h"
#include "vmmemory.h"
#include "vmcmd.h"
#include "vmlog.h"
#include "vmfs.h"
#include "vmchset.h"
#include "vmaudio_stream_play.h"
#include "vmtimer.h"

#include "lua.h"
#include "lauxlib.h"

#define MAX_NAME_LEN 260 /* Max length of file name */

VM_AUDIO_HANDLE as_handle = 0; /* Audio player handle */

/* Variable to help getting the audio file data */
VMUINT as_file_size;
VMINT as_file_hdl = -1;
VMINT as_offset = 0;
VMINT as_flag = 0;

/* Timer to help streaming audio data */
VM_TIMER_ID_PRECISE as_timer_id;

int audiostream_stop() 
{
	vm_log_debug("audiostream_stop, as_handle=%d, as_timer_id=%d, as_file_hdl=%d\n", 
			as_handle, as_timer_id, as_file_hdl);

	if (as_handle != 0)
	{
		vm_log_debug("calling finish, stop and close\n");
		vm_audio_stream_play_finish(as_handle);
		vm_audio_stream_play_stop(as_handle);
		vm_audio_stream_play_close(as_handle);
		as_handle = 0;
	}
	if (as_timer_id > 0)
	{
		vm_log_debug("deleting timer\n");
		vm_timer_delete_precise(as_timer_id);
		as_timer_id = 0;
	}
	
	if (as_file_hdl > 0)
	{
		vm_log_debug("closing file\n");
		vm_fs_close(as_file_hdl);
		as_file_hdl = 0;
	}

	return 1;
}

/* Put the file data into the stream player buffer */
static void audio_put_data()
{
	printf("CRAIG audio_put_data()\n");

  VMINT ret;
  VMUINT8* data = NULL;
  VMUINT readed;
  VMUINT used_size;
  vm_audio_stream_play_buffer_status_t status;
  vm_audio_stream_play_start_parameters_t param;

  printf("CRAIG as_offset=%d, as_file_size=%d\n", as_offset, as_file_size);

  /* Stop the timer if the pointer reaches the end of file */
  if (as_offset >= as_file_size)
  {
	  audiostream_stop();
    return;
  }
  
  /* get the stream player buffer status */
  vm_audio_stream_play_get_buffer_status(as_handle, &status);
  
  /* put data into the buffer if there is free space */
  printf("CRAIG status free_buffer_size=%d\n", status.free_buffer_size);

  // CRAIG TODO this 4800 number should be dependent on sample size...
  // our timer is set at 1 second... 48k samples per second...
  //  CRAIG TODO make this chunk_size vary according to sample rate!!!
  int chunk_size = 4800;
  if (status.free_buffer_size > chunk_size)  
  {
    if((as_file_size - as_offset)< chunk_size)
      used_size = as_file_size - as_offset;
    else
	    used_size = status.free_buffer_size; // for now, just shove as much as possible
//      used_size = chunk_size;
  }
  else
  {
     return ;
  }
  
  vm_log_debug("used_size=%d\n", used_size);

  data = (VMUINT8*)vm_calloc(used_size);  
  vm_fs_seek(as_file_hdl, as_offset, VM_FS_BASE_BEGINNING);
  vm_fs_read(as_file_hdl, data, used_size, &readed);


  printf("CRAIG read %d bytes\n", readed);

  /* put data into the buffer */
  ret = vm_audio_stream_play_put_data(as_handle, data, readed, &used_size);
  printf("CRAIG vm_audio_stream_play_put_data => %d\n", ret);


  if(VM_IS_SUCCEEDED(ret))
  as_offset += used_size;


  vm_free(data);

  /* after putting the data into the buffer, start playing the audio */
  //if(as_flag==0 )
  {
    param.start_time = 0;
    param.audio_path = VM_AUDIO_DEVICE_SPEAKER2;
    param.volume= VM_AUDIO_VOLUME_3;
    ret = vm_audio_stream_play_start(as_handle, &param);
    vm_log_fatal("vm_audio_stream_play_start(as_handle=%d) ret=>%d", as_handle, ret);
    as_flag = 1;
  }
   
  if(as_offset == as_file_size)
  {
	/* no data to play hence stop the playback */
	  printf("CRAIG calling vm_audio_stream_play_finish()\n");
	  audiostream_stop();
  }
}

/* Stream callback function */
void as_audio_callback(VM_AUDIO_HANDLE handle, VM_AUDIO_RESULT result, void* user_data)
{
	printf("CRAIG as_audio_callback, result=%d\n", result);

  switch (result) 
  {
      case VM_AUDIO_DATA_REQUEST:
        /* should put data in this event */
        vm_log_fatal("VM_AUDIO_DATA_REQUEST");
        audio_put_data();
        break;
      default:
        break;
  }
}

// CRAIG TODO audio_stream(char *buffer)
// then use in sam.say()
// then play!!!


/* Open file and prepare to put data */
int audio_open_file(char *name, int is_stereo, int bit_per_sample,
		int sample_frequency, int codec)
{
	printf("CRAIG audio_open_file(name=%s) ENTER \n", name);

  VMINT drv ;
  VMWCHAR w_file_name[MAX_NAME_LEN] = {0};
  VMCHAR file_name[MAX_NAME_LEN];
  vm_audio_stream_play_config_t audio_cfg;

  as_offset = 0; // reset to 0 on repeated plays
  
  drv = vm_fs_get_removable_drive_letter();
  if(drv <0)
  {
	  drv = vm_fs_get_internal_drive_letter();
	  if(drv <0)
	  {
		  vm_log_fatal("couldn't find removable or internal drive letter");
		  return drv;
	  }
  }

  sprintf(file_name, "%c:\\%s", drv, name);
  printf("CRAIG file_name=%s\n", file_name);


  vm_chset_ascii_to_ucs2(w_file_name, MAX_NAME_LEN, file_name);
  as_file_hdl = vm_fs_open(w_file_name, VM_FS_MODE_READ, VM_TRUE);
  
  printf("CRAIG as_file_hdl = %d\n", as_file_hdl);

  if (as_file_hdl < 0)
  {
	  vm_log_info("vm_file_open error = %d",as_file_hdl);
	  return as_file_hdl;
  }
  vm_fs_get_size(as_file_hdl, &as_file_size);

  printf("CRAIG as_file_size=%d\n", as_file_size);
  
//  audio_cfg.is_stereo = FALSE;
//  audio_cfg.bit_per_sample = 8; // hack for wav files TODO make configable or detectable from header.
//  audio_cfg.sample_frequency = VM_AUDIO_SAMPLE_FREQUENCY_22050; // hack for wav files, TODO make configable/detectable from file header...

  if(is_stereo >= 0)
  {
	  vm_log_debug("setting is_stereo=%d\n", is_stereo);
	audio_cfg.is_stereo = is_stereo;
  }
  if(bit_per_sample >= 0)
  {
	  vm_log_debug("setting bit_per_sample=%d\n", bit_per_sample);
	audio_cfg.bit_per_sample = bit_per_sample;
  }

  if(sample_frequency >= 0)
  {
	  vm_log_debug("setting sample_frequency=%d\n", sample_frequency);
	  vm_log_debug("VM_AUDIO_SAMPLE_FREQUENCY_22050=%d\n", VM_AUDIO_SAMPLE_FREQUENCY_22050);
	audio_cfg.sample_frequency = sample_frequency; // CRAIG TODO make this a checked int instead of an enum for lua purposes...
  }
  if(codec >= 0)
  {
	  vm_log_debug("setting codec=%d\n", codec);
	  audio_cfg.codec_type = codec;
  }
//  audio_cfg.codec_type = VM_AUDIO_CODEC_MP3;
//  audio_cfg.codec_type = VM_AUDIO_CODEC_WAV;
  VMINT res;

  res = vm_audio_stream_play_open(&as_handle, &audio_cfg, as_audio_callback, NULL);

  printf("CRAIG vm_audio_stream_play_open => %d, as_handle=%d\n", res, as_handle);

  if (VM_IS_FAILED(res))
  {
	  vm_log_debug("audio_open_file fail, vm_audio_stream_play_open => %d\n", res);
	  VMINT res2 = vm_fs_close(as_file_hdl);
	  vm_log_debug("audio_open_file fail, cleanup so call vm_fs_close => %d\n", res2);
	  return res;
  }

  printf("CRAIG audio_open_file EXIT\n");

}

/* Timer callback to put audio data */
void audio_put_data_timer_cb (VMINT tid, void* user_data)
{
  audio_put_data();
}

int audiostream_play(lua_State *L)
{
	// CRAIG TODO inhibit repeated calls since after this
	// things are async.
	printf("CRAIG audiostream_play ENTER\n");

	char *name = lua_tostring(L, 1);
	int is_stereo = luaL_checkint(L,2);
	int bit_per_sample = luaL_checkint(L,3);
	int sample_frequency = luaL_checkint(L,4);
	int codec = luaL_checkint(L,5);
	vm_log_debug("audiostream_play(), name=%s, is_stereo=%d, bit_per_sample=%d, sample_frequency=%d, codec=%d\n", name, is_stereo, bit_per_sample, sample_frequency, codec);

	as_offset = 0;

	VMINT res;
	res = audio_open_file(name,is_stereo,bit_per_sample,sample_frequency,codec);
	vm_log_debug("audio_open_file returned %d\n", res);
	if (VM_IS_SUCCEEDED(res))
	{
		// timer of 1000 seems too slow, esp for 16-bit 44.1khz wav data...
		// CRAIG TODO, a formula for bitrate etc informing the audio_put frequency!?!?
		// this cooperates with the number of samples we load at a time
		// looks like the max buffer size is 16kb
		as_timer_id = vm_timer_create_precise(250, audio_put_data_timer_cb, NULL);
		printf("CRAIG as_timer_id=%d\n", as_timer_id);
	} else {
		vm_log_debug("audio_open_file failed with res=%d\n", res);
	}

	printf("CRAIG audiostream_play EXIT\n");

	return 1;
}


#undef MIN_OPT_LEVEL
#define MIN_OPT_LEVEL 0 // ???
#include "lrodefs.h"

const LUA_REG_TYPE audiostream_map[] =
{
	{LSTRKEY("play"), LFUNCVAL(audiostream_play)},
	{LSTRKEY("stop"), LFUNCVAL(audiostream_stop)},
	{LNILKEY, LNILVAL}
};

LUALIB_API int luaopen_audiostream(lua_State *L)
{
	luaL_register(L, "audiostream", audiostream_map);
	return 1;
}
