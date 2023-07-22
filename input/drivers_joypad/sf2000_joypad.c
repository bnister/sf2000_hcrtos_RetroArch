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

#include "input/input_driver.h"
#include "verbosity.h"

#include "logx.h"
#define LOG_TAG "[SF2000][Joypad]"
//#define LOG_BTN_PRESS

#include <hcuapi/gpio.h>
#include <hcuapi/pinpad.h>

#define KEY_SHIFTER_CLK_PIN PINPAD_L24 // the clock pin is shared
//#define KEY_SHIFTER_PL1_PIN PINPAD_L25 // X60
#define KEY_SHIFTER_PL1_PIN PINPAD_L23 // SF2000
#define KEY_SHIFTER_PL2_PIN PINPAD_L26 // currently unimplemented

// TODO: the X60 handheld have BTN_L and BTN_R swapped compared to SF2000
// need to conditionaly adjust the enum based on if we compile for X60 or SF2000
typedef enum {
	SF2000_BTN_R = 0,
	SF2000_BTN_Y,
	SF2000_BTN_X,
	SF2000_BTN_L,
	SF2000_BTN_A,
	SF2000_BTN_B,
	SF2000_BTN_SELECT,
	SF2000_BTN_START,
	SF2000_BTN_UP,
	SF2000_BTN_DOWN,
	SF2000_BTN_LEFT,
	SF2000_BTN_RIGHT,
	SF2000_BTN_LAST,
} sf2000_btn_e;

// TODO: static var?
static int btn_state[SF2000_BTN_LAST];


static const char *sf2000_joypad_name(unsigned pad)
{
	return "SF2000 Controller";
}

static void *sf2000_joypad_init(void *data)
{
	RARCH_LOG(LOG_TAG " Init\n");

	memset(btn_state, 0, sizeof(btn_state));

	return (void*)-1;
}

static void sf2000_joypad_destroy(void)
{
	RARCH_LOG(LOG_TAG " Destroy\n");
}

static int32_t sf2000_joypad_button(unsigned port, uint16_t joykey)
{
	//LOGX("port=%u, joykey=%u\n", port, joykey);

	if (port != 0)
		return 0;

	// TODO: sf2000 buttons and the retroarch buttons are different bits 
	// for a nicer code maybe create a static array to map between them

	joykey &= RETRO_DEVICE_ID_JOYPAD_MASK;

	// caller wants all buttons state
	if (joykey == RETRO_DEVICE_ID_JOYPAD_MASK) {
		return
			btn_state[SF2000_BTN_A] << RETRO_DEVICE_ID_JOYPAD_A |
			btn_state[SF2000_BTN_B] << RETRO_DEVICE_ID_JOYPAD_B |
			btn_state[SF2000_BTN_X] << RETRO_DEVICE_ID_JOYPAD_X |
			btn_state[SF2000_BTN_Y] << RETRO_DEVICE_ID_JOYPAD_Y |
			btn_state[SF2000_BTN_L] << RETRO_DEVICE_ID_JOYPAD_L |
			btn_state[SF2000_BTN_R] << RETRO_DEVICE_ID_JOYPAD_R |
			btn_state[SF2000_BTN_UP] << RETRO_DEVICE_ID_JOYPAD_UP |
			btn_state[SF2000_BTN_DOWN] << RETRO_DEVICE_ID_JOYPAD_DOWN |
			btn_state[SF2000_BTN_LEFT] << RETRO_DEVICE_ID_JOYPAD_LEFT |
			btn_state[SF2000_BTN_RIGHT] << RETRO_DEVICE_ID_JOYPAD_RIGHT |
			btn_state[SF2000_BTN_START] << RETRO_DEVICE_ID_JOYPAD_START |
			btn_state[SF2000_BTN_SELECT] << RETRO_DEVICE_ID_JOYPAD_SELECT;
	}
	else {
		// caller way want indevidual button state
		switch (joykey) {
			case RETRO_DEVICE_ID_JOYPAD_A: return btn_state[SF2000_BTN_A];
			case RETRO_DEVICE_ID_JOYPAD_B: return btn_state[SF2000_BTN_B];
			case RETRO_DEVICE_ID_JOYPAD_X: return btn_state[SF2000_BTN_X];
			case RETRO_DEVICE_ID_JOYPAD_Y: return btn_state[SF2000_BTN_Y];
			case RETRO_DEVICE_ID_JOYPAD_L: return btn_state[SF2000_BTN_L];
			case RETRO_DEVICE_ID_JOYPAD_R: return btn_state[SF2000_BTN_R];
			case RETRO_DEVICE_ID_JOYPAD_UP: return btn_state[SF2000_BTN_UP];
			case RETRO_DEVICE_ID_JOYPAD_DOWN: return btn_state[SF2000_BTN_DOWN];
			case RETRO_DEVICE_ID_JOYPAD_LEFT: return btn_state[SF2000_BTN_LEFT];
			case RETRO_DEVICE_ID_JOYPAD_RIGHT: return btn_state[SF2000_BTN_RIGHT];
			case RETRO_DEVICE_ID_JOYPAD_START: return btn_state[SF2000_BTN_START];
			case RETRO_DEVICE_ID_JOYPAD_SELECT: return btn_state[SF2000_BTN_SELECT];
		}
	}

	return 0;
}

static void sf2000_joypad_get_buttons(unsigned port, input_bits_t *state)
{
	LOGX("port=%u\n", port);
}

static int16_t sf2000_joypad_axis(unsigned port, uint32_t joyaxis)
{
   return 0;
}

static void sf2000_joypad_poll(void)
{
#ifdef LOG_BTN_PRESS
	static const char * const key_names[] = {
		"R", "Y", "X", "L", "A", "B", "Select", "Start", "Up", "Down", "Left", "Right"
	};

	int prev[ARRAY_SIZE(btn_state)];
	memcpy(prev, btn_state, sizeof(btn_state));
#endif

	//LOGX("poll\n");

	gpio_configure(KEY_SHIFTER_CLK_PIN, GPIO_DIR_OUTPUT);
	gpio_set_output(KEY_SHIFTER_CLK_PIN, 1); // shifts on 1->0 transition

	// probably latches the state while the pin is actively driven
	// setup the shifter to collect the inputs
	gpio_configure(KEY_SHIFTER_PL1_PIN, GPIO_DIR_OUTPUT);
	gpio_set_output(KEY_SHIFTER_PL1_PIN, 0);

	usleep(4);

	// setup the shifter for serial reading of the inputs it collected
	gpio_configure(KEY_SHIFTER_PL1_PIN, GPIO_DIR_INPUT);

	for (int i = 0; i < ARRAY_SIZE(btn_state); i++) {
		// read one input bit for one button state
		// returned values are 0=press and 1=release, hence flip the values
		// so that btn_state will represent 1=pressed and 0=released
		btn_state[i] = 1 ^ gpio_get_input(KEY_SHIFTER_PL1_PIN);

		// pulse the shifter clock to prepare reading the next bit
		gpio_set_output(KEY_SHIFTER_CLK_PIN, 0);
		usleep(2);
		gpio_set_output(KEY_SHIFTER_CLK_PIN, 1);
	}

#ifdef LOG_BTN_PRESS
	for (int i = 0; i < ARRAY_SIZE(btn_state); i++) if (prev[i] != btn_state[i]) {
		RARCH_LOG(LOG_TAG "poll = %s(%d) %s\n", key_names[i], i, btn_state[i] ? "pressed" : "released");
	}
#endif
}

static int16_t sf2000_joypad_state(
      rarch_joypad_info_t *joypad_info,
      const struct retro_keybind *binds,
      unsigned port)
{
	// TODO: what is the diff between this function and sf2000_joypad_button?
	// which one get called, when, why and by whom?

	//LOGX("port=%u\n", port);
	return (int16_t)sf2000_joypad_button(port, RETRO_DEVICE_ID_JOYPAD_MASK);
}

static bool sf2000_joypad_query_pad(unsigned pad)
{
	// TODO: when, why and by whom this is being called?
	LOGX("pad=%u\n", pad);
	return false;
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
