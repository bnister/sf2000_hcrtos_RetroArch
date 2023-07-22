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

#include "verbosity.h"
#include <inttypes.h>
#include "logx.h"

#include <kernel/io.h>
#include <kernel/fb.h>
#include <hcuapi/fb.h>
#include <hcge/ge_api.h>
#include <cpu_func.h>

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

// TODO: move the statics to sf2000_gfx_data_t 
static hcge_context *ctx = NULL;
static int fbdev;
static struct fb_fix_screeninfo fix;    /* Current fix */
static struct fb_var_screeninfo var;    /* Current var */
static uint32_t screen_size;
static uint8_t *fb_base;
static uint32_t line_width;
static uint32_t pixel_size;
//static uint8_t *screen_buffer[2];
static int buffer_num  = 4;

typedef struct sf2000_gfx_data
{
	bool vsync;
} sf2000_gfx_data_t;


static int init_fb_device(void)
{
    int ret;
    if(hcge_open(&ctx) != 0) {
        LOGX("Init hcge error.\n");
        return -1;
    }
    fbdev = open("/dev/fb0", O_RDWR);

    ioctl(fbdev, FBIOGET_FSCREENINFO, &fix);
    ioctl(fbdev, FBIOGET_VSCREENINFO, &var);

    line_width  = var.xres * var.bits_per_pixel / 8;
    pixel_size = var.bits_per_pixel / 8;
    screen_size = var.xres * var.yres * var.bits_per_pixel / 8;

    buffer_num = fix.smem_len / screen_size;

    // Make sure that the display is on.
    if (ioctl(fbdev, FBIOBLANK, FB_BLANK_UNBLANK) != 0) {
        LOGX("FB_BLANK_UNBLANK failed\n");
    }

	LOGX("buffer_num=%d\n", buffer_num);
	LOGX("xres=%d, yres=%d, xres_virtual=%d, yres_virtual=%d\n",
	       (int)var.xres, (int)var.yres, (int)var.xres_virtual, (int)var.yres_virtual);
	LOGX("bits_per_pixel=%d, red.length=%d, green.length=%d, blue.length=%d, transp.length=%d\n",
	       (int)var.bits_per_pixel, (int)var.red.length, (int)var.green.length,
	       (int)var.blue.length, (int)var.transp.length);

    /*var.activate = FB_ACTIVATE_VBL;*/
    //var.activate = FB_ACTIVATE_NOW;
    var.yoffset = 0;
    var.xoffset = 0;
	var.xres_virtual = var.xres;
	var.yres_virtual = var.yres;
	var.bits_per_pixel = 16;
	var.red.length = 5;
	var.green.length = 6;
	var.blue.length = 5;
    //var.transp.length = 8;
    //var.yres_virtual = buffer_num * var.yres;

    //set variable information
    if(ioctl(fbdev, FBIOPUT_VSCREENINFO, &var) == -1) {
        LOGX("FBIOPUT_VSCREENINFO failed\n");
        return -1;
    }

    fb_base = (unsigned char *)mmap(NULL, fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbdev, 0);
    if (fb_base == MAP_FAILED) {
        LOGX("mmap failed\n");
        return -1;
    }
    memset(fb_base, 0, fix.smem_len);
	memset(fb_base, 0x55, fix.smem_len);

    ret = ioctl(fbdev, FBIOPAN_DISPLAY, &var);
    if(ret < 0)
        LOGX("FBIOPAN_DISPLAY failed. ret=%d\n", ret);

    ret = ioctl(fbdev, FBIO_WAITFORVSYNC, &ret);
    if (ret < 0)
        LOGX("FBIO_WAITFORVSYNC failed. ret=%d\n", ret);

    return 0;
}

static void deinit_fb_device(void)
{
    if(fbdev > 0) {
        if(fb_base) {
            munmap(fb_base, screen_size);
            fb_base = NULL;
        }
        close(fbdev);
        fbdev = -1;
    }
}

static void blit(const void *frame, unsigned width, unsigned height, unsigned pitch)
{
	// TODO: do i need a special case for when frame is already full screen and use possibly a bit faster 
	// regular hcge_blit, or just let hcge_stretch_blit handle it internaly?

	// TODO: implement other stretching options to preserve the original image ratio
	
	hcge_state *state = &ctx->state;
	HCGERectangle srect = {0, 0, width, height};
	HCGERectangle drect = {0, 0, var.xres, var.yres};

    state->render_options = HCGE_DSRO_NONE;
    state->drawingflags = HCGE_DSDRAW_NOFX;
    state->blittingflags = HCGE_DSBLIT_NOFX;

    state->src_blend = HCGE_DSBF_SRCALPHA;
    state->dst_blend = HCGE_DSBF_ZERO;

    state->destination.config.size.w = var.xres;
    state->destination.config.size.h = var.yres;
    state->destination.config.format = HCGE_DSPF_RGB16;
    state->dst.phys = PHY_ADDR(fb_base);
    state->dst.pitch = line_width;

    state->source.config.size.w = width;
    state->source.config.size.h = height;
    state->source.config.format = HCGE_DSPF_RGB16;
    state->src.phys = PHY_ADDR(frame);
    state->src.pitch = pitch;

	// NOTE: this fixes the artifacts on the left side of the screen
	cache_flush(frame, pitch * height);

    state->accel = HCGE_DFXL_STRETCHBLIT;
    hcge_set_state(ctx, &ctx->state, state->accel);
	if (!hcge_stretch_blit(ctx, &srect, &drect)) {
		// TODO:
		static int count = 0;
		if (count == 0)
			LOGX("hcge_stretch_blit failed\n");
		count = (count + 1) % 60;
	}
    hcge_engine_sync(ctx);
}

static void *sf2000_gfx_init(const video_info_t *video,
      input_driver_t **input, void **input_data)
{
	sf2000_gfx_data_t *ctx = (sf2000_gfx_data_t*)calloc(1, sizeof(sf2000_gfx_data_t));
	if (!ctx)
		return NULL;

	LOGX("w=%u h=%u rgb32=%d fullscreen=%d vsync=%d path_font=%s\n", 
		video->width, video->height, video->rgb32, video->fullscreen, video->vsync,
		video->path_font ? video->path_font : "Null");

	settings_t *settings                          = config_get_ptr();
	const char *input_drv_name                    = settings->arrays.input_driver;
	const char *joypad_drv_name                   = settings->arrays.input_joypad_driver;
	LOGX("input_drv_name=%s, joypad_drv_name=%s\n", input_drv_name, joypad_drv_name);

	*input_data = input_driver_init_wrap(&input_sf2000, "sf2000");
	if (*input_data)
		*input = &input_sf2000;

	init_fb_device();

	return ctx;
}

static void sf2000_gfx_free(void *data)
{
	sf2000_gfx_data_t* ctx = (sf2000_gfx_data_t*)data;
	if (!ctx)
		return;

	deinit_fb_device();

	free(ctx);
}

static bool sf2000_gfx_frame(void *data, const void *frame, unsigned width,
      unsigned height, uint64_t frame_count, unsigned pitch, const char *msg,
      video_frame_info_t *video_info)
{
	sf2000_gfx_data_t* ctx = (sf2000_gfx_data_t*)data;
	if (!ctx)
		return false;

	// TODO: need to display the msg on screen
	if (msg && msg[0]) {
		static int count = 0;

		if (count == 0)
			LOGX("msg=[%s]\n", msg);

		// only log msgs approximately once per 2seconds
		count = (count+1) % (2*60);
	}

	// TODO: better handle the case when menu is showing.
	// for exmaple on windows i've noticed that the menu frame is partially translucent
	// while the game frame still showing underneath.
	bool menu_is_alive = video_info->menu_is_alive;
	if (menu_is_alive) {
		// TODO: what does it do? draws the menu frame?
		menu_driver_frame(menu_is_alive, video_info);
	}
	else
		// only blit game frame when menu is not active
		blit(frame, width, height, width*2);

	//LOGX("width=%d height=%d pitch=%d\n", width, height, pitch);
	return true;
}

static void sf2000_gfx_set_nonblock_state(void *data, bool state, 
      bool adaptive_vsync_enabled, unsigned swap_interval)
{
	sf2000_gfx_data_t* ctx = (sf2000_gfx_data_t*)data;
	if (!ctx)
		return;

	ctx->vsync = !state;
}

static bool sf2000_gfx_alive(void *data) { return true; /* always alive */ }
static bool sf2000_gfx_focus(void *data) { return true; /* fb device always has focus */ }

static void sf2000_gfx_viewport_info(void *data, struct video_viewport *vp)
{
	sf2000_gfx_data_t* ctx = (sf2000_gfx_data_t*)data;
	if (!ctx)
		return;

   vp->x = vp->y = 0;

   vp->width  = vp->full_width  = SCREEN_WIDTH;
   vp->height = vp->full_height = SCREEN_HEIGHT;
}

static bool sf2000_gfx_suppress_screensaver(void *data, bool enable) { return false; }

static bool sf2000_gfx_set_shader(void *data,
      enum rarch_shader_type type, const char *path) { return false; }

static void sf2000_gfx_set_texture_frame(void *data, const void *frame, bool rgb32,
      unsigned width, unsigned height, float alpha)
{
	sf2000_gfx_data_t* ctx = (sf2000_gfx_data_t*)data;
	if (!ctx)
		return;

	//LOGX("rgb32=%s width=%d height=%d alpha=%f\n", rgb32 ? "True" : "False", width, height, alpha);

	blit(frame, width, height, width*2/*TODO: fixme*/);
}

static void sf2000_gfx_set_texture_enable(void *data, bool state, bool full_screen)
{
	sf2000_gfx_data_t* ctx = (sf2000_gfx_data_t*)data;
	if (!ctx)
		return;

	//LOGX("state=%s full_screen=%s\n", state ? "True" : "False", full_screen ? "True" : "False");
}

static float sf2000_gfx_get_refresh_rate(void *data)
{
	return 60.0f;
}

static const video_poke_interface_t sf2000_gfx_poke_interface = {
   NULL, /* get_flags  */
   NULL, /* load_texture */
   NULL, /* unload_texture */
   NULL, /* set_video_mode */
   sf2000_gfx_get_refresh_rate,
   NULL, /* set_filtering */
   NULL, /* get_video_output_size */
   NULL, /* get_video_output_prev */
   NULL, /* get_video_output_next */
   NULL, /* get_current_framebuffer */
   NULL, /* get_proc_address */
   NULL, /* set_aspect_ratio */
   NULL, /* apply_state_changes */
   sf2000_gfx_set_texture_frame,
   sf2000_gfx_set_texture_enable,
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

static void sf2000_gfx_get_poke_interface(void *data,
      const video_poke_interface_t **iface)
{
   *iface = &sf2000_gfx_poke_interface;
}

video_driver_t video_sf2000 = {
   sf2000_gfx_init,
   sf2000_gfx_frame,
   sf2000_gfx_set_nonblock_state,
   sf2000_gfx_alive,
   sf2000_gfx_focus,
   sf2000_gfx_suppress_screensaver,
   NULL, /* has_windowed */
   sf2000_gfx_set_shader,
   sf2000_gfx_free,
   "sf2000",
   NULL, /* set_viewport */
   NULL, /* set_rotation */
   sf2000_gfx_viewport_info,
   NULL, /* read_viewport  */
   NULL, /* read_frame_raw */
#ifdef HAVE_OVERLAY
   NULL, /* get_overlay_interface */
#endif
   sf2000_gfx_get_poke_interface,
   NULL, /* wrap_type_to_enum */
#ifdef HAVE_GFX_WIDGETS
   NULL  /* gfx_widgets_enabled */
#endif
};
