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

#define DEFAULT_DEV "/dev/audio"

static void *sf2000_audio_init(const char *device, unsigned rate, unsigned latency,
      unsigned block_frames,
      unsigned *new_out_rate)
{
   return NULL;
}

static ssize_t sf2000_audio_write(void *data, const void *buf, size_t size)
{
   ssize_t ret;
   return ret;
}

static bool sf2000_audio_stop(void *data)
{
   return false;
}

static bool sf2000_audio_start(void *data, bool is_shutdown)
{
   return false;
}

static bool sf2000_audio_alive(void *data)
{
    return false;
}

static void sf2000_audio_set_nonblock_state(void *data, bool state)
{
}

static void sf2000_audio_free(void *data)
{
}

static size_t sf2000_audio_buffer_size(void *data)
{
   return 0;
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
