# OS project 3

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

- _Syscall 등록과 관련한 파일 수정 내용은 Project 1, 2과 거의 동일하므로 생략함_

- [include/linux/sched.h](./include/linux/sched.h)
  - `sched_wrr_entity` struct 정의, task_struct에 `sched_wrr_entity wrr` 속성 추가

- [include/linux/init_task.h](./include/linux/init_task.h)
  - INIT_TASK 매크로에 `wrr` 속성 초기화 추가

- [include/uapi/linux/sched.h](./include/uapi/linux/sched.h)
  - `SCHED_WRR` 추가

- [kernel/sched/core.c](./kernel/sched/core.c)
  - `sched_setweight`, `sched_getweight` syscall 구현
  - `sched_setscheduler` syscall에 WRR 스케쥴러 관련 설정 추가
  - 프로세스 fork 시 WRR 스케쥴러 설정 관련 핸들링

- [kernel/sched/sched.h](./kernel/sched/sched.h)
  - `wrr_rq` sturct 정의, rq에 `wrr_rq wrr` 속성 추가
  - 필요한 매크로 및 상수 정의

- [kernel/sched/wrr.c](./kernel/sched/wrr.c)
  - `wrr_sched_class` 구현

- [kernel/sched/rt.c](./kernel/sched/rt.c)
  - rt_sched_class의 next 값을 cfs_sched_class에서 wrr_sched_class로 변경

### 2.2. Data structure for WRR Scheduler

```c
struct sched_wrr_entity {
	struct list_head		run_list;
	unsigned int			weight;
	unsigned int			time_slice;
	unsigned short			on_rq;
};
```

`sched_wrr_entity`는 스케쥴러가 스케쥴링하는 하나의 태스크 단위를 나타낸다.

- weight: WRR 태스크의 weight 값을 나타낸다.
- time_slice: 태스크의 time_slice, 현재 돌아가고 있는 태스크라면 tick마다 time_slice값이 감소하며, 0이 되었을 때에 다시 time_slice를 초기화하고 run queue를 round robin한다.
- on_rq: 태스크가 현재 run queue에 들어가있는지 나타내는 값, enqueue 및 dequeue될 때 값이 변경된다.

```c
struct wrr_rq {
	struct wrr_array active;
	unsigned int wrr_nr_running;
	unsigned int weight_sum;
	struct rq *rq;
	unsigned int load_balance_time;
};

struct wrr_array {
	struct list_head queue;
};
```

`wrr_rq`는 WRR 스케쥴러에서 사용하는 run queue를 나타낸다.

- active: 현재 해당 스케쥴러에 enqueue되어 있는 태스크 (sched_wrr_entity)의 리스트.
- wrr_nr_running: 현재 run queue에 들어있는 태스크의 개수를 나타낸다.
- weight_sum: 현재 run queue에 들어있는 태스크의 weight 함을 나타낸다.
- rq: rq struct를 가리키는 포인터
- load_balance_time: 로드밸런싱 타이밍을 나타내는 값. scheduler tick마다 값이 감소하며, 0이 되었을 때에 다시 값을 초기화하고 load balancing을 수행하여야하는 CPU라면 load balancing을 수행한다.

### 2.3. Syscall implementation

#### sched_setweight

Parameter와 user를 검증한 뒤, pid에 해당하는 태스크 하나의 weight (sched_wrr_entity.weight)를 업데이트 한다.
만약 해당 태스크가 run queue에 포함되어있는 상태라면, wrr run queue의 weight_sum (wrr_rq.weight_sum)를 같이 업데이트 한다.

#### sched_getweight

Parameter와 user를 검증한 뒤, pid에 해당하는 태스크 하나의 weight (sched_wrr_entity.weight)를 반환한다.

### 2.4. wrr_sched_class

WRR scheduler를 구현하기 위해, sched_class struct에 요구되는 아래의 함수들을 구현하였다.

아래의 함수들은 외부에서 호출될 때에 synchronization 문제가 발생하지 않도록 적절한 lock을 잡을 상태에서 수행됨이 보장된다. 따라서 synchronization과 관련한 부분은 고려할 필요가 없다.

#### enqueue_task

`enqueue_task`는 새로운 태스크를 run queue에 추가할 때 호출된다.

1. 태스크의 sched_wrr_entity (이하 wrr_se) 포인터를 wrr_rq의 리스트 끝에 추가한다.
2. wrr_rq의 wrr_nr_running, weight_sum 값을 업데이트 한다.
3. wrr_se의 on_rq 값을 업데이트 한다.
4. 전체 run queue의 running 태스크 개수를 업데이트 한다.

#### dequeue_task

`dequeue_task`는 태스크가 run queue에서 제거될 때에 호출된다.

1. wrr_se 포인터를 wrr_rq의 리스트에서 제거한다.
2. wrr_rq의 wrr_nr_running, weight_sum 값을 업데이트 한다.
3. wrr_se의 on_rq 값을 업데이트 한다.
4. 전체 run queue의 running 태스크 개수를 업데이트 한다.

#### yield_task

`yield_task`는 태스크가 자발적으로 CPU를 놓았을 때 호출된다.

1. wrr_rq의 리스트에 들어있던 wrr_se 포인터를 리스트의 맨 뒤로 옮긴다.

이때, yield한 태스크의 time_slice는 초기화하지 않도록 하였다. 즉 yield한 태스크가 다시 run queue의 최상단으로 돌아와
스케쥴링된다면, 기존에 소모되어 남아있는 time_slice만 CPU를 사용한다. (이는 RT scheduler의 round robin 방식과 동일함)

#### pick_next_task

`pick_next_task`는 WRR 스케쥴러가 스케쥴링할 태스크를 찾고자 할때 호출된다.

1. wrr_rq의 리스트 제일 앞에 위치한 태스크를 반환한다.
2. 단, 해당 태스크의 time_slice가 일정한 값 이하 (5 = 5ms)이하 일 때는 NULL을 반환한다. (이에 대한 이유는 discussion에서 서술함)

#### select_task_rq

`select_task_rq`는 태스크가 수행될 CPU를 선택할 때 호출된다.

1. RCU readlock을 잡고 CPU를 순회한다.
2. 각 CPU의 wrr_rq의 weight_sum 값을 확인하여, 가장 작은 값을 가진 CPU 번호를 반환한다.
3. 단, 이때 CPU가 미리 지정된 비워둘 CPU인 경우 또는 태스크의 cpumask가 해당 CPU에서 실행될 수 없도록 지정 되어 있는 경우 제외한다.

#### task_tick

`task_tick`은 tick마다 호출된다.

1. 현재 wrr_rq의 리스트 제일 앞에 위치한 태스크 (실행 중인 태스크)의 time_slice 값을 1 감소시킨다.
2. time_slice 값이 0이 되었을 경우, time_slice 값을 다시 초기화 (10ms * weight)하고, 태스크를 wrr_rq 리스트의 맨 뒤로 보낸다.

### 2.5. Forking a new task

새로운 프로세스가 fork되면, 내부적으로 core.c의 `__sched_fork()` 함수가 호출된다.

이 함수 내부에 새로 생성된 프로세스에 대하여 WRR 스케쥴러와 관련된 속성들을 초기화하고, 부모의 weight를 상속받는 부분을 추가하였다.

### 2.6. Load balancing

매 tick마다 호출되는 core.c의 `scheduler_tick()` 함수 내부에서 `trigger_load_balance_wrr()` 함수를 호출한다.

`trigger_load_balance_wrr()` 함수는 wrr_rq의 load_balance_time값을 1씩 감소시키며, 0이 되면 값을 초기화(2000ms)하며 load balancing을 수행한다.

load balancing은 하나의 CPU만 수행하면 되므로, `cpumask_first()` 함수를 사용하여, 현재 CPU가 online인 CPU 중 가장 번호가 빠른 CPU인 경우에만 load balancing을 수행하도록 하였다.

load balancing 과정은 다음과 같다.

1. RCU read lock을 잡고 CPU를 순회하면서 wrr_rq의 weight sum이 가장 작은 CPU와 큰 CPU를 찾는다. (이 때 비워둘 CPU는 제외한다.)
2. 두 CPU의 run queue에 대한 락을 잡고, wrr_rq를 순회하면서 조건에 맞는 (weight_sum이 역전되지 않고, cpumask 상 migration이 가능하며, 그 중에서 weight가 최대인) 태스크를 찾는다.
3. 찾은 태스크를 한 run queue에서 다른 run queue로 옮긴다.

### 2.7. Emptying one CPU

프로젝트 스펙에 따라, 0번 CPU에는 WRR 태스크가 실행되지 않도록 하였다. 이를 달성하기 위해 아래와 같은 조건을 추가하였다.

1. `select_task_rq`에서 0번 CPU를 리턴하지 않도록 함 (2.4 참고)
2. load balancing 수행 시 0번 CPU를 고려하지 않도록 함 (2.6 참고)
3. `sched_setscheduler` syscall 호출시 0번 CPU가 할당되지 않도록 함

이때, 3번의 경우는 다음과 같이 구현하였다.

- cpumask가 0번 CPU에만 할당이 가능하도록 되어있는 경우: scheduler를 변경하지 않고 syscall이 에러를 리턴함
- cpumask가 0번 CPU를 포함한 두 개 이상의 CPU에 할당이 가능하도록 되어있는 경우: 0번 CPU를 cpumask에서 지우고 이후의 syscall 과정을 진행함.

### 3. Discussion

#### Bandwidth for CFS Task

WRR 태스크는 CFS 태스크보다 우선순위가 높기 때문에, WRR 태스크가 끝나지 않고 CPU를 점유하고 있다면
CFS 태스크가 실행될 수 없다.

RT 스케쥴러에서는 동일한 문제를 RT 스케쥴러가 CPU를 점유 할 수 있는 bandwidth를 지정하는 방식으로 해결하였는데, RT 스케쥴러의 bandwidth 구현은 상당히 복잡하므로, 본 프로젝트에서는 WRR 태스크가 가진 time_slice의 일부를 CFS 태스크를 위해 할당하는 단순한 방식으로 문제를 해결하였다.

`pick_next_task`가 호출될 때에, 현재 wrr_rq의 맨 앞에 있는 (=실행되어야 하는) WRR 태스크의 time_slice가 5ms 이하로 남아있을 경우, `pick_next_task` 해당 태스크를 리턴하지 않고 NULL을 리턴하게 하여, CFS 태스크가 실행될 수 있도록 하였다. 즉, weight가 10인 WRR 태스크인 경우 100ms의 시간을 할당받는데, 이 중 95ms는 WRR 태스크가 수행되고, 5ms는 CFS 태스크가 수행되도록 구현하였다.

### 4. Test code

TODO

## 5. Lessons learned

- 여러 단계로 구성된 리눅스의 스케쥴러 우선순위 방식이 확장하기 쉽게 잘 구성되어있다고 느낌

- 효율적이고 공정하게 프로세스를 스케쥴링하기 위해 priority, load balancing, bandwidth, group scheduling 등 수많은 아이디어들이 필요함을 느낌


## 6. References

http://jake.dothome.co.kr/scheduler/

https://mgouzenko.github.io/jekyll/update/2016/11/24/understanding-sched-class.html
