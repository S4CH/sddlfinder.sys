// This is template for basic device driver all credits goes to my mate, Rohit Dhill (https://github.com/s4dr0t1)

#include <ntddk.h>

// Define a custom IOCTL code
#define IOCTL_SEND_MESSAGE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Define the device and symbolic link names
UNICODE_STRING DeviceName;
UNICODE_STRING SymLinkName;

// Function to handle IOCTL requests
NTSTATUS DispatchIoControl(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp) {
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpSp;
    PVOID inputBuffer;
    ULONG inputBufferLength;
    PVOID outputBuffer;
    ULONG outputBufferLength;
    ULONG code;

    // Ignore unused parameters
    UNREFERENCED_PARAMETER(DeviceObject);

    // Get the current IRP stack location
    irpSp = IoGetCurrentIrpStackLocation(Irp);

    // Get input buffer and length
    inputBuffer = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;

    // Get output buffer and length
    outputBuffer = Irp->AssociatedIrp.SystemBuffer;
    outputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

    // Get the IOCTL code
    code = irpSp->Parameters.DeviceIoControl.IoControlCode;

    // Handle different IOCTL codes
    switch (code) {
    case IOCTL_SEND_MESSAGE:
        KdPrint(("Received IOCTL_SEND_MESSAGE: %ws\n", (WCHAR*)inputBuffer));
        break;
    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    // Set the IRP status and information
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;

    // Complete the IRP
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

// Function to unload the driver
void SampleUnload(_In_ PDRIVER_OBJECT DriverObject) {
    // Delete the symbolic link
    IoDeleteSymbolicLink(&SymLinkName);

    // Delete the device
    IoDeleteDevice(DriverObject->DeviceObject);

    KdPrint(("Driver unloaded\n"));
}

// Driver entry point
extern "C" NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);

    // Set the driver unload function
    DriverObject->DriverUnload = SampleUnload;

    // Initialize device and symbolic link names
    RtlInitUnicodeString(&DeviceName, L"\\Device\\MyDriver");
    RtlInitUnicodeString(&SymLinkName, L"\\DosDevices\\MyDriver");

    // Create the device object
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS status;

    status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Create the symbolic link
    status = IoCreateSymbolicLink(&SymLinkName, &DeviceName);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(DeviceObject);
        return status;
    }

    // Set the major functions for create, close, and IOCTL
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchIoControl;

    // Print a message to indicate successful driver initialization
    KdPrint(("My name is Sachin\n"));

    return STATUS_SUCCESS;
}
