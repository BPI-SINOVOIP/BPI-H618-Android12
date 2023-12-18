#!/bin/sh

##############################################################################
# \version     1.0.0
# \date        2022年05月26日
# \author      wangs <wangs@allwinnertech.com>
# \Descriptions:
#			人工确认重力传感器正常时按一下屏幕上确认按钮，程序会执行一次
##############################################################################

source send_cmd_pipe.sh
source script_parser.sh

echo "$3 $4" >> /tmp/cmd_pipe
