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

#include "../input_keymaps.h"
#include "../input_driver.h"

#include "../../verbosity.h"

#include "logx.h"
#define LOG_TAG "[SF2000][Input]"

#include "hcuapi/gpio.h"
#include "hcuapi/pinpad.h"

#include <kernel/delay.h>

#define KEY_SHIFTER_CLK_PIN PINPAD_L24 // the clock pin is shared
//#define KEY_SHIFTER_PL1_PIN PINPAD_L25 // X60
#define KEY_SHIFTER_PL1_PIN PINPAD_L23 // SF2000
#define KEY_SHIFTER_PL2_PIN PINPAD_L26 // currently unimplemented


typedef struct sf2000_input
{
   int state[12];
} sf2000_input_t;

static void *sf2000_input_init(const char *joypad_driver)
{
	RARCH_LOG(LOG_TAG " Init - joypad_driver=%s\n", joypad_driver ? joypad_driver : "Null");

	sf2000_input_t *ctx = (sf2000_input_t*)calloc(1, sizeof(sf2000_input_t));
	if (!ctx)
		return NULL;

	for (int i = 0; i < 12; i++)
		ctx->state[i] = 1;

	return ctx;
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
	sf2000_input_t *ctx = (sf2000_input_t*)data;
	if (!ctx)
		return 0;

	//LOGX("port=%u, device=%u, idx=%u, id=%u\n", port, device, idx, id);

	// TODO: clean this up

typedef enum {
	SF2000_BTN_L = 0,
	SF2000_BTN_Y,
	SF2000_BTN_X,
	SF2000_BTN_R,
	SF2000_BTN_A,
	SF2000_BTN_B,
	SF2000_BTN_SELECT,
	SF2000_BTN_START,
	SF2000_BTN_UP,
	SF2000_BTN_DOWN,
	SF2000_BTN_LEFT,
	SF2000_BTN_RIGHT,
} sf2000_btn_e;

#define CHECK_BTN(RETRO_BTN, SF2000_BTN) \
		case RETRO_BTN:						\
			if (0) printf(#RETRO_BTN "=%d, " #SF2000_BTN "=%d, %s\n", RETRO_BTN, SF2000_BTN, ctx->state[SF2000_BTN] ? "released" : "pressed"); \
			return 1 ^ ctx->state[SF2000_BTN];

	if (device == RETRO_DEVICE_JOYPAD) {
		// TODO: the 'id' we get does not match any of the standard RETRO_DEVICE_JOYPAD buttons. why?
		// so for now we simulate some basic RETRO_DEVICE_KEYBOARD keys below
		switch (id) {
			CHECK_BTN(RETRO_DEVICE_ID_JOYPAD_A, SF2000_BTN_A);
			CHECK_BTN(RETRO_DEVICE_ID_JOYPAD_B, SF2000_BTN_B);
			CHECK_BTN(RETRO_DEVICE_ID_JOYPAD_X, SF2000_BTN_X);
			CHECK_BTN(RETRO_DEVICE_ID_JOYPAD_Y, SF2000_BTN_Y);
			CHECK_BTN(RETRO_DEVICE_ID_JOYPAD_UP, SF2000_BTN_UP);
			CHECK_BTN(RETRO_DEVICE_ID_JOYPAD_DOWN, SF2000_BTN_DOWN);
			CHECK_BTN(RETRO_DEVICE_ID_JOYPAD_LEFT, SF2000_BTN_LEFT);
			CHECK_BTN(RETRO_DEVICE_ID_JOYPAD_RIGHT, SF2000_BTN_RIGHT);
			CHECK_BTN(RETRO_DEVICE_ID_JOYPAD_L, SF2000_BTN_L);
			CHECK_BTN(RETRO_DEVICE_ID_JOYPAD_R, SF2000_BTN_R);
			CHECK_BTN(RETRO_DEVICE_ID_JOYPAD_START, SF2000_BTN_START);
			CHECK_BTN(RETRO_DEVICE_ID_JOYPAD_SELECT, SF2000_BTN_SELECT);
		}
	} else if (device == RETRO_DEVICE_KEYBOARD) {
		switch (id) {
			CHECK_BTN(RETROK_BACKSPACE, SF2000_BTN_A);
			CHECK_BTN(RETROK_RETURN, SF2000_BTN_B);
			CHECK_BTN(RETROK_UP, SF2000_BTN_UP);
			CHECK_BTN(RETROK_DOWN, SF2000_BTN_DOWN);
			CHECK_BTN(RETROK_LEFT, SF2000_BTN_LEFT);
			CHECK_BTN(RETROK_RIGHT, SF2000_BTN_RIGHT);
		}
	}

	return 0;
}


static void sf2000_input_free(void *data)
{
	sf2000_input_t *ctx = (sf2000_input_t*)data;
	if (!ctx)
		free(ctx);
}

static void sf2000_input_poll(void *data)
{
	sf2000_input_t *ctx = (sf2000_input_t*)data;
	if (!ctx)
		return;

	static const char * const key_names[] = {
		"L", "Y", "X", "R", "A", "B", "Select", "Start", "Up", "Down", "Left", "Right"
	};

	int prev[12];
	for (int i = 0; i < 12; i++)
		prev[i] = ctx->state[i];

	//LOGX("poll\n");

	gpio_configure(KEY_SHIFTER_CLK_PIN, GPIO_DIR_OUTPUT);
	gpio_set_output(KEY_SHIFTER_CLK_PIN, 1); // shifts on 1->0 transition

	// probably latches the state while the pin is actively driven
	gpio_configure(KEY_SHIFTER_PL1_PIN, GPIO_DIR_OUTPUT);
	gpio_set_output(KEY_SHIFTER_PL1_PIN, 0);
	usleep(4);
	gpio_configure(KEY_SHIFTER_PL1_PIN, GPIO_DIR_INPUT);
	for (int i = 0; i < 12; i++) {
		ctx->state[i] = gpio_get_input(KEY_SHIFTER_PL1_PIN);
		gpio_set_output(KEY_SHIFTER_CLK_PIN, 0);
		usleep(2);
		gpio_set_output(KEY_SHIFTER_CLK_PIN, 1);
	}

	for (int i = 0; i < 12; i++) if (prev[i] != ctx->state[i]) {
		RARCH_LOG(LOG_TAG "== %s(%d) %s\n", key_names[i], i, ctx->state[i] ? "released" : "pressed");
	}
}

static uint64_t sf2000_get_capabilities(void *data)
{
	// TODO: remove RETRO_DEVICE_KEYBOARD when sf2000_input_state is fixed
	return (1 << RETRO_DEVICE_JOYPAD) | (1 << RETRO_DEVICE_KEYBOARD);
}

// TODO: do we even need the poll/state functionality here?
// maybe move it to the joypad driver instead like some other input drivers
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
