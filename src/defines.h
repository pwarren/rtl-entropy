/*
 * Copyright (C) 2013 Paul Warren <pwarren@pwarren.id.au>
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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */



#define DEFAULT_PID_FILE "/var/run/rtl_entropy.pid"
#define DEFAULT_OUT_FILE "/var/run/rtl_entropy.fifo"
#define DEFAULT_CONFIGURATION_FILE_1      "/etc/rtl_entropy.conf"
#define DEFAULT_CONFIGURATION_FILE_2      "/etc/sysconfig/rtl_entropy.conf"

#define MHZ(x)	((x)*1000*1000)
#define DEFAULT_SAMPLE_RATE		3200000
#define DEFAULT_ASYNC_BUF_NUMBER	32
#define DEFAULT_BUF_LENGTH		(16 * 16384)
#define MINIMAL_BUF_LENGTH		512
#define MAXIMAL_BUF_LENGTH		(256 * 16384)
#define BUFFER_SIZE                     2500 /* need 2500 bits for FIPS */
#define DEFAULT_FREQUENCY MHZ(70)
#define HASH_BUFFER_SIZE  64 /* Bytes */

#define GFLAGS_DETACH 0
#define GFLAGS_DEBUG 1
