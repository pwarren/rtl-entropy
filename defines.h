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
 */



#define DEFAULT_PID_FILE "/var/run/rtl_entropy.pid"
#define DEFAULT_OUT_FILE "/var/run/rtl_entropy.fifo"

#define MHZ(x)	((x)*1000*1000)
#define DEFAULT_SAMPLE_RATE		2048000
#define DEFAULT_ASYNC_BUF_NUMBER	32
#define DEFAULT_BUF_LENGTH		(16 * 16384)
#define MINIMAL_BUF_LENGTH		512
#define MAXIMAL_BUF_LENGTH		(256 * 16384)
#define BUFFER_SIZE                     2500 // need 2500 bits for FIPS
#define DEFAULT_FREQUENCY MHZ(70)

#define GFLAGS_DETACH 0
#define GFLAGS_DEBUG 1
