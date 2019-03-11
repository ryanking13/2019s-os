#!/usr/bin/python3

import getpass
import re
import subprocess as sp
import sys

def error(msg, **kwargs):
    print('\033[0;31m[-] {}\033[0m'.format(msg), **kwargs)


def info(msg, **kwargs):
    print('\033[1;33m[*] {}\033[0m'.format(msg), **kwargs)

if getpass.getuser() != 'root':
    error('please run with sudo')
    exit(1)

_exec = sp.check_output
if '-v' in sys.argv:
    _exec = sp.call


# build all files
info('Building...')
build_cmd = './build-rpi3-arm64.sh'
try:
    _exec(build_cmd)
except sp.CalledProcessError as e:
    error('{} failed with return code {}'.format(build_cmd, e.returncode))
    exit(1)

# make boot img
info('Generating boot image...')
gen_boot_img_cmd = './scripts/mkbootimg_rpi3.sh'
try:
    _exec(gen_boot_img_cmd)
except sp.CalledProcessError as e:
    error('{} failed with return code {}'.format(gen_boot_img_cmd, e.returncode))
    exit(1)

info('Packing boot image...')
pack_cmd = 'tar -zcvf tizen-unified_20181024.1_iot-boot-arm64-rpi3.tar.gz boot.img modules.img' 
_exec(pack_cmd.split())

# flash usb drive
info ('Flashing use drive...')
flash_cmd = './flash-sdcard.sh /dev/{}'

out = sp.check_output('dmesg')
msgs = out.split(b'\n')

for msg in msgs[::-1]:
    msg = msg.decode()
    if 'Attached SCSI removable disk' in msg:
        device_name = re.findall('\[[a-zA-Z0-9]+\]', msg)
        if not device_name:
            error('USB not connected')
            exit(1)
        device_name = device_name[0][1:-1]
        info('USB is recognized to /dev/{}'.format(device_name))
        flash_cmd = flash_cmd.format(device_name)
        break

try:
    p = sp.Popen('yes', stdout=sp.PIPE)
    _exec(flash_cmd.split(), stdin=p.stdout)
except sp.CalledProcessError as e:
    error('{} failed with return code {}'.format(flash_cmd, e.returncode))

info('Done')

if '-e' in sys.argv:
    info('Ejecting USB drive')
    _exec('eject {}'.format(device), shell=True)
