#include <string.h>
#include <stdio.h>

#include "vmtype.h" 
#include "vmboard.h"
#include "vmsystem.h"
#include "vmlog.h" 
#include "vmcmd.h" 
#include "vmdcl.h"
#include "vmdcl_gpio.h"
#include "vmthread.h"

#include "vmhttps.h"
#include "vmtimer.h"
#include "vmgsm_gprs.h"
#include "vmwdt.h"
#include "vmpwr.h"

extern void retarget_setup();

VM_TIMER_ID_PRECISE g_timer_id;
int count;

void timer_cb (VMINT tid, void* user_data)
{
	printf("CRAIG timer_cb, count=%d\n", count++);
}

void handle_sysevt(VMINT message, VMINT param) 
{
	int i = 0;
	printf("\nhandle_sysevt(message=%d, param=%d)\n", message, param);
    switch (message) 
    {
    case VM_EVENT_CREATE:
	    count = 0;
	    g_timer_id = vm_timer_create_precise(1000, timer_cb, NULL);

	    while(1) 
	    {
		    putchar(getchar());
	    }

        break;

    case VM_EVENT_QUIT:
        break;
    }
}

void vm_main(void) 
{
    retarget_setup();
    printf("\nHello World!\n");
    
    /* register system events handler */
    vm_pmng_register_system_event_callback(handle_sysevt);
}

