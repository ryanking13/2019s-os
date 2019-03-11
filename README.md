# OS project 1

> 김도윤(ddoyoon) / 최경재(ryanking13) / 최원석(MKRoughDiamond)

## Files modified

TODO: update detailed explanation of modification

```
ptree/Makefile
ptree/ptree.c
include/linux/prinfo.h
test/ptree_test.c
arch/arm64/include/asm/unistd32.h
arch/arm64/include/asm/unistd.h
include/linux/syscalls.h
arch/x86/entry/syscalls/syscall_32.tbl
```


## Custom Scripts

> _Time is gold, save your time :)_

_python3 is needed_

```sh
# 커널 빌드 / 부팅 이미지 생성 / USB flash

./build_kernel.py

./build_kernel.py -v  # verbose mode (stdout으로 오는 메세지를 모두 출력)
./build_kernel.py -e  # eject (완료 후 자동 USB eject)
./build_kernel.py -v -e
```

```sh
# 테스트 파일 빌드 / USB에 파일 업로드

./build_test.py

./build_test.py -v
./build_test.py -e
./build_test.py -v -e
```

```sh
./comm
# alias of "screen /dev/ttyUSB0 115200"
```

## Tips

- 시리얼 통신할 때, 안정적으로 부팅되게 하는 법
  - 부팅 성공 시, `shutdown -h now`로 완전히 셧다운 한 후 sd 카드 제거
  - 연결 시, 시리얼 케이블을 먼저 연결하고, `screen` 화면을 띄운 뒤, 전원 케이블을 연결