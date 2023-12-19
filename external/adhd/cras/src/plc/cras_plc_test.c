/* Copyright 2019 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "cras_sbc_codec.h"
#include "cras_plc.h"

#define MSBC_CODE_SIZE 240
#define MSBC_PKT_FRAME_LEN 57
#define RND_SEED 7

static const uint8_t msbc_zero_frame[] = {
	0xad, 0x00, 0x00, 0xc5, 0x00, 0x00, 0x00, 0x00, 0x77, 0x6d, 0xb6, 0xdd,
	0xdb, 0x6d, 0xb7, 0x76, 0xdb, 0x6d, 0xdd, 0xb6, 0xdb, 0x77, 0x6d, 0xb6,
	0xdd, 0xdb, 0x6d, 0xb7, 0x76, 0xdb, 0x6d, 0xdd, 0xb6, 0xdb, 0x77, 0x6d,
	0xb6, 0xdd, 0xdb, 0x6d, 0xb7, 0x76, 0xdb, 0x6d, 0xdd, 0xb6, 0xdb, 0x77,
	0x6d, 0xb6, 0xdd, 0xdb, 0x6d, 0xb7, 0x76, 0xdb, 0x6c
};

bool *generate_pl_seq(int input_file_size, float pl_percent)
{
	unsigned pk_count, pl_count;
	bool *seq;

	pk_count = input_file_size / MSBC_CODE_SIZE;
	pl_count = pk_count * (pl_percent / 100.0);
	seq = (bool *)calloc(pk_count, sizeof(*seq));
	srand(RND_SEED);
	while (pl_count > 0) {
		bool *missed = &seq[rand() % pk_count];
		if (!*missed) {
			*missed = true;
			pl_count--;
		}
	}
	return seq;
}

/* pl_hex is expected to be consecutive bytes(two chars) in hex format.*/
bool *parse_pl_hex(int input_file_size, const char *pl_hex)
{
	char tmp[3];
	uint8_t val = 0;
	int i, pl_hex_len, seq_len;
	bool *seq;

	pl_hex_len = strlen(pl_hex);
	seq_len = MAX(1 + input_file_size / MSBC_CODE_SIZE, pl_hex_len * 4);
	seq = (bool *)calloc(seq_len, sizeof(*seq));

	for (i = 0; i < seq_len; i++) {
		/* If sequence is longer then the provided pl_hex, leave the
		 * rest to all zeros. */
		if (i > pl_hex_len * 4)
			break;
		if (i % 8 == 0) {
			memcpy(tmp, pl_hex + i / 4, 2);
			tmp[2] = '\0';
			val = strtol(tmp, NULL, 16);
		}
		seq[i] = val & 1U;
		val >>= 1;
	}
	printf("pl_hex string maps to %ld ms, total sequence size %f ms\n",
	       strlen(pl_hex) * 30, seq_len * 7.5f);
	return seq;
}

void plc_experiment(const char *input_filename, bool *pl_seq, bool with_plc)
{
	char output_filename[255];
	int input_fd, output_fd, rc;
	struct cras_audio_codec *msbc_input = cras_msbc_codec_create();
	struct cras_audio_codec *msbc_output = cras_msbc_codec_create();
	struct cras_msbc_plc *plc = cras_msbc_plc_create();
	uint8_t buffer[MSBC_CODE_SIZE], packet_buffer[MSBC_PKT_FRAME_LEN];
	size_t encoded, decoded;
	unsigned count = 0;

	input_fd = open(input_filename, O_RDONLY);
	if (input_fd == -1) {
		fprintf(stderr, "Cannout open input file %s\n", input_filename);
		return;
	}

	if (with_plc)
		sprintf(output_filename, "output_with_plc.raw");
	else
		sprintf(output_filename, "output_with_zero.raw");

	output_fd = open(output_filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
	if (output_fd == -1) {
		fprintf(stderr, "Cannot open output file %s\n",
			output_filename);
		return;
	}

	while (1) {
		rc = read(input_fd, buffer, MSBC_CODE_SIZE);
		if (rc < 0) {
			fprintf(stderr, "Cannot read file %s", input_filename);
			return;
		} else if (rc == 0 || rc < MSBC_CODE_SIZE)
			break;

		msbc_input->encode(msbc_input, buffer, MSBC_CODE_SIZE,
				   packet_buffer, MSBC_PKT_FRAME_LEN, &encoded);

		if (pl_seq[count]) {
			if (with_plc) {
				cras_msbc_plc_handle_bad_frames(
					plc, msbc_output, buffer);
				decoded = MSBC_CODE_SIZE;
			} else
				msbc_output->decode(msbc_output,
						    msbc_zero_frame,
						    MSBC_PKT_FRAME_LEN, buffer,
						    MSBC_CODE_SIZE, &decoded);
		} else {
			msbc_output->decode(msbc_output, packet_buffer,
					    MSBC_PKT_FRAME_LEN, buffer,
					    MSBC_CODE_SIZE, &decoded);
			cras_msbc_plc_handle_good_frames(plc, buffer, buffer);
		}

		count++;
		rc = write(output_fd, buffer, decoded);
		if (rc < 0) {
			fprintf(stderr, "Cannot write file %s\n",
				output_filename);
			return;
		}
	}
}

static void show_usage()
{
	printf("This test only supports reading/writing raw audio with format:\n"
	       "\t16000 sample rate, mono channel, S16_LE\n");
	printf("--help - Print this usage.\n");
	printf("--input_file - path to an audio file.\n");
	printf("--pattern - Hex string representing consecutive packets'"
	       "status.\n");
	printf("--random - Percentage of packet loss.\n");
}

int main(int argc, char **argv)
{
	int fd;
	struct stat st;
	float pl_percent;
	int pl_percent_set = 0;
	int option_character;
	int option_index = 0;
	const char *input_file = NULL;
	const char *pl_hex = NULL;
	bool *pl_seq = NULL;
	static struct option long_options[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "input", required_argument, NULL, 'i' },
		{ "pattern", required_argument, NULL, 'p' },
		{ "random", required_argument, NULL, 'r' },
		{ NULL, 0, NULL, 0 },
	};

	while (true) {
		option_character = getopt_long(argc, argv, "i:r:p:h",
					       long_options, &option_index);
		if (option_character == -1)
			break;
		switch (option_character) {
		case 'h':
			show_usage();
			break;
		case 'i':
			input_file = optarg;
			break;
		case 'p':
			pl_hex = optarg;
			break;
		case 'r':
			pl_percent = atof(optarg);
			pl_percent_set = 1;
			break;
		default:
			break;
		}
	}

	if ((!pl_percent_set && !pl_hex) || !input_file) {
		show_usage();
		return 1;
	}

	fd = open(input_file, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Cannout open input file %s\n", input_file);
		return 1;
	}
	fstat(fd, &st);
	close(fd);
	if (pl_percent_set)
		pl_seq = generate_pl_seq(st.st_size, pl_percent);
	else if (pl_hex)
		pl_seq = parse_pl_hex(st.st_size, pl_hex);

	plc_experiment(input_file, pl_seq, true);
	plc_experiment(input_file, pl_seq, false);
}
