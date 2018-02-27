#include <windows.h>
#include <stdio.h>

#pragma pack(push, 8)
struct RTL_PROCESS_MODULE_INFORMATION {
  HANDLE Section;
  PVOID MappedBase;
  PVOID ImageBase;
  ULONG ImageSize;
  ULONG Flags;
  USHORT LoadOrderIndex;
  USHORT InitOrderIndex;
  USHORT LoadCount;
  USHORT OffsetToFileName;
  UCHAR FullPathName[256];
};

struct SYSTEM_MODULE_INFORMATION {
  ULONG ModulesCount;
  RTL_PROCESS_MODULE_INFORMATION Modules[256];
};
#pragma pack(pop)

#define IOCTL_START_TIMER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x888, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define IOCTL_GET_ADDRESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x999, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

void CollectKernelInfo() {
  auto NtQuerySystemInformation =
    reinterpret_cast<NTSTATUS (WINAPI*)(DWORD, PVOID, ULONG, PULONG)>(
      GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtQuerySystemInformation"));

  constexpr DWORD SystemModuleInformation = 11;
  SYSTEM_MODULE_INFORMATION module_info{};
  DWORD bytes_returned;
  NTSTATUS status = NtQuerySystemInformation(SystemModuleInformation,
                                             &module_info,
                                             sizeof(module_info),
                                             &bytes_returned);
  if (status == 0) {
    auto kernel_on_user =
      reinterpret_cast<LPBYTE>(LoadLibraryEx(L"ntoskrnl.exe",
                                             /*hFile*/nullptr,
                                             LOAD_LIBRARY_AS_IMAGE_RESOURCE));
    kernel_on_user -= 2;
    printf("FullPathName: %hs\n"
           "ImageBase:    %p\n"
           "ImageSize:    %08x\n",
           module_info.Modules[0].FullPathName,
           module_info.Modules[0].ImageBase,
           module_info.Modules[0].ImageSize);

    DWORD offset = 0x0051d000;
    auto pk = reinterpret_cast<LPBYTE>(module_info.Modules[0].ImageBase) + offset;
    auto pu = kernel_on_user + offset;
    printf("%p (= nt + %08x ):\n", pk, offset);
    for (int i = 0; i < 16; ++i) printf(" %02x", *(pu++));
    putchar('\n');
  }
}

class MeltdownDriver final {
private:
  HANDLE device_;

public:
  MeltdownDriver() {
    device_ = CreateFile(L"\\\\.\\meltdown",
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ,
                         /*lpSecurityAttributes*/nullptr,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         /*hTemplateFile*/nullptr);
    if (device_ == INVALID_HANDLE_VALUE) {
      printf("CreateFile failed - %08x\n", GetLastError());
    }
  }

  ~MeltdownDriver() {
    if (device_ != INVALID_HANDLE_VALUE) CloseHandle(device_);
  }

  operator bool() const {
    return device_ != INVALID_HANDLE_VALUE;
  }

  LPVOID GetSecretAddress() const {
    DWORD dw;
    LPVOID ret = nullptr;
    DeviceIoControl(device_,
                    IOCTL_GET_ADDRESS,
                    /*lpInBuffer*/nullptr,
                    /*nInBufferSize*/0,
                    /*lpOutBuffer*/&ret,
                    /*nOutBufferSize*/sizeof(ret),
                    /*lpBytesReturned*/&dw,
                    /*lpOverlapped*/nullptr);
    return ret;
  }

  void StartTimer(DWORD stop_if_nonzero) const {
    DWORD dw;
    DeviceIoControl(device_,
                    IOCTL_START_TIMER,
                    /*lpInBuffer*/&stop_if_nonzero,
                    /*nInBufferSize*/sizeof(stop_if_nonzero),
                    /*lpOutBuffer*/nullptr,
                    /*nOutBufferSize*/0,
                    /*lpBytesReturned*/&dw,
                    /*lpOverlapped*/nullptr);
  }
};

void ShowUsage() {
  printf("USAGE: mdc [COMMAND]\n\n"
         "  --info\n"
         "  --timer_start\n"
         "  --timer_stop\n"
         );
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    ShowUsage();
    return 1;
  }

  enum {invalid, timer_start, timer_stop, info} command = invalid;
  if (strcmp(argv[1], "--info") == 0)
    command = info;
  else if (strcmp(argv[1], "--timer_start") == 0)
    command = timer_start;
  else if (strcmp(argv[1], "--timer_stop") == 0)
    command = timer_stop;

  if (command != invalid) {
    MeltdownDriver driver;
    if (driver) {
      switch (command) {
      case info:
        CollectKernelInfo();
        printf("\nSecret data should be placed at %p\n",
               driver.GetSecretAddress());
        break;
      case timer_start:
        driver.StartTimer(/*stop_if_nonzero*/0);
        break;
      case timer_stop:
        driver.StartTimer(/*stop_if_nonzero*/1);
        break;
      }
    }
  }
  else {
    ShowUsage();
    return 1;
  }
  return 0;
}
