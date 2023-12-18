TOPDIR=$(CURDIR)
SRCTREE=$(TOPDIR)

COPYTODIR = $(TOPDIR)/../../../../tools/pack/pctools/linux/mod_update

all:
	make -C create_mbr
	make -C update_boot0
	make -C update_checksum
	make -C update_fes1
	make -C update_signature
	make -C update_simg
	make -C update_uboot
	make -C parser_img
	make -C openssl_signature
	make -C script

	cp create_mbr/update_mbr 				$(COPYTODIR)/update_mbr
	cp update_boot0/update_boot0 			$(COPYTODIR)/update_boot0
	cp update_checksum/genchecksum_uboot 	$(COPYTODIR)/genchecksum_uboot
	cp update_fes1/update_fes1  			$(COPYTODIR)/update_fes1
	cp update_signature/signature 			$(COPYTODIR)/signature
	cp update_simg/simg 					$(COPYTODIR)/simg
	cp update_uboot/update_uboot 			$(COPYTODIR)/update_uboot
	cp parser_img/parser_img 			    $(COPYTODIR)/parser_img
	cp openssl_signature/sunxi_sign 		$(COPYTODIR)/sunxi_sign
	cp script/script 		$(COPYTODIR)/script


