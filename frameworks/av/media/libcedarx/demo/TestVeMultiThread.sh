#!/bin/sh

# use the demo of demoVdecoder,demoVencoder,demoVencVdec to do the testing

# test step
# 1. compile the demo of demoVdecoder,demoVencoder,demoVencVdec; and push bin file to system/bin:
#    adb push demoVdecoder /system/bin && adb push testEnc /system/bin && adb push testEncDec /system/bin
# 2. push test file to sdcard, such as:
#    adb push A002_480P_AVC_MPEG_2M_24F.avi /sdcard/ && adb push 720p.yuv /sdcard/
# 3. push sh file to sdcard:
#    adb push TestMultiThread.sh /sdcard/
# 4. execute sh :
#    adb shell
#    cd /sdcard/ && sh TestMultiThread.sh


# ps: you can config test param as follows



#function config
enable_multi_decoder=1
enable_multi_encoder=1
enable_multi_enc_dec=1
enable_multi_process=1

# decoder config
d_input=A002_480P_AVC_MPEG_2M_24F.avi
d_output=d_out.yuv
d_save_start_num=2
d_save_total_num=6
d_test_frame_num=100
d_thread_num=2

# encoder config
e_input=720p.yuv
e_output=e_out.264
e_insize=720
e_outsize=720
e_encoder_format=0
e_test_frame_num=100
e_thread_num=2

# encoder_decoder config
ed_input=720p.yuv
ed_output=ed_out.yuv
ed_insize=720
ed_outsize=720
ed_encoder_format=0
ed_save_start_num=2
ed_save_total_num=6
ed_test_frame_num=100
ed_thread_num=2


if [ x"1" == x"$enable_multi_decoder" ];then
	echo "\n*************************************"
	echo "***** test decoder multi thread *****"
	echo "demoVdecoder -i $d_input -o $d_output -ss $d_save_start_num -sn $d_save_total_num  -n $d_test_frame_num -decNum $d_thread_num \n"
	demoVdecoder -i $d_input -o $d_output -ss $d_save_start_num -sn $d_save_total_num  -n $d_test_frame_num -decNum $d_thread_num
else
    echo "\n\n&&&&& not test decoder multi thread &&&&&"
fi


if [ x"1" == x"$enable_multi_encoder" ];then
	echo "\n*************************************"
	echo "***** test encoder multi thread *****"
	echo "testEnc -i $e_input -o $e_output -s $e_insize -d $e_outsize  -f $e_encoder_format -n $e_test_frame_num -enc_num $e_thread_num \n"
	testEnc -i $e_input -o $e_output -s $e_insize -d $e_outsize  -f $e_encoder_format -n $e_test_frame_num -enc_num $e_thread_num
else
    echo "\n\n&&&&& not test encoder multi thread &&&&&"
fi

if [ x"1" == x"$enable_multi_enc_dec" ];then
	echo "\n*******************************************"
	echo "**** test encoder_decoder multi thread ****"
	echo "testEncDec -i $ed_input -o $ed_output -s $ed_insize -d $ed_outsize -ss $ed_save_start_num -sn $ed_save_total_num -codec_num $ed_thread_num -n $ed_test_frame_num -f $ed_encoder_format \n"
	testEncDec -i $ed_input -o $ed_output -s $ed_insize -d $ed_outsize -ss $ed_save_start_num -sn $ed_save_total_num -codec_num $ed_thread_num -n $ed_test_frame_num -f $ed_encoder_format
else
    echo "\n\n&&&&& not test encoder_decoder multi thread &&&&&"
fi

if [ x"1" == x"$enable_multi_process" ];then
	echo "\n*****************************************"
	echo "*** test encoder decoder multi process ***"
	echo "demoVdecoder -i $d_input -o $d_output -ss $d_save_start_num -sn $d_save_total_num  -n $d_test_frame_num -decNum $d_thread_num"
	echo "testEnc -i $e_input -o $e_output -s $e_insize -d $e_outsize  -f $e_encoder_format -n $e_test_frame_num -enc_num $e_thread_num \n"
	demoVdecoder -i $d_input -o $d_output -ss $d_save_start_num -sn $d_save_total_num  -n $d_test_frame_num -decNum $d_thread_num&testEnc -i $e_input -o $e_output -s $e_insize -d $e_outsize  -f $e_encoder_format -n $e_test_frame_num -enc_num $e_thread_num
else
    echo "\n\n&&&&& not test encoder decoder multi process &&&&&"
fi
