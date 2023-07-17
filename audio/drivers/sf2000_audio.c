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

#include <sys/poll.h>
#include <hcuapi/avsync.h>
#include <hcuapi/snd.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#include "audio/audio_driver.h"
#include "verbosity.h"
#include "logx.h"
#define LOG_TAG "[SF2000][Audio]"

#define DEFAULT_SND_DEV "/dev/sndC0i2so"

#define AUDIO_BUFFER_SIZE	128 * 1024
#define AUDIO_CHANNELS		2
#define AUDIO_BITS			16

typedef struct sf2000_audio
{
	bool nonblock;
	bool running;
	int snd_fd;
	struct pollfd pfd;
} sf2000_audio_t;


static void *sf2000_audio_init(const char *device, unsigned rate, unsigned latency,
      unsigned block_frames,
      unsigned *new_out_rate)
{
	int ret;

	LOGX("device=%s rate=%u latency=%u block_frames=%u\n", device ? device : "Null", rate, latency, block_frames);

	sf2000_audio_t *ctx = (sf2000_audio_t*)calloc(1, sizeof(sf2000_audio_t));
	if (!ctx)
		return NULL;

	int snd_fd = open(DEFAULT_SND_DEV, O_WRONLY);
	if (snd_fd == -1) {
		LOGX("open(" DEFAULT_SND_DEV ") errno=%d\n", errno);
		free(ctx);
		return NULL;
	}

	struct snd_pcm_params params = {0};
	params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params.format = SND_PCM_FORMAT_S16_LE;
	params.sync_mode = AVSYNC_TYPE_FREERUN;
	params.align = 0;
	params.rate = rate;

	int read_size = 24000;
	snd_pcm_uframes_t poll_size = read_size;

	params.channels = 2;
	params.period_size = read_size;
	params.periods = 8;
	params.bitdepth = 16;
	params.start_threshold = 2;
	ret = ioctl(snd_fd, SND_IOCTL_HW_PARAMS, &params);
	if (ret < 0)
		LOGX("SND_IOCTL_HW_PARAMS error\n");

	ret = ioctl(snd_fd, SND_IOCTL_AVAIL_MIN, &poll_size);
	if (ret < 0)
		LOGX("SND_IOCTL_HW_PARAMS error\n");

	ctx->snd_fd = snd_fd;
	ctx->pfd.fd = snd_fd;
	ctx->pfd.events = POLLOUT | POLLWRNORM;

	return ctx;
}

static ssize_t sf2000_audio_write(void *data, const void *buf, size_t size)
{
	sf2000_audio_t* ctx = (sf2000_audio_t*)data;
	if (!ctx)
		return -1;

	if (!ctx->running)
		return -1;

	//LOGX("size=%u\n", size);

	int count = 0;
	int ret;
	do {
		struct snd_xfer xfer = {0};
		xfer.data = buf;
		xfer.frames = size/4;	// 4 is 2channels*16bitsample
		//xfer.tstamp_ms = pts;
		ret = ioctl(ctx->snd_fd, SND_IOCTL_XFER, &xfer);
		if (ret < 0) {
			LOGX("poll. SND_IOCTL_XFER ret=%d\n", ret);
			poll(&ctx->pfd, 1, 100);
		}
		if (++count > 20) {
			LOGX("forcefully break out of the loop\n");
			break;
		}
	} while (ret < 0);

	return size;
}

static bool sf2000_audio_stop(void *data)
{
	sf2000_audio_t* ctx = (sf2000_audio_t*)data;
	if (!ctx)
		return false;

	LOGX("stop\n");

	//ioctl(ctx->snd_fd, SND_IOCTL_PAUSE, 0);
	int ret = ioctl(ctx->snd_fd, SND_IOCTL_DROP, 0);
	if (ret < 0)
		LOGX("SND_IOCTL_DROP ret=%d\n", ret);

	ctx->running = false;
	return true;
}

static bool sf2000_audio_start(void *data, bool is_shutdown)
{
	sf2000_audio_t* ctx = (sf2000_audio_t*)data;
	if (!ctx)
		return false;

	LOGX("start\n");

	//ioctl(ctx->snd_fd, SND_IOCTL_RESUME, 0);
	int ret = ioctl(ctx->snd_fd, SND_IOCTL_START, 0);
	if (ret < 0)
		LOGX("SND_IOCTL_START ret=%d\n", ret);

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

	LOGX("free\n");

	ioctl(ctx->snd_fd, SND_IOCTL_HW_FREE, 0);

	close(ctx->snd_fd);
	free(ctx);
	ctx = NULL;
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
