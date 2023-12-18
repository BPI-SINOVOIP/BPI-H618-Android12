#!/system/bin/sh

# sleep_exit [<exit_code>]
function sleep_exit() {
  # Sleep before exit to ensure logs flushes.
  sleep 5s
  exit $@
}

# log_gki <priority> <message>
function log_gki() {
  log -t gki -p $@
}

log_gki i "GKI APEX preinstall hook starting."

mydir=$(dirname $(dirname $(realpath "$0")))
if [[ $(dirname $mydir) != "/apex" ]] || [[ $(basename $mydir) != com.android.gki.* ]]; then
  log_gki e "This script must be installed under /apex/com.android.gki.*"
  sleep_exit 1
fi

# The pre-installed APEX does not contain any OTA payload. Just skip installing anything.
if [[ ! -f ${mydir}/etc/ota/payload.bin ]]; then
  log_gki e "No payload.bin found."
  sleep_exit 1
fi

log_gki i "Applying payload.";

if ! ${mydir}/bin/update_engine_stable_client \
      --payload ${mydir}/etc/ota/payload.bin \
      --headers "$(cat ${mydir}/etc/ota/payload_properties.txt)"; then
    log_gki e "OTA failed"
    sleep_exit 1
fi

log_gki i "OTA successful"
sleep_exit 0
