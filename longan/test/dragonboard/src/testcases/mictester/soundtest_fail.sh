#!/bin/sh

##############################################################################
# \version     1.0.0
# \date        2022年05月26日
##############################################################################

source send_cmd_pipe.sh
source script_parser.sh

echo "SoundTest is Failed!"
echo "$1 1" >> /tmp/cmd_pipe
