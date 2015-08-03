/*
 * amls.c
 * Reads from  stdin data.
 * Outputs whitened data to stdout.
 *
 * Copyright (C) 2013 by Paul Warren <pwarren@pwarren.id.au>


 * Adapted from amls.c as on
 * http://www.ciphergoth.org/software/unbiasing
 *
 * Paul Crowley <paul@ciphergoth.org>, corners@sbcglobal.net
 * December 2001
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

struct bitpointer {
  unsigned int *byte;
  unsigned int offset;
};

void amls_round(int *input_start, int * input_end, int **output ) {
  
  int *low = input_start;
  int *high = input_end -1;
  int *doubles = input_start;

  if (high <= low)
    return;
  do {
    //if (value at low == value at high) {
    //  set value at doubles to value at low
    //  increment doubles location
    //  set bit at high to 0
    // } else {
    // set value at value of output to value at low
    // increment value at output
    // }
    // increment low
    // decrement high

  } while (high > low);
  //recurse it!
  amls_round(input_start, doubles, output);
  amls_round(high + 1, input_end, output);
}
  
int main(int argc, char **argv) {

  int *output;
  
  n_read = fread(&buffer, sizeof(buffer[0]), BUFFER_SIZE, stdin);
  while ( n_read > 0) {      
    // do amls on the read bits.
    amls_round(&buffer, &buffer + nread, &output);
    fwrite(&output,sizeof(bitbuffer[0]),wherever amls got to, stdout);
    
    n_read = fread(&buffer, sizeof(buffer[0]), BUFFER_SIZE, stdin);
  }
  
  return 0;
  
}
