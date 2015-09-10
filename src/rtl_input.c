/*
 * rtl-entropy, turns your Realtek RTL2832 based DVB dongle into a
 * high quality entropy source.
 *
 * Copyright (C) 2013 by Paul Warren <pwarren@pwarren.id.au>
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


/*
This file contains the input reader for rtl-sdr devices
*/

#include "rtl-sdr.h"
#include "util.h"
#include "log.h"
#include "defines.h"

int nearest_gain(int target_gain){
  int i, err1, err2, count, close_gain;
  int* gains;
  count = rtlsdr_get_tuner_gains(dev, NULL);
  if (count <= 0) {
    return 0;
k
  }
  gains = malloc(sizeof(int) * count);
  count = rtlsdr_get_tuner_gains(dev, gains);
  close_gain = gains[0];
  log_line(LOG_DEBUG,"Your device is capable of gains at...");
  for (i=0; i<count; i++) {
    log_line(LOG_DEBUG," : %0.2f", gains[i]/10.0);
    err1 = abs(target_gain - close_gain);
    err2 = abs(target_gain - gains[i]);
    if (err2 < err1) {
      close_gain = gains[i];
    }
  }
  free(gains);
  return close_gain;
}

void setup_rtl_device(int *write_to) {
  device_count = rtlsdr_get_device_count();
  if (!device_count) {
    suicide("No supported devices found, shutting down");
  }
  // Check for unused rtl devices, maybe warn, not error
  
  log_line(LOG_DEBUG, "Found %d device(s):", device_count);
  for (i = 0; i <(unsigned int)device_count; i++)
    log_line(LOG_DEBUG, "  %d:  %s", i, rtlsdr_get_device_name(i));
  log_line(LOG_DEBUG, "Using device %d: %s", dev_index,
	   rtlsdr_get_device_name(dev_index));
  
  r = rtlsdr_open(&dev, dev_index);
  if (r < 0) {
    log_line(LOG_DEBUG, "Failed to open rtlsdr device #%d.", dev_index);
    exit(EXIT_FAILURE);
  }
  
  /* Set the sample rate */
  r = rtlsdr_set_sample_rate(dev, rtl_samp_rate);
  if (r < 0)
    log_line(LOG_DEBUG, "WARNING: Failed to set sample rate.");
  
  /* Reset endpoint before we start reading from it (mandatory) */
  r = rtlsdr_reset_buffer(dev);
  if (r < 0)
    log_line(LOG_DEBUG, "WARNING: Failed to reset buffers.");
  
  log_line(LOG_DEBUG, "Setting Frequency to %d", rtl_frequency);
  r = rtlsdr_set_center_freq(dev, (uint32_t)rtl_frequency);
  
  rtl_gain = nearest_gain(rtl_gain); // maybe do this in argument parsing?
  log_line(LOG_DEBUG, "Setting gain to %0.2f", gain/10.0);
  /* Manual gain mode */
  r = rtlsdr_set_tuner_gain_mode(dev, 1);
  if (r < 0)
    log_line(LOG_DEBUG, "WARNING: Failed to set manual gain");
  r = rtlsdr_set_tuner_gain(dev, gain);
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
  }


void (void) shutdown {
  rtlsdr_close(dev);
  free(buffer);
  fclose(output);
  return 0;
}

