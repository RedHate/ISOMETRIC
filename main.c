/*
    This file is part of ISO-METRIC.

    Ciso is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Ciso is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


    Copyright 2024 Ultros (RedHate)
*/
 
// standard libs
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
 
// sony libs
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspumd.h>
#include <pspgu.h>
#include <pspgum.h>
#include <psppower.h>

// extra libs
#include "ciso/ciso.h"
#include "error/error.h"
#include "common/colors.h"
#include "umd/umd.h"

// module info, userspace.
PSP_MODULE_INFO("ISOMETRIC", PSP_MODULE_USER, 0, 0);

// make sure the thread is userspace
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

// super important, we need to set heap size max to alloc all the userspace memory to our program
PSP_HEAP_SIZE_MAX();

// controls all running loops if its set to 0 they will all exit
int running = 1;

int exit_callback(int arg1, int arg2, void *common){
	
	// terminate the running loops
	running = 0;
	// exit game mode
	sceKernelExitGame();
	return 0;
	
}

int CallbackThread(SceSize args, void *argp){
	
	// register a callback id and sleep the callback
	int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
	
}

int main(void){
	
	// set up the callback thread
	SceUID CallbackThreadID = sceKernelCreateThread("CallbackThread", CallbackThread, 0x11, 0xFA0, THREAD_ATTR_USER, 0);
	if(CallbackThreadID >= 0){
		sceKernelStartThread(CallbackThreadID, 0, 0);
	}
	else{
		pspDebugScreenPuts(" Error while creating callback thread.\r\n\r\n");
	}

	// init pspDebug and print the header
	pspDebugScreenInit();
	pspDebugScreenPuts("\r\n");
	pspDebugScreenSetTextColor(RED);
	pspDebugScreenPuts(" I");
	pspDebugScreenSetTextColor(YELLOW);
	pspDebugScreenPuts("S");
	pspDebugScreenSetTextColor(GREEN);
	pspDebugScreenPuts("O");
	pspDebugScreenSetTextColor(WHITE);
	pspDebugScreenPuts("-");
	pspDebugScreenSetTextColor(CYAN);
	pspDebugScreenPuts("METRIC\r\n");
	pspDebugScreenSetTextColor(WHITE);
	pspDebugScreenPuts(" RedHate 2024\r\n");
	pspDebugScreenSetTextColor(WHITE);
	pspDebugScreenPuts("--------------------------------------------------------------------");
	
	pspDebugScreenSetTextColor(RED); 
	pspDebugScreenPuts(" Cross:    ");
	pspDebugScreenSetTextColor(GRAY); 
	pspDebugScreenPuts("Dump to ISO ");
	pspDebugScreenSetTextColor(GRAY); 
	pspDebugScreenPuts("(");
	pspDebugScreenSetTextColor(GREEN); 
	pspDebugScreenPuts("Fast");
	pspDebugScreenSetTextColor(GRAY); 
	pspDebugScreenPuts(")\r\n");
	
	pspDebugScreenSetTextColor(YELLOW); 
	pspDebugScreenPuts(" Square:   ");
	pspDebugScreenSetTextColor(GRAY); 
	pspDebugScreenPuts("Dump to CSO ");
	pspDebugScreenSetTextColor(GRAY); 
	pspDebugScreenPuts("(");
	pspDebugScreenSetTextColor(RED); 
	pspDebugScreenPuts("Slow");
	pspDebugScreenSetTextColor(GRAY); 
	pspDebugScreenPuts(")\r\n");
	
	pspDebugScreenSetTextColor(GREEN); 
	pspDebugScreenPuts(" Triangle: ");
	pspDebugScreenSetTextColor(GRAY); 
	pspDebugScreenPuts("Fetch UMD sha256 sum\r\n");
	
	pspDebugScreenSetTextColor(CYAN); 
	pspDebugScreenPuts(" Circle:");
	pspDebugScreenSetTextColor(GRAY); 
	pspDebugScreenPuts("   Hold Cancel \r\n");
	
	pspDebugScreenSetTextColor(WHITE);
	pspDebugScreenPuts("--------------------------------------------------------------------");
	
	//init umd drive and grab info
	umd_init();

	// finally set up the controls
	SceCtrlData pad;
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	
	// loop it! get loopy!
	while(running){
		
		// read the button input
    	sceCtrlReadBufferPositive(&pad, 1); 

		// if some button input has happened
		if (pad.Buttons != 0){
			

			// have to write function for hash checking files and discs
			// if triangle has been pressed dump the text file for the disc in the drive
			if (pad.Buttons & PSP_CTRL_TRIANGLE){
				//pspDebugScreenPuts(&img_path);
				if(umd_sha_256() > 0){
					//success
				}
			}
			
			// if cross has been pressed dump iso and txt file
			if (pad.Buttons & PSP_CTRL_CROSS){
				//dump the umd to iso
				if(dump_umd(0) > 0){
					// success
				}
			}
			
			// if cross has been pressed dump iso and txt file and compress to cso
			if (pad.Buttons & PSP_CTRL_SQUARE){
				//dump the umd to cso level 9
				if(dump_umd(9) > 0){
					// success
				}
			}
			
		}
	}

	// deactivate the disc drive
	sceUmdDeactivate(1, "disc0:");
	// terminate the callback thread as well if its still going for some reason
	sceKernelTerminateThread(CallbackThreadID);
	// exit
	sceKernelExitGame();
	
	return 0;
	
}
