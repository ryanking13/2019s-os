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

### 2.2. Location struct

`gps_location` struct는 프로젝트 스펙과 동일하게 정의되었으며, global singleton instance 인 `init_location`이 device의 location을 나타낸다.

`init_location`에 접근할 때는 spinlock을 사용하여 synchronization과 관련한 문제가 발생하지 않도록 하였다.


### 2.3. Updating location

EXT2 inode의 location 정보는 파일이 새롭게 생성되거나 (create), 수정될 때 (modify) 업데이트 되어야 한다.

이를 위하여 새로운 파일이 생성되는 경우인 `ext2_create`와, 파일이 수정되는 경우인 `vfs_write` 함수에 각각 location을 업데이트하는 로직을 추가하였다.

### 2.4. Calculating distance

__TODO__

## 3. Discussion

#### Location-based file access

프로젝트 스펙에서 요구하는 Location-based file access 를 구현하는 방법은 다양한데, 우리는 inode_operations 구조체의 `permission` 속성을 구현하여 사용하는 것을 선택하였다. `permission` 속성은 inode에 access 할 수 있는지 확인하는 데에 사용되는 `inode_permission` 함수 내부에서 호출되므로, 해당 속성을 사용하여 access restriction을 구현하는 것이 가장 표준적인 방법이라고 판단하였다.

`permission` 속성의 구현체인 `ext2_permission` 함수는 아래와 같다. `can_access_here` 함수로 현대 device의 location과 파일의 location의 위치를 비교하여 access 가능 여부를 판단하고, 실패 시에는 에러를 리턴하며, 성공 시에는 이후의 검사를 수행(`generic_permission`) 수행한다.

```c
int ext2_permission(struct inode *inode, int mask) {
	struct gps_location loc;
	ext2_get_gps_location(inode, &loc);

	// gps_get_location() syscall must preceed, so check MAY_GET_LOCATION mask
	if (!(mask & MAY_GET_LOCATION) && !can_access_here(&loc)) {
		return -EPERM;
	}

	return generic_permission(inode, mask);
}
```

이 때, 이번 프로젝트에서 구현한 `get_gps_location` syscall의 경우에도 `inode_permission` 함수를 사용하여 해당 파일(inode)에 접근이 가능한지 확인하게 된다. `get_gps_location`의 경우는 파일의 location과는 무관하게 access 가능 여부를 판단하여야 하므로, `get_gps_location` 에서만 사용하는 `MAY_GET_LOCATION` mask를 정의하여, `get_gps_location`에서는 파일의 location을 검사하지 않도록 하였다.


## 4. Test code

__TODO__

## 5. Lessons learned

## 6. References

http://pages.cpsc.ucalgary.ca/~crwth/programming/VFS/inodes.php
