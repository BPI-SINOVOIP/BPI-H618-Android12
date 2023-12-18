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

struct cert_header {
	char cert_magic[16];
	unsigned int cert_len;
	unsigned int hash_tree_len;
	char salt_str[80];
	char root_hash_str[80];
	int reserved[18];
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

static void usage(void)
{
	printf("usage:\n");
	printf("\tupdate_squashfs_verity -f <squashfs_img> -c <cert_file> -t <hash_tree> -s <salt_str> -r <root_hash_str>\n\n");
}

#define ALIGNED_LEN		4096
#define SQUASHFS_MAGIC		0x73717368

int main(int argc, char *argv[])
{
	struct squashfs_super_block rootfs_sb;
	struct cert_header ht_cert_header;
	struct stat buf;

	int ch = 0;

	int fd_squashfs = -1;
	int fd_cert = -1;
	int fd_hash_tree = -1;

	char *squashfs_name = NULL;
	char *cert_name = NULL;
	char *hash_tree_name = NULL;

	int read_len, write_len, cert_len, hash_tree_len;
	char *squashfs_header = NULL;
	char *cert_buf = NULL;
	char *hash_tree_buf = NULL;
	char *salt_str = NULL;
	char *root_hash_str = NULL;
	int rootfs_size = 0;
	int i = 0;
	int ret = 0;

	if (argc != 11) {
		usage();
		return -1;
	}

	while ((ch = getopt(argc, argv, "f:c:t:s:r:")) != -1) {
		switch (ch) {
		case 'f':
			squashfs_name = optarg;
			break;
		case 'c':
			cert_name = optarg;
			break;
		case 't':
			hash_tree_name = optarg;
			break;
		case 's':
			salt_str = optarg;
			break;
		case 'r':
			root_hash_str = optarg;
			break;
		case '?':
		default:
			printf("Unknown option: %c\n", (char)optopt);
			usage();
			return -1;
			break;
		}
	}

	memset(&ht_cert_header, 0, sizeof(struct cert_header));

	if ((salt_str == NULL) || (root_hash_str == NULL)
		|| (strlen(salt_str) != 64) || (strlen(root_hash_str) != 64)) {
		printf("The size of salt_str/root_hash_str should be 64B!\n");
		return -1;
	}

	memcpy(ht_cert_header.cert_magic, "squashfs_verity", 16);

	memcpy(ht_cert_header.salt_str, salt_str, 64);
	memcpy(ht_cert_header.root_hash_str, root_hash_str, 64);

	fd_squashfs = open(squashfs_name, O_RDWR);
	if (fd_squashfs == -1) {
		printf("Could not open squashfs file: %s\n", squashfs_name);
		return -1;
	}

	fd_cert = open(cert_name, O_RDONLY);
	if (fd_cert == -1) {
		printf("Could not open cert file: %s\n", cert_name);
		ret = -1;
		goto out;
	}

	fd_hash_tree = open(hash_tree_name, O_RDONLY);
	if (fd_hash_tree == -1) {
		printf("Could not open hash_tree file: %s\n", hash_tree_name);
		ret = -1;
		goto out;
	}

	if (lseek(fd_squashfs, 0, SEEK_SET) == -1) {
		ret = -1;
		goto out;
	}

	/* 1. check the squashfs magic */
	squashfs_header = (char *)malloc(sizeof(struct squashfs_super_block));
	if (squashfs_header == NULL) {
		printf("malloc squashfs_header faile\n");
		ret = -1;
		goto out;
	}

	read_len = read(fd_squashfs, squashfs_header, sizeof(struct squashfs_super_block));
	if (read_len == -1) {
		ret = -1;
		goto out;
	}

	// sunxi_dump(squashfs_header, sizeof(struct squashfs_super_block));

	memcpy(&rootfs_sb, squashfs_header, sizeof(struct squashfs_super_block));
	if (rootfs_sb.s_magic != SQUASHFS_MAGIC) {
		ret = -1;
		goto out;
	}

	/* 2. get squashfs size, squashfs archives are often padded to 4KiB. */
	rootfs_size = (rootfs_sb.bytes_used + ALIGNED_LEN - 1) & (~(ALIGNED_LEN - 1));


	/* 3. get the hash_tree_len and content. */
	hash_tree_len = lseek(fd_hash_tree, 0, SEEK_END);
	if (hash_tree_len == -1) {
		ret = -1;
		goto out;
	}

	hash_tree_buf = (char *)malloc(hash_tree_len);
	if (hash_tree_buf == NULL) {
		printf("malloc hash_tree_buf failed\n");
		ret = -1;
		goto out;
	}

	lseek(fd_hash_tree, 0, SEEK_SET);
	if (read(fd_hash_tree, hash_tree_buf, hash_tree_len) == -1) {
		ret = -1;
		goto out;
	}


	/* 4. get the cert_len and content. */
	ht_cert_header.hash_tree_len = hash_tree_len;
	ht_cert_header.cert_len = lseek(fd_cert, 0, SEEK_END);
	if (ht_cert_header.cert_len == -1) {
		ret = -1;
		goto out;
	}

#define SUNXI_X509_CERTIFF_MAX_LEN 4096
	if (ht_cert_header.cert_len > SUNXI_X509_CERTIFF_MAX_LEN) {
		printf("cert_len %d should not bigger than %d\n", ht_cert_header.cert_len, SUNXI_X509_CERTIFF_MAX_LEN);
		ret = -1;
		goto out;
	}

	cert_len = (SUNXI_X509_CERTIFF_MAX_LEN + sizeof(struct cert_header) + ALIGNED_LEN - 1) & (~(ALIGNED_LEN - 1));

	cert_buf = (char *)malloc(cert_len);
	if (cert_buf == NULL) {
		printf("malloc cert_buf failed\n");
		ret = -1;
		goto out;
	}

	memset(cert_buf, 0, cert_len);
	memcpy(cert_buf, &ht_cert_header, sizeof(struct cert_header));

	lseek(fd_cert, 0, SEEK_SET);
	if (read(fd_cert, cert_buf + sizeof(struct cert_header), ht_cert_header.cert_len) == -1) {
		ret = -1;
		goto out;
	}


	/* 5. to the end of squashfs archives */
	if (lseek(fd_squashfs, rootfs_size, SEEK_SET) == -1) {
		ret = -1;
		goto out;
	}


	/* 6. write the cert content */
	write_len = write(fd_squashfs, cert_buf, cert_len);
	if (write_len == -1) {
		ret = -1;
	}


	/* 7. write the hash_tree content */
	write_len = write(fd_squashfs, hash_tree_buf, hash_tree_len);
	if (write_len == -1) {
		ret = -1;
	}

out:
	if (fd_squashfs != -1)
		close(fd_squashfs);

	if (fd_cert != -1)
		close(fd_cert);

	if (fd_hash_tree != -1)
		close(fd_hash_tree);

	if (!squashfs_header)
		free(squashfs_header);

	if (!cert_buf)
		free(cert_buf);

	if (!hash_tree_buf)
		free(hash_tree_buf);

	return ret;
}

