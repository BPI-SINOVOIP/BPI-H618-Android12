
COMDIR=../common
INCDIR=../include
TOPDIR=$(CURDIR)/..
SRCTREE=$(TOPDIR)

COPYTODIR = $(TOPDIR)/../../../../tools/pack/pctools/linux/mod_update
UBOOT_SCRIPT_DIR = $(TOPDIR)/../../../../brandy-2.0/u-boot-2018/scripts

CFLAGS += -I$(INCDIR)

# Rename $1 to $2 only if file content differs. Otherwise just delete $1.
define mv-if-changed
	if [ -r $2 ] && cmp -s $2 $1; then			\
		rm -f $1;					\
	else							\
		$(CMD_ECHO_SILENT) '  UPD     $2';		\
		mv $1 $2;					\
	fi
endef

define update-commit-info
	$(COMDIR)/generate_hash_header_file.sh $(CURDIR)> \
		$(CURDIR)/commit_info.h.tmp
	$(call mv-if-changed,$(CURDIR)/commit_info.h.tmp,$(INCDIR)/commit_info.h)
endef

commit_version:
	@echo "update commit====>"
	$(call update-commit-info)
commit.o:commit_version
	echo "commit_info.o:commit_info.c commit_info.h"
	gcc $(CFLAGS) -c $(COMDIR)/commit.c