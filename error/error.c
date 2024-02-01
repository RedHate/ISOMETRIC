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

// extra libs
#include "../common/colors.h"

void error(const char *reason, int fd_in, int fd_out, const char *img_path){
	
	// umd dump specific error instance
	pspDebugScreenPuts("\r\n");
	// print the information
	pspDebugScreenSetTextColor(RED);
	if(reason != NULL) pspDebugScreenPuts(reason);
	pspDebugScreenSetTextColor(WHITE);
	// close the file descriptors
	if(fd_out != 0) sceIoClose(fd_out);
	if(fd_in != 0) sceIoClose(fd_in);
	// remove the iso
	if(img_path != NULL) sceIoRemove(img_path);
	sceKernelDelayThread(1000000);
	
}
