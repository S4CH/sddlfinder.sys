#include <ntddk.h>
#include <ntstrsafe.h>

NTSTATUS GetSDDLStringFromINF(_In_ PUNICODE_STRING InfFilePath, _Out_ PUNICODE_STRING SddlString)
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE fileHandle = NULL;
    IO_STATUS_BLOCK ioStatus;
    OBJECT_ATTRIBUTES fileAttributes;
    LARGE_INTEGER fileSize;
    PVOID fileBuffer = NULL;

    RtlInitUnicodeString(SddlString, NULL);

    InitializeObjectAttributes(&fileAttributes, InfFilePath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ZwCreateFile(&fileHandle,
        FILE_READ_DATA | SYNCHRONIZE,
        &fileAttributes,
        &ioStatus,
        &fileSize,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0);

    if (!NT_SUCCESS(status)) {
        KdPrint(("Failed to open INF file: 0x%X\n", status));
        return status;
    }

    // Determine the size of the file
    fileSize.QuadPart = 0;
    status = ZwQueryInformationFile(fileHandle, &ioStatus, &fileSize, sizeof(fileSize), FileStandardInformation);

    if (!NT_SUCCESS(status)) {
        KdPrint(("Failed to query file size: 0x%X\n", status));
        ZwClose(fileHandle);
        return status;
    }

    // Allocate memory to read the file content
    fileBuffer = ExAllocatePool2(NonPagedPoolNx, (SIZE_T)fileSize.QuadPart, 'SDDL');

    if (fileBuffer == NULL) {
        KdPrint(("Memory allocation failed\n"));
        ZwClose(fileHandle);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Read the file content
    status = ZwReadFile(fileHandle, NULL, NULL, NULL, &ioStatus, fileBuffer, (ULONG)fileSize.QuadPart, NULL, NULL);

    if (!NT_SUCCESS(status)) {
        KdPrint(("Failed to read INF file: 0x%X\n", status));
        ExFreePool(fileBuffer);
        ZwClose(fileHandle);
        return status;
    }

    // Find the SDDL string in the file content (you may need to adjust this part)
    // Here, we assume that the SDDL string is within the first 4096 bytes of the file.
    PCWSTR sddlDelimiter = L"SDDL=";

    RtlInitUnicodeString(SddlString, NULL);

    if (wcsstr((PWSTR)fileBuffer, sddlDelimiter) != NULL) {
        WCHAR* start = wcsstr((PWSTR)fileBuffer, sddlDelimiter) + wcslen(sddlDelimiter);
        WCHAR* end = wcsstr(start, L"\r\n");

        if (end != NULL) {
            ULONG sddlLength = (ULONG)(end - start);
            if (sddlLength > 0) {
                SddlString->Buffer = (PWCH)ExAllocatePool2(NonPagedPoolNx, (sddlLength + 1) * sizeof(WCHAR), 'SDDL');
                if (SddlString->Buffer != NULL) {
                    RtlCopyMemory(SddlString->Buffer, start, sddlLength * sizeof(WCHAR));
                    SddlString->Buffer[sddlLength] = L'\0';
                    SddlString->Length = (USHORT)sddlLength * sizeof(WCHAR);
                    SddlString->MaximumLength = SddlString->Length + sizeof(WCHAR);
                }
            }
        }
    }

    ExFreePool(fileBuffer);
    ZwClose(fileHandle);

    return STATUS_SUCCESS;
}



NTSTATUS DriverInitialize()
// NTSTATUS DriverInitialize(_In_ PDRIVER_OBJECT DriverObject)
{
    // NTSTATUS status;

    // Suppose we want to create a device object
    // PDEVICE_OBJECT deviceObject;
    // UNICODE_STRING deviceName;
    // UNICODE_STRING sddlString;

    // Replace 'YourDeviceName' with your actual device name
    // RtlInitUnicodeString(&deviceName, L"\\Device\\YourDeviceName");

    // Replace 'YourSDDLString' with the SDDL string obtained earlier
    // RtlInitUnicodeString(&sddlString, L"YourSDDLString");

    // status = IoCreateDeviceSecure(
    //     DriverObject,                            // Driver object
    //     sizeof(DEVICE_EXTENSION),               // Device extension size
    //     &deviceName,                            // Device name
    //     FILE_DEVICE_UNKNOWN,                    // Device type
    //     0,                                      // Device characteristics
    //     FALSE,                                  // Exclusive
    //     &sddlString,                            // Security descriptor
    //     NULL,                                   // Device class GUID
    //     &deviceObject                           // Created device object
    // );

    // if (!NT_SUCCESS(status)) {
    //     KdPrint(("Failed to create device: 0x%X\n", status));
    //     return status;
    // }
    // Add any other driver-specific initialization code here.
    // For instance, you can initialize global variables or data structures.

    return STATUS_SUCCESS; // Replace with an appropriate status code
}


NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    UNICODE_STRING infFilePath;
    UNICODE_STRING sddlString;
    NTSTATUS status;

    RtlInitUnicodeString(&infFilePath, L"\\??\\C:\\Path\\To\\Your\\INF\\File.inf");  // Replace with the path to your INF file
    status = GetSDDLStringFromINF(&infFilePath, &sddlString);

    if (NT_SUCCESS(status)) {
        KdPrint(("SDDL String: %wZ\n", &sddlString));

        // Now you have the SDDL string in sddlString
        // You can use it as needed, for example, passing it to IoCreateDeviceSecure

        ExFreePool(sddlString.Buffer);
    }
    else {
        KdPrint(("Failed to retrieve SDDL String: 0x%X\n", status));
    }

    // Call your driver's initialization function
    status = DriverInitialize();

    if (!NT_SUCCESS(status)) {
        KdPrint(("Driver initialization failed: 0x%X\n", status));
        // Handle initialization failure here
    }

    return status;
}
