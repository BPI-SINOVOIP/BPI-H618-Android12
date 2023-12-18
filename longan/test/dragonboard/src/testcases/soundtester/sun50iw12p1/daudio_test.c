/*
 * Copyright (c) 2012 Allwinner Technology. All Rights Reserved.
 *
 * file:		daudio_test.c
 * version:		1.0.0
 * date:		2022/02/09
 * author:		AW1638
 * descriptions:
 *		add the inital version for i2s module's dragonboard test of sun50iw12p1 platform.
 *
 * func:
 *      1. I2S TX->RX loopback and then playback the rec wav by audiocodec spk;
 *
 */

#include <asoundlib.h>
#include <sound/asound.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/input.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdbool.h>

#include "../tinyalsa/include/tinyalsa/asoundlib.h"
#include "dragonboard.h"
#include "dragonboard_inc.h"

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

#define FORMAT_PCM 1

struct wav_header {
	uint32_t riff_id;
	uint32_t riff_sz;
	uint32_t riff_fmt;
	uint32_t fmt_id;
	uint32_t fmt_sz;
	uint16_t audio_format;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bits_per_sample;
	uint32_t data_id;
	uint32_t data_sz;
};

struct riff_wave_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t wave_id;
};

struct chunk_header {
    uint32_t id;
    uint32_t sz;
};

struct chunk_fmt {
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

int capturing = 1;

unsigned int play_sample(unsigned int card, unsigned int device, char path[256],
                            int argc, char **argv);

unsigned int cap_sample(unsigned int card, unsigned int device, char path[256], unsigned int channels,
									unsigned int rate, unsigned int bits, int argc, char **argv);

static void tinymix_set_value_byname(struct mixer *mixer, const char *name,
				     unsigned int value)
{
	struct mixer_ctl *ctl;
	unsigned int num_values;
	unsigned int i;

	ctl = mixer_get_ctl_by_name(mixer, name);
	mixer_ctl_get_type(ctl);
	num_values = mixer_ctl_get_num_values(ctl);

	for (i = 0; i < num_values; i++)
	{
		if (mixer_ctl_set_value(ctl, i, value))
		{
			fprintf(stderr, "Error: invalid value\n");
			return;
		}
	}
}

void set_to_speaker(struct mixer *mixer)
{
	tinymix_set_value_byname(mixer,"Speaker", 1);
}

void set_to_loopback(struct mixer *mixer)
{
	tinymix_set_value_byname(mixer,"sunxi daudio loopback debug", 1);
}

int main(int argc, char **argv)
{
	struct mixer *mixer;
	struct mixer *mixer2;
	int card = 0;
	int card2 = 0;
	int device = 0;
	int device2 = 0;
	int sample_rate = 44100;
	int channels = 2;
	int bits = 16;

	if (script_fetch("sound", "codec_card", &card, 1)) {
		printf("mictester: can't fetch soundcard, use default value: %d\n", card);
		card = 0;
	}
	if (script_fetch("sound", "codec_dev", &device, 1)) {
		printf("mictester: can't fetch sounddevice, use default value: %d\n", device);
		device = 0;
	}

	if (script_fetch("sound", "i2s_card", &card2, 1)) {
		printf("mictester: can't fetch soundcard, use default value: %d\n", card);
		card2 = 5;
	}
	if (script_fetch("sound", "i2s_dev", &device2, 1)) {
		printf("mictester: can't fetch sounddevice, use default value: %d\n", device);
		device2 = 0;
	}

	printf("audiocodec card: %d, audiocodec device: %d\n", card, device);
	printf("i2s card: %d, i2s device: %d\n", card2, device2);

    if (argc < 2) {
        fprintf(stderr, "\n Input error! -> Usage: %s TX/RX/CHECK\n\n", argv[0]);
        return 1;
    }

	mixer = mixer_open(card);
	mixer2 = mixer_open(card2);

    if (strcmp(argv[1], "TX") == 0) {
		printf("\n\n I2S Loopback TX Playback Test start......\n\n");
		set_to_loopback(mixer2);
		play_sample(card2, device2, "/dragonboard/data/boot_start_test.wav", argc, argv);
    } else if (strcmp(argv[1], "RX") == 0) {
		printf("\n\n I2S Loopback RX Capture Test start......\n\n");
		set_to_loopback(mixer2);
		cap_sample(card2, device2, "/dragonboard/data/i2s_loop_rec.wav", channels, sample_rate, bits, argc, argv);
    } else if (strcmp(argv[1], "CHECK") == 0) {
		printf("\n\n I2S Loopback Wav Playback Check by AudioCodec SPK......\n\n");
		set_to_speaker(mixer);
		play_sample(card, device, "/dragonboard/data/i2s_loop_rec.wav", argc, argv);
    } else {
        printf("invalid test mode: '%s'\n", argv[1]);
        return -1;
    }

	mixer_close(mixer);

	return 0;
}

unsigned int play_sample(unsigned int card, unsigned int device, char path[256],
                            int argc, char ** argv)
{
	FILE *file;
	struct chunk_header chunk_header;
	struct chunk_fmt chunk_fmt;
	struct riff_wave_header riff_wave_header;
	struct pcm_config config;
	struct pcm *pcm;
	char *buffer;
	unsigned int size, read_sz;
	int num_read = 0;
	int more_chunks = 1;
	unsigned int data_sz = 0;

    /* play the test wav */
    file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Unable to open file\n");
        return 1;
    }

    fread(&riff_wave_header, sizeof(riff_wave_header), 1, file);
    if ((riff_wave_header.riff_id != ID_RIFF) ||
        (riff_wave_header.wave_id != ID_WAVE)) {
        fprintf(stderr, "Error: it is not a riff/wave file\n");
        fclose(file);
        return 1;
    }

    do {
        fread(&chunk_header, sizeof(chunk_header), 1, file);

        switch (chunk_header.id) {
        case ID_FMT:
            fread(&chunk_fmt, sizeof(chunk_fmt), 1, file);
            /* If the format header is larger, skip the rest */
            if (chunk_header.sz > sizeof(chunk_fmt))
                fseek(file, chunk_header.sz - sizeof(chunk_fmt), SEEK_CUR);
            break;
        case ID_DATA:
            /* Stop looking for chunks */
            more_chunks = 0;
            chunk_header.sz = le32toh(chunk_header.sz);
            break;
        default:
            /* Unknown chunk, skip bytes */
            fseek(file, chunk_header.sz, SEEK_CUR);
        }
    } while (more_chunks);

    memset(&config, 0, sizeof(config));
    config.channels = chunk_fmt.num_channels;
    config.rate = chunk_fmt.sample_rate;
    config.period_size = 1024;
    config.period_count = 4;
    config.format = PCM_FORMAT_S16_LE;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;

    pcm = pcm_open(card, device, PCM_OUT, &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        fprintf(stderr, "Unable to open PCM device %u (%s)\n",
                device, pcm_get_error(pcm));
		return 1;
    }

    size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    buffer = malloc(size);
    if (!buffer) {
        fprintf(stderr, "Unable to allocate %d bytes\n", size);
        free(buffer);
        pcm_close(pcm);
		return 1;
    }

    data_sz = chunk_header.sz;
    printf("Playing sample: %u ch, %u hz, %u bit %u bytes\n",
		chunk_fmt.num_channels, chunk_fmt.sample_rate, chunk_fmt.audio_format, data_sz);

    do {
        read_sz = size < data_sz ? size : data_sz;
        num_read = fread(buffer, 1, read_sz, file);
        if (num_read > 0) {
            if (pcm_write(pcm, buffer, num_read)) {
                fprintf(stderr, "Error playing sample\n");
                break;
            }
            data_sz -= num_read;
        }
    } while (num_read > 0 && data_sz > 0);

    free(buffer);
    pcm_close(pcm);
    fclose(file);

    return 0;
}

unsigned int cap_sample(unsigned int card, unsigned int device, char path[256], unsigned int channels,
								unsigned int rate, unsigned int bits, int argc, char ** argv)
{
	FILE *file;
	unsigned int frames;
	struct wav_header header;
	enum pcm_format format;
	struct pcm_config config;
	struct pcm *pcm;
	char *buffer;
	int test_wav_time = 10;
	unsigned int size;
	unsigned int bytes_read = 0;
	struct timeval tv;
	struct timezone tz;
	long time_sec1;
	long time_sec2;
	long time_total;

	header.riff_id = ID_RIFF;
	header.riff_sz = 0;
	header.riff_fmt = ID_WAVE;
	header.fmt_id = ID_FMT;
	header.fmt_sz = 16;
	header.audio_format = FORMAT_PCM;
	header.num_channels = channels;
	header.sample_rate = rate;
	header.bits_per_sample = bits;
	header.byte_rate = (header.bits_per_sample / 8) * channels * rate;
	header.block_align = channels * (header.bits_per_sample / 8);
	header.data_id = ID_DATA;

	file = fopen(path, "w");
	if (file == NULL){
		fprintf(stderr, "audio Unable to create file\n");
		return 0;
	}

    switch (bits) {
    case 32:
        format = PCM_FORMAT_S32_LE;
        break;
    case 24:
        format = PCM_FORMAT_S32_LE;
        break;
    case 16:
        format = PCM_FORMAT_S16_LE;
        break;
    default:
        fprintf(stderr, "%u bits is not supported.\n", bits);
        fclose(file);
        return 1;
    }

    /* leave enough room for header */
    fseek(file, sizeof(struct wav_header), SEEK_SET);

    memset(&config, 0, sizeof(config));
    config.channels = channels;
    config.rate = rate;
    config.period_size = 1024;
    config.period_count = 4;
    config.format = format;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;

    pcm = pcm_open(card, device, PCM_IN, &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        fprintf(stderr, "Unable to open PCM device (%s)\n",
                pcm_get_error(pcm));
        return 0;
    }

    size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    buffer = malloc(size);
    if (!buffer) {
        fprintf(stderr, "Unable to allocate %u bytes\n", size);
        free(buffer);
        pcm_close(pcm);
        return 0;
    }

    printf("Capturing sample: %u ch, %u hz, %u bit\n", channels, rate, bits);

	gettimeofday(&tv, &tz);
	time_sec1 = tv.tv_sec;

    while (capturing && !pcm_read(pcm, buffer, size)) {
        if (fwrite(buffer, 1, size, file) != size) {
            fprintf(stderr,"Error capturing sample\n");
            break;
        }
        bytes_read += size;
		gettimeofday(&tv, &tz);
		time_sec2 = tv.tv_sec;
		time_total = time_sec2 - time_sec1;
		if (time_total >= (test_wav_time)) {
			printf("time_total: %ld, capture_seconds:%d\n", time_total, test_wav_time);
			break;
		}
    }

    frames = pcm_bytes_to_frames(pcm, bytes_read);
    free(buffer);
    pcm_close(pcm);

    /* write header now all information is known */
    header.data_sz = frames * header.block_align;
    header.riff_sz = header.data_sz + sizeof(header) - 8;
    fseek(file, 0, SEEK_SET);
    fwrite(&header, sizeof(struct wav_header), 1, file);
    fclose(file);

    return 0;
}
