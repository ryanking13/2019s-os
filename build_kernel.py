#!/usr/bin/python3

import argparse
import getpass
import re
from pathlib import Path
import subprocess as sp
import sys


def error(msg, **kwargs):
    print('\033[0;31m[-] {}\033[0m'.format(msg), **kwargs)


def info(msg, **kwargs):
    print('\033[1;33m[*] {}\033[0m'.format(msg), **kwargs)


def find_device_name():
    out = sp.check_output('dmesg')
    msgs = out.split(b'\n')
    for msg in msgs[::-1]:
        msg = msg.decode()
        if 'Attached SCSI removable disk' in msg:
            device_name = re.findall('\[[a-zA-Z0-9]+\]', msg)
            if not device_name:
                error('USB not connected')
                exit(1)
            device_name = '/dev/{}'.format(device_name[0][1:-1])
            info('USB is automatically recognized to {}'.format(device_name))
            break
    
    return device_name


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='print all messages while building kernel')
    parser.add_argument('-e', '--eject', action='store_true',
                        help='eject usb drive after build is complete')
    parser.add_argument('--no-flash', action='store_true',
                        help='do not flash kernel to USB, just build and exit')
    parser.add_argument('--device', default=None,
                        help='force device path (e.g. /dev/sdb), use this argument if device is not automatically detected')
    
    args = parser.parse_args()
    return args


def build():
    if getpass.getuser() != 'root':
        error('please run with sudo')
        exit(1)

    args = parse_args()
    _exec = sp.call if args.verbose else sp.check_output


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

    if args.no_flash:
        return
        
    # flash usb drive
    info ('Flashing use drive...')
    device_name = args.device or find_device_name()
    flash_cmd = './flash-sdcard.sh {}'.format(device_name)

    # Note: `mnt_tmp` is temporary mount path used while flashing
    #       this directory is removed after flashing, but sometimes remains in whatever reason
    #       if this directory exists while flashing, flash fails. Therefore, check if it exists and remove it.
    tmp_mount_path = Path('mnt_tmp')
    if tmp_mount_path.is_dir():
        info('mnt_tmp found, removing it...')
        for f in tmp_mount_path.glob('*'):
            f.unlink()
        tmp_mount_path.rmdir()

    try:
        p = sp.Popen('yes', stdout=sp.PIPE)
        _exec(flash_cmd.split(), stdin=p.stdout)
    except sp.CalledProcessError as e:
        error('{} failed with return code {}'.format(flash_cmd, e.returncode))

    info('Done flashing')

    if args.eject:
        info('Ejecting USB drive')
        _exec('eject {}'.format(device_name), shell=True)


if __name__ == '__main__':
    build()


