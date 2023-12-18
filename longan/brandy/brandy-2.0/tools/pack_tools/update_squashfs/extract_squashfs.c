#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

typedef long long		squashfs_inode;
struct squashfs_super_block {
	unsigned int		s_magic;
	unsigned int		inodes;
	int			mkfs_time /* time of filesystem creation */;
	unsigned int		block_size;
	unsigned int		fragments;
	unsigned short		compression;
	unsigned short		block_log;
	unsigned short		flags;
	unsigned short		no_ids;
	unsigned short		s_major;
	unsigned short		s_minor;
	squashfs_inode		root_inode;
	long long		bytes_used;
	long long		id_table_start;
	long long		xattr_id_table_start;
	long long		inode_table_start;
	long long		directory_table_start;
	long long		fragment_table_start;
	long long		lookup_table_start;
};

#define ALIGNED_LEN		4096
#define SQUASHFS_MAGIC		0x73717368

int main(int argc, char *argv[])
{
	struct squashfs_super_block rootfs_sb;
	struct stat buf;
	int fd_in;
	int fd_out;
	int read_len, write_len;
	int extract_per_MB = 0;
	char *extract_buf = NULL;
	int rootfs_size = 0;
	int rootfs_size_m = 0;
	int i = 0;
	int ret = 0;
	int full_flag = 0;

	if (argc != 4) {
		printf("Usage: extract_squashfs <extract_per_MB> <squashfs img> <destination file>\n");
		return -1;
	}

	if (strcmp(argv[1], "full") == 0) {
		full_flag = 1;
	} else {
		extract_per_MB = atoi(argv[1]);
		if (extract_per_MB < 4096 || extract_per_MB > 1048576) {
			printf("extract_per_MB should bigger then 4096 and less than 1048576\n");
			return -1;
		} else if (extract_per_MB % 4096 != 0) {
			printf("extract_per_MB should be 4K aligned\n");
			return -1;
		}
	}

	fd_in = open(argv[2], O_RDONLY);
	if (fd_in == -1) {
		printf("Could not open source file\n");
		return -1;
	}

	fd_out = open(argv[3], O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd_out == -1) {
		printf("Could not create destination file\n");
		ret = -1;
		goto out;
	}

	if (lseek(fd_in, 0, SEEK_SET) == -1) {
		ret = -1;
		goto out1;
	}

	read_len = read(fd_in, &rootfs_sb, sizeof(struct squashfs_super_block));
	if (read_len == -1) {
		ret = -1;
		goto out1;
	}

	/* check the squashfs magic */
//	memcpy(&rootfs_sb, extract_buf, sizeof(struct squashfs_super_block));
	if (rootfs_sb.s_magic != SQUASHFS_MAGIC) {
		ret = -1;
		goto out1;
	}

	rootfs_size = (rootfs_sb.bytes_used + ALIGNED_LEN - 1) & (~(0x1000 - 1));
	rootfs_size_m = rootfs_size >> 20;
	if ((rootfs_size < 1024 * 1024) && (full_flag == 0)) {
		printf("file: %s is less than 1MB, please set xxx_per_MB to 'full'\n", argv[2]);
		ret = -1;
		goto out1;
	}

	if (full_flag == 1) {
		extract_buf = (char *)malloc(rootfs_size);
		if (extract_buf == NULL) {
			printf("malloc extract_buf faile\n");
			return -1;
		}

		if (lseek(fd_in, 0, SEEK_SET) == -1) {
			ret = -1;
			goto out1;
		}

		read_len = read(fd_in, extract_buf, rootfs_size);
		if (read_len == -1) {
			ret = -1;
			goto out1;
		}

		if (lseek(fd_out, 0, SEEK_SET) == -1) {
			ret = -1;
			goto out1;
		}

		write_len = write(fd_out, extract_buf, rootfs_size);
		if (write_len == -1) {
			ret = -1;
			goto out1;
		}
	} else {
		extract_buf = (char *)malloc(extract_per_MB);
		if (extract_buf == NULL) {
			printf("malloc extract_buf faile\n");
			return -1;
		}

		for (i = 0; i < rootfs_size_m; i++) {
			if (lseek(fd_in, i * 1024 * 1024, SEEK_SET) == -1) {
				ret = -1;
				goto out1;
			}

			read_len = read(fd_in, extract_buf, extract_per_MB);
			if (read_len == -1) {
				ret = -1;
				goto out1;
			}

			if (lseek(fd_out, i * extract_per_MB, SEEK_SET) == -1) {
				ret = -1;
				goto out1;
			}

			write_len = write(fd_out, extract_buf, extract_per_MB);
			if (write_len == -1) {
				ret = -1;
				goto out1;
			}
		}
	}

out1:
	close(fd_in);
out:
	close(fd_out);
	free(extract_buf);

	return ret;
}

