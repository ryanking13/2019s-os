#!/usr/bin/python3

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

if getpass.getuser() != 'root':
    error('please run with sudo')
    exit(1)

_exec = sp.check_output
if '-v' in sys.argv:
    _exec = sp.call

info('Compiling all test codes')
test_build_cmd = 'arm-linux-gnueabi-gcc -Iinclude test/{source}.c -o test/{source}'
p = Path('test')
source_files = p.glob('*.c')
for f in source_files:
    _exec(test_build_cmd.format(source=f.name.split('.')[0]), shell=True)


out = sp.check_output('dmesg')
msgs = out.split(b'\n')

device = '/dev/{}'
for msg in msgs[::-1]:
    msg = msg.decode()
    if 'Attached SCSI removable disk' in msg:
        device_name = re.findall('\[[a-zA-Z0-9]+\]', msg)
        if not device_name:
            error('USB not connected')
            exit(1)
        device_name = device_name[0][1:-1]
        info('USB is recognized to /dev/{}'.format(device_name))
        device = device.format(device_name)
        break

info('Copying test binaries to usb')
fs = device + '2' # 2 is the partition which file system exists
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

info('Done')

if '-e' in sys.argv:
    info('Ejecting USB drive')
    _exec('eject {}'.format(device), shell=True)
