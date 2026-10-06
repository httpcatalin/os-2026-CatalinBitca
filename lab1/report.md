# Lab 1 - The OS as a Resource Manager

Bîtca Cătălin, FAF-242

I did this lab on my own Azure VM (Ubuntu 24.04) instead of the provided one. I connected to it with ssh.

```
$ mkdir -p ~/os-lab1 && cd ~/os-lab1
$ whoami
azureuser
$ uname -a
Linux myVPS 6.17.0-1022-azure #22-Ubuntu SMP Mon Jul 27 17:24:03 UTC 2026 x86_64 x86_64 x86_64 GNU/Linux
$ uptime
 11:46:04 up 39 min,  2 users,  load average: 0.00, 0.00, 0.00
```

## Part 1 - Files and directories

```
$ cd ~/os-lab1
$ mkdir demo && cd demo
$ echo "hello operating systems" > note.txt
$ cp note.txt copy.txt
$ mv copy.txt renamed.txt
$ ls -l
total 8
-rw-rw-r-- 1 azureuser azureuser 24 Sep 29 11:37 note.txt
-rw-rw-r-- 1 azureuser azureuser 24 Sep 29 11:37 renamed.txt
$ rm renamed.txt
$ ls -l note.txt
-rw-rw-r-- 1 azureuser azureuser 24 Sep 29 11:37 note.txt
$ chmod 600 note.txt
$ ls -l note.txt
-rw------- 1 azureuser azureuser 24 Sep 29 11:37 note.txt
$ chmod 644 note.txt
```

**1. Who owns the files and which group?**

The owner is me (`azureuser`) and the group is also `azureuser`. On Ubuntu every user gets a group with the same name as the user.

**2. What do the 10 characters mean after chmod 600?**

It was `-rw-------`:
- first `-` = it's a normal file (for a folder it would be `d`)
- `rw-` = the owner (me) can read and write
- `---` = the group can't do anything
- `---` = the others can't do anything

So after chmod 600 only I can read or change the file.

**3. Two directories under /**

- `/etc` - configuration files of the system (I saw `apt`, `adduser.conf`, `bash.bashrc` there)
- `/home` - the home folders of the users, on my VM there is only `azureuser`

**Interesting output**

```
$ ls -la /
lrwxrwxrwx   1 root root     7 Apr 22  2024 bin -> usr/bin
lrwxrwxrwx   1 root root     7 Apr 22  2024 lib -> usr/lib
dr-xr-xr-x 179 root root     0 Sep 29 11:07 proc
```

I didn't know that `/bin` is just a link to `/usr/bin` on this Ubuntu. Also `/proc` has size 0 because it's not a real folder on the disk, the kernel creates it.

## Part 2 - Processes

```
$ ps aux | wc -l
119
$ sleep 300 &
[1] 2796
$ jobs
[1]+  Running                 sleep 300 &
$ ps -ef | grep sleep
azureus+    2796    2790  0 11:46 pts/1    00:00:00 sleep 300
azureus+    2799    2790  0 11:46 pts/1    00:00:00 grep --color=auto sleep
$ kill 2796
[1]+  Terminated              sleep 300
```

**1. What is PID 1?**

PID 1 is `systemd`. In `ps` it shows as `/sbin/init`, but I checked and it's a link to systemd:

```
$ ls -l /sbin/init
lrwxrwxrwx 1 root root 22 Jul 28 15:04 /sbin/init -> ../lib/systemd/systemd
```

It's the first process the kernel starts when the system boots, and after that it starts all the other services (ssh, cron, etc).

**2. How many processes?**

`ps aux | wc -l` gave 119, so 118 processes (the first line is the header). `top` also showed `Tasks: 118 total`. I thought it would be less on an empty VM, but most of them are kernel threads (the ones in square brackets like `[kworker/...]`).

**3. State of the sleep process**

```
$ sleep 300 & PID=$!
$ cat /proc/$PID/status | head -20
Name:	sleep
Umask:	0002
State:	S (sleeping)
Tgid:	2702
Ngid:	0
Pid:	2702
PPid:	2670
...
```

It says `S (sleeping)`. The process is just waiting for the 300 seconds to pass, so it doesn't use the CPU.

**Interesting output**

```
Tasks: 118 total,   1 running, 117 sleeping,   0 stopped,   0 zombie
```

Out of 118 processes only 1 was running (top itself), all the others were sleeping.

## Part 3 - Memory

```
$ free -h
               total        used        free      shared  buff/cache   available
Mem:           842Mi       385Mi       145Mi       4.0Mi       451Mi       457Mi
Swap:             0B          0B          0B
$ cat /proc/meminfo | head -6
MemTotal:         862984 kB
MemFree:          149380 kB
MemAvailable:     468488 kB
Buffers:           22052 kB
Cached:           419072 kB
SwapCached:            0 kB
$ sleep 300 & PID=$!
$ grep VmRSS /proc/$PID/status
VmRSS:	    1832 kB
$ kill $PID
```

**1. How much RAM?**

The VM has 842 MiB total. Only 145 MiB is free, but 457 MiB is "available", because Linux uses the free RAM for cache (`buff/cache`) and gives it back when a program needs it.

**2. What is swap?**

Swap is space on the disk that the OS uses like extra RAM when the real RAM is full. It's much slower than RAM. On my VM swap is 0B, so there is no swap configured.

**3. VmRSS of sleep**

It uses 1832 kB, so about 1.8 MB. It surprised me a little because sleep does nothing. I think most of it are the libraries that every program loads (like libc), not sleep itself. I checked and `RssAnon` (the memory only for this process) was just 104 kB, the rest was `RssFile`.

## Part 4 - Devices and storage

```
$ df -h
Filesystem      Size  Used Avail Use% Mounted on
/dev/root        29G  1.6G   27G   6% /
tmpfs           422M     0  422M   0% /dev/shm
/dev/sda16      881M   64M  756M   8% /boot
/dev/sda15      105M  6.2M   99M   6% /boot/efi
$ lsblk
NAME    MAJ:MIN RM  SIZE RO TYPE MOUNTPOINTS
sda       8:0    0   30G  0 disk
|-sda1    8:1    0   29G  0 part /
|-sda14   8:14   0    4M  0 part
|-sda15   8:15   0  106M  0 part /boot/efi
`-sda16 259:0    0  913M  0 part /boot
sr0      11:0    1  628K  0 rom
$ du -sh ~/os-lab1
12K	/home/azureuser/os-lab1
$ mount | head -1
/dev/sda1 on / type ext4 (rw,relatime,discard,errors=remount-ro,commit=30)
```

**1. Which device is / on?**

`/dev/sda1`. In `df` it shows as `/dev/root`, but `mount` shows it's `/dev/sda1`. It's the 29G partition of the 30G disk and it uses ext4.

**2. One entry from /dev**

`/dev/null` - anything you write to it just disappears. Another one is `/dev/sda1`, which is the partition of the disk where my system is.

```
$ ls -l /dev/null /dev/sda1
crw-rw-rw- 1 root root 1, 3 Sep 29 11:07 /dev/null
brw-rw---- 1 root disk 8, 1 Sep 29 11:31 /dev/sda1
```

**3. "Everything is a file"**

In Linux disks, terminals and even processes (in `/proc`) show up as files, so you can use the same commands like `ls` and `cat` on all of them.

## Extra

I tried `htop` too, it's like `top` but with colors and bars for CPU and memory, easier to read.

I also ran `strace -c ls`:

```
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 22.92    0.000468          14        33        12 openat
 21.65    0.000442          14        30           mmap
 12.83    0.000262          11        23           close
 12.34    0.000252          11        22           fstat
 10.82    0.000221         221         1           execve
...
------ ----------- ----------- --------- --------- ----------------
100.00    0.002042          14       143        18 total
```

Just `ls` made 143 system calls. Most of them are `openat` and `mmap`, for loading the libraries. Only 1 `write` was for printing the result.

## Conclusion

The OS manages 4 resources: files, processes, memory and devices.
For files I used `ls -l` and for processes I used `ps aux`.
For memory I used `free -h` and for devices I used `lsblk`.
