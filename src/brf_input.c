/*
 * brf-entropy, turns your BladeRF into a high bandwidth,
 * high quality entropy source.
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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <grp.h>
#include <openssl/sha.h>
#include <openssl/aes.h>

#ifdef __APPLE__
#include <sys/time.h>
#else
#include <time.h>
#include <sys/capability.h>
#include <sys/prctl.h>
#endif

#include <pthread.h>
#include <libbladeRF.h>
#include "fips.h"
#include "util.h"
#include "log.h"
#include "defines.h"

/*  Globals. */
static int do_exit = 0;
static fips_ctx_t fipsctx;

/* bladerf bits */
uint32_t samp_rate = 40000000;
int actual_samp_rate = 0;
long frequency = 434000000;
float gain = 1000.0;
struct bladerf *dev;
pthread_t rx_task;
struct bladerf_stream *rx_stream;

/* flags */
int output_ready;

/* daemon */
int uid = -1, gid = -1;

/* File handling stuff */
FILE *output = NULL;

/* Buffers */
unsigned char bitbuffer[BUFFER_SIZE] = {0};
unsigned char bitbuffer_old[BUFFER_SIZE] = {0};
void **buffers;

/* Counters */
unsigned int bitcounter = 0;
unsigned int buffercounter = 0;


/* on Callback, write the output, dispatch a handler?
 * Not sure on architecture here.
 *
 */
static void *rx_stream_callback(struct bladerf *dev,
				struct bladerf_stream *stream,
				struct bladerf_metadata *meta,
				void *samples,
				size_t num_samples,
				void *user_data)
{ 
  fwrite(&samples,num_samples,BUFFER_SIZE,output);
}

void * rx_task_run(void *inputs) {
  int r;
  r = bladerf_stream(rx_stream, BLADERF_MODULE_RX);
  if (r < 0) {
    log_line(LOG_DEBUG,"RX Stream failure: %s\n",bladerf_strerror(r));
  }
  return NULL;
}


/* TODO will have to rename main */
int main(int argc, char **argv) {
  struct sigaction sigact;
  int samples_per_buffer = 65536;
  int r;

  /* Open device */
  r = bladerf_open(&dev, NULL);
  if (r < 0) {
    log_line(LOG_DEBUG,"Failed to open device: %s", bladerf_strerror(r));
    suicide("");
  }
  
  /* Is FPGA ready? */
  r = bladerf_is_fpga_configured(dev);
  if (r < 0) {
    log_line(LOG_DEBUG, "Failed to determine if FPGA is loaded: %s",
	     bladerf_strerror(r));
    exit(EXIT_FAILURE);
  } else if (r == 0) {
    log_line(LOG_DEBUG, "FPGA is not loaded. Aborting.");
    exit(EXIT_FAILURE);
  }
  log_line(LOG_DEBUG, "FPGA Loaded");

  /* set up verbose logging of libbladerf calls */
  /* bladerf_log_set_verbosity(BLADERF_LOG_LEVEL_VERBOSE); */

  /* Set the sample rate */
  r = bladerf_set_sample_rate(dev, BLADERF_MODULE_RX, samp_rate, &actual_samp_rate);
  if (r < 0) {
    log_line(LOG_DEBUG,"Failed to set sample rate: %s", bladerf_strerror(r));
    exit(EXIT_FAILURE);
  }
  log_line(LOG_DEBUG, "Sample rate set to %d", actual_samp_rate);

  log_line(LOG_DEBUG, "Setting Frequency to %d", frequency);
  r = bladerf_set_frequency(dev, BLADERF_MODULE_RX, frequency);
  if (r < 0) {
    log_line(LOG_DEBUG,"Failed to set frequency: %s", bladerf_strerror(r));
    exit(EXIT_FAILURE);
  }  
  
  /* Set gain */
  r = bladerf_set_rxvga1(dev, gain);
  if (r < 0) {
    log_line(LOG_DEBUG,"Failed to set pre gain: %s",bladerf_strerror(r));
    exit(EXIT_FAILURE);
  }  

  r = bladerf_set_rxvga2(dev, gain);
  if (r < 0) {
    log_line(LOG_DEBUG,"Failed to set post gain: %s", bladerf_strerror(r));
    exit(EXIT_FAILURE);
  }  
  
  r = bladerf_set_lpf_mode(dev, BLADERF_MODULE_RX, BLADERF_LPF_BYPASSED);
  if (r < 0) {
    log_line(LOG_DEBUG, "Failed to  set filter bypass mode: %s", 
	     bladerf_strerror(r));
  }
  
  r = bladerf_enable_module(dev, BLADERF_MODULE_RX, true);
  if (r < 0) {
    log_line(LOG_DEBUG, "Failed to enable RX module: %s",
	     bladerf_strerror(r));
    exit(EXIT_FAILURE);
  } else {
    log_line(LOG_DEBUG,"Enabled RX module");
  }
 
  /*  Initialise the receive stream */
  r = bladerf_init_stream(&rx_stream,
			  dev,
			  rx_stream_callback,
			  &buffers,
			  32,
			  BLADERF_FORMAT_SC16_Q11,
			  samples_per_buffer,
			  32,
			  NULL);
    
  log_line(LOG_DEBUG, "Reading samples!");
  output_ready = 0;
  r = pthread_create(&rx_task, NULL, rx_task_run, NULL);
  if (r < 0) {
    log_line(LOG_DEBUG,"pthread_create() failed");
    bladerf_close(dev);
    bladerf_deinit_stream(rx_stream);
    exit(EXIT_FAILURE);
  }

  pthread_join(rx_task, NULL);
  bladerf_deinit_stream(rx_stream);

  if (do_exit > 0) {
    log_line(LOG_DEBUG, "\nUser cancel, exiting...");
  }  else {
    log_line(LOG_DEBUG, "\nLibrary error %d, exiting...", r);
  }

  bladerf_close(dev);
  fclose(output);
  return 0;
}
