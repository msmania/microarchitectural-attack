#include <ntddk.h>
#include <stdio.h>
#include <stdarg.h>

extern "C" {
  DRIVER_INITIALIZE DriverEntry;
  DRIVER_UNLOAD DriverUnload;
  NTSTATUS MeltdownDispatchRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp);
}

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#pragma alloc_text(PAGE, MeltdownDispatchRoutine)
#endif

#define IOCTL_START_TIMER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x888, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define IOCTL_GET_ADDRESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x999, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

const char TheAnswer[] = "Answer to the Ultimate Question of Life, The Universe, and Everything is 42";
constexpr ULONG TAG = 'oOOo';
constexpr INT64 ONE_SECOND = 10000000;
LPSTR SuperConfidentialArea = nullptr;
KDPC g_dpc;
KTIMER g_timer;

VOID Log(_In_ PCCH Format, _In_ ...) {
  CHAR Message[512];
  va_list VaList;
  va_start(VaList, Format);
  CONST ULONG N = _vsnprintf_s(Message, sizeof(Message) - sizeof(CHAR), Format, VaList);
  Message[N] = '\0';
  vDbgPrintExWithPrefix("[MELTDOWN] ", DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, Message, VaList);
  va_end(VaList);
}

VOID TimerDPC(PKDPC, PVOID, PVOID, PVOID) {
  static UINT32 junk = 42;
  junk ^= *reinterpret_cast<PUINT32>(SuperConfidentialArea);
  ++junk;
  junk ^= *reinterpret_cast<PUINT32>(SuperConfidentialArea + 4);
}

NTSTATUS MeltdownDispatchRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
  UNREFERENCED_PARAMETER(DeviceObject);
  PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;

  switch (IrpStack->MajorFunction) {
  case IRP_MJ_CREATE:
    break;
  case IRP_MJ_CLOSE:
    break;
  case IRP_MJ_DEVICE_CONTROL:
    switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_START_TIMER:
      if (IrpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(ULONG32)) {
        if (auto stop_if_nonzero = reinterpret_cast<ULONG32*>(Irp->AssociatedIrp.SystemBuffer)) {
          if (*stop_if_nonzero) {
            KeCancelTimer(&g_timer);
          }
          else {
            LARGE_INTEGER due;
            due.QuadPart = -ONE_SECOND;
            KeSetTimerEx(&g_timer, due, 10, &g_dpc);
          }
        }
      }
      break;
    case IOCTL_GET_ADDRESS:
      if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(PVOID)) {
        if (auto p = reinterpret_cast<PVOID*>(Irp->AssociatedIrp.SystemBuffer)) {
          *p = SuperConfidentialArea;
          Irp->IoStatus.Information = sizeof(PVOID);
        }
      }
      break;
    }
    break;
  }

  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  return Irp->IoStatus.Status;
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject,
                     _In_ PUNICODE_STRING RegistryPath) {
  PAGED_CODE();
  UNREFERENCED_PARAMETER(RegistryPath);
  NTSTATUS status = STATUS_SUCCESS;
  UNICODE_STRING deviceName;
  UNICODE_STRING linkName;
  RtlInitUnicodeString(&deviceName, L"\\Device\\meltdown");
  RtlInitUnicodeString(&linkName, L"\\DosDevices\\meltdown");

  DriverObject->DriverUnload = DriverUnload;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = MeltdownDispatchRoutine;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = MeltdownDispatchRoutine;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MeltdownDispatchRoutine;

  PDEVICE_OBJECT deviceObject = nullptr;
  status = IoCreateDevice(DriverObject,
                          /*DeviceExtensionSize*/0,
                          &deviceName,
                          FILE_DEVICE_UNKNOWN,
                          /*DeviceCharacteristics*/0,
                          /*Exclusive*/FALSE,
                          &deviceObject);
  if (!NT_SUCCESS(status))  {
    Log("IoCreateDevice failed - %08x\n", status);
    goto cleanup;
  }

  status = IoCreateSymbolicLink(&linkName, &deviceName);
  if (!NT_SUCCESS(status)) {
    Log("IoCreateSymbolicLink failed - %08x\n", status);
    goto cleanup;
  }

  SuperConfidentialArea = static_cast<LPSTR>(ExAllocatePoolWithTag(NonPagedPoolNx, 4096, TAG));
  RtlCopyMemory(SuperConfidentialArea, TheAnswer, sizeof(TheAnswer));
  Log("Secret is at %p\n", SuperConfidentialArea);

  KeInitializeDpc(&g_dpc, TimerDPC, nullptr);
  KeInitializeTimer(&g_timer);

cleanup:
  if (!NT_SUCCESS(status) && deviceObject) {
    IoDeleteDevice(deviceObject);
  }
  return status;
}

VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject) {
  PAGED_CODE();

  KeCancelTimer(&g_timer);

  if (SuperConfidentialArea) {
    ExFreePoolWithTag(SuperConfidentialArea, 'oOOo');
    SuperConfidentialArea = nullptr;
  }

  UNICODE_STRING linkName;
  RtlInitUnicodeString(&linkName, L"\\DosDevices\\meltdown");
  IoDeleteSymbolicLink(&linkName);
  IoDeleteDevice(DriverObject->DeviceObject);

  Log("Driver unloaded.\n");
}
