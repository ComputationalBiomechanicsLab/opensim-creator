#!/usr/bin/env bash

set -xeo pipefail

# if root is running this script then do not use `sudo` (some distros
# do not have 'sudo' available)
if [[ "${UID}" == 0 ]]; then
    sudo=''
else
    sudo='sudo'
fi

if [[ ! -z "${OSC_BUILD_DOCS}" ]]; then
   echo "----- installing docs dependencies -----"
   ${sudo} apt-get update
   ${sudo} apt-get install python3 python3-pip
   ${sudo} pip3 install -r docs/requirements.txt
fi
   
