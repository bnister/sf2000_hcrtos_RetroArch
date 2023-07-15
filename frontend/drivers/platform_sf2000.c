/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2017 - Daniel De Matteis
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
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#include <rthreads/rthreads.h>

#include <boolean.h>
#include <compat/strl.h>

#include <file/file_path.h>
#ifndef IS_SALAMANDER
#include <lists/file_list.h>
#endif
#include <string/stdstring.h>
#include <streams/file_stream.h>

#include "../frontend_driver.h"

#include "../../command.h"
#include "../../defaults.h"
#include "../../msg_hash.h"
#include "../../retroarch_types.h"
#include "../../verbosity.h"

#if !defined(IS_SALAMANDER)
#include "../../paths.h"
#include "../../menu/menu_entries.h"

#endif

#ifdef USBGECKO
#include <debug.h>
#endif

#include "gfx/video_driver.h"

// TODO: get the mounted sdcard directory at runtime
#define SD_PREFIX "/media/mmcblk0p2"

#define LOG_TAG "[SF2000][Frontend]"

#if defined(HW_RVL) && ! defined(IS_SALAMANDER)
static enum frontend_fork sf2000_fork_mode = FRONTEND_FORK_NONE;
#endif

#ifdef IS_SALAMANDER
extern char sf2000_rom_path[PATH_MAX_LENGTH];
#endif


static void frontend_sf2000_get_env(int *argc, char *argv[],
      void *args, void *params_data)
{
	// TODO: research how these params are passed. do we need them?
	// maybe is has something to do with how salamander works?
	// for now they always zero and null
	if (*argc >= 0 && argv)
		for (int i=0; i<*argc; i++)
			RARCH_LOG(LOG_TAG " get_env - argv[%d]=%s\n", i, argv[i]);


   char *slash;
#ifndef IS_SALAMANDER
   struct rarch_main_wrap *params = (struct rarch_main_wrap*)params_data;
#endif

#ifdef HW_DOL
   chdir("carda:/retroarch");
#endif

   getcwd(g_defaults.dirs[DEFAULT_DIR_CORE],
      sizeof(g_defaults.dirs[DEFAULT_DIR_CORE]));

#ifndef IS_SALAMANDER
#ifdef HAVE_LOGGER
   logger_init();
// TODO: is it necessary to do it here. maybe it should be in main() or set by configuration
//#elif defined(HAVE_FILE_LOGGER)
//   retro_main_log_file_init("/retroarch-log.txt");
#endif

   /* This situation can happen on some loaders so we really need some fake
      args or else RetroArch will just crash on parsing NULL pointers. */
   if (*argc <= 0 || !argv)
   {
      if (params)
      {
         params->content_path  = NULL;
         params->sram_path     = NULL;
         params->state_path    = NULL;
         params->config_path   = NULL;
         params->libretro_path = NULL;
         params->flags        &= ~(RARCH_MAIN_WRAP_FLAG_VERBOSE
                                 | RARCH_MAIN_WRAP_FLAG_NO_CONTENT);
         params->flags        |=   RARCH_MAIN_WRAP_FLAG_TOUCHED;
      }
   }
#ifdef HW_RVL
   else if (*argc > 2 &&
         !string_is_empty(argv[1]) && !string_is_empty(argv[2]))
   {
#ifdef HAVE_NETWORKING
      /* If the process was forked for netplay purposes,
         DO NOT touch the arguments. */
      if (!string_is_equal(argv[1], "-H") && !string_is_equal(argv[1], "-C"))
#endif
      {
         /* When using external loaders (Wiiflow, etc),
            getcwd doesn't return the path correctly and as a result,
            the cfg file is not found. */
         if (     string_starts_with_size(argv[0], "usb1", STRLEN_CONST("usb1"))
               || string_starts_with_size(argv[0], "usb2", STRLEN_CONST("usb2")))
         {
            size_t _len = strlcpy(g_defaults.dirs[DEFAULT_DIR_CORE], "usb",
                  sizeof(g_defaults.dirs[DEFAULT_DIR_CORE]));
            strlcpy(g_defaults.dirs[DEFAULT_DIR_CORE]       + _len,
                  argv[0] + 4,
                  sizeof(g_defaults.dirs[DEFAULT_DIR_CORE]) - _len);
         }

         /* Needed on Wii; loaders follow a dumb standard where the path and
            filename are separate in the argument list. */
         if (params)
         {
            static char path[PATH_MAX_LENGTH];

            fill_pathname_join(path, argv[1], argv[2], sizeof(path));

            params->content_path  = path;
            params->sram_path     = NULL;
            params->state_path    = NULL;
            params->config_path   = NULL;
            params->libretro_path = NULL;
            params->flags        &= ~(RARCH_MAIN_WRAP_FLAG_VERBOSE
                  | RARCH_MAIN_WRAP_FLAG_NO_CONTENT);
            params->flags        |=   RARCH_MAIN_WRAP_FLAG_TOUCHED;
         }
      }
   }
#endif
#else
   if (*argc > 2 && argv &&
         !string_is_empty(argv[1]) && !string_is_empty(argv[2]))
      fill_pathname_join(sf2000_rom_path, argv[1], argv[2], sizeof(sf2000_rom_path));
   else
      *sf2000_rom_path = '\0';
#endif


	// TODO: deside which dirs should be initialize here
	// and where they should reside on the sdcard

   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_PORT],
      SD_PREFIX, "retroarch",
      sizeof(g_defaults.dirs[DEFAULT_DIR_PORT]));

   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_CORE],
      g_defaults.dirs[DEFAULT_DIR_PORT], "cores",
      sizeof(g_defaults.dirs[DEFAULT_DIR_CORE]));

   /* System paths */
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_CORE_INFO],
      g_defaults.dirs[DEFAULT_DIR_PORT], "info",
      sizeof(g_defaults.dirs[DEFAULT_DIR_CORE_INFO]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_AUTOCONFIG],
      g_defaults.dirs[DEFAULT_DIR_PORT], "autoconfig",
      sizeof(g_defaults.dirs[DEFAULT_DIR_AUTOCONFIG]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_OVERLAY],
      g_defaults.dirs[DEFAULT_DIR_PORT], "overlays",
      sizeof(g_defaults.dirs[DEFAULT_DIR_OVERLAY]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_VIDEO_FILTER],
      g_defaults.dirs[DEFAULT_DIR_PORT], "filters/video",
      sizeof(g_defaults.dirs[DEFAULT_DIR_VIDEO_FILTER]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_AUDIO_FILTER],
      g_defaults.dirs[DEFAULT_DIR_PORT], "filters/audio",
      sizeof(g_defaults.dirs[DEFAULT_DIR_AUDIO_FILTER]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_ASSETS],
      g_defaults.dirs[DEFAULT_DIR_PORT], "assets",
      sizeof(g_defaults.dirs[DEFAULT_DIR_ASSETS]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_CHEATS],
      g_defaults.dirs[DEFAULT_DIR_PORT], "cheats",
      sizeof(g_defaults.dirs[DEFAULT_DIR_CHEATS]));

	fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_DATABASE], 
		g_defaults.dirs[DEFAULT_DIR_PORT], "database/rdb", 
		sizeof(g_defaults.dirs[DEFAULT_DIR_DATABASE]));

   /* User paths */
   fill_pathname_join(g_defaults.path_config,
      g_defaults.dirs[DEFAULT_DIR_PORT], "retroarch.cfg",
      sizeof(g_defaults.path_config));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_SYSTEM],
      g_defaults.dirs[DEFAULT_DIR_PORT], "system",
      sizeof(g_defaults.dirs[DEFAULT_DIR_SYSTEM]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_SRAM],
      g_defaults.dirs[DEFAULT_DIR_PORT], "savefiles",
      sizeof(g_defaults.dirs[DEFAULT_DIR_SRAM]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_SAVESTATE],
      g_defaults.dirs[DEFAULT_DIR_PORT], "savestates",
      sizeof(g_defaults.dirs[DEFAULT_DIR_SAVESTATE]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_PLAYLIST],
      g_defaults.dirs[DEFAULT_DIR_PORT], "playlists",
      sizeof(g_defaults.dirs[DEFAULT_DIR_PLAYLIST]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_LOGS],
      g_defaults.dirs[DEFAULT_DIR_PORT], "logs",
      sizeof(g_defaults.dirs[DEFAULT_DIR_LOGS]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_MENU_CONFIG],
      g_defaults.dirs[DEFAULT_DIR_PORT], "config",
      sizeof(g_defaults.dirs[DEFAULT_DIR_MENU_CONFIG]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_REMAP],
      g_defaults.dirs[DEFAULT_DIR_MENU_CONFIG], "remaps",
      sizeof(g_defaults.dirs[DEFAULT_DIR_REMAP]));

	// TODO: do we really need to create these dirs here
	for (int i = 0; i < DEFAULT_DIR_LAST; i++)
	{
		const char *dir_path = g_defaults.dirs[i];
		char new_path[PATH_MAX_LENGTH];

		if (string_is_empty(dir_path))
			continue;

		fill_pathname_expand_special(new_path, dir_path, sizeof(new_path));

		//RARCH_LOG(LOG_TAG " g_defaults.dirs[%d]=%s\n", i, g_defaults.dirs[i]);

		if (!path_is_directory(new_path))
			path_mkdir(new_path);
	}


#ifndef IS_SALAMANDER
   dir_check_defaults("custom.ini");
#endif
}

// TODO: need to implement it for our platform (?)
// there is an implementation for this for wii - wii/libogc/libogc/exception.c
// something to do with auto restart after exception maybe
extern void __exception_setreload(int t);

/* Fake sysconf for page size and processor count */
// copied from wiiu/system/missing_libc_functions.c
// TODO: is it the right place for this function?
long sysconf(int name) {
   switch (name) {
      case _SC_PAGESIZE:
      //case _SC_PAGE_SIZE:
         return 128 * 1024;			// TODO: what is it used for?
      case _SC_NPROCESSORS_CONF:
      case _SC_NPROCESSORS_ONLN:
         return 1;
      default:
         errno = EINVAL;
         return -1;
   }
}

static const struct video_driver *frontend_sf2000_get_video_driver(void)
{
	// TODO: is this necessary? didn't see other frontend drivers do that
	return &video_sf2000;
}

static void frontend_sf2000_init(void *data)
{
	RARCH_LOG(LOG_TAG " Init\n");
	
#ifdef HW_RVL
   IOS_ReloadIOS(IOS_GetVersion());
   L2Enhance();
#endif

#ifdef USBGECKO
   DEBUG_Init(GDBSTUB_DEVICE_USB, 1);
   _break();
#endif

#if defined(DEBUG) && defined(IS_SALAMANDER)
   VIInit();
   sf2000RModeObj *rmode = VIDEO_GetPreferredMode(NULL);
   void *xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
   console_init(xfb, 20, 20, rmode->fbWidth,
         rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);
   VIConfigure(rmode);
   VISetNextFramebuffer(xfb);
   VISetBlack(FALSE);
   VIFlush();
   VIWaitForRetrace();
   VIWaitForRetrace();
#endif

#ifndef DEBUG
	// TODO: comment this out for now
	//__exception_setreload(8);
#endif
}

static void frontend_sf2000_deinit(void *data)
{
}

static void frontend_sf2000_exec(const char *path, bool should_load_game)
{
}

static void frontend_sf2000_exitspawn(char *s, size_t len, char *args)
{
   bool should_load_game = false;
#if defined(IS_SALAMANDER)
   if (!string_is_empty(sf2000_rom_path))
      should_load_game = true;
#elif defined(HW_RVL)
   char salamander_basename[PATH_MAX_LENGTH];

   if (sf2000_fork_mode == FRONTEND_FORK_NONE)
      return;

   switch (sf2000_fork_mode)
   {
      case FRONTEND_FORK_CORE_WITH_ARGS:
         should_load_game = true;
         break;
      case FRONTEND_FORK_CORE:
         /* fall-through */
      case FRONTEND_FORK_RESTART:
         {
            char new_path[PATH_MAX_LENGTH];
            char salamander_name[PATH_MAX_LENGTH];

            if (frontend_driver_get_salamander_basename(salamander_name,
                     sizeof(salamander_name)))
            {
               fill_pathname_join(new_path, g_defaults.dirs[DEFAULT_DIR_CORE],
                     salamander_name, sizeof(new_path));
               path_set(RARCH_PATH_CONTENT, new_path);
            }
         }
         break;
      case FRONTEND_FORK_NONE:
      default:
         break;
   }

   system_exec_wii(s, should_load_game);
   frontend_driver_get_salamander_basename(salamander_basename,
         sizeof(salamander_basename));

   /* FIXME/TODO - hack
    * direct loading failed (out of memory),
    * try to jump to Salamander,
    * then load the correct core */
   fill_pathname_join(s, g_defaults.dirs[DEFAULT_DIR_CORE],
         salamander_basename, len);
#endif
   frontend_sf2000_exec(s, should_load_game);
}

static void frontend_sf2000_process_args(int *argc, char *argv[])
{
#ifndef IS_SALAMANDER
   /* A big hack: sometimes Salamander doesn't save the new core
    * it loads on first boot, so we make sure
    * active core path is set here. */
   if (path_is_empty(RARCH_PATH_CORE) && *argc >= 1 && strrchr(argv[0], '/'))
   {
      char path[PATH_MAX_LENGTH] = {0};
      strlcpy(path, strrchr(argv[0], '/') + 1, sizeof(path));
      if (path_is_valid(path))
         path_set(RARCH_PATH_CORE, path);
   }
#endif
}

#if defined(HW_RVL) && !defined(IS_SALAMANDER)
static bool frontend_sf2000_set_fork(enum frontend_fork fork_mode)
{
   switch (fork_mode)
   {
      case FRONTEND_FORK_CORE:
         sf2000_fork_mode  = fork_mode;
         break;
      case FRONTEND_FORK_CORE_WITH_ARGS:
         sf2000_fork_mode  = fork_mode;
         break;
      case FRONTEND_FORK_RESTART:
         sf2000_fork_mode  = fork_mode;
         command_event(CMD_EVENT_QUIT, NULL);
         break;
      case FRONTEND_FORK_NONE:
      default:
         return false;
   }

   return true;
}
#endif

static int frontend_sf2000_get_rating(void)
{
   return 6;
}

static enum frontend_architecture frontend_sf2000_get_arch(void)
{
   return FRONTEND_ARCH_MIPS;
}

static int frontend_sf2000_parse_drive_list(void *data, bool load_content)
{
#ifndef IS_SALAMANDER
#endif

   return 0;
}

static void frontend_sf2000_shutdown(bool unused)
{
#ifndef IS_SALAMANDER
   exit(0);
#endif
}

static uint64_t frontend_sf2000_get_total_mem(void)
{
   return 0;
}

static uint64_t frontend_sf2000_get_free_mem(void)
{
   return 0;
}

frontend_ctx_driver_t frontend_ctx_sf2000 = {
   frontend_sf2000_get_env,             /* get_env */
   frontend_sf2000_init,
   frontend_sf2000_deinit,
   frontend_sf2000_exitspawn,
   frontend_sf2000_process_args,
   frontend_sf2000_exec,
#if defined(HW_RVL) && !defined(IS_SALAMANDER)
   frontend_sf2000_set_fork,            /* set_fork */
#else
   NULL,                            /* set_fork */
#endif
   frontend_sf2000_shutdown,            /* shutdown */
   NULL,                            /* get_name */
   NULL,                            /* get_os */
   frontend_sf2000_get_rating,          /* get_rating */
   NULL,                            /* content_loaded */
   frontend_sf2000_get_arch,            /* get_architecture */
   NULL,                            /* get_powerstate */
   frontend_sf2000_parse_drive_list,    /* parse_drive_list */
   frontend_sf2000_get_total_mem,       /* get_total_mem */
   frontend_sf2000_get_free_mem,        /* get_free_mem */
   NULL,                            /* install_signal_handler */
   NULL,                            /* get_sighandler_state */
   NULL,                            /* set_sighandler_state */
   NULL,                            /* destroy_signal_handler_state */
   NULL,                            /* attach_console */
   NULL,                            /* detach_console */
   NULL,                            /* get_lakka_version */
   NULL,                            /* set_screen_brightness */
   NULL,                            /* watch_path_for_changes */
   NULL,                            /* check_for_path_changes */
   NULL,                            /* set_sustained_performance_mode */
   NULL,                            /* get_cpu_model_name  */
   NULL,                            /* get_user_language   */
   NULL,                            /* is_narrator_running */
   NULL,                            /* accessibility_speak */
   NULL,                            /* set_gamemode        */
   "sf2000",                            /* ident               */
   frontend_sf2000_get_video_driver /* get_video_driver    */
};
