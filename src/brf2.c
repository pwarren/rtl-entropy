#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <libbladeRF.h>

#include "log.h"
#include "util.h"

struct bladerf *device;
pthread_t rx_task;
struct bladerf_stream *rx_stream;


void *buffer;

int samples_per_buffer = 65536;
int do_exit = 0;

static void sighandler(int signum) {
  do_exit = signum;
}

static void *rx_stream_callback(struct bladerf *dev,
                         struct bladerf_stream *stream,
                         struct bladerf_metadata *meta,
                         void *samples,
                         size_t num_samples,
                         void *user_data)
{
  int16_t *bitbuffer;
  int i ,j;
  int bitcounter = 0;
  int buffercounter = 0;
  int16_t *sample = (int16_t *)samples;
  int ch, ch2;
  int bufsize = 5500;
  bitbuffer = malloc(bufsize * sizeof(int16_t));
  memset(bitbuffer,0,bufsize);
  
  buffercounter = 0;
  bitcounter = 0;
  
  for(i=0; i<num_samples*2; i++) {
    for (j=0; j < 10; j+= 2) {
      ch = (*sample >> j) & 0x01;
      ch2 = (*sample >> (j+1)) & 0x01;
      if (ch != ch2) {
	if (ch) {
	  /* store a 1 in our bitbuffer */
	  bitbuffer[buffercounter] |= 1 << bitcounter;
	} /* else, leave the buffer alone, it's already 0 at this bit */
	bitcounter++;
      }
      
      /* is int16_t full? */
      if (bitcounter >= sizeof(bitbuffer[0]) * 8) {
	buffercounter++;
	bitcounter = 0;
      }

      /* Not threaadsafe!
       * if you set bufsize too high, the next callback might 
       * happen before you get here to write anything to output! 
       * time for locking, and discarding packets!
      */
      
      if (buffercounter >= bufsize) {
	fwrite(bitbuffer, 
	       sizeof(int16_t), 
	       buffercounter,
	       stdout);
	buffercounter = 0;
	bitcounter = 0;
	memset(bitbuffer,0,bufsize);
      }
    }
    sample ++;
  }
  
  free(bitbuffer);

  if (!do_exit) {
    return samples;
  } else {
    return NULL;
  }
}

void * rx_task_run(void *inputs) {
  int r;
  r = bladerf_stream(rx_stream, BLADERF_MODULE_RX);
  if (r < 0) {
    log_line(LOG_DEBUG,"RX Stream failure: %s\n",bladerf_strerror(r));
  }
  return NULL;
}

int main(int argc, char * argv) {
  int r;
  unsigned int actual_value;
  
  uint32_t samp_rate = 40000000;
  uint32_t frequency = 434000000;
  float gain = 1000.0;
  struct sigaction sigact;

  buffer = malloc(2 * samples_per_buffer * sizeof(int16_t));

  /* Setup Signal handlers */
  sigact.sa_handler = sighandler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;
  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
  sigaction(SIGQUIT, &sigact, NULL);
  sigaction(SIGPIPE, &sigact, NULL);
  
  /* Open device */
  r = bladerf_open(&device, NULL);
  if (r < 0) {
    log_line(LOG_DEBUG, "Failed to open device: %s", bladerf_strerror(r));
    exit(EXIT_SUCCESS);
  }

  /* Is FPGA ready? */
  r = bladerf_is_fpga_configured(device);
  if (r < 0) {
    log_line(LOG_DEBUG, "Failed to determine if FPGA is loaded: %s",
	     bladerf_strerror(r));
    bladerf_close(device);
    exit(EXIT_FAILURE);
  } else if (r == 0) {
    log_line(LOG_DEBUG, "FPGA is not loaded. Aborting.");
    exit(EXIT_FAILURE);
  }
  log_line(LOG_DEBUG, "FPGA Loaded");
  
  /* set up verbose logging of libbladerf calls */
  /* bladerf_log_set_verbosity(BLADERF_LOG_LEVEL_VERBOSE); */
  
  /* Set the sample rate */
  r = bladerf_set_sample_rate(device, BLADERF_MODULE_RX, samp_rate, &actual_value);
  if (r < 0) {
    log_line(LOG_DEBUG,"Failed to set sample rate: %s", bladerf_strerror(r));
    bladerf_close(device);
    exit(EXIT_FAILURE);
  }
  log_line(LOG_DEBUG, "Sample rate set to %d", actual_value);
  
  /* Set the filter bandwidth to the same as the sample rate for now */
  r = bladerf_set_bandwidth(device, BLADERF_MODULE_RX, samp_rate, &actual_value);
  if (r < 0) {
    log_line(LOG_DEBUG,"Failed to set sample rate: %s", bladerf_strerror(r));
    exit(EXIT_FAILURE);
  }
  log_line(LOG_DEBUG, "Bandwidth rate set to %d", actual_value);
  
  log_line(LOG_DEBUG, "Setting Frequency to %d", frequency);
  
  r = bladerf_set_frequency(device, BLADERF_MODULE_RX, frequency);
  if (r < 0) {
    log_line(LOG_DEBUG,"Failed to set frequency: %s", bladerf_strerror(r));
    bladerf_close(device);
    exit(EXIT_FAILURE);
  }  
  
  /* Set gain */
  r = bladerf_set_rxvga1(device, gain);
  if (r < 0) {
    log_line(LOG_DEBUG,"Failed to set pre gain: %s",bladerf_strerror(r));
    bladerf_close(device);
    exit(EXIT_FAILURE);
  }  

  r = bladerf_set_rxvga2(device, gain);
  if (r < 0) {
    log_line(LOG_DEBUG,"Failed to set post gain: %s", bladerf_strerror(r));
    bladerf_close(device);
    exit(EXIT_FAILURE);
  }  
  
  r = bladerf_enable_module(device, BLADERF_MODULE_RX, true);
  if (r < 0) {
    log_line(LOG_DEBUG, "Failed to enable RX module: %s",
	     bladerf_strerror(r));
    bladerf_close(device);
    exit(EXIT_FAILURE);
  } else {
    log_line(LOG_DEBUG,"Enabled RX module");
  }

  /*  Initialise the receive stream */
  r = bladerf_init_stream(&rx_stream,
			  device,
			  rx_stream_callback,
			  buffer,
			  1,
			  BLADERF_FORMAT_SC16_Q12,
			  samples_per_buffer,
			  1,
			  NULL);
  
  if (r < 0) {
    log_line(LOG_DEBUG, "Failed to initialize RX stream: %s\n",
	     bladerf_strerror(r));
    bladerf_close(device);
    exit(EXIT_FAILURE);
  }
  
  r = pthread_create(&rx_task, NULL, rx_task_run, NULL);
  if (r < 0) {
    log_line(LOG_DEBUG,"pthread_create failed");
  }
  pthread_join(rx_task, NULL);
  bladerf_deinit_stream(rx_stream);     
  
  log_line(LOG_DEBUG,"do_exit: %d",do_exit);
  bladerf_close(device);
  return 0;
}
