
#include <stdio.h>
#include <string.h>

#include "vmtype.h"
#include "vmlog.h"
#include "vmsystem.h"
#include "vmgsm_tel.h"
#include "vmgsm_sim.h"
#include "vmtimer.h"
#include "vmdcl.h"
#include "vmdcl_gpio.h"
#include "vmdcl_kbd.h"
#include "vmkeypad.h"
#include "vmthread.h"
#include "vmwdt.h"
#include "vmpwr.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

extern void retarget_setup();
extern int gpio_get_handle(int pin, VM_DCL_HANDLE* handle);

void lua_setup()
{
	int status, result, i;
	lua_State *L;

	L = luaL_newstate();
	status - luaL_loadfile(L, "script.lua");
	if(status) {
		vm_log_warn("couldn't load file %s\n", lua_tostring(L, 01));
		return;
	}
}

void bluelight()
{
	VM_DCL_HANDLE handle;
#define RED 17
#define GREEN 15
#define BLUE 12

	int pin = RED;
	if(gpio_get_handle(pin, &handle) == VM_DCL_HANDLE_INVALID) {
		return;
	}
	vm_dcl_control(handle, VM_DCL_GPIO_COMMAND_SET_MODE_0, NULL);
	vm_dcl_control(handle, VM_DCL_GPIO_COMMAND_SET_DIRECTION_OUT, NULL);
	vm_dcl_control(handle, VM_DCL_GPIO_COMMAND_WRITE_LOW, NULL);

	pin = GREEN;
	if(gpio_get_handle(pin, &handle) == VM_DCL_HANDLE_INVALID) {
		return;
	}
	vm_dcl_control(handle, VM_DCL_GPIO_COMMAND_SET_MODE_0, NULL);
	vm_dcl_control(handle, VM_DCL_GPIO_COMMAND_SET_DIRECTION_OUT, NULL);
	vm_dcl_control(handle, VM_DCL_GPIO_COMMAND_WRITE_HIGH, NULL);


}

void handle_sysevt(VMINT message, VMINT param)
{
    switch (message) {
        case VM_EVENT_CREATE:
            //sys_timer_id = vm_timer_create_precise(SYS_TIMER_INTERVAL, sys_timer_callback, NULL);

	    bluelight();

//            lua_setup();

            break;
//		case SHELL_MESSAGE_ID:
			// MANY vm_xxx FUNCTIONS CAN BE EXECUTED ONLY FROM THE MAIN THREAD!!
			// execute lua "docall(L, 0, 0)", WAITS for execution!!
//			shell_docall(L);
//			break;

//        case SHELL_MESSAGE_QUIT:
//        	printf("\n[SYSEVT] APP RESTART\n");
//            vm_pmng_restart_application();
//            break;
        case VM_EVENT_PAINT:
        	// The graphics system is ready for application to use
        	//printf("\n[SYSEVT] GRAPHIC READY\n");
        	break;
        case VM_EVENT_QUIT:
        	printf("\n[SYSEVT] QUIT\n");
            break;
        default:
        	printf("\n[SYSEVT] UNHANDLED EVENT: %d\n", message);
    }
}

/****************/
/*  Entry point */
/****************/
void vm_main(void)
{
    retarget_setup();
    vm_log_info("LUA for RePhone started");

    bluelight();

//    key_init();
//    vm_keypad_register_event_callback(handle_keypad_event);

    /* register system events handler */
//    vm_pmng_register_system_event_callback(handle_sysevt);
}
