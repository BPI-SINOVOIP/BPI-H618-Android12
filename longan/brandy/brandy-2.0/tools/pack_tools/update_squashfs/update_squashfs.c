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

void sunxi_dump(char *buf, int ttl_len)
{
	int len;
	for (len = 0; len < ttl_len; len++) {
		printf("0x%02x ", ((char *)buf)[len]);
		if (len % 8 == 7) {
			printf("\n");
		}
	}
	printf("\n");
}

#define ALIGNED_LEN		4096
#define SQUASHFS_MAGIC		0x73717368

int main(int argc, char *argv[])
{
	struct squashfs_super_block rootfs_sb;
	struct stat buf;
	int fd_in;
	int fd_cert;
	int read_len, write_len, cert_len;
	char *squashfs_header = NULL;
	char *cert_buf = NULL;
	int rootfs_size = 0;
	int i = 0;
	int ret = 0;

	if (argc != 3) {
		printf("Usage: update_squashfs <squashfs_file> <cert_file>\n");
		return -1;
	}

	fd_in = open(argv[1], O_RDWR);
	if (fd_in == -1) {
		printf("Could not open source file\n");
		return -1;
	}

	fd_cert = open(argv[2], O_RDONLY);
	if (fd_cert == -1) {
		printf("Could not open cert file\n");
		ret = -1;
		goto out;
	}

	if (lseek(fd_in, 0, SEEK_SET) == -1) {
		ret = -1;
		goto out1;
	}

	/* 1. check the squashfs magic */
	squashfs_header = (char *)malloc(sizeof(struct squashfs_super_block));
	if (squashfs_header == NULL) {
		printf("malloc squashfs_header faile\n");
		ret = -1;
		goto out1;
	}

	read_len = read(fd_in, squashfs_header, sizeof(struct squashfs_super_block));
	if (read_len == -1) {
		ret = -1;
		goto out1;
	}

	// sunxi_dump(squashfs_header, sizeof(struct squashfs_super_block));

	memcpy(&rootfs_sb, squashfs_header, sizeof(struct squashfs_super_block));
	if (rootfs_sb.s_magic != SQUASHFS_MAGIC) {
		ret = -1;
		goto out1;
	}

	/* 2. get squashfs size, squashfs archives are often padded to 4KiB. */
	rootfs_size = (rootfs_sb.bytes_used + ALIGNED_LEN - 1) & (~(0x1000 - 1));

	/* 3. get the cert_len and content. */
	cert_len = lseek(fd_cert, 0, SEEK_END);
	if (cert_len == -1) {
		ret = -1;
		goto out1;
	}

	cert_buf = (char *)malloc(cert_len);
	if (cert_buf == NULL) {
		printf("malloc cert_buf failed\n");
		ret = -1;
		goto out1;
	}

	lseek(fd_cert, 0, SEEK_SET);
	if (read(fd_cert, cert_buf, cert_len) == -1) {
		ret = -1;
		goto out1;
	}

	/* 4. to the end of squashfs archives */
	if (lseek(fd_in, rootfs_size, SEEK_SET) == -1) {
		ret = -1;
		goto out1;
	}

	/* 5. write the cert length 4Bytes */
	if (write(fd_in, &cert_len, 4) == -1) {
		ret = -1;
		goto out1;
	}

	/* 6. write the cert content */
	write_len = write(fd_in, cert_buf, cert_len);
	if (write_len == -1) {
		ret = -1;
	}

out1:
	close(fd_in);
out:
	close(fd_cert);

	if (!squashfs_header)
		free(squashfs_header);

	if (!cert_buf)
		free(cert_buf);

	return ret;
}

