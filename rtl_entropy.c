/*
 * rtl-entropy, turns your Realtek RTL2832 based DVB dongle into a
 * high quality entropy source.
 *
 * Copyright (C) 2013 by Paul Warren <pwarren@pwarren.id.au>

 * Parts taken from:
 *  rtl_test. Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
 *  http://openfortress.org/cryptodoc/random/noise-filter.c
 *    by Rick van Rein <rick@openfortress.nl>
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

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#ifdef __APPLE__
#include <sys/time.h>
#else
#include <time.h>
#endif

#ifndef _WIN32
#include <unistd.h>
#else
#include <Windows.h>
#include "getopt/getopt.h"
#endif

#include "rtl-sdr.h"
#include "fips.h"

#define DEFAULT_SAMPLE_RATE		2048000
#define DEFAULT_ASYNC_BUF_NUMBER	32
#define DEFAULT_BUF_LENGTH		(16 * 16384)
#define MINIMAL_BUF_LENGTH		512
#define MAXIMAL_BUF_LENGTH		(256 * 16384)
#define BUFFER_SIZE                     2500 // need 2500 bits for FIPS

#define MHZ(x)	((x)*1000*1000)

#define DEFAULT_FREQUENCY MHZ(70)

static int do_exit = 0;
static rtlsdr_dev_t *dev = NULL;
static fips_ctx_t fipsctx;		/* Context for the FIPS tests */
int randfd;
int isdaemon = 0;

void usage(void)
{
  fprintf(stderr,
	  "rtl_entropy, a high quality entropy source using RTL2832 based DVB-T receivers\n\n"
	  "Usage:\n"
	  "\t[-s samplerate (default: 2048000 Hz)]\n"
	  "\t[-d device_index (default: 0)]\n"
	  "\t[-f set frequency to listen (default: 54MHz )]\n"
	  "\t[-g daemonise and add to system entropy pool (linux only)]\n"
	  "\t[-o output file] (default: STDOUT)\n"
	  );
  exit(1);
}

#ifdef _WIN32
BOOL WINAPI
sighandler(int signum)
{
  if (CTRL_C_EVENT == signum) {
    do_exit = 1;
    rtlsdr_cancel_async(dev);
    return TRUE;
  }
  return FALSE;
}
#else
static void sighandler(int signum)
{
  do_exit = 1;
  rtlsdr_cancel_async(dev);
}
#endif

double atofs(char* f)
/* standard suffixes */
{
  char* chop;
  double suff = 1.0;
  chop = malloc((strlen(f)+1)*sizeof(char));
  strncpy(chop, f, strlen(f)-1);
  switch (f[strlen(f)-1]) {
  case 'G':
    suff *= 1e3;
  case 'M':
    suff *= 1e3;
  case 'k':
    suff *= 1e3;
    suff *= atof(chop);}
  free(chop);
  if (suff != 1.0) {
    return suff;}
  return atof(f);
}

int main(int argc, char **argv)
{
#ifndef _WIN32
  struct sigaction sigact;
#endif
  int n_read;
  int r, opt;
  unsigned int i, j;
  uint8_t *buffer;
  uint32_t dev_index = 0;
  uint32_t samp_rate = DEFAULT_SAMPLE_RATE;
  uint32_t out_block_size = MAXIMAL_BUF_LENGTH;
  uint32_t frequency = DEFAULT_FREQUENCY;
  int device_count;
  int ch, ch2;
  unsigned char bitbuffer[BUFFER_SIZE] = {0};
  unsigned int bitcounter = 0;
  int buffercounter = 0;
  int gains[100];
  int count, fips_result;

  // File handling stuff
  FILE *output = NULL;
  int redirect_output = 0;

  /*  struct {
    int entropy_count;
    int buf_size;
    char buf[1024];
  } entropy;
  */

  while ((opt = getopt(argc, argv, "d:s:f:g:o:")) != -1) {
    switch (opt) {
    case 'd':
      dev_index = atoi(optarg);
      break;
    case 's':
      samp_rate = (uint32_t)atofs(optarg);
      break;
    case 'f':
      frequency = (uint32_t)atofs(optarg);
      break;
    case 'g':
      isdaemon = 1;
      // TODO: Insert code here to daemonize
    case 'o':
      redirect_output = 1;
      output = fopen(optarg,"w");
      if (output == NULL) {
	perror("Couldn't open output file");
	return 1;
      }
      break;
    default:
      printf("In Default!\n");
      usage();
      break;
    }
  }
  
  if (redirect_output) {
    fclose(stdout);
  } else {
    output = stdout;
  }
  
    
  buffer = malloc(out_block_size * sizeof(uint8_t));
  device_count = rtlsdr_get_device_count();
  if (!device_count) {
    fprintf(stderr, "No supported devices found.\n");
    exit(1);
  }
  
  fprintf(stderr, "Found %d device(s):\n", device_count);
  for (i = 0; i <(unsigned int)device_count; i++)
    fprintf(stderr, "  %d:  %s\n", i, rtlsdr_get_device_name(i));
  fprintf(stderr, "\n");
  
  fprintf(stderr, "Using device %d: %s\n",
	  dev_index,
	  rtlsdr_get_device_name(dev_index));
  
  r = rtlsdr_open(&dev, dev_index);
  if (r < 0) {
    fprintf(stderr, "Failed to open rtlsdr device #%d.\n", dev_index);
    exit(1);
  }
  
#ifndef _WIN32
  sigact.sa_handler = sighandler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;
  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
  sigaction(SIGQUIT, &sigact, NULL);
  sigaction(SIGPIPE, &sigact, NULL);
#else
  SetConsoleCtrlHandler( (PHANDLER_ROUTINE) sighandler, TRUE );
#endif
  
  /* Set the sample rate */
  r = rtlsdr_set_sample_rate(dev, samp_rate);
  if (r < 0)
    fprintf(stderr, "WARNING: Failed to set sample rate.\n");
  
  /* Reset endpoint before we start reading from it (mandatory) */
  r = rtlsdr_reset_buffer(dev);
  if (r < 0)
    fprintf(stderr, "WARNING: Failed to reset buffers.\n");
  
  /* Set start frequency */
  fprintf(stderr, "Setting Frequency to %d\n", frequency);
  r = rtlsdr_set_center_freq(dev, (uint32_t)frequency);
    
  /* set gain to max */
  count = rtlsdr_get_tuner_gains(dev, gains);
  fprintf(stderr, "Setting gain to %.1f\n", (float)(gains[count-1]/10));
  r = rtlsdr_set_tuner_gain_mode(dev, 1);
  if (r < 0)
    fprintf(stderr, "WARNING: Couldn't set gain mode to manual\n");
  
  r = rtlsdr_set_tuner_gain(dev, gains[count-1]);
  if (r < 0)
    fprintf(stderr, "WARNING: Failed to set gain\n");

  fprintf(stderr, "Doing FIPS init\n");
  fips_init(&fipsctx, (int)0);

  fprintf(stderr, "Reading samples in sync mode...\n");
  while (!do_exit) {
    r = rtlsdr_read_sync(dev, buffer, out_block_size, &n_read);
    if (r < 0) {
      fprintf(stderr, "ERROR: sync read failed.\n");
      break;
    }
    if ((uint32_t)n_read < out_block_size) {
      fprintf(stderr, "ERROR: Short read, samples lost, exiting!\n");
      break;
    }
    
    // for each byte in the read buffer
    // pick least significant 4 bits
    // good compromise between output throughput, and entropy quality
    // get less FIPS fails with 2 bits, but half the througput
    // get lots of FIPS fails and only a slight throughput increase with 6 bits
    
    // debias and store in the write buffer till it's full
    for (i=0; i < n_read * sizeof(buffer[0]); i++) {
      for (j=0; j < 4; j+= 2) {
	ch = (buffer[i] >> j) & 0x01;
	ch2 = (buffer[i] >> (j+1)) & 0x01;
	if (ch != ch2) {
	  if (ch) {
	    // store a 1 in our bitbuffer
	    bitbuffer[buffercounter] |= 1 << bitcounter;
	    // the buffer will already be all zeroes, as it's set to that when full, and when initialised.
	    //} else {
	    // store a 0, yay for bitwise C magic (aka "I've no idea how this works!")
	    //bitbuffer[buffercounter] &= ~(1 << bitcounter);
	  }
	  bitcounter++;
	}
	// if our byte is full 
	if (bitcounter >= sizeof(bitbuffer[0]) * 8) { //bits per byte
	  buffercounter++;
	  bitcounter = 0;
	}
	// if our buffer is full
	if (buffercounter > BUFFER_SIZE) {
	  // We have 2500 bytes of entropy
	  // Can now send it to FIPS!
	  fips_result = fips_run_rng_test(&fipsctx, &bitbuffer);
	  if (!fips_result) {
	    // hooray it's proper random data
	    fwrite(&bitbuffer,sizeof(bitbuffer[0]),BUFFER_SIZE,output);	 
	  } else {
	    // FIPS test failed
	    for (j=0; j< N_FIPS_TESTS; j++) {
	      if (fips_result & fips_test_mask[j]) {
		fprintf(stderr, "Failed: %s\n", fips_test_names[j]);
	      }
	    }
	  }
	  // reset it, and the counter
	  memset(bitbuffer,0,sizeof(bitbuffer));
	  buffercounter = 0;
	}
      }
    }
  }
  if (do_exit) {
    fprintf(stderr, "\nUser cancel, exiting...\n");
  }
  else
    fprintf(stderr, "\nLibrary error %d, exiting...\n", r);
  
  rtlsdr_close(dev);
  free (buffer);
  fclose(output);
  return 0;
}


