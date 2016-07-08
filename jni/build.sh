#!/bin/bash
set -e


mods="faac rtmpdump x264"
for mod in $mods; do
    echo && echo
    echo "[INFO] for $mod"
    pushd .
    cd include/$mod
    bash run_cfg.sh
    popd
    echo
    sleep 2
done

exit 0
