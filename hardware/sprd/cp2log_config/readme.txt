新增unisoc_cp2log_config.txt配置文件，文件内容如下：
wcn_cp2_log_limit_size=20M;
wcn_cp2_file_max_num=3;
wcn_cp2_file_cover_old=true;
wcn_cp2_log_path="/data/unisoc_dbg";

若使用配置文件，请将其push到系统的/data路径下，文件权限修改为666或777。

wcn_cp2_log_limit_size为每个log文件最大尺寸，默认为20M，同code中的默认值，
当某个文件达到最大尺寸（文件最终大小可能在wcn_cp2_log_limit_size上下浮动1KB），
则后边log需要存入到其他文件；

wcn_cp2_file_max_num为存储的log文件最大个数，此项在wcn_cp2_file_cover_old为true时生效，默认值为3；

wcn_cp2_file_cover_old代表是否循环使用已经存在的log文件记录log，默认值为true，同code中的默认值。
当wcn_cp2_file_cover_old为true时，log文件循环的在wcn_cp2_file_max_num个数的log文件中存储，
假设wcn_cp2_file_max_num为3的话，当2号文件（unisoc_cp2log_2.txt）文件达到wcn_cp2_log_limit_size后，
之后的log将记录于0号文件（unisoc_cp2log_0.txt），当0号文件达到最大size后，log将被记录于1号文件，
如此循环记录。每次开机后，如果文件路径下的log文件个数小于wcn_cp2_file_max_num，则新建log文件记录log，
当文件个数已经达到最大值，则log将存储于0~wcn_cp2_file_max_num-1号文件中第一个小于wcn_cp2_log_limit_size的文件。
若修改配置文件存储路径到u盘、sd卡等外接设备，可以依据设备剩余内存合理设置文件个数及文件大小。
可以通过如下规则确定最后一次存储的log：当wifi开启后使用的log文件可以显示正确的时间（否则文件ls –al显示的时间可能为出厂时间），
可以根据时间确定；不然的话若文件个数小于wcn_cp2_file_max_num则最后一个文件为最后的log；
若文件个数等于wcn_cp2_file_max_num，由于文件内容在所有文件中循环存储，文件尺寸小于wcn_cp2_log_limit_size的那个文件为最后的log；
同时bsp driver的log中每次切换文件都会有打印，如：“WCN: log_rx_callback cp2 log file is /data/unisoc_dbg/unisoc_cp2log_2.txt”。

wcn_cp2_file_cover_old为false的话log文件个数不受限制，每次开机后的log文件从0号文件开始存储，若0号文件已经存在，
先将其清空，每当文件size达到wcn_cp2_log_limit_size后会新建log文件存储后来的log，直到log文件路径内存被全部用完。
因为每次开机后上次的0号文件会清空，请注意log的保存！

wcn_cp2_log_path为默认log存储位置，默认位置为"/data/unisoc_dbg"，同code中默认位置相同，可通过修改该项来调整log存储路径，
若新设置的文件路径不存在则默认还在"/data/unisoc_dbg"中存储，新设置的文件路径长度请不要超过100！
当然老的切换log存储路径的at指令依然可以用：echo "logpath=/xxx\r" > /proc/mdbg/at_cmd
增加配置文件后开机第一次可能会提示："new path [/data/unisoc_dbg] is invalid”这个是正常情况，
下一步的初始化过程会新建路径/data/unisoc_dbg。

假设config文件中某条设置不需要可以直接删除，其他设置内容依然生效，建议修改config文件后将原来已经存在的log文件删除掉。
若文件系统中没有config文件，配置文件中的四项设置依据code中的默认值。

若code中CONFIG_CPLOG_DEBUG关闭，需要看cp2 log的时候请在芯片上电后执行echo "at+armlog=1" > /proc/mdbg/at_cmd来打开cp侧log上报功能。
