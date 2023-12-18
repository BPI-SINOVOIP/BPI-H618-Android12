#!/bin/sh
##############################################################################
# \version     1.0.0
# \date        2014.04.11
# \author      MINI_NGUI<liaoyongming@allwinnertech.com>
# \Descriptions:
#			add test LED for homlet
##############################################################################
source send_cmd_pipe.sh
source script_parser.sh
source log.sh

LOG_TAG="syswittester"

echo "write test result to secure storage"
sst_demo -w dragonboard_test pass
sst_demo -r dragonboard_test
sst_demo -w set-active-boot-slot 1
sst_demo -r set-active-boot-slot
echo "switch to android system"
bootctl set-active-boot-slot  0






