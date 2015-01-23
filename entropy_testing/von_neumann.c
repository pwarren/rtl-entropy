/*
 * Von Neumann whitener, 
 * If specified, reads from given file, else reads from  stdin data.
 * Outputs whitened data to file or stdout.
 *
 * Copyright (C) 2013 by Paul Warren <pwarren@pwarren.id.au>

 * Parts taken from:
 *  - rtl_test. Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
 *  - http://openfortress.org/cryptodoc/random/noise-filter.c
 *      by Rick van Rein <rick@openfortress.nl>
 *  - snd-egd Copyright (C) 2008-2010 Nicholas J. Kain <nicholas aatt kain.us>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 10000

/* Buffers */
unsigned char bitbuffer[BUFFER_SIZE] = {0};
unsigned char buffer[BUFFER_SIZE] = {0};

/* Counters */
unsigned int bitcounter = 0;
unsigned int buffercounter = 0;
unsigned int n_read = 0;

int main(int argc, char **argv) {
  unsigned int i, j;
  int ch, ch2;

  n_read = fread(&buffer, sizeof(buffer[0]), BUFFER_SIZE, stdin);
  while ( n_read > 0) {      
    /* for each byte in the rtl-sdr read buffer
       pick least significant 6 bits
       for now:
       debias, storing useful bits in write buffer, 
       and discarded bits in hash buffer
       until the write buffer is full.
       create a key by SHA512() hashing the hash buffer
       encrypt write buffer with key
       output encrypted buffer
       
    */
    
    /* debias(buffer, bitbuffer, n_read, sizeof(buffer[0])); */
    for (i=0; i < n_read * sizeof(buffer[0]); i++) {
      for (j=0; j < 6; j+= 2) {
	ch = (buffer[i] >> j) & 0x01;
	ch2 = (buffer[i] >> (j+1)) & 0x01;
	if (ch != ch2) {
	  if (ch) {
	    /* store a 1 in our bitbuffer */
	    bitbuffer[buffercounter] |= 1 << bitcounter;
	  } /* else leave it alone, we have a 0 alread */
	  bitcounter++;
	}
      }
      
      /* is byte full? */
      if (bitcounter >= sizeof(bitbuffer[0]) * 8) {
	buffercounter++;
	bitcounter = 0;
      }

      //fprintf(stderr,"bitcounter: %d \t buffercounter: %d\n",bitcounter,buffercounter);
      
      /* is buffer full? */
      if (buffercounter >= BUFFER_SIZE) {
	/* We have 2500 bytes of entropy 
	   Can now write it out! */
	fwrite(&bitbuffer,sizeof(bitbuffer[0]),BUFFER_SIZE,stdout);
	memset(bitbuffer,0,sizeof(bitbuffer));
	buffercounter = 0;
      }
    }
    n_read = fread(&buffer, sizeof(buffer[0]), BUFFER_SIZE, stdin);
  }
  
  return 0;
  
}
