/*
 * Copyright (c) 2012 Allwinner Technology. All Rights Reserved.
 *
 * file:		audiocodec_test.c
 * version:		1.0.0
 * date:		2022/02/09
 * author:		AW1638
 * descriptions:
 *		add the inital version for audiocodec module's dragonboard test of sun50iw12p1 platform.
 *
 * func:
 *      1. LINEOUT + Speaker / HPOUT + Headphone playback func test;
 *      2. LINEIN1/LINEIN2 capture func test;
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

unsigned int cap_and_play_sample(unsigned int card, unsigned int device, unsigned int channels,
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

void set_to_headphone(struct mixer *mixer)
{
	tinymix_set_value_byname(mixer,"Headphone", 1);
}

void set_to_speaker(struct mixer *mixer)
{
	tinymix_set_value_byname(mixer,"Speaker", 1);
}

void set_to_linein1(struct mixer *mixer)
{
	tinymix_set_value_byname(mixer,"LINEINL", 2);
	tinymix_set_value_byname(mixer,"LINEINR", 2);
}

void set_to_linein2(struct mixer *mixer)
{
	tinymix_set_value_byname(mixer,"LINEINL", 1);
	tinymix_set_value_byname(mixer,"LINEINR", 1);
}

void close_to_default(struct mixer *mixer)
{
	tinymix_set_value_byname(mixer,"Headphone", 0);
	tinymix_set_value_byname(mixer,"Speaker", 0);
	tinymix_set_value_byname(mixer,"LINEINL", 0);
	tinymix_set_value_byname(mixer,"LINEINR", 0);
}

int main(int argc, char **argv)
{
	struct mixer *mixer;
	int card = 0;
	int device = 0;
	int sample_rate = 48000;
	int channels = 2;
	int bits = 16;

	if (script_fetch("sound", "codec_card", &card, 1)) {
		printf("mictester: can't fetch soundcard, use default value: %d\n", card);
	}
	if (script_fetch("sound", "codec_dev", &device, 1)) {
		printf("mictester: can't fetch sounddevice, use default value: %d\n", device);
	}

	printf("soundcard: card: %d, device: %d\n", card, device);

    if (argc < 2) {
        fprintf(stderr, "\n Input error! -> Usage: %s SPK/HP/LINEIN1/LINEIN2\n\n", argv[0]);
        return 1;
    }

	mixer = mixer_open(card);
	close_to_default(mixer);
    if (strcmp(argv[1], "SPK") == 0) {
		printf("\n\n audiocodec SPK Playback Test start......\n\n");
		set_to_speaker(mixer);
		play_sample(card, device, "/dragonboard/data/boot_start_test.wav", argc, argv);
    } else if (strcmp(argv[1], "HP") == 0) {
		printf("\n\n AudioCodec HP Playback Test start......\n\n");
		set_to_headphone(mixer);
		play_sample(card, device, "/dragonboard/data/boot_start_test.wav", argc, argv);
    } else if (strcmp(argv[1], "LINEIN1") == 0) {
		printf("\n\n AudioCodec Linein1 Capture Test start......\n\n");
		set_to_linein1(mixer);
		set_to_speaker(mixer);
		cap_and_play_sample(card, device, channels, sample_rate, bits, argc, argv);
    } else if (strcmp(argv[1], "LINEIN2") == 0) {
		printf("\n\n AudioCodec Linein2 Capture Test start......\n\n");
		set_to_linein2(mixer);
		set_to_speaker(mixer);
		cap_and_play_sample(card, device, channels, sample_rate, bits, argc, argv);
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

unsigned int cap_and_play_sample(unsigned int card, unsigned int device, unsigned int channels,
								unsigned int rate, unsigned int bits, int argc, char ** argv)
{
	FILE *file;
	unsigned int frames;
	struct wav_header header;
	struct chunk_header chunk_header;
	struct chunk_fmt chunk_fmt;
	struct riff_wave_header riff_wave_header;
	enum pcm_format format;
	struct pcm_config config;
	struct pcm *pcm;
	char *buffer;
	int test_wav_time = 0;
	unsigned int size, read_sz;
	int num_read = 0;
	int more_chunks = 1;
	unsigned int bytes_read = 0;
	unsigned int data_sz = 0;
	struct timeval tv;
	struct timezone tz;
	long time_sec1;
	long time_sec2;
	long time_total;

	play_sample(card, device, "/dragonboard/data/cap_test_start.wav", argc, argv);

	if (script_fetch("soundcard", "test_wav_time", &test_wav_time, 1)) {
		test_wav_time = 5;
	}

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

	file = fopen(AUDIO_TEST_WAVE, "w");
	if(file==NULL){
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

    play_sample(card, device, "/dragonboard/data/play_test_start.wav", argc, argv);

    /* play the capture wav */
    file = fopen(AUDIO_TEST_WAVE, "rb");
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

    pcm = pcm_open(card, device, PCM_OUT, &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        fprintf(stderr, "Unable to open PCM device %u (%s)\n",
                device, pcm_get_error(pcm));
//        return;
    }

    size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    buffer = malloc(size);
    if (!buffer) {
        fprintf(stderr, "Unable to allocate %d bytes\n", size);
        free(buffer);
        pcm_close(pcm);
//        return;
    }

    data_sz = chunk_header.sz;
    printf("Playing sample: %u ch, %u hz, %u bit %u bytes\n", channels, rate, bits, data_sz);

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
