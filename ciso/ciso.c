/*
    This file is part of Ciso.

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


    Copyright 2005 BOOSTER
*/

#include <pspkernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <zconf.h>
#include <pspdebug.h>
#include <pspctrl.h>

#include "ciso.h"
#include "../error/error.h"
#include "../common/colors.h"

char debug_buffer[128];

z_stream z;

uint32_t *index_buf = NULL;
uint32_t *crc_buf = NULL;
uint8_t *block_buf1 = NULL;
uint8_t *block_buf2 = NULL;

/****************************************************************************
	compress ISO to CSO (ported to psp)
****************************************************************************/

CISO_H ciso;
int ciso_total_block;

uint64_t check_file_size(int fp){
	
	uint64_t pos = sceIoLseek(fp, 0, SEEK_END); 
				   sceIoLseek(fp, 0, SEEK_SET);

	if(pos==-1) return pos;

	/* init ciso header */
	memset(&ciso,0,sizeof(ciso));

	ciso.magic[0] = 'C';
	ciso.magic[1] = 'I';
	ciso.magic[2] = 'S';
	ciso.magic[3] = 'O';
	ciso.ver      = 0x01;

	ciso.block_size  = 0x800; /* ISO9660 one of sector */
	ciso.total_bytes = pos;
#if 0
	/* align >0 has bug */
	for(ciso.align = 0 ; (ciso.total_bytes >> ciso.align) >0x80000000LL ; ciso.align++);
#endif

	ciso_total_block = pos / ciso.block_size ;

	sceIoLseek(fp,0,SEEK_SET);

	return pos;
	
}

/****************************************************************************
	decompress CSO to ISO (not ported)
****************************************************************************/
/*
int decomp_ciso(const char *fname_in, const char *fname_out){
	
	FILE *fin;
	if ((fin = fopen(fname_in, "rb")) == NULL){
		sprintf(debug_buffer," CISO: Can't open %s\n", fname_in);
		error(debug_buffer, fin, NULL, fname_out);
		return 0;
	}
	FILE *fout;
	if ((fout = fopen(fname_out, "wb")) == NULL){
		sprintf(debug_buffer," CISO: Can't create %s\n", fname_out);
		error(debug_buffer, NULL, fout, fname_out);
		return 0;
	}
	
	uint32_t index , index2;
	uint64_t read_pos , read_size;
	int index_size;
	int block;
	int cmp_size;
	int status;
	int percent_period;
	int percent_cnt;
	int plain;

	// read header 
	if( fread(&ciso, 1, sizeof(ciso), fin) != sizeof(ciso) ){
		error(" CISO: File read error\n", fin, fout, fname_out);
		return 0;
	}

	// check header
	if(ciso.magic[0] != 'C' ||
	   ciso.magic[1] != 'I' ||
	   ciso.magic[2] != 'S' ||
	   ciso.magic[3] != 'O' ||
	   ciso.block_size ==0  ||
	   ciso.total_bytes == 0){
		error(" CISO: File format error\n", fin, fout, fname_out);
		return 0;
	   }
	 
	ciso_total_block = ciso.total_bytes / ciso.block_size;

	// allocate index block
	index_size = (ciso_total_block + 1 ) * sizeof(uint32_t);
	index_buf  = malloc(index_size);
	block_buf1 = malloc(ciso.block_size);
	block_buf2 = malloc(ciso.block_size*2);

	if( !index_buf || !block_buf1 || !block_buf2 ){
		error(" CISO: Can't allocate memory\n", fin, fout, fname_out);
		return 0;
	}
	memset(index_buf,0,index_size);

	// read index block
	if( fread(index_buf, 1, index_size, fin) != index_size ){
		error(" CISO: File read error\n", fin, fout, fname_out);
		return 0;
	}

	// show info
	//sprintf(debug_buffer," Decompress '%s' to '%s'\n",fname_in,fname_out);	pspDebugScreenPuts(debug_buffer);
	
	pspDebugScreenSetTextColor(WHITE);	pspDebugScreenPuts(" CISO: Total File Size: "); 	pspDebugScreenSetTextColor(CYAN);   sprintf(debug_buffer,"%lld bytes\n",ciso.total_bytes);		pspDebugScreenPuts(debug_buffer);
	pspDebugScreenSetTextColor(WHITE);  pspDebugScreenPuts(" CISO: Block Size: "); 		    pspDebugScreenSetTextColor(GREEN);  sprintf(debug_buffer,"%d  bytes\n",ciso.block_size);			pspDebugScreenPuts(debug_buffer);
	pspDebugScreenSetTextColor(WHITE);	pspDebugScreenPuts(" CISO: Total Blocks: "); 	    pspDebugScreenSetTextColor(YELLOW); sprintf(debug_buffer,"%d  blocks\n",ciso_total_block);		pspDebugScreenPuts(debug_buffer);
	pspDebugScreenSetTextColor(WHITE);	pspDebugScreenPuts(" CISO: Index Alignment: "); 	pspDebugScreenSetTextColor(RED);    sprintf(debug_buffer,"%d\n",1<<ciso.align);            		pspDebugScreenPuts(debug_buffer);
	pspDebugScreenSetTextColor(WHITE);
	
	// init zlib
	z.zalloc = Z_NULL;
	z.zfree  = Z_NULL;
	z.opaque = Z_NULL;

	// decompress data
	percent_period = ciso_total_block/100;
	percent_cnt = 0;

	// finally set up the controls
	SceCtrlData pad;
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

	for(block = 0;block < ciso_total_block ; block++){
		
		if(--percent_cnt<=0){
			percent_cnt = percent_period;
			pspDebugScreenSetTextColor(WHITE); 
			pspDebugScreenPuts(" CISO: Decompress: "); 
			pspDebugScreenSetTextColor(CYAN);
			sprintf(debug_buffer,"%d%%\r",block / percent_period);
			pspDebugScreenPuts(debug_buffer);
		}

		if (inflateInit2(&z,-15) != Z_OK){;
			sprintf(debug_buffer," CISO:  DeflateInit: %s\n", (z.msg) ? z.msg : "???");
			pspDebugScreenPuts(debug_buffer);
			error(debug_buffer, fin, fout, fname_out);
			return 0;
		}

		// check index
		index  = index_buf[block];
		plain  = index & 0x80000000;
		index  &= 0x7fffffff;
		read_pos = index << (ciso.align);
		if(plain){
			read_size = ciso.block_size;
		}
		else{
			index2 = index_buf[block+1] & 0x7fffffff;
			read_size = (index2-index) << (ciso.align);
		}
		fseek(fin,read_pos,SEEK_SET);

		z.avail_in  = fread(block_buf2, 1, read_size , fin);
		if(z.avail_in != read_size){
			sprintf(debug_buffer," CISO:  block: %d read error\r\n",block);
			error(debug_buffer, fin, fout, fname_out);
			return 0;
		}

		if(plain){
			memcpy(block_buf1,block_buf2,read_size);
			cmp_size = read_size;
		}
		else{
			z.next_out  = block_buf1;
			z.avail_out = ciso.block_size;
			z.next_in   = block_buf2;
			status = inflate(&z, Z_FULL_FLUSH);
			if (status != Z_STREAM_END){
				sprintf(debug_buffer," CISO:  block %d:inflate : %s[%d]\r\n", block,(z.msg) ? z.msg : "error",status);
				error(debug_buffer, fin, fout, fname_out);
				return 0;
			}
			cmp_size = ciso.block_size - z.avail_out;
			if(cmp_size != ciso.block_size){
				sprintf(debug_buffer," CISO:  block %d : block size error %d != %d\r\n",block,cmp_size , ciso.block_size);
				error(debug_buffer, fin, fout, fname_out);
				return 0;
			}
		}
		// write decompressed block
		if(fwrite(block_buf1, 1,cmp_size , fout) != cmp_size){
			sprintf(debug_buffer," CISO:  block %d : Write error\r\n",block);
			error(debug_buffer, fin, fout, fname_out);
			return 0;
		}

		// term zlib
		if (inflateEnd(&z) != Z_OK){
			sprintf(debug_buffer," CISO:  inflateEnd : %s\r\n", (z.msg) ? z.msg : "error");
			error(debug_buffer, fin, fout, fname_out);
			return 0;
		}
		
		// read the button input
    	sceCtrlReadBufferPositive(&pad, 1); 

		// if some button input has happened
		if (pad.Buttons != 0){
			// if circle has been pressed break the main loop and exit
			if (pad.Buttons & PSP_CTRL_CIRCLE){
				error(debug_buffer, fin, fout, fname_out);
				// break the loop
				break;
			}
		}
		
	}

	//pspDebugScreenPuts(" CISO: compress completed\r\n");
	
	// free memory
	if(index_buf) free(index_buf);
	if(crc_buf) free(crc_buf);
	if(block_buf1) free(block_buf1);
	if(block_buf2) free(block_buf2);

	// close files
	fclose(fin);
	fclose(fout);
	
	return 1;
	
}
*/

/****************************************************************************
	compress ISO (ported to psp)
****************************************************************************/
int comp_ciso(int level, const char *fname_in, const char *fname_out){
	
	int fin = sceIoOpen(fname_in, PSP_O_RDONLY, 0777);
	if(fin <= 0){
		sprintf(debug_buffer," CISO: Failed to open %s\r\n", fname_in);
		error(debug_buffer, fin, 0, fname_out);
		return 0;
	}
	int fout = sceIoOpen(fname_out, PSP_O_WRONLY | PSP_O_CREAT, 0777);
	if(fout <= 0){
		sprintf(debug_buffer," CISO: Failed to create %s\r\n", fname_out);
		error(debug_buffer, 0, fout, fname_out);
		return 0;
	}

	uint64_t file_size;
	uint64_t write_pos;

	int index_size;
	int block;
	unsigned char buf4[64];
	int cmp_size;
	int status;
	int percent_period;
	int percent_cnt;
	int align,align_b,align_m;

	file_size = check_file_size(fin);
	if(file_size==(uint64_t)-1LL){
		error(" CISO: Can't get file size\r\n", fin, fout, fname_out);
		return 0;
	}

	// allocate index block
	index_size = (ciso_total_block + 1 ) * sizeof(uint32_t);
	index_buf  = malloc(index_size);
	crc_buf    = malloc(index_size);
	block_buf1 = malloc(ciso.block_size);
	block_buf2 = malloc(ciso.block_size*2);

	if( !index_buf || !crc_buf || !block_buf1 || !block_buf2 ){
		error(" CISO:  Can't allocate memory\r\n", fin, fout, fname_out);
		return 0;
	}
	memset(index_buf,0,index_size);
	memset(crc_buf,0,index_size);
	memset(buf4,0,sizeof(buf4));

	// init zlib
	z.zalloc = Z_NULL;
	z.zfree  = Z_NULL;
	z.opaque = Z_NULL;

	// show inf
	//sprintf(debug_buffer," Compress '%s' to '%s'\n",fname_in,fname_out);		pspDebugScreenPuts(debug_buffer);
	pspDebugScreenSetTextColor(CYAN);     pspDebugScreenPuts(" CISO: Total File Size: "); 	 pspDebugScreenSetTextColor(WHITE);   sprintf(debug_buffer,"%lld bytes\r\n",ciso.total_bytes);	pspDebugScreenPuts(debug_buffer);
	pspDebugScreenSetTextColor(GREEN);    pspDebugScreenPuts(" CISO: Block Size: "); 		 pspDebugScreenSetTextColor(WHITE);  sprintf(debug_buffer,"%d  bytes\r\n",ciso.block_size);		pspDebugScreenPuts(debug_buffer);
	pspDebugScreenSetTextColor(YELLOW);   pspDebugScreenPuts(" CISO: Index Alignment: "); 	 pspDebugScreenSetTextColor(WHITE); sprintf(debug_buffer,"%d\r\n",1<<ciso.align);				pspDebugScreenPuts(debug_buffer);
	pspDebugScreenSetTextColor(RED); 	  pspDebugScreenPuts(" CISO: Compression Level: ");  pspDebugScreenSetTextColor(WHITE);    sprintf(debug_buffer,"%d\r\n",level);					    pspDebugScreenPuts(debug_buffer);
	pspDebugScreenSetTextColor(WHITE);
	
	
	// write header block
	sceIoWrite(fout, &ciso, sizeof(ciso));

	// dummy write index block
	sceIoWrite(fout, index_buf, index_size);

	write_pos = sizeof(ciso) + index_size;

	// compress data 
	percent_period = ciso_total_block/100;
	percent_cnt    = ciso_total_block/100;

	align_b = 1<<(ciso.align);
	align_m = align_b -1;

	// finally set up the controls
	SceCtrlData pad;
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

	for(block = 0;block < ciso_total_block ; block++){
		
		if(--percent_cnt<=0){
			percent_cnt = percent_period;
			pspDebugScreenSetTextColor(CYAN);
			pspDebugScreenPuts(" CISO: Compress ");
			pspDebugScreenSetTextColor(WHITE);
			sprintf(debug_buffer,"%3d%%", block / percent_period);
			pspDebugScreenPuts(debug_buffer);
			pspDebugScreenSetTextColor(CYAN);
			pspDebugScreenPuts("  Avarage Rate ");
			pspDebugScreenSetTextColor(WHITE);
			sprintf(debug_buffer,"%3d%%\r", block==0 ? 0 : (uint32_t)(100*write_pos/(block*0x800)));
			pspDebugScreenPuts(debug_buffer);
			pspDebugScreenSetTextColor(WHITE);
			
		}

		if (deflateInit2(&z, level , Z_DEFLATED, -15,8,Z_DEFAULT_STRATEGY) != Z_OK){
			sprintf(debug_buffer," CISO: deflateInit : %s\r\n", (z.msg) ? z.msg : "???");
			error(debug_buffer, fin, fout, fname_out);
			return 0;
		}

		/* write align */
		align = (int)write_pos & align_m;
		if(align){
			align = align_b - align;
			
			if(sceIoWrite(fout, buf4, align) != align){
				sprintf(debug_buffer," CISO: block %d : Write error\r\n",block);
				pspDebugScreenPuts(debug_buffer);
				return 0;
			}
			write_pos += align;
		}

		// mark offset index
		index_buf[block] = write_pos>>(ciso.align);

		// read buffer
		z.next_out  = block_buf2;
		z.avail_out = ciso.block_size*2;
		z.next_in   = block_buf1;
		z.avail_in  = sceIoRead(fin, block_buf1, ciso.block_size);
		
		if(z.avail_in != ciso.block_size){
			sprintf(debug_buffer," CISO: block=%d : read error\r\n",block);
			error(debug_buffer, fin, fout, fname_out);
			return 0;
		}

		// compress block
		// status = deflate(&z, Z_FULL_FLUSH);
		status = deflate(&z, Z_FINISH);
		if (status != Z_STREAM_END){
			sprintf(debug_buffer," CISO: block %d:deflate : %s[%d]\r\n", block,(z.msg) ? z.msg : "error",status);
			error(debug_buffer, fin, fout, fname_out);
			return 0;
		}

		cmp_size = ciso.block_size*2 - z.avail_out;

		// choise plain / compress
		if(cmp_size >= ciso.block_size){
			cmp_size = ciso.block_size;
			memcpy(block_buf2,block_buf1,cmp_size);
			/* plain block mark */
			index_buf[block] |= 0x80000000;
		}

		// write compressed block
		if(sceIoWrite(fout, block_buf2, cmp_size) != cmp_size){
			sprintf(debug_buffer," CISO: block %d : Write error\r\n",block);
			error(debug_buffer, fin, fout, fname_out);
			return 0;
		}

		// mark next index
		write_pos += cmp_size;

		// term zlib
		if (deflateEnd(&z) != Z_OK){
			sprintf(debug_buffer," CISO: deflateEnd : %s\r\n", (z.msg) ? z.msg : "error");
			error(debug_buffer, fin, fout, fname_out);
			return 0;
		}
	
		// read the button input
    	sceCtrlReadBufferPositive(&pad, 1); 

		// if some button input has happened
		if (pad.Buttons != 0){
			// if circle has been pressed break the main loop and exit
			if (pad.Buttons & PSP_CTRL_CIRCLE){
				// break the loop
				break;
			}
		}
	
	}

	// last position (total size)
	index_buf[block] = write_pos>>(ciso.align);

	// write header & index block
	sceIoLseek(fout,sizeof(ciso),SEEK_SET);
	sceIoWrite(fout, index_buf, index_size);

	sprintf(debug_buffer," CISO: Compress completed , total size = %8d bytes , rate %d%%\r\n",
	(int)write_pos,(int)(write_pos*100/ciso.total_bytes));
	pspDebugScreenPuts(debug_buffer);
	
	// free memory
	if(index_buf) free(index_buf);
	if(crc_buf) free(crc_buf);
	if(block_buf1) free(block_buf1);
	if(block_buf2) free(block_buf2);

	// close files
	sceIoClose(fin);
	sceIoClose(fout);
	
	
	return 1;
	
}
