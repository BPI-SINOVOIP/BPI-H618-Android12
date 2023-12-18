all:
	gcc -o extract_squashfs extract_squashfs.c
	gcc -o update_squashfs update_squashfs.c
	gcc -o update_squashfs_verity update_squashfs_verity.c
	@cp -v extract_squashfs ../../../../../tools/pack/pctools/linux/mod_update/extract_squashfs
	@cp -v update_squashfs ../../../../../tools/pack/pctools/linux/mod_update/update_squashfs
	@cp -v update_squashfs_verity ../../../../../tools/pack/pctools/linux/mod_update/update_squashfs_verity

clean:
	rm -rf extract_squashfs update_squashfs update_squashfs_verity
