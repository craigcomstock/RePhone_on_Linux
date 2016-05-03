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
	  vm_audio_stream_play_finish(as_handle);
    return;
  }
  
  /* get the stream player buffer status */
  vm_audio_stream_play_get_buffer_status(as_handle, &status);
  
  /* put data into the buffer if there is free space */
  printf("CRAIG status free_buffer_size=%d\n", status.free_buffer_size);

  if (status.free_buffer_size > 4800 ) 
  {
    if((as_file_size - as_offset)< 4800)
      used_size = as_file_size - as_offset;
    else
      used_size = 4800;
  }
  else
  {
     return ;
  }
  
  data = (VMUINT8*)vm_calloc(used_size);  
  vm_fs_seek(as_file_hdl, as_offset, VM_FS_BASE_BEGINNING);
  vm_fs_read(as_file_hdl, data, used_size, &readed);


  printf("CRAIG read %d bytes\n", readed);

  /* put data into the buffer */
  ret = vm_audio_stream_play_put_data(as_handle, data, readed, &used_size);
  printf("CRAIG vm_audio_stream_play_put_data => %d\n", ret);


  if(VM_OK == ret)
  as_offset += used_size;


  vm_free(data);

  /* after putting the data into the buffer, start playing the audio */
  //if(as_flag==0 )
  {
    param.start_time = 0;
    param.audio_path = VM_AUDIO_DEVICE_SPEAKER2;
    param.volume= VM_AUDIO_VOLUME_3;
    ret = vm_audio_stream_play_start(as_handle, &param);
    vm_log_fatal("play_start ret=>%d", ret);
    printf("CRAIG play_start ret=>%d", ret);
    as_flag = 1;
  }
   
  if(as_offset == as_file_size)
  {
	/* no data to play hence stop the playback */
	  printf("CRAIG calling vm_audio_stream_play_finish()\n");



    vm_audio_stream_play_finish(as_handle);
  }
}

/* Stream callback function */
void as_audio_callback(VM_AUDIO_HANDLE handle, VM_AUDIO_RESULT result, void* user_data)
{
	printf("CRAIG as_audio_callback, result=%\n", result);

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



/* Open file and prepare to put data */
void audio_open_file(char *name)
{
	printf("CRAIG audio_open_file(name=%s) ENTER \n", name);

  VMINT drv ;
  VMWCHAR w_file_name[MAX_NAME_LEN] = {0};
  VMCHAR file_name[MAX_NAME_LEN];
  vm_audio_stream_play_config_t audio_cfg;
  
  drv = vm_fs_get_removable_drive_letter();
  if(drv <0)
  {
	  drv = vm_fs_get_internal_drive_letter();
	  if(drv <0)
	  {
		  vm_log_fatal("couldn't find removable or internal drive letter");
		  return ;
	  }
  }

  sprintf(file_name, "%c:\\%s", drv, name);
  printf("CRAIG file_name=%s\n", file_name);


  vm_chset_ascii_to_ucs2(w_file_name, MAX_NAME_LEN, file_name);
  as_file_hdl = vm_fs_open(w_file_name, VM_FS_MODE_READ, TRUE);
  
  printf("CRAIG as_file_hdl = %d\n", as_file_hdl);

  if (as_file_hdl < 0)
  {
  vm_log_info("vm_file_open error = %d",as_file_hdl);
  return;
  }
  vm_fs_get_size(as_file_hdl, &as_file_size);

  printf("CRAIG as_file_size=%d\n", as_file_size);
  
  audio_cfg.codec_type = VM_AUDIO_CODEC_MP3;
  VMINT res;

  res = vm_audio_stream_play_open(&as_handle, &audio_cfg, as_audio_callback, NULL);

  printf("CRAIG VM_OK=%d\n", VM_OK);

  printf("CRAIG vm_audio_stream_play_open => %d, as_handle=%d\n", res, as_handle);

  if (res != VM_OK)
  {
	  res = vm_fs_close(as_file_hdl);
	  printf("CRAIG vm_fs_close => %d\n", res);

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
	printf("CRAIG audiostream_play ENTER\n");

	char *name = lua_tostring(L, -1);

	audio_open_file(name);

	as_timer_id = vm_timer_create_precise(1000, audio_put_data_timer_cb, NULL);
	printf("CRAIG as_timer_id=%d\n", as_timer_id);

	printf("CRAIG audiostream_play EXIT\n");

	return 1;
}


#undef MIN_OPT_LEVEL
#define MIN_OPT_LEVEL 0 // ???
#include "lrodefs.h"

const LUA_REG_TYPE audiostream_map[] =
{
	{LSTRKEY("play"), LFUNCVAL(audiostream_play)},
	{LNILKEY, LNILVAL}
};

LUALIB_API int luaopen_audiostream(lua_State *L)
{
	luaL_register(L, "audiostream", audiostream_map);
	return 1;
}
