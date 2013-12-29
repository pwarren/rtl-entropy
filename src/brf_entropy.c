/*
 * rtl-entropy, turns your Realtek RTL2832 based DVB dongle into a
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

#include <libbladeRF.h>
#include "fips.h"
#include "util.h"
#include "log.h"
#include "defines.h"

/*  Globals. */
static int do_exit = 0;
static fips_ctx_t fipsctx;

uint32_t samp_rate = DEFAULT_SAMPLE_RATE;
int actual_samp_rate = 0;
uint32_t frequency = DEFAULT_FREQUENCY;
int opt = 0;
int redirect_output = 0;
int gflags_encryption = 0;
int device_count = 0;
float gain = 1000.0;

/* daemon */
int uid = -1, gid = -1;

/* File handling stuff */
FILE *output = NULL;

/* Buffers */
unsigned char bitbuffer[BUFFER_SIZE] = {0};
unsigned char bitbuffer_old[BUFFER_SIZE] = {0};

/* Counters */
unsigned int bitcounter = 0;
unsigned int buffercounter = 0;

/* Other bits */
AES_KEY wctx;
EVP_CIPHER_CTX en;


void usage(void) {
  fprintf(stderr,
	  "rtl_entropy, a high quality entropy source using RTL2832 based DVB-T receivers\n\n"
	  "Usage: rtl_entropy [options]\n"
	  "\t-a Set gain (default: max for dongle)\n"
	  "\t-d Device index (default: 0)\n"
	  "\t-e Encrypt output\n"
	  "\t-f Set frequency to listen (default: 70MHz )\n"
	  "\t-s Samplerate (default: 3200000 Hz)\n");
  fprintf(stderr,
	  "\t-o Output file (default: STDOUT, /var/run/rtl_entropy.fifo for daemon mode (-b))\n"
#ifndef __APPLE__
	  "\t-p PID file (default: /var/run/rtl_entropy.pid)\n"
	  "\t-b Daemonize\n"
	  "\t-u User to run as (default: rtl_entropy)\n"
	  "\t-g Group to run as (default: rtl_entropy)\n"
#endif
	  );
  exit(EXIT_SUCCESS);
}


void parse_args(int argc, char ** argv) {
  char *arg_string= "a:d:ef:g:o:p:s:u:hb";
    
  opt = getopt(argc, argv, arg_string);
  while (opt != -1) {
    switch (opt) {
    case 'a':
      gain = (int)(atof(optarg) * 10);
      break;

    case 'b':
      gflags_detach = 1;
      break;
      
    case 'd':
      /* dev_index = atoi(optarg); */
      break;

    case 'e':
      gflags_encryption = 1;
      break;
      
    case 'f':
      frequency = (uint32_t)atofs(optarg);
      break;
      
    case 'g':
      gid = parse_group(optarg);
      break;
      
    case 'h':
      usage();
      break;
      
    case 'o':
      redirect_output = 1;
      output = fopen(optarg,"w");
      if (output == NULL) {
	suicide("Couldn't open output file");
      }
      fclose(stdout);
      break;
      
    case 'p':
      pidfile_path = strdup(optarg);
      break;
      
    case 's':
      samp_rate = (uint32_t)atofs(optarg);
      break;
      
    case 'u':
      uid = parse_user(optarg, &gid);
      break;
      
    default:
      usage();
      break;
    }
    opt = getopt(argc, argv, arg_string);
  }
}

static void sighandler(int signum)
{
  do_exit = signum;
}

#ifndef __APPLE__
static void drop_privs(int uid, int gid)
{
  cap_t caps;
  prctl(PR_SET_KEEPCAPS, 1);
  caps = cap_from_text("cap_sys_admin=ep");
  if (!caps)
    suicide("cap_from_text failed");
  if (setgroups(0, NULL) == -1)
    suicide("setgroups failed");
  if (setegid(gid) == -1 || seteuid(uid) == -1)
    suicide("dropping privs failed");
  if (cap_set_proc(caps) == -1)
    suicide("cap_set_proc failed");
  cap_free(caps);
}
#endif

void route_output(void) {
  /* Redirect output if directed */   
  if (!redirect_output) {
    if (mkfifo(DEFAULT_OUT_FILE,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) {
      if (errno != EEXIST) {
	perror("Bad FIFO");
      }
    }
    log_line(LOG_INFO, "Waiting for a Reader...");
    output = fopen(DEFAULT_OUT_FILE,"w");
    if (output == NULL) {
      suicide("Couldn't open output file");
    }
    redirect_output = 1;
    fclose(stdout);
  }
}

int nearest_gain(int target_gain){
  /* will have to work out what the bladeRF can do */
  return target_gain;
}

static void *rx_stream_callback(struct bladerf *dev,
                                struct bladerf_stream *stream,
                                struct bladerf_metadata *meta,
                                void *samples,
                                size_t num_samples,
                                void *user_data)
{

  /* Do stuff with the samples buffer */
  buffercounter += debias(samples, bitbuffer, hash_data_buffer, num_samples);
  
  
}


int main(int argc, char **argv) {
  struct sigaction sigact;
  struct bladerf *dev;
  struct bladerf_stream *stream;
  int n_read;
  int r;
  unsigned int i, j;
  uint8_t *buffer;
  uint8_t *ciphertext;
  uint32_t out_block_size = MAXIMAL_BUF_LENGTH;
  int ch, ch2;
  int fips_result;
  int aes_len;
  
  
  fprintf(stderr,"Not doing anything right now!\n");
  exit(EXIT_SUCCESS);
  
  parse_args(argc, argv);
  
  if (gflags_detach) {
#ifndef __APPLE__
    daemonize();
#endif
  }
  log_line(LOG_INFO,"Options parsed, continuing.");
  
  if (gflags_detach)
    route_output();
  
  if (!redirect_output)
    output = stdout;
#ifndef __APPLE__  
  if (uid != -1 && gid != -1)
    drop_privs(uid, gid);
#endif

  /* allocate buffers */
  buffer = malloc(out_block_size * sizeof(uint8_t));

  /* Setup Signal handlers */
  sigact.sa_handler = sighandler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;
  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
  sigaction(SIGQUIT, &sigact, NULL);
  sigaction(SIGPIPE, &sigact, NULL);

  /* Is FPGA ready? */
  r = bladerf_is_fpga_configured(dev);
  if (r < 0) {
    log_line(LOG_DEBUG, "Failed to determine if FPGA is loaded: %s\n",
	     bladerf_strerror(r));
    exit(EXIT_FAILURE);
  } else if (r == 0) {
    log_line(LOG_DEBUG, "FPGA is not loaded. Aborting.\n");
    exit(EXIT_FAILURE);
  }

  /* Open device */
  bladerf_open(&dev, NULL);
  
  /* Set the sample rate */
  r = bladerf_set_sample_rate(dev, BLADERF_MODULE_RX, samp_rate, &actual_samp_rate);
  log_line(LOG_DEBUG, "Sample rate set to %d", actual_samp_rate);

  log_line(LOG_DEBUG, "Setting Frequency to %d", frequency);
  r = bladerf_select_band(dev, BLADERF_MODULE_RX, frequency);
  if (r < 0) {
    log_line(LOG_DEBUG,"Failed to select band: %s", bladerf_strerror(r));
    exit(EXIT_FAILURE);
  }

  r = bladerf_set_frequency(dev, BLADERF_MODULE_RX, frequency);
  if (r < 0) {
    log_line(LOG_DEBUG,"Failed to set frequency: %s", bladerf_strerror(r));
    exit(EXIT_FAILURE);
  }  

  /* Set gain */
  gain = nearest_gain(gain);
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

  r = bladerf_enable_module(dev, BLADERF_MODULE_RX, true);
  if (r < 0) {
    log_line(LOG_DEBUG, "Failed to enable RX module: %s",
	     bladerf_strerror(r));
    exit(EXIT_FAILURE);
  } else {
    log_line(LOG_DEBUG,"Enabled RX module");
  }

  /* Initialise the receive stream */
  /*  r = bladerf_init_stream(stream,
			       dev,
			       rx_stream_callback,
			       samples,
                               num_buffers,
			       BLADERF_FORMAT_SC16_Q12,
			       samples_per_buffer,
			       num_transfers,
			       repeater);
  */

  if (r < 0) {
    log_line(LOG_DEBUG, "Failed to initialize RX stream: %s\n",
	    bladerf_strerror(r));
    bladerf_close(dev);
    exit(EXIT_FAILURE);
  }

  log_line(LOG_DEBUG, "Doing FIPS init");
  fips_init(&fipsctx, (int)0);
      
  log_line(LOG_DEBUG, "Reading samples in sync mode...");
  while ( (!do_exit) || (do_exit == SIGPIPE)) {
    if (do_exit == SIGPIPE) {
      log_line(LOG_DEBUG, "Reader went away, closing FIFO");
      fclose(output);
      if (gflags_detach) {
	log_line(LOG_DEBUG, "Waiting for a Reader...");
	output = fopen(DEFAULT_OUT_FILE,"w");
      }
      else {
	break;
      }
    }

    /* do stuff forever here! */ 

  }
  if (do_exit) {
    log_line(LOG_DEBUG, "\nUser cancel, exiting...");
  }  else {
    log_line(LOG_DEBUG, "\nLibrary error %d, exiting...", r);
  }
  
  bladerf_deinit_stream(stream);
  bladerf_close(dev);
  free(buffer);
  fclose(output);
  return 0;
}
