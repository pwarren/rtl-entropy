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
#include <sys/capability.h>
#include <sys/types.h>
#include <sys/prctl.h>
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
#endif

#include "rtl-sdr.h"
#include "fips.h"
#include "util.h"
#include "log.h"
#include "defines.h"

/*  Globals. */
static int do_exit = 0;
/*@NULL@*/
static rtlsdr_dev_t *dev = NULL;
static fips_ctx_t fipsctx;		/* Context for the FIPS tests */
uint32_t dev_index = 0;
uint32_t samp_rate = DEFAULT_SAMPLE_RATE;
uint32_t frequency = DEFAULT_FREQUENCY;
int opt;
int redirect_output = 0;
int gflags_encryption = 0;
int device_count;
/* daemon */
int uid = -1, gid = -1;

/* File handling stuff */
FILE *output = NULL;

/* Buffers */
unsigned char bitbuffer[BUFFER_SIZE] = {0};
unsigned char bitbuffer_old[BUFFER_SIZE] = {0};
unsigned char hash_buffer[SHA512_DIGEST_LENGTH] = {0};
unsigned char hash_data_buffer[SHA512_DIGEST_LENGTH] = {0};

/* Counters */
unsigned int bitcounter = 0;
unsigned int buffercounter = 0;
unsigned int hash_data_counter = 0;
unsigned int hash_data_bit_counter = 0;
unsigned int hash_loop = 0;

/* Other bits */
AES_KEY wctx;
EVP_CIPHER_CTX en;


void usage(void) {
  fprintf(stderr,
	  "rtl_entropy, a high quality entropy source using RTL2832 based DVB-T receivers\n\n"
	  "Usage: rtl_entropy [options]\n"
	  "\t[-b Daemonize]\n"
	  "\t[-d Device index (default: 0)]\n"
	  "\t[-e Encrypt output\n"
	  "\t[-f Set frequency to listen (default: 54MHz )]\n"
	  "\t[-s Samplerate (default: 2048000 Hz)]\n"
	  "\t[-o Output file] (default: STDOUT, /var/run/rtl_entropy.fifo for daemon mode (-b))\n"
	  "\t[-p PID file] (default: /var/run/rtl_entropy.pid)\n"
	  "\t[-u User to run as] (default: rtl_entropy)\n"
	  "\t[-g Group to run as] (default: rtl_entropy)\n"
	  );
  exit(EXIT_SUCCESS);
}


void parse_args(int argc, char ** argv) {
  char *arg_string= "d:ef:g:o:p:s:u:hb";
    
  opt = getopt(argc, argv, arg_string);
  while (opt != -1) {
    switch (opt) {
    case 'b':
      gflags_detach = 1;
      break;
      
    case 'd':
      dev_index = atoi(optarg);
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


void store_hash_data(int bit) {
  /* store data in a sort of ring buffer */
  if (bit) {
    hash_data_buffer[hash_data_counter] |= 1 << hash_data_bit_counter;
  } else {
    hash_data_buffer[hash_data_counter] &= ~(1 << hash_data_bit_counter);
  }
  hash_data_bit_counter++;
  if (hash_data_bit_counter == sizeof(hash_data_buffer[0]) * 8) {
    hash_data_bit_counter = 0;
    hash_data_counter++;
  }
  
  if (hash_data_counter == SHA512_DIGEST_LENGTH) {
    hash_data_counter = 0;      
    hash_loop=1;
  }
}

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

int main(int argc, char **argv) {
  struct sigaction sigact;
  int n_read;
  int r;
  unsigned int i, j;
  uint8_t *buffer;
  uint8_t *ciphertext;
  uint32_t out_block_size = MAXIMAL_BUF_LENGTH;
  int ch, ch2;
  int gains[100];
  int count, fips_result;
  int aes_len;
  
  parse_args(argc, argv);
  
  if (gflags_detach) {
    daemonize();
  }
  log_line(LOG_INFO,"Options parsed, continuing.");
  
  if (gflags_detach)
    route_output();
  
  if (!redirect_output)
    output = stdout;
  
  if (uid != -1 && gid != -1)
    drop_privs(uid, gid);
  
  /* get to the important stuff! */
  buffer = malloc(out_block_size * sizeof(uint8_t));
  device_count = rtlsdr_get_device_count();
  if (!device_count) {
    suicide("No supported devices found, shutting down");
  }
  
  log_line(LOG_DEBUG, "Found %d device(s):", device_count);
  for (i = 0; i <(unsigned int)device_count; i++)
    log_line(LOG_DEBUG, "  %d:  %s", i, rtlsdr_get_device_name(i));
    
  log_line(LOG_DEBUG, "Using device %d: %s",
	  dev_index,
	  rtlsdr_get_device_name(dev_index));
  
  r = rtlsdr_open(&dev, dev_index);
  if (r < 0) {
    log_line(LOG_DEBUG, "Failed to open rtlsdr device #%d.", dev_index);
    exit(EXIT_FAILURE);
  }

  /* Setup Signal handlers */
  sigact.sa_handler = sighandler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;
  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
  sigaction(SIGQUIT, &sigact, NULL);
  sigaction(SIGPIPE, &sigact, NULL);
  sigaction(SIGKILL, &sigact, NULL);
  
  /* Set the sample rate */
  r = rtlsdr_set_sample_rate(dev, samp_rate);
  if (r < 0)
    log_line(LOG_DEBUG, "WARNING: Failed to set sample rate.");
  
  /* Reset endpoint before we start reading from it (mandatory) */
  r = rtlsdr_reset_buffer(dev);
  if (r < 0)
    log_line(LOG_DEBUG, "WARNING: Failed to reset buffers.");
  
  log_line(LOG_DEBUG, "Setting Frequency to %d", frequency);
  r = rtlsdr_set_center_freq(dev, (uint32_t)frequency);
  
  count = rtlsdr_get_tuner_gains(dev, gains);
  log_line(LOG_DEBUG, "Setting gain to %.1f", (float)(gains[count-1]/10));
  r = rtlsdr_set_tuner_gain_mode(dev, 1);
  if (r < 0)
    log_line(LOG_DEBUG, "WARNING: Couldn't set gain mode to manual");

  /* min gain for higher throughput when in a faraday cage */
  r = rtlsdr_set_tuner_gain(dev, gains[0]);
  if (r < 0)
    log_line(LOG_DEBUG, "WARNING: Failed to set gain");
  
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
    r = rtlsdr_read_sync(dev, buffer, out_block_size, &n_read);
    if (r < 0) {
      log_line(LOG_DEBUG, "ERROR: sync read failed.");
      break;
    }
    if ((uint32_t)n_read < out_block_size) {
      log_line(LOG_DEBUG, "ERROR: Short read, samples lost, exiting!");
      break;
    }
    
    /* for each byte in the rtl-sdr read buffer
       pick least significant 6 bits
       for now:
       debias and store in the write buffer till it's full
       xor with old data and write to output

       soon will be: 
       debias, storing useful bits in write buffer, and discarded bits in hash buffer
       until the write buffer is full.
       create a key by SHA512() hashing the hash buffer
       encrypt write buffer with key
       output encrypted buffer
       
    */
    for (i=0; i < n_read * sizeof(buffer[0]); i++) {
      for (j=0; j < 6; j+= 2) {
	ch = (buffer[i] >> j) & 0x01;
	ch2 = (buffer[i] >> (j+1)) & 0x01;
	if (ch != ch2) {
	  if (ch) {
	    /* store a 1 in our bitbuffer */
	    bitbuffer[buffercounter] |= 1 << bitcounter;
	  } /* else, leave the buffer alone, it's already 0 at this bit */
	  bitcounter++;
	} else {
	  if (ch) {
	    store_hash_data(1);
	  } else {
	    store_hash_data(0);
	  }
	}
	
	/* is byte full? */
	if (bitcounter >= sizeof(bitbuffer[0]) * 8) {
	  buffercounter++;
	  bitcounter = 0;
	}
	
	/* is buffer full? */
	if (buffercounter > BUFFER_SIZE) {
	  /* We have 2500 bytes of entropy 
	     Can now send it to FIPS! */
	  fips_result = fips_run_rng_test(&fipsctx, &bitbuffer);
	  if (!fips_result) {
	    if (gflags_encryption) {
	      if (hash_loop) {
		/*   /\* Get a key from disacarded bits *\/ */
		SHA512(hash_data_buffer, sizeof(hash_data_buffer), hash_buffer);
		/* use key to encrypt output */
		/* AES_set_encrypt_key(hash_buffer, 128, &wctx); */
		/* AES_encrypt(bitbuffer, bitbuffer_old, &wctx); */
		aes_init(hash_buffer, sizeof(hash_buffer), &en);
		aes_len = sizeof(bitbuffer);
		ciphertext = aes_encrypt(&en, bitbuffer, &aes_len);
		/* yay, send it to the output! */
		fwrite(ciphertext,sizeof(ciphertext[0]),aes_len,output);
		free(ciphertext);
	      }
	    } else {
	      /* xor with old data */
	      for (buffercounter = 0; buffercounter < BUFFER_SIZE; buffercounter++) {
		bitbuffer[buffercounter] = bitbuffer[buffercounter] ^ bitbuffer_old[buffercounter];
	      }
	      fwrite(&bitbuffer_old,sizeof(bitbuffer_old[0]),BUFFER_SIZE,output);
	      /* swap old data */
	      memcpy(bitbuffer_old,bitbuffer,BUFFER_SIZE);
	    }
	  } else {   /* FIPS test failed */
	    for (j=0; j< N_FIPS_TESTS; j++) {
	      if (fips_result & fips_test_mask[j]) {
		if (!gflags_detach)
		  log_line(LOG_DEBUG, "Failed: %s", fips_test_names[j]);
	      }
	    }
	  }
	  /* reset buffers, and the counter */
	  /* memset(bitbuffer_old,0,sizeof(bitbuffer_old)); */
	  memset(bitbuffer,0,sizeof(bitbuffer));
	  buffercounter = 0;
	}
      }
    }
  }
  if (do_exit) {
    log_line(LOG_DEBUG, "\nUser cancel, exiting...");
  }  else {
    log_line(LOG_DEBUG, "\nLibrary error %d, exiting...", r);
  }
  
  rtlsdr_close(dev);
  free(buffer);
  fclose(output);
  return 0;
}
