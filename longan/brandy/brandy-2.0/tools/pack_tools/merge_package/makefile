

objects = script.o merge_package.o commit.o


edit:$(objects)
	gcc -o merge_full_img $(objects) -static
	rm -rf $(objects)
	@if [ "x$(TARGET_BUILD_VARIANT)" = "xtina" ] ; then \
		cp -v merge_full_img ../../../../tools/pack-bintools/src/merge_full_img; \
	else \
		cp -v merge_full_img ../../../../../tools/pack/pctools/linux/mod_update/merge_full_img; \
	fi


clean:
	rm -rf merge_full_img $(objects)

include ../common/common.mk

