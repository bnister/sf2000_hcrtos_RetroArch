/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2019 - Daniel De Matteis
 *  Copyright (C) 2012-2015 - Michael Lelli
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

#include <stdlib.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <signal.h>

#include <boolean.h>

#include "../../verbosity.h"

#include "../input_keymaps.h"
#include "../input_driver.h"

typedef struct sf2000_input
{
   bool state[0x80];
} sf2000_input_t;

static void *sf2000_input_init(const char *joypad_driver)
{
   return NULL;
}

static int16_t sf2000_input_state(
      void *data,
      const input_device_driver_t *joypad,
      const input_device_driver_t *sec_joypad,
      rarch_joypad_info_t *joypad_info,
      const retro_keybind_set *binds,
      bool keyboard_mapping_blocked,
      unsigned port,
      unsigned device,
      unsigned idx,
      unsigned id)
{
   return 0;
}

static void sf2000_input_free(void *data)
{
}

static void sf2000_input_poll(void *data)
{
}

static uint64_t sf2000_get_capabilities(void *data)
{
   return 0;
}

input_driver_t input_sf2000 = {
   sf2000_input_init,
   sf2000_input_poll,
   sf2000_input_state,
   sf2000_input_free,
   NULL,
   NULL,
   sf2000_get_capabilities,
   "sf2000",
   NULL,                         /* grab_mouse */
   NULL, /* grab stdin */
   NULL
};
