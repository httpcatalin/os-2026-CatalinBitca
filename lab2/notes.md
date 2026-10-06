# Lab 2 - Meet the OS You Will Build (xv6)

Bîtca Cătălin, FAF-242

I did this lab on my Azure VM (Ubuntu 24.04). The toolchain was not installed there, so first I installed it:

```
sudo apt update && sudo apt install -y git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
git clone https://github.com/mit-pdos/xv6-riscv ~/xv6-riscv
cd ~/xv6-riscv
```

QEMU version is 8.2.2 and the xv6 commit I cloned is `06aad25`.

## Part 1 - Build and boot

`make qemu` compiled the kernel and all the user programs, made the `fs.img` disk and started QEMU:

```
qemu-system-riscv64 -machine virt -bios none -kernel kernel/kernel -m 128M -smp 3 -nographic ...
xv6 kernel is booting
hart 2 starting
hart 1 starting
init: starting sh
$
```

It boots in a few seconds. The machine has 128 MB of RAM and 3 CPUs ("harts"). To quit I used Ctrl-A then X.

## Part 2 - Using xv6

```
$ echo hello xv6
hello xv6
$ ls | grep c
cat            2 3 35376
echo           2 4 34312
wc             2 17 36336
sync           2 22 33712
console        3 23 0
$ wc README
48 336 2441 README
```

I also ran `usertests -q`, it took a few minutes and at the end it said `ALL TESTS PASSED` (62 tests).

**1. Three programs xv6 ships with**

`ls`, `cat`, `echo` (also `grep`, `wc`, `sh`, `kill`, `mkdir`, `rm`).

**2. Which two OS features are needed for a pipe?**

- **Processes**: the shell has to create two processes (with `fork` and `exec`), one for `ls` and one for `grep`, and they run at the same time.
- **Communication between processes**: the `pipe()` system call, which gives a buffer in the kernel with a read end and a write end (as file descriptors). The output of `ls` goes into the write end and `grep` reads from the read end.

**3. xv6 shell vs Linux shell**

The basic things work the same (running programs, `|`, `>`, `<`), but the xv6 shell is much simpler: no history with the arrow keys, no tab completion, no variables, and `ls` shows only name, type, inode number and size instead of permissions and owners.

## Part 3 - Reading the source

**1. Which system calls does `user/cat.c` use?**

- `open(path, O_RDONLY)` - asks the kernel to open the file and give back a file descriptor
- `read(fd, buf, 512)` - asks the kernel to copy up to 512 bytes from the file into `buf`
- `write(1, buf, n)` - asks the kernel to write `n` bytes to fd 1 (the console / stdout). `fprintf(2, ...)` for errors also ends up calling `write`, on fd 2
- `close(fd)` - tells the kernel we are done with that file descriptor
- `exit(0)` / `exit(1)` - ends the process and gives the status back to the parent

They are all declared in `user/user.h` under `// system calls`. In `user/usys.S` I saw that `read` is only 3 instructions: it puts the number `SYS_read` (5) in register `a7` and does `ecall`, which jumps into the kernel.

**2. Where is `sys_read` implemented?**

In `kernel/sysfile.c`, line 69:

```
$ grep -n "sys_read\|sys_write" kernel/sysfile.c
69:sys_read(void)
83:sys_write(void)
```

It takes the arguments (fd, buffer address, n) and calls `fileread()`.

**3. Difference between `kernel/` and `user/`**

The code in `kernel/` is the OS itself and runs in privileged (supervisor) mode with access to all the memory and hardware, while the code in `user/` are normal programs that run in user mode and can only ask the kernel for things through system calls.

## Part 4 - My sleep program

I wrote `user/sleep.c` exactly like in the lab and added `$U/_sleep\` to `UPROGS` in the Makefile, but `make qemu` failed:

```
user/sleep.c: In function 'main':
user/sleep.c:11:3: error: implicit declaration of function 'sleep' [-Werror=implicit-function-declaration]
   11 |   sleep(atoi(argv[1]));
cc1: all warnings being treated as errors
make: *** [<builtin>: user/sleep.o] Error 1
```

I looked in `user/user.h` and there is no `sleep()`, only `int pause(int);`. With `git log` I found that in the current xv6 (commit `f5dea58`, 2025-08-19) the system call was renamed from `sleep` to `pause` (`SYS_sleep` -> `SYS_pause`, `sys_sleep()` -> `sys_pause()`). The same commit also renamed kernel functions like `fork()` -> `kfork()` and `exec()` -> `kexec()`, I think so the user and kernel names don't get mixed up (the kernel has its own internal `sleep()`). It's the same system call, only the name is different. So I call `pause()` instead:

```c
#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc != 2){
    fprintf(2, "usage: sleep <ticks>\n");
    exit(1);
  }
  pause(atoi(argv[1]));
  exit(0);
}
```

After that it compiled and works:

```
$ sleep 10
$ sleep
usage: sleep <ticks>
$
```

`sleep 10` waits about 1 second before the prompt comes back. I measured it from outside: `sleep 10` = 1.02 s, `sleep 50` = 5.07 s, `sleep 100` = 10.13 s. So 1 tick is about 0.1 s. In `kernel/trap.c` there is a comment that says the timer interrupt is set to 1000000 cycles, "about a tenth of a second", so it matches. The kernel part is `sys_pause()` in `kernel/sysproc.c` (line 68): it waits in a loop until `ticks` grows by `n`.

## Optional - pingpong

I wrote `user/pingpong.c` and added `$U/_pingpong\` to `UPROGS`. It makes two pipes (one parent -> child, one child -> parent) and then calls `fork()`. The parent writes one byte to the child, the child reads it, prints `received ping` and writes the byte back, then the parent reads it and prints `received pong`.

```
$ pingpong
7: received ping
6: received pong
```

The child has pid 7 and the parent (pid 6) is the process that the shell created for `pingpong`. The order is always correct because the parent is blocked in `read()` until the child sends the byte back.
