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
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>


#include <fcntl.h>

#include <compat/strl.h>
#include <string/stdstring.h>

#include "../input_driver.h"

#include "../../verbosity.h"
#include "../../tasks/tasks_internal.h"

#define NUM_BUTTONS 32
#define NUM_AXES 32

struct sf2000_joypad
{
   int fd;
   uint32_t buttons;
   int16_t axes[NUM_AXES];

   char *ident;
};

/* TODO/FIXME - static globals */
static struct sf2000_joypad sf2000_pads[MAX_USERS];
static int sf2000_epoll                              = 0;
static int sf2000_inotify                            = 0;
static bool sf2000_hotplug                           = false;

static void sf2000_poll_pad(struct sf2000_joypad *pad)
{
}

static bool sf2000_joypad_init_pad(const char *path,
      struct sf2000_joypad *pad)
{
   return false;
}

static const char *sf2000_joypad_name(unsigned pad)
{
    return NULL;
}

static void sf2000_joypad_poll(void)
{
}

static void *sf2000_joypad_init(void *data)
{
   return (void*)-1;
}

static void sf2000_joypad_destroy(void)
{
}

static int32_t sf2000_joypad_button(unsigned port, uint16_t joykey)
{
   return 0;
}

static void sf2000_joypad_get_buttons(unsigned port, input_bits_t *state)
{
}

static int16_t sf2000_joypad_axis_state(
      const struct sf2000_joypad *pad,
      unsigned port, uint32_t joyaxis)
{
   return 0;
}

static int16_t sf2000_joypad_axis(unsigned port, uint32_t joyaxis)
{
   return 0;
}

static int16_t sf2000_joypad_state(
      rarch_joypad_info_t *joypad_info,
      const struct retro_keybind *binds,
      unsigned port)
{
   return 0;
}

static bool sf2000_joypad_query_pad(unsigned pad)
{
   return pad < MAX_USERS && sf2000_pads[pad].fd >= 0;
}

input_device_driver_t sf2000_joypad = {
   sf2000_joypad_init,
   sf2000_joypad_query_pad,
   sf2000_joypad_destroy,
   sf2000_joypad_button,
   sf2000_joypad_state,
   sf2000_joypad_get_buttons,
   sf2000_joypad_axis,
   sf2000_joypad_poll,
   NULL,
   NULL,
   sf2000_joypad_name,
   "sf2000",
};
