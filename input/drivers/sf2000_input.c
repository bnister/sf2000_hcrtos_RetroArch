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

#include "input/input_driver.h"
#include "verbosity.h"

#include "logx.h"
#define LOG_TAG "[SF2000][Input]"

// Empty input driver - all functionality handled in sf2000_joypad.c 

static void *sf2000_input_init(const char *joypad_driver)
{
	RARCH_LOG(LOG_TAG " Init - joypad_driver=%s\n", joypad_driver ? joypad_driver : "Null");
	return (void*)-1;
}

static void sf2000_input_free(void *data)
{}

static uint64_t sf2000_get_capabilities(void *data)
{
	return (1 << RETRO_DEVICE_JOYPAD);
}

// TODO: do we even need the poll/state functionality here?
// maybe move it to the joypad driver instead like some other input drivers
input_driver_t input_sf2000 = {
   sf2000_input_init,
   NULL,                        /* poll */
   NULL,                        /* input_state */
   sf2000_input_free,
   NULL,
   NULL,
   sf2000_get_capabilities,
   "sf2000",
   NULL,						/* grab_mouse */
   NULL, 						/* grab stdin */
   NULL
};
