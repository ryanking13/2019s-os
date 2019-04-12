# OS project 2

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
sudo ./build_test.py --no-copy  # build only
sudo ./build_test.py --eject    # eject sdcard after upload
```

</p>
</details>

## 2. Implementation

### 2.1. Files modified

- _Syscall 등록과 관련한 파일 수정 내용은 Project 1과 거의 동일하므로 생략함_

- [include/linux/rotation.h](./include/linux/rotation.h)
  - rotation lock과 관련한 구조체 정의
  - 기본적인 헬퍼 함수 정의/구현

- [kernel/rotation.c](./kernel/rotation.c)
  - rotation lock/unlock syscall 구현

### 2.2. Data structrue for handling rotation lock

```c
typedef struct {
    int degree;
    rotation_lock_list read_lock_wait_list;
    rotation_lock_list write_lock_wait_list;
    rotation_lock_list read_lock_list;
    rotation_lock_list write_lock_list;
    struct mutex lock;
} rotation_state;
```

`rotation_state`은 rotation lock과 관련한 모든 정보를 가지고 있는 구조체이며, 단 하나의 인스턴스만 존재한다.

- `degree` : 현재 device의 rotation
- `read_lock_list` / `write_lock_list` : 현재 rotation lock을 잡고 있는 reader/writer process의 리스트
- `read_lock_wait_list` / `write_lock_wait_list` : 현재 rotation lock을 잡기 위해 wait하고 있는 reader/writer process의 리스트
- `lock` : rotation_state 인스턴스에 접근할 때 사용되는 mutex lock

`rotation_state` 인스턴스에 접근하는 임의의 프로세스 (reader/writer 불문)은 먼저 lock을 acquire 하여야 한다. 따라서 임의의 두 프로세스가 동시에 인스턴스를 수정할 수 없으며 이를 통해 syncronization 문제를 해결한다.

```c
typedef struct {
    int degree;
    int range;
    int flag;
    wait_queue_head_t queue;
    struct list_head lock_list;
    struct task_struct *task_struct;
} rotation_lock_list;
```

`rotation_state`가 가진 네 개의 `rotation_lock_list` 구조체는 프로세스가 잡은(잡고자 하는) 각각의 rotation lock에 대한 정보를 나타낸다.

- `degree` / `range` : rotation lock의 범위
- `queue` / `flag` : process가 바로 lock을 잡지 못하고 sleep할 경우, 해당 프로세스가 삽입되는 wait queue와, 프로세스를 wake up 할 때의 조건으로 사용되는 flag
- `task_struct` : 이 lock을 잡은(잡고자 하는) process의 task_struct pointer

### 2.3. Syscall implementation

이번 프로젝트에서 구현한 각 syscall의 동작 알고리즘은 아래와 같다. rotation_state 인스턴스의 lock을 acquire/release하는 부분은 모두 공통적으로 수행하므로 생략하였다.

#### set_rotation

```
1. device의 새로운 degree(rotation)을 설정
2. degree 범위의 lock을 잡고 있는 writer가 있다면, 새로 lock을 잡을 수 있는 프로세스가 없으므로, 그대로 종료
3.A. degree 범위의 lock을 잡고 있는 reader가 있다면,
  3.A.1. lock을 잡기를 원하는 reader 중 degree 범위를 포함하는 모든 reader를 wake up
3.B. degree 범위의 lock을 잡고 있는 reader가 없다면,
  3.B.1. lock을 잡기를 원하는 writer 중 degree 범위를 포함하는 가장 일찍 등록된 writer를 wake up
  3.B.2. (3.B.1 에서 해당하는 writer가 없었다면), lock을 잡기를 원하는 reader 중 degree 범위를 포함하는 모든 reader를 wake up
```

#### rotlock_read

```
1.A. device의 degree가 잡고자하는 lock의 범위 밖인 경우,
1.B. 겹치는 범위에서 lock을 잡고 있는 writer가 있을 경우,
1.C. 겹치는 범위에서 lock을 잡기를 기다리고 있는 writer가 있을 경우,
  1.(A, B, C).1. lock을 잡기를 원하는 리스트에 자신을 추가하고 sleep
1.D. 그 외의 경우,
  1.D.1. lock을 잡고 있는 reader 리스트에 자신을 추가
```

#### rotlock_write

```
1.A. device의 degree가 잡고자하는 lock의 범위 밖인 경우,
1.B. 겹치는 범위에서 lock을 잡고 있는 writer가 있을 경우,
1.C. 겹치는 범위에서 lock을 잡기를 기다리고 있는 writer가 있을 경우,
1.D. 겹치는 범위에서 lock을 잡기를 기다리고 있는 reader가 있을 경우,
  1.(A, B, C, D).1. lock을 잡기를 원하는 리스트에 자신을 추가하고 sleep
1.E. 그 외의 경우,
  1.E.1. lock을 잡고 있는 writer 리스트에 자신을 추가
```

#### rotunlock_read

```
1. lock을 잡고 있는 reader 리스트를 순회하며, 자신이 잡고 있는 정확한 범위의 lock을 리스트에서 제거
2. 새로 lock을 잡을 수 있게된 reader/writer를 wake up (set_rotation의 2-3번 과정과 동일)
```

#### rotunlock_write

```
1. lock을 잡고 있는 writer 리스트를 순회하며, 자신이 잡고 있는 정확한 범위의 lock을 리스트에서 제거
2. 새로 lock을 잡을 수 있게된 reader/writer를 wake up (set_rotation의 2-3번 과정과 동일)
```

### 2.4. Sleep / Wake up mechanism

lock을 잡지 못한 프로세스가 sleep 하고, lock을 얻을 수 있게 되었을 때 wake up하여 lock을 잡는 과정은 다음과 같다.

(reader 기준으로 작성하였으나, 리스트명을 제외하면 writer도 동일함)

__sleep__

- lock을 잡지 못한 reader는 자신을 `rotation_state` 인스턴스의 `read_lock_wait_list` 리스트에 등록한다.
- 이 때 리스트의 노드에는 reader가 자신을 넣을 `wait_queue`와, 자신을 깨울 때 사용할 `flag` 변수가 포함되어있다.
- 이후 reader는 `wait_queue`에 자신을 queue 하고 (`wait_event_interruptible()`) sleep 상태가 된다.

__wake up__

- sleep 상태인 프로세스가 lock을 잡을 수 있게 되면,
- `read_lock_wait_list`에서 프로세스를 나타내는 노드를 찾아, `flag` 값을 세팅하고, 프로세스를 wake up 한다. (`wake_up_inturruptible()`)
- `read_lock_wait_list`에서 해당 노드를 제거하고, `read_lock_list` 리스트에 노드를 등록한다.

### 2.5. Handling process exit

__TODO (MK_RD)__

```
1. lock을 잡은(잡고자 하는) reader/writer 리스트를 순회하며, 자신이 등록한 모든 노드를 리스트에서 제거
2. 새로 lock을 잡을 수 있게된 reader/writer를 wake up (set_rotation의 2-3번 과정과 동일)
```

### 3. Discussion

- `roatation_state` 객체에 fine-grained lock 을 사용하지 않은 이유?

`rotation_state`의 각 reader/writer lock list에 find-grained lock을 사용하는 대신, 인스턴스 전체에 대한 global lock을 사용하였다. 이와 같이 구현한 이유는, rotation lock과 관련된 동작이 하나의 리스트에 국한하여 이루어지지 않기 때문이다. 예를 들어, writer가 rotation lock을 잡을 수 있는 지의 여부는, 현재 rotation lock을 잡고 있는 reader/writer, rotation lock을 기다리고 있는 reader/writer 모두를 보아야 결정 할 수 있으므로, 네 개의 리스트 전체에 대한 mutual exclusion이 확보된 상태에서 결정될 수 있다. 따라서 개별적으로 각 리스트에 대한 fine-grained lock을 잡는 것의 효용이 적다고 판단되어 `rotation_state` 인스턴스 전체에 대한 global lock을 사용하였다.


### 4. Test code

__TODO__

- selector.c
- trial.c
- test_code_which_checks_various_cases.c

## 5. Lessons learned

__TODO__

## 6. References

https://www.linuxjournal.com/article/8144
