# OS project 1

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

# Example

sudo ./build_test.py            # build & upload
sudo ./build_test.py --no-copy  # build only
sudo ./build_test.py --eject    # eject sdcard after upload
```

</p>
</details>

## 2. Implementation

### 2.1. Files modified

- [ptree/ptree.c](./ptree/ptree.c)
  - `ptree` 시스템콜 정의 (로직은 아래에 설명)
- [include/linux/prinfo.h](./include/linux/prinfo.h)
  - `prinfo` struct 정의
  - duplicate symbol 문제를 해결하기 위하여 kernel build시와 test code build 시의 동작을 달리함
- [Makefile](./Makefile), [ptree/Makefile](./ptree/Makefile)
  - ptree, prinfo를 빌드시 포함하기 위하여 수정

- [arch/x86/entry/syscalls/syscall_32.tbl](./arch/x86/entry/syscalls/syscall_32.tbl)
  - 시스템콜 테이블에 398번 ptree 추가

- [include/linux/syscalls.h](./include/linux/syscalls.h)
  - ptree 함수 프로토타입 추가

- [arch/arm64/include/asm/unistd32.h](./arch/arm64/include/asm/unistd32.h)
  - ptree 매크로 추가

- [arch/arm64/include/asm/unistd.h](./arch/arm64/include/asm/unistd.h)
  - 시스템 콜 개수 수정 (398 -> 399)

### 2.2. ptree syscall implementation

__(To Be updated)__

- DFS
  - kernel space 에서 수행되는 로직임을 고려하여 DFS 구현에 있어 recursion 방식이나 stack을 사용하는 방식 대신 pointer operation으로 구현함

- Error handling behavior
  - 비정상 종료시에는 -1을 리턴하고 `errno` 값을 업데이트 함.

### 2.3. Test code


[test/test_ptree.c](./test/ptree.c)

```sh
./test_ptree
```
- 먼저 5가지 error case 를 test 함
  - Covered Cases
    1. `nr` < 1
    2. `buf` == `NULL`
    3. `&nr` == `NULL`
    4. `buf` is outside the accessible address space
    5. `nr` is outside the accessible address space
  - Printed Output
    - Case description and expected error name
    - `buf`, `&nr` values
    - Return code of `syscall`
    - Error message for the `errno` value (using `perror()`)
- Error testing 후, 사용자로부터 `nr` 값을 입력으로 받아 `syscall` 의 return code 와 해당하는 process tree 를 출력하는 무한루프가 실행됨
  - Indent 할 tab 의 개수는 `pid_t depth[nr]` 배열과 index cursor 를 사용해 구현함
  - `buf` 에 반환된 결과가 이미 DFS 순으로 정렬되어 있으므로, `depth[0]=0, cursor=0` 으로 초기화 한 후 각 process 에 대해 아래의 과정을 거침
    - Move cursor backwards until it points to parent_pid
    - `cursor++` and `depth[cursor]=process`
    - Print tab `cursor` times and print the process

## 3. Process tree investigation
```sh
swapper/0,0,0,0,1,0,0
    systemd,1,1,0,147,2,0
        systemd-journal,147,1,1,0,179,0
        systemd-udevd,179,1,1,0,237,0
        ...
        login,263,1,1,498,264,0
            bash,498,1,263,505,0,0
                test_ptree,505,0,498,0,0,0
        license-manager,264,1,1,0,265,402
        systemd-logind,265,1,1,0,266,0
        ...
    kthreadd,2,1,0,3,0,0
            kworker/0:0,3,1026,2,0,4,0
            kworker/0:0H,4,1026,2,0,5,0
            ...
```
- `swapper` (PID 0): System process containing idle threads.
  - The idle thread does nothing but loop indefinitely, and is always runnable. The idle task has the lowest scheduling priority, so it runs when no other task is runnable. For power management, modern idle threads instruct the executing processor to wait, potentially lowering its clock rate to save power, for it to receive a hardware interrupt that will break the idle thread's infinite loop.
- `systemd` (PID 1): System and service manager that starts the rest of the system.
  - `systemd-journal` (PID 160): System service that collects and stores logging data
  - `systemd-udevd` (PID 203): udev daemon that receives device uevents directly from the kernel whenever a device is added or removed from the system, or it changes its state.
  - `systemd-logind` (PID 265): System service that manages user logins.
- `kthreadd` (PID 2): Kernel thread daemon. All kthreads are forked from this thread.
  - `kworker`: Placeholder process for kernel worker threads.

## 4. Lessons learned

__(To Be Updated)__

## 5. References
Bovet, D. P., & Cesati, M. (2005). _Understanding the Linux Kernel: from I/O ports to process management._ " O'Reilly Media, Inc.". 124-125

https://www.freedesktop.org/software/systemd/man/