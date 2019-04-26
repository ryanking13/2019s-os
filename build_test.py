#!/usr/bin/python3

import argparse
import getpass
import re
import subprocess as sp
import sys
from pathlib import Path
import uuid

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
    parser.add_argument('--no-copy', action='store_true',
                        help='do not move test binaries to USB, just build and exit')
    parser.add_argument('--device', default=None,
                        help='force device path (e.g. /dev/sdb), use this argument if device is not automatically detected')
    parser.add_argument('--qemu', action='store_true',
                        help='move test binaries to file system for qemu')

    args = parser.parse_args()
    return args


def build():
    if getpass.getuser() != 'root':
        error('please run with sudo')
        exit(1)

    args = parse_args()
    _exec = sp.call if args.verbose else sp.check_output

    info('Compiling all test codes')
    test_build_cmd = 'arm-linux-gnueabi-gcc -Iinclude test/{source}.c -o test/{source} -lpthread'
    p = Path('test')
    source_files = p.glob('*.c')
    for f in source_files:
        _exec(test_build_cmd.format(source=f.name.split('.')[0]), shell=True)

    if args.qemu:
        info('Copying test binaries to qemu file system')
        tmp_dir = '/tmp/{}'.format(str(uuid.uuid4()))
        p = Path(tmp_dir)
        p.mkdir()
        _exec('mount {device} {path}'.format(device='tizen-image/rootfs.img', path=tmp_dir), shell=True)

        try:
            # make file system writable
            _exec('sed -i \'s/defaults,noatime,ro/defaults,noatime/\' {path}/etc/fstab'.format(path=tmp_dir), shell=True)
            _exec('cp test/* {path}/root/'.format(path=tmp_dir), shell=True)
        except sp.CalledProcessError as e:
            error('copy failed with return code {}'.format(e.returncode))
        finally:
            _exec('umount {path}'.format(path=tmp_dir), shell=True)
            p.rmdir()

        info('Done copying')

    if args.no_copy:
        return

    device_name = args.device or find_device_name()

    info('Copying test binaries to usb')
    fs = device_name + '2' # 2 is the partition which file system exists
    tmp_dir = '/tmp/{}'.format(str(uuid.uuid4()))
    p = Path(tmp_dir)
    p.mkdir()
    _exec('mount {device} {path}'.format(device=fs, path=tmp_dir), shell=True)

    try:
        _exec('cp test/* {path}/root/'.format(path=tmp_dir), shell=True)
    except sp.CalledProcessError as e:
        error('copy failed with return code {}'.format(e.returncode))
    finally:
        _exec('umount {path}'.format(path=tmp_dir), shell=True)
        p.rmdir()

    info('Done copying')

    if args.eject:
        info('Ejecting USB drive')
        _exec('eject {}'.format(device_name), shell=True)


if __name__ == '__main__':
    build()