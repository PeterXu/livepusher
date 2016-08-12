#!/bin/bash
set -e

ARCH=${1:-arm}

mods="faac rtmpdump x264 openh264"
for mod in $mods; do
    echo && echo
    echo "[INFO] for $mod"
    pushd .
    cd include/$mod
    bash run_cfg.sh $ARCH
    popd
    echo
    sleep 2
done

exit 0
