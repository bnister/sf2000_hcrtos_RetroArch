/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2017 - Daniel De Matteis
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#include "../audio_driver.h"
#include "../../verbosity.h"

#include "logx.h"

#define DEFAULT_DEV "/dev/audio"

#define AUDIO_BUFFER_SIZE	128 * 1024
#define AUDIO_CHANNELS		2
#define AUDIO_BITS			16

typedef struct sf2000_audio
{
	bool nonblock;
	bool running;
} sf2000_audio_t;


static void *sf2000_audio_init(const char *device, unsigned rate, unsigned latency,
      unsigned block_frames,
      unsigned *new_out_rate)
{
	LOGX("device=%s rate=%u latency=%u block_frames=%u\n", device ? device : "Null", rate, latency, block_frames);

	sf2000_audio_t *ctx = (sf2000_audio_t*)calloc(1, sizeof(sf2000_audio_t));

	if (!ctx)
		return NULL;

	return ctx;
}

static ssize_t sf2000_audio_write(void *data, const void *buf, size_t size)
{
	return size;
}

static bool sf2000_audio_stop(void *data)
{
	sf2000_audio_t* ctx = (sf2000_audio_t*)data;
	if (!ctx)
		return false;

	ctx->running = false;
	return true;
}

static bool sf2000_audio_start(void *data, bool is_shutdown)
{
	sf2000_audio_t* ctx = (sf2000_audio_t*)data;
	if (!ctx)
		return false;

	ctx->running = true;
	return true;
}

static bool sf2000_audio_alive(void *data)
{
	sf2000_audio_t* ctx = (sf2000_audio_t*)data;
	if (!ctx)
		return false;

	return ctx->running;
}

static void sf2000_audio_set_nonblock_state(void *data, bool state)
{
	sf2000_audio_t* ctx = (sf2000_audio_t*)data;
	if (!ctx)
		return;

	ctx->nonblock = state;
}

static void sf2000_audio_free(void *data)
{
	sf2000_audio_t* ctx = (sf2000_audio_t*)data;
	if (!ctx)
		return;

	free(ctx);
}

static size_t sf2000_audio_buffer_size(void *data)
{
	return AUDIO_BUFFER_SIZE;
}

static size_t sf2000_audio_write_avail(void *data)
{
	return sf2000_audio_buffer_size(data);
}

static bool sf2000_audio_use_float(void *data)
{
	(void)data;
	return false;
}

audio_driver_t audio_sf2000 = {
   sf2000_audio_init,
   sf2000_audio_write,
   sf2000_audio_stop,
   sf2000_audio_start,
   sf2000_audio_alive,
   sf2000_audio_set_nonblock_state,
   sf2000_audio_free,
   sf2000_audio_use_float,
   "sf2000",
   NULL,
   NULL,
   sf2000_audio_write_avail,
   sf2000_audio_buffer_size,
};
