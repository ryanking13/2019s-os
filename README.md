# OS project 4

> 김도윤(ddoyoon) / 최경재(ryanking13) / 최원석(MKRoughDiamond)

## 1. How to build the kernel

기존 방식과 동일하게 빌드

_or_

> _Time is gold, save your time :)_

파이썬 스크립트를 사용하여 빌드 (_python3 is needed_)

```sh
sudo ./build_kernel.py
sudo ./build_test.py
```

__for QEMU__

```sh
sudo ./build_kernel.py --no-flash --qemu
sudo ./build_test.py --no-copy --qemu
```

<details><summary>스크립트 설명</summary>
<p>

### build_kernel.py

```sh
# build kernel / generate boot image / flash SD card

usage: build_kernel.py [-h] [-v] [-e] [--no-flash] [--device DEVICE]

optional arguments:
  -h, --help       show this help message and exit
  -v, --verbose    print all messages while building kernel
  -e, --eject      eject usb drive after build is complete
  --no-flash       do not flash kernel to USB, just build and exit
  --device DEVICE  force device path (e.g. /dev/sdb), use this argument if
                   device is not automatically detected
  --qemu           generate boot images for qemu

# Example

sudo ./build_kernel.py             # build & flash
sudo ./build_kernel.py --no-flash  # build only
```

### bulid_test.py

```sh
# build test files(test/*.c) / upload it to SD card
usage: build_test.py [-h] [-v] [-e] [--no-copy] [--device DEVICE]

optional arguments:
  -h, --help       show this help message and exit
  -v, --verbose    print all messages while building kernel
  -e, --eject      eject usb drive after build is complete
  --no-copy        do not move test binaries to USB, just build and exit
  --device DEVICE  force device path (e.g. /dev/sdb), use this argument if
                   device is not automatically detected
  --qemu           move test binaries to file system for qemu

# Example

sudo ./build_test.py            # build & upload
sudo ./build_test.py --no-c강opy  # build only
sudo ./build_test.py --eject    # eject sdcard after upload
```

</p>
</details>

## 2. Implementation

### 2.1. Files modified

- _Syscall 등록과 관련한 파일 수정 내용은 Project 1, 2, 3과 거의 동일하므로 생략함_

- [fs/ext2/ext2.h](./fs/ext2/ext2.h)
  - `ext2_inode`, `ext2_inode_info`에 GPS location 관련 속성 추가

- [include/linux/fs.h](./include/linux/fs.h)
  - `inode_permission` 함수에 사용될 마스크인 `MAY_GET_LOCATION` 매크로 추가
  - `inode_operations` 구조체에 `set_gps_location`, `get_gps_location` 속성 추가

- [fs/ext2/file.c](./fs/ext2/file.c)
  - `ext2_file_inode_operations`에 `set_gps_location`, `get_gps_location`, `permission` 속성 함수 포인터 추가
  - `ext2_set_gps_location`, `ext2_get_gps_location`, `ext2_permission` 구현

- [fs/ext2/inode.c](./fs/ext2/inode.c)
  - `ext2_iget` 함수에서 `ext2_inode` 인스턴스의 GPS 관련 속성을 `ext2_inode_info` 인스턴스로 가져오는 부분 추가
  - `__ext2_write_inode` 함수에서 `ext2_inode_info` 인스턴스의 GPS 관련 속성을 `ext2_inode` 인스턴스에 쓰는 부분 추가

- [fs/ext2/namei.c](./fs/ext2/namei.c)
  - `ext2_create`에서 파일이 새롭게 생성되었을 때에 `set_gps_location`이 호출되도록 함

- [fs/read_write.c](./fs/read_write.c)
  - `vfs_write`에서 파일에 write할 때, ext2 파일시스템의 경우 `set_gps_location`이 호출되도록 함

- [include/linux/gps.h](./include/linux/gps.h)
  - `gps_location` 구조체 및 기타 함수 프로토타입 정의

- [kernel/gps.c](./kernel/gps.c)
  - `set_gps_location`, `get_gps_location` syscall 구현
  - locking, 거리 계산 등 기타 함수 구현

### 2.2. Location struct, syscall

### 2.3. EXT2 inode modification / updating location

### 2.4. Calculating distance

## 3. Discussion

#### Access restriction

__TODO__

## 4. Test code

__TODO__

## 5. Lessons learned

## 6. References

http://pages.cpsc.ucalgary.ca/~crwth/programming/VFS/inodes.php