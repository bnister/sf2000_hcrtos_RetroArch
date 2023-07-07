/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2017 - Daniel De Matteis
 *  Copyright (C) 2013-2014 - Tobias Jakobi
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
#include <string.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/mman.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#ifdef HAVE_MENU
#include "../../menu/menu_driver.h"
#endif

#include <retro_inline.h>
#include <gfx/scaler/scaler.h>
#include <gfx/video_frame.h>
#include <string/stdstring.h>

#include "../font_driver.h"

#include "../../configuration.h"
#include "../../driver.h"
#include "../../retroarch.h"

#include <hcuapi/fb.h>

typedef struct sf2000_data
{
} sf2000_data_t;

static void sf2000_free(void *data)
{

}

static void *sf2000_init(const video_info_t *video,
      input_driver_t **input, void **input_data)
{
   return NULL;
}

static bool sf2000_frame(void *data, const void *frame, unsigned width,
      unsigned height, uint64_t frame_count, unsigned pitch, const char *msg,
      video_frame_info_t *video_info)
{
   return true;
}

static void sf2000_set_nonblock_state(void *data, bool state, 
      bool adaptive_vsync_enabled, unsigned swap_interval)
{
}

static bool sf2000_alive(void *data) { return true; /* always alive */ }
static bool sf2000_focus(void *data) { return true; /* fb device always has focus */ }

static void sf2000_viewport_info(void *data, struct video_viewport *vp)
{

   vp->x = vp->y     = 0;

   vp->width         = vp->full_width  = 320;
   vp->height        = vp->full_height = 240;
}

static bool sf2000_suppress_screensaver(void *data, bool enable) { return false; }
static bool sf2000_has_windowed(void *data) { return true; }

static bool sf2000_set_shader(void *data,
      enum rarch_shader_type type, const char *path) { return false; }

static void sf2000_set_texture_frame(void *data, const void *frame, bool rgb32,
      unsigned width, unsigned height, float alpha)
{
}

static void sf2000_set_texture_enable(void *data, bool state, bool full_screen)
{
}

static float sf2000_get_refresh_rate(void *data)
{
   return 60.0f;
}

static const video_poke_interface_t sf2000_poke_interface = {
   NULL, /* get_flags  */
   NULL, /* load_texture */
   NULL, /* unload_texture */
   NULL, /* set_video_mode */
   sf2000_get_refresh_rate,
   NULL, /* set_filtering */
   NULL, /* get_video_output_size */
   NULL, /* get_video_output_prev */
   NULL, /* get_video_output_next */
   NULL, /* get_current_framebuffer */
   NULL, /* get_proc_address */
   NULL, /* set_aspect_ratio */
   NULL, /* apply_state_changes */
   sf2000_set_texture_frame,
   sf2000_set_texture_enable,
   NULL, /* set_osd_msg */
   NULL, /* show_mouse */
   NULL, /* grab_mouse_toggle */
   NULL, /* get_current_shader */
   NULL, /* get_current_software_framebuffer */
   NULL, /* get_hw_render_interface */
   NULL, /* set_hdr_max_nits */
   NULL, /* set_hdr_paper_white_nits */
   NULL, /* set_hdr_contrast */
   NULL  /* set_hdr_expand_gamut */
};

static void sf2000_get_poke_interface(void *data,
      const video_poke_interface_t **iface)
{
   *iface = &sf2000_poke_interface;
}

video_driver_t video_sf2000 = {
   sf2000_init,
   sf2000_frame,
   sf2000_set_nonblock_state,
   sf2000_alive,
   sf2000_focus,
   sf2000_suppress_screensaver,
   sf2000_has_windowed,
   sf2000_set_shader,
   sf2000_free,
   "sf2000",
   NULL, /* set_viewport */
   NULL, /* set_rotation */
   sf2000_viewport_info,
   NULL, /* read_viewport  */
   NULL, /* read_frame_raw */
#ifdef HAVE_OVERLAY
   NULL, /* get_overlay_interface */
#endif
   sf2000_get_poke_interface,
   NULL, /* wrap_type_to_enum */
#ifdef HAVE_GFX_WIDGETS
   NULL  /* gfx_widgets_enabled */
#endif
};
