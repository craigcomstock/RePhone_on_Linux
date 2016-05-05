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

void handle_sysevt(VMINT message, VMINT param) 
{
	int i = 0;
	printf("\nhandle_sysevt(message=%d, param=%d)\n", message, param);
    switch (message) 
    {
    case VM_EVENT_CREATE:
	    while(1) {
		    printf("CRAIG boink %d\n", i++);
		    vm_thread_sleep(1000);
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

