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

// extra libs
#include "../ciso/ciso.h"
#include "../error/error.h"
#include "../common/colors.h"
#include "../sha/sha256.h"

// a couple important buffers
char GameID[10];

// holder for original game name
unsigned char GameName[256];

// holder for mutated game name
unsigned char FileName[256];

// holder for param.sfo
unsigned char ParamSFO[1024];

// holder for path to iso|cso
char img_path[256];


// holder for sha256 sum
uint8_t umd_sha_sum[32];

// holder for sha256 sum
uint8_t iso_sha_sum[32];

// holder for sha256 sum
uint8_t cso_sha_sum[32];


// not completely happy with this function but it is what it is for now
// it does what it's supposed to do and strips any symbols away
void derive_game_name(unsigned char *buffer){
	
	int i;
	// single pass works
	// replace odd symbols in game name with ? first pass
	for(i=0;i<strlen((char*)FileName);i++){
		//check byte boundaries to make sure they are in ascii bounds
		if(((buffer[i]>=0x00) && (buffer[i]<0x20)) || 
		   ((buffer[i]>=0x7A) && (buffer[i]<=0xFF))){
			// we use a ? so in the two passes we call at the bottom of the
			// parent function it knows what to strip out.
			buffer[i]='?';
		}
	}
	
	// single pass not working so fnc we call twice.... bleh the only kludge...
	void strip_string(unsigned char *buffer){
		
		// strip symbols from game names these arent allowed by most file systems
		for(i=0;i<strlen((char*)buffer);i++){
			
		 //list of conditionals to be stripped from strings
		 if((buffer[i] == '\\') ||
		    (buffer[i] == '/') ||
		    (buffer[i] == ':') ||
		    (buffer[i] == '*') ||
		    (buffer[i] == '?') ||
		    (buffer[i] == '"') ||
		    (buffer[i] == '>') ||
		    (buffer[i] == '<') ||
		    (buffer[i] == '|')){
				//loop through the string starting at the current offset
				int j;
				for(j=i;j<strlen((char*)buffer);j++){
					// shift the string to the left to remove any off characters from strings
					buffer[j]=buffer[j+1];
				}
			}
		}
	}
	
	// simply couldnt devise a better way of making this happen... 
	strip_string(buffer);
	// so we call it twice... 
	strip_string(buffer);

}

int make_info_txt(){
	

	// open a file to print all the information too!
	char txt_path[128];
	sprintf(txt_path, "ms0:/iso/%s - [%s].txt", FileName, GameID);
	int fd = sceIoOpen(txt_path, PSP_O_WRONLY | PSP_O_CREAT, 0777);
	if(fd > 0){
		
		sceIoWrite(fd, "Game ID:", 8);
		sceIoWrite(fd, &GameID[0], 10);
		sceIoWrite(fd, "\r\n", 2);
		
		sceIoWrite(fd, "Game Name:", 10);
		sceIoWrite(fd, &GameName[0], strlen((char*)&GameName[0]));
		sceIoWrite(fd, "\r\n", 2);

		sceIoWrite(fd, "sha256:", 7);
		char sum_buf[2];
		int i;
		for(i=0;i<32;i++){
			sprintf(sum_buf, "%02hx", (unsigned char)iso_sha_sum[i]);
			//pspDebugScreenPuts(sum_buf);
			sceIoWrite(fd, sum_buf, 2);
		}
		sceIoWrite(fd, "\r\n", 2);
		sceIoClose(fd);
		return 1;
	}
	
	return 0;
	
}

void umd_init(){
	
	// check for presence of umd / wait for one to be present
	if(sceUmdCheckMedium() == 0){
	   pspDebugScreenPuts(" Insert a UMD to continue\r\n\r\n");
	   sceUmdWaitDriveStat(PSP_UMD_PRESENT);
	}
	// Mount UMD to disc0: file system
	sceUmdActivate(1, "disc0:"); 
	sceUmdWaitDriveStat(PSP_UMD_READY);


	// UMD_DATA.BIN
	// grab the game id from the umd_data.bin
	signed int fda;
	do{
		fda=sceIoOpen("disc0:/UMD_DATA.BIN", PSP_O_RDONLY, 0777); 
		sceKernelDelayThread(10000);
	} while(fda<=0);
	// read the 10 byte game id from the the umd_data.bin
	sceIoRead(fda, GameID, 10);
	sceIoClose(fda);
	// print the game id
	pspDebugScreenSetTextColor(RED);
	pspDebugScreenPuts(" UMD: Game ID:   ");
	pspDebugScreenSetTextColor(WHITE);
	pspDebugScreenPuts(GameID);
	pspDebugScreenPuts("\r\n");
	
	
	// PARAM.SFO
	// grab the game name from the param.sfo
	do{
		fda=sceIoOpen("disc0:/PSP_GAME/PARAM.SFO", PSP_O_RDONLY, 0777); 
		sceKernelDelayThread(10000);
	} while(fda<=0);
	// read the full param.sfo into memory
	sceIoRead(fda, ParamSFO, 1024);
	sceIoClose(fda);
	// scan the param.sfo for the name of the game
	int i;
	for(i=0;i<1024;i++){
		// since we know its a umd we can scan for the UG tag 
		// and from there scan for the 0x80 byte that occurs 2 bytes before the game title
		if((ParamSFO[i] == 'U') && (ParamSFO[i+1] == 'G')){
			int j;
			for(j=0;j<1024;j++){
				if(ParamSFO[i+j] == 0x80){
					
					// works but picks up widechars wtf
					memcpy(&GameName[0],(char*)&ParamSFO[i+j+3],strlen((char*)&ParamSFO[i+j+3]));
					memcpy(&FileName[0],(char*)&ParamSFO[i+j+3],strlen((char*)&ParamSFO[i+j+3]));
					
				}
			}
		}
	}
	// print the game name
	pspDebugScreenSetTextColor(YELLOW);
	pspDebugScreenPuts(" UMD: Real Name: ");
	pspDebugScreenSetTextColor(WHITE);
	pspDebugScreenPuts((char*)GameName);
	pspDebugScreenPuts("\r\n");


	// strip odd symbols from game names
	derive_game_name(FileName);
	// print the output file name
	pspDebugScreenSetTextColor(GREEN);
	pspDebugScreenPuts(" ISO: File Name: ");
	pspDebugScreenSetTextColor(WHITE);
	pspDebugScreenPuts((char*)FileName);
	pspDebugScreenPuts(".(iso|cso)\r\n");
	
	// set up the file path for the image
	sprintf(img_path, "ms0:/iso/%s - [%s].iso", FileName, GameID);
	
	
}

int dump_umd(int ciso_level){
	
	sha256_state_t s;
	sha256_init(&s);
	
	// set up controls
	SceCtrlData exit;
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	
	// open the input file descriptor (UMD)
	int fd_in = sceIoOpen("umd:", PSP_O_RDONLY, 0777);
	if(fd_in > 0){
		
		// Get the size of the file we opened
		unsigned int max_sec = sceIoLseek(fd_in, 0, SEEK_END); 
							   sceIoLseek(fd_in, 0, SEEK_SET);
		
		// this is the metric part it scales the read / write buffer to 1% of the overall file size.
		#define SECTOR_SIZE 	2048							// block size in relation to iso joilet
		#define CYCLE_PERCISION 100								// units to divide the overall amount of sectors by
		#define COPY_SIZE 		(max_sec / CYCLE_PERCISION) 	// the overall amount of sectors / 100
		#define ROUND_OFFSET  	(COPY_SIZE * CYCLE_PERCISION)	// sectors rounded to the hundredth
		#define REMAINER  		(max_sec - ROUND_OFFSET)		// remainder of sectors rounded from the hundredth (read and written in the first pass)
		
		// print some info about the discs size
		pspDebugScreenSetTextColor(CYAN);
		pspDebugScreenPuts(" UMD: Size MB:   ");
		char max_sec_buf[32];
		sprintf(max_sec_buf, "%d\r\n", (max_sec*SECTOR_SIZE/(1024*1024)));
		pspDebugScreenSetTextColor(WHITE);
		pspDebugScreenPuts(max_sec_buf);
		
		// remove the image if it's already there
		sceIoRemove(img_path);
		
		// open output file descriptor (ISO)
		int fd_out = sceIoOpen(img_path, PSP_O_WRONLY | PSP_O_CREAT, 0777);
		if(fd_out > 0){
		
			// for keeping track of the position, used to calculate mb written and determin the end of the read write process
			unsigned int cur_sec=0;
			
			// set up read write buffer
			unsigned char *rw_buffer = malloc(SECTOR_SIZE*COPY_SIZE*sizeof(unsigned char));
			
			// info on the screen
			char pos_buf[32];
			
			// first we write the odd amount of sectors so we can proceed with rounded math
			if(sceIoRead(fd_in,  &rw_buffer[0], REMAINER) > 0){
				if(sceIoWrite(fd_out, &rw_buffer[0], REMAINER*SECTOR_SIZE) > 0){
					// update sha256
					sha256_update(&s, &rw_buffer[0], REMAINER*SECTOR_SIZE);
					// increment offset
					cur_sec+=REMAINER;
					// try to pring in mb.
					pspDebugScreenSetTextColor(RED);
					pspDebugScreenPuts(" UMD: MB Copied: ");
					pspDebugScreenSetTextColor(WHITE);
					sprintf(pos_buf, "%d\r", cur_sec*SECTOR_SIZE/(1024*1024));
					pspDebugScreenPuts(pos_buf);
				}
				else{
					// we encountered a write error
					error(" ISO: Write error! \r\n",fd_in,fd_out,img_path);
					return 0;
				}
			}
			else{
				// we encountered a read error
				error(" UMD: Read error! \r\n",fd_in,fd_out,img_path);
				return 0;
			}
			
			// now we start looping through the sectors
			while(1){
				
				// exit the loop if it's hit the max sector otherwise it will throw a read error and delete the file
				if(cur_sec >= max_sec){
					break;
				}
				
				// read in the buffer and decided buffer size sceIoRead returns sectors in this instance
				if(sceIoRead(fd_in , &rw_buffer[0], COPY_SIZE) > 0){
					// sceIoWrite expects the amount of sectors times sector size for write
					if(sceIoWrite(fd_out, &rw_buffer[0], COPY_SIZE*SECTOR_SIZE) > 0){
						// update sha256
						sha256_update(&s, &rw_buffer[0], COPY_SIZE*SECTOR_SIZE);
						// increment offset
						cur_sec+=COPY_SIZE;
						// try to pring in mb.
						pspDebugScreenSetTextColor(RED);
						pspDebugScreenPuts(" UMD: MB Copied: ");
						pspDebugScreenSetTextColor(WHITE);
						sprintf(pos_buf, "%d\r", cur_sec*SECTOR_SIZE/(1024*1024));
						pspDebugScreenPuts(pos_buf);
					}
					else{
						// we encountered a write error
						error(" ISO: Write error! \r\n",fd_in,fd_out,img_path);
						return 0;
					}
				}
				else{
					// we encountered a read error
					error(" UMD: Read error! \r\n",fd_in,fd_out,img_path);
					return 0;
				}
				
				// check for bailout conditions
				sceCtrlReadBufferPositive(&exit, 1);
				if (exit.Buttons & PSP_CTRL_CIRCLE){
					error(" UMD: Dump cancelled! \r\n",fd_in,fd_out,img_path);
					return 0;
				}
				
			}
			
			// close the output image
			sceIoClose(fd_out);
			// free the read write buffer
			free(rw_buffer);
			
			// finish the sha count
			sha256_final(iso_sha_sum, &s);
			// space
			pspDebugScreenSetTextColor(MAGENTA);
			pspDebugScreenPuts("\r\n ISO: SHA256:\r\n ");
			pspDebugScreenSetTextColor(WHITE);
			// print the hash sum
			char sum_buf[2];
			int i;
			for(i=0;i<32;i++){
				sprintf(sum_buf, "%02hx", (unsigned char)iso_sha_sum[i]);
				pspDebugScreenPuts(sum_buf);
			}
			pspDebugScreenPuts("\r\n");
			
			// make the information file
			if(make_info_txt() > 0){
				//success
			}
			
			//pspDebugScreenSetXY(0, 20);
			// compression with ciso
			if(ciso_level > 0){
				// compress the cso
				char infile[128], outfile[128];
				sprintf(infile,  "ms0:/iso/%s - [%s].iso", FileName, GameID);
				sprintf(outfile, "ms0:/iso/%s - [%s].cso", FileName, GameID);
				if(comp_ciso(ciso_level, &infile[0], &outfile[0]) > 0){
					// success
					// get rid of the iso
					pspDebugScreenPuts(" ISO: Deleting iso\r\n");
					char img_path[128];
					sprintf(img_path, "ms0:/iso/%s - [%s].iso", FileName, GameID);
					sceIoRemove(img_path);
					
				}
				else{
					// failure delete the files
					char img_path[128];
					sprintf(img_path, "ms0:/iso/%s - [%s].iso", FileName, GameID);
					sceIoRemove(img_path);
					sprintf(img_path, "ms0:/iso/%s - [%s].cso", FileName, GameID);
					sceIoRemove(img_path);
					return 0;
				}
			}
			
			
		}
		else{
			// iso write error
			pspDebugScreenPuts(" ISO: Failed to open output file descriptor.\r\n");
			return 0;
		}
		
		// close the input image
		sceIoClose(fd_in);
		
	}
	else{
		// umd read error
		pspDebugScreenPuts(" UMD: Failed to open input file descriptor.\r\n");
		return 0;
	}
	
	
	return 1;
	
}

// read only get hash from path
int umd_sha_256(){
	
	sha256_state_t s;
	sha256_init(&s);
	
	// set up controls
	SceCtrlData exit;
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	
	// open the input file descriptor (UMD)
	int fd_in = sceIoOpen("umd:", PSP_O_RDONLY, 0777);
	if(fd_in > 0){
		
		// Get the size of the file we opened
		unsigned int max_sec = sceIoLseek(fd_in, 0, SEEK_END); 
							   sceIoLseek(fd_in, 0, SEEK_SET);
		
		// this is the metric part it scales the read / write buffer to 1% of the overall file size.
		#define SECTOR_SIZE 	2048							// block size in relation to iso joilet
		#define CYCLE_PERCISION 100								// units to divide the overall amount of sectors by
		#define COPY_SIZE 		(max_sec / CYCLE_PERCISION) 	// the overall amount of sectors / 100
		#define ROUND_OFFSET  	(COPY_SIZE * CYCLE_PERCISION)	// sectors rounded to the hundredth
		#define REMAINER  		(max_sec - ROUND_OFFSET)		// remainder of sectors rounded from the hundredth (read and written in the first pass)
		
		// print some info about the discs size
		pspDebugScreenSetTextColor(CYAN);
		pspDebugScreenPuts(" SHA: Size MB:   ");
		char max_sec_buf[32];
		sprintf(max_sec_buf, "%d\r\n", (max_sec*SECTOR_SIZE/(1024*1024)));
		pspDebugScreenSetTextColor(WHITE);
		pspDebugScreenPuts(max_sec_buf);
		
		// for keeping track of the position, used to calculate mb written and determin the end of the read write process
		unsigned int cur_sec=0;
		
		// set up read write buffer
		unsigned char *rw_buffer = malloc(SECTOR_SIZE*COPY_SIZE*sizeof(unsigned char));
		
		// info on the screen
		char pos_buf[32];
		
		// first we write the odd amount of sectors so we can proceed with rounded math
		if(sceIoRead(fd_in,  &rw_buffer[0], REMAINER) > 0){
			// update sha256
			sha256_update(&s, &rw_buffer[0], REMAINER*SECTOR_SIZE);
			// increment offset
			cur_sec+=REMAINER;
			// try to pring in mb.
			pspDebugScreenSetTextColor(RED);
			pspDebugScreenPuts(" SHA: MB Read: ");
			pspDebugScreenSetTextColor(WHITE);
			sprintf(pos_buf, "%d\r", cur_sec*SECTOR_SIZE/(1024*1024));
			pspDebugScreenPuts(pos_buf);
		}
		else{
			// we encountered a read error
			error(" SHA: Read error! \r\n",fd_in,0,NULL);
			return 0;
		}
			
		// now we start looping through the sectors
		while(1){
			
			// exit the loop if it's hit the max sector otherwise it will throw a read error and delete the file
			if(cur_sec >= max_sec){
				break;
			}
			
			// read in the buffer and decided buffer size sceIoRead returns sectors in this instance
			if(sceIoRead(fd_in , &rw_buffer[0], COPY_SIZE) > 0){
				// update sha256
				sha256_update(&s, &rw_buffer[0], COPY_SIZE*SECTOR_SIZE);
				// increment offset
				cur_sec+=COPY_SIZE;
				// try to pring in mb.
				pspDebugScreenSetTextColor(RED);
				pspDebugScreenPuts(" SHA: MB Read: ");
				pspDebugScreenSetTextColor(WHITE);
				sprintf(pos_buf, "%d\r", cur_sec*SECTOR_SIZE/(1024*1024));
				pspDebugScreenPuts(pos_buf);
			}
			else{
				// we encountered a read error
				error(" SHA: Read error! \r\n",fd_in,0,NULL);
				return 0;
			}
			
			// check for bailout conditions
			sceCtrlReadBufferPositive(&exit, 1);
			if (exit.Buttons & PSP_CTRL_CIRCLE){
				error(" SHA: hash cancelled! \r\n",fd_in,0,NULL);
				return 0;
			}
			
		}
		
		// free the read write buffer
		free(rw_buffer);
		
		// close the input image
		sceIoClose(fd_in);
		
		// finish the sha count
		sha256_final(umd_sha_sum, &s);
		// space
		pspDebugScreenSetTextColor(MAGENTA);
		pspDebugScreenPuts("\r\n SHA: SHA256:\r\n ");
		pspDebugScreenSetTextColor(WHITE);
		// print the hash sum
		char sum_buf[2];
		int i;
		for(i=0;i<32;i++){
			sprintf(sum_buf, "%02hx", (unsigned char)umd_sha_sum[i]);
			pspDebugScreenPuts(sum_buf);
		}
		pspDebugScreenPuts("\r\n");
		
	}
	else{
		// umd read error
		pspDebugScreenPuts(" SHA: Failed to open input file descriptor.\r\n");
		return 0;
	}
	
	
	
	return 1;
	
}
