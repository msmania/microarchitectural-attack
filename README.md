# Meltdown/Spectre Proof-of-Concept for Windows

This repository contains PoC codes to demonstrate the famous Meltdown/Spectre vulnerability.

## Requirements to build

Don't worry, all free :smiley:.

- Visual Studio Community 2017<br />[https://www.visualstudio.com/downloads/](https://www.visualstudio.com/downloads/)
- Windows 10 SDK<br />[https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk)
- WDK for Windows 10<br />[https://developer.microsoft.com/en-us/windows/hardware/download-kits-windows-hardware-development](https://developer.microsoft.com/en-us/windows/hardware/download-kits-windows-hardware-development)

## Build

Launch "x64 Native Tools Command Prompt for VS 2017" and run the following commands.  Binaries will be generated under the subdirectory "bin\amd64".

The current code supports x64 only.  32bit compilation will fail.

```
> git clone git@github.com:msmania/microarchitectural-attack.git
> cd microarchitectural-attack
> nmake
```
