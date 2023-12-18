ARMGCC = /home/cuiyuntao/f133_tina/lichee/brandy-2.0/tools/toolchain/riscv64-linux-x86_64-20200528/bin/riscv64-unknown-linux-gnu-gcc

src = ./trans_info.c ./pq_main.c   ./dexxx/de_generic.c ./dexxx/de201_tans.c
flg =  -Wall -std=c99
all:$(src)
	$(ARMGCC) $(flg)  $(src) -lpthread -lm -o pqd
