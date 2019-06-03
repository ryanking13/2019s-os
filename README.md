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

거리를 계산하기 위해 우리는 구면 상의 두 점 간의 거리를 이용하여 계산하였다. 구면 상의 두 점 사이의 각도(지구 중심점을 기준으로)를 구하여 평균 지구 반지름을 곱하는 방식을 사용하였다.

기본적으로 각도를 구하는 방식은 여러 종류가 존재하지만, 그 중 acos를 사용하는 방식과 asin을 사용하는 방식을 혼용하여 구현하였다. acos을 이용한 방식만으로는 정확도를 보장할 수 없었기에(위키피디아의 [Great-circle_distance](https://en.wikipedia.org/wiki/Great-circle_distance)의 computational formulas 부분) 중간 계산 값의 범위를 임의의 구간으로 나누어 계산하게 되었다.

결과 계산을 위해서는 삼각함수와 그 역함수(sin, cos, asin, acos)에 대한 연산이 필요하다. 해당 함숫값을 얻는 방법으로 lookup table과 그 값들 사이의 내삽(interpolation)을 이용하여 계산하게 되었다. 해당 방식을 통해 편의성을 얻을 수 있었으나, 오차의 발생 원인이 되기도 하였다.(선형근사가 실패하는 경우)

asin을 이용하는 경우 제곱근 연산(sqrt)가 필요하다. 해당 연산은 이분탐색을 사용하여 빠르고 비교적 정확하게 계산할 수 있었다. 하지만 asin의 경우 1km 이내의 거리에 대해 계산하는 과정에서 값이 사라지는 문제가 발생하여(너무 작아서 제곱하는 과정에서 값이 0이 됨) 거리가 0으로 나오는 문제가 발생하였고, 이에 대해 유클리드 거리로 계산하였다.

결과적으로 모든 연산을 주어진 스펙(integer part, fractional part로 구분, fractional part의 값은 -999999~999999 사이, 음수의 경우 integer part가 0일 때 활용)대로 구현하기 위해 이를 만족하는 연산 체계(`dec6.c`)를 만들어서 활용하였다. 해당 연산체계 내에서 삼각함수, sqrt가 1e-6 정도의 오차를 발생하므로 같은 연산체계를 유지하면서 계산하는 것은 오차 누적의 문제가 있다.(이 문제는 floating point 연산이 가지고 있는 문제이기도 하다.)

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

#### Errors and their possible reasons

1. asin을 활용한 계산방법에서 발생하는 오차

asin을 활용하는 계산 방법에서 sin 값을 제곱하여 연산에 활용한다. 만일 sin값이 0.000XXX의 값을 가지고 있다면, 제곱을 할 경우 해당 값이 disappear되는 경우가 발생한다. 이를 최대한 해소하고자 제곱하는 sin 값에 임의의 상수를 곱하고, 제곱근 연산까지 진행한 뒤에 해당 상수로 다시 나누는 방식을 채택하였다. 이를 이용하여 연산의 오차를 어느정도 해소할 수 있었다.

하지만 해당 방식으로도 두 점 사이의 거리가 매우 짧은 경우(넉넉하게 1km 이내) asin을 활용하는 계산 방식이 실패하는 경우(거리가 0으로 계산)가 발생한다. 해당 문제를 해결하기 위해서는 두 점을 2차원 유클리드 공간으로 옮겨서 직선거리를 계산하는 것이 상대적으로 낫다고 판단하였다.

1km 이내의 거리의 경우 지구의 곡률을 무시하더라도 충분히 유사한 거리를 산출해 낼 수 있었다. 그러므로, asin이 실패하는 거리(넉넉하게 1km 이내)에서 유클리드 거리를 계산함으로서 원하는 결과를 얻을 수 있었다.

2. 소숫점 6째 자리까지 사용하는 데에서 발생하는 오차

상술했지만, 모든 연산을 소수점 6째 자리로 고정하여 연산을 진행하였고, 이를 위해 해당 구조체와 그를 활용하는 연산체계를 정의하였다. 해당 연산체계 내에서 소수점 6째자리 이하는 버림 처리하였고, 이로 인해 곱셈, 정수 나눗셈(divide by integer), 삼각함수와 그 역함수들에서 오차가 발생하게 되었다.

가장 큰 오차를 생성했던 것은 acos(0.999xxx) 부분인데, acos 함수는 해당 부분에서 급격한 값의 변화를 가지기 때문에 선형근사를 할 수 없고, 충분히 촘촘하게 선형근사를 하더라도 1e-6의 오차만으로 매우 큰 값의 변화를 만들어 내기 때문에 해당 부분에서 asin을 사용하는 계산방식을 활용할 수밖에 없었다.

3. "지구"이기에 발생하는 오차

지구는 이상적인 형태의 구형이 아니기 때문에 발생하는 오차이다. 지구는 적도 반지름이 극 반지름보다 더 큰 타원체이다. 이로서 발생하는 문제를 최소화하기 위해 평균적으로 얻은 지구 반지름(6371km)를 사용하였지만, 여전히 해당 문제로 인해 거리가 큰 경우, 오차가 발생할 수밖에 없다.

두번째로는 고도의 문제이다. 일반적인 고도에서는 오차가 크게 발생하지 않겠지만, 만일 에베레스트 초입과 정상 간의 거리 등을 측정할 경우 반드시 문제가 발생할 수 밖에 없다. 평균적으로 고도의 차이가 큰 두 지점간의 거리에서 어느정도 유의미한 오차가 발생하게 된다.

## 4. Test code

[proj4.fs](./proj4.fs) 파일을 마운트하여 테스트할 수 있음. (주의: `test/proj4.fs` 는 비어있음)

__Directory structure of proj4.fs__

_please ignore `lost+found`_

- test
  - tfile1
  - tfile2

```sh
root:~> ./file_loc proj4/test/tfile1
GPS location information of: proj4/test/tfile1
Latitude: 12.000034
Longitude: 56.000078
Accuracy: 90
http://maps.google.com/maps?q=12.000034,56.000078
root:~> ./file_loc proj4/test/tfile2
GPS location information of: proj4/test/tfile2
Latitude: 90.000078
Longitude: 56.000034
Accuracy: 12
http://maps.google.com/maps?q=90.000078,56.000034
```

## 5. Lessons learned

- 리눅스 시스템에서 파일을 표현하기 위하여 사용하는 inode에 대하여 이해함
- 여러가지 파일 시스템을 공존시키기 위한 VFS 방식에 대해서 이해함

## 6. References

http://pages.cpsc.ucalgary.ca/~crwth/programming/VFS/inodes.php

https://en.wikipedia.org/wiki/Great-circle_distance
