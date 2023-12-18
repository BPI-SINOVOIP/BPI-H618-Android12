#!/system/bin/sh

RUNIN_DIR=/data/local/tmp/.runin
RUNIN_LOG=$RUNIN_DIR/log
CURRENT_LOOP_FILE=$RUNIN_DIR/loop_index
CURRENT_CASE_FILE=$RUNIN_DIR/case_index
CURRENT_REBOOT_FILE=$RUNIN_DIR/reboot_time
RUNIN_PREFIX="__runin_"
DEFAULT_CONFIG_FILE=/system/etc/runin_config
CUSTOM_CONFIG_FILE=/sdcard/runin_config

function __runin_log()
{
    echo "$(date '+%m-%d %H:%M:%S') $@" | tee -a $RUNIN_LOG
}

function __runin_memtester_test()
{
    local pids
    __runin_log "memtester $@ x 4"
    memtester $1 $2 > /dev/null &
    pids+=("$!")
    memtester $1 $2 > /dev/null &
    pids+=("$!")
    memtester $1 $2 > /dev/null &
    pids+=("$!")
    memtester $1 $2 > /dev/null &
    pids+=("$!")
    wait ${pids[@]}
    __runin_log "memtester test done"
}

function __runin_sleepawake_test()
{
    __runin_log "sleepawake test $@"
    local i=0;
    while (true); do
        sleep $1
        __runin_log "going to sleep $i"
        # 30秒后唤醒
        echo +$2 > /sys/class/rtc/rtc0/wakealarm
        # 休眠
        echo mem > /sys/power/state
        __runin_log "wakeup $i"
        input keyevent SLEEP
        input keyevent WAKEUP
        wm dismiss-keyguard
        let i=i+1
        if [ $i -ge $3 ]; then
            break
        fi
    done
    __runin_log "sleepawake test done"
}

function __runin_reboot_test()
{
    local reboot_time=`cat $CURRENT_REBOOT_FILE`
    if [ -z $reboot_time ]; then
        __runin_log "reboot test $@"
        reboot_time=0
    fi
    let reboot_time=reboot_time+1
    echo $reboot_time > $CURRENT_REBOOT_FILE
    if [ $reboot_time -gt $1 ]; then
        rm $CURRENT_REBOOT_FILE
        __runin_log "reboot test done"
        return
    fi
    __runin_log "reboot $(($reboot_time-1))"
    reboot
    exit 0
}

function __led_init()
{
    echo PF3 1 > /sys/kernel/debug/sunxi_pinctrl/function
    echo PF3 3 > /sys/kernel/debug/sunxi_pinctrl/dlevel
    echo PF3 0 > /sys/kernel/debug/sunxi_pinctrl/pull
    echo PF5 1 > /sys/kernel/debug/sunxi_pinctrl/function
    echo PF5 3 > /sys/kernel/debug/sunxi_pinctrl/dlevel
    echo PF5 0 > /sys/kernel/debug/sunxi_pinctrl/pull
    echo PF3 1 > /sys/kernel/debug/sunxi_pinctrl/data
    echo PF5 1 > /sys/kernel/debug/sunxi_pinctrl/data
}

# initialize
#echo sb > /sys/power/wake_lock
mkdir -p $RUNIN_DIR
if [ x"$1" == x"restart" ]; then
    echo 0 > $CURRENT_LOOP_FILE
    echo 0 > $CURRENT_CASE_FILE
fi

loop=1
config=$DEFAULT_CONFIG_FILE
if [ -f $CUSTOM_CONFIG_FILE ]; then
    config=$CUSTOM_CONFIG_FILE
fi
while read line; do
    if [[ $line == "loop="* ]]; then
        eval $line
    else
        cases+=("${RUNIN_PREFIX}${line}")
    fi
done < $config

case_size=${#cases[@]}
loop_index=`cat $CURRENT_LOOP_FILE`
case_index=`cat $CURRENT_CASE_FILE`
if [ -z "$loop_index" ]; then
    exit 0
fi
settings put system screen_off_timeout 2147483647
input keyevent WAKEUP
wm dismiss-keyguard

while (true); do
    if [ $loop_index -ge $loop ]; then
        break
    fi
    if [ $case_index -eq 0 ]; then
        __runin_log "running loop $loop_index"
    fi
    while (true); do
        if [ $case_index -ge $case_size ]; then
            break
        fi
        testcase=${cases[$case_index]}
        __runin_log "execute $testcase"
        $testcase
        let case_index=case_index+1
        echo $case_index > $CURRENT_CASE_FILE
    done
    let loop_index=loop_index+1
    echo $loop_index > $CURRENT_LOOP_FILE
    case_index=0
    rm $CURRENT_CASE_FILE
done
rm $CURRENT_LOOP_FILE
__runin_log "runin test done"

#echo sb > /sys/power/wake_unlock

