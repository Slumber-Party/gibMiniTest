#include "Global.h"

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CheckDirMatch(
    _In_ PFLT_CALLBACK_DATA CallbackData
)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;
    PFLT_FILE_NAME_INFORMATION fileInfo;
    PFLT_FILE_NAME_INFORMATION* pFileInfo;
    UNICODE_STRING targetDir;

    CONST WCHAR targetDirBuff[] = L"\\Users\\TestUser\\Downloads\\";

    pFileInfo = (PFLT_FILE_NAME_INFORMATION*)ExAllocatePool(PagedPool, sizeof(PFLT_FILE_NAME_INFORMATION));

    if (pFileInfo == NULL)
    {
        KdPrint(("PFLT_FILE_NAME_INFORMATION allocation error\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = FltGetFileNameInformation(CallbackData, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP, pFileInfo);

    if (!NT_SUCCESS(status))
    {
        if (status == STATUS_FLT_NAME_CACHE_MISS)
        {
            status = FltGetFileNameInformation(CallbackData, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY, pFileInfo);

            if (!NT_SUCCESS(status))
            {
                KdPrint(("FltGetFileNameInformation 2nd call failed (after STATUS_FLT_NAME_CACHE_MISS failure for first) with status: %u\n", status));
                ExFreePool(pFileInfo);
                return status;
            }
        }
        else
        {
            KdPrint(("FltGetFileNameInformation failed with status: %u\n", status));
            ExFreePool(pFileInfo);
            return status;
        }
    }

    fileInfo = *pFileInfo;

    status = FltParseFileNameInformation(fileInfo);

    if (!NT_SUCCESS(status))
    {
        KdPrint(("FltParseFileNameInformation failed with status: %u\n", status));
        FltReleaseFileNameInformation(fileInfo);
        ExFreePool(pFileInfo);
        return status;
    }

    if (fileInfo->ParentDir.Length != 0)
    {
        RtlInitUnicodeString(&targetDir, targetDirBuff);

        if (!RtlEqualUnicodeString(&fileInfo->ParentDir, &targetDir, TRUE))
        {
            FltReleaseFileNameInformation(fileInfo);
            ExFreePool(pFileInfo);
            return STATUS_INVALID_DISPOSITION;
        }
        else
        {
          //  KdPrint(("%wZ targetDir == %wZ file info dir. Name: %wZ\n", &targetDir, &fileInfo->ParentDir,&fileInfo->Name));
        }
    }
    else
    {
        FltReleaseFileNameInformation(fileInfo);
        ExFreePool(pFileInfo);
        return STATUS_INVALID_DISPOSITION;
    }

    FltReleaseFileNameInformation(fileInfo);
    ExFreePool(pFileInfo);

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS GetFileNameWithExt(
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _Out_ PUNICODE_STRING fileNameWithExt
)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;
    PFLT_FILE_NAME_INFORMATION fileInfo;
    PFLT_FILE_NAME_INFORMATION* pFileInfo;
    WCHAR targetDir[] = L"\\DosDevices\\C:\\Windows\\Temp\\";

    CONST ULONG size = sizeof(targetDir) + 256*sizeof(WCHAR);

    pFileInfo = (PFLT_FILE_NAME_INFORMATION*)ExAllocatePool(PagedPool, sizeof(PFLT_FILE_NAME_INFORMATION));

    if (pFileInfo == NULL)
    {
        KdPrint(("PFLT_FILE_NAME_INFORMATION allocation error\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = FltGetFileNameInformation(CallbackData, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP, pFileInfo);

    if (!NT_SUCCESS(status))
    {
        if (status == STATUS_FLT_NAME_CACHE_MISS)
        {
            status = FltGetFileNameInformation(CallbackData, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY, pFileInfo);

            if (!NT_SUCCESS(status))
            {
                KdPrint(("FltGetFileNameInformation 2nd call failed (after STATUS_FLT_NAME_CACHE_MISS failure for first) with status: %u\n", status));
                ExFreePool(pFileInfo);
                return status;
            }
        }
        else
        {
            KdPrint(("FltGetFileNameInformation failed with status: %u\n", status));
            ExFreePool(pFileInfo);
            return status;
        }
    }

    fileInfo = *pFileInfo;

    status = FltParseFileNameInformation(fileInfo);

    if (!NT_SUCCESS(status))
    {
        KdPrint(("FltParseFileNameInformation failed with status: %u\n", status));
        FltReleaseFileNameInformation(fileInfo);
        ExFreePool(pFileInfo);
        return status;
    }

    PWCHAR buff = (PWCHAR)ExAllocatePool(PagedPool, size);

    if (buff == NULL)
    {
        KdPrint(("PFLT_FILE_NAME_INFORMATION allocation error\n"));
        FltReleaseFileNameInformation(fileInfo);
        ExFreePool(pFileInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitEmptyUnicodeString(fileNameWithExt, buff, size);

    
    RtlAppendUnicodeToString(fileNameWithExt, targetDir);
    RtlAppendUnicodeStringToString(fileNameWithExt, &fileInfo->FinalComponent);

    FltReleaseFileNameInformation(fileInfo);
    ExFreePool(pFileInfo);

    return status;
}

NTSTATUS HandlePreCleanup(
    _In_ PFLT_CALLBACK_DATA Data
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFLT_INSTANCE Instance;
    PFILE_OBJECT FileObject;
    LARGE_INTEGER FileOffset;
    LARGE_INTEGER size;
    ULONG BytesRead;
    PVOID buff = NULL;

    OBJECT_ATTRIBUTES objAttr;
    IO_STATUS_BLOCK ioSb, writeIoSb;
    HANDLE hFile;
    UNICODE_STRING fileName;

    FileObject = Data->Iopb->TargetFileObject;
    Instance = Data->Iopb->TargetInstance;
    
    Status = FsRtlGetFileSize(FileObject, &size);

    if (!NT_SUCCESS(Status))
    {
        KdPrint(("Failed FsRtlGetFileSize with status %X\n",Status));
        goto out;
    }

    buff = ExAllocatePoolWithTag(PagedPool, size.QuadPart, PPFILTER_FILE_POOLTAG);

    if (buff == NULL) {
        KdPrint(("Failed allocating file buffer\n"));
        Status = STATUS_MEMORY_NOT_ALLOCATED;
        goto out;
    }

    FileOffset.QuadPart = 0;

    Status = FltReadFile(
       Instance, FileObject, &FileOffset,
        size.QuadPart, buff,
        FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET |
        FLTFL_IO_OPERATION_NON_CACHED,
        &BytesRead, NULL, NULL
    );

    if (!NT_SUCCESS(Status) && Status != STATUS_END_OF_FILE)
    {
        KdPrint(("Failed reading from file %wZ: error %d\n",
            &FileObject->FileName, Status));
        goto out;
    }

    Status = GetFileNameWithExt(Data, &fileName);

    if (!NT_SUCCESS(Status))
    {
        KdPrint(("Failed to get file name with ext\n"));
        if(fileName.Length != 0)
            ExFreePool(fileName.Buffer);
        goto out;
    }

    InitializeObjectAttributes(&objAttr, &fileName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL, NULL);

    Status = FltCreateFile(
        MinitestFilterData.FilterHandle,
        Instance,
        &hFile,
        GENERIC_WRITE,
        &objAttr,
        &ioSb,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_VALID_FLAGS,
        FILE_OVERWRITE_IF,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
        NULL,
        0,
        IO_IGNORE_SHARE_ACCESS_CHECK);

    if (!NT_SUCCESS(Status))
    {
        KdPrint(("Failed creating file %wZ: error %d\n",
            &fileName, Status));
        ExFreePool(fileName.Buffer);
        goto out;
    }

    Status = ZwWriteFile(hFile, NULL, NULL, NULL, &writeIoSb, buff, size.QuadPart, NULL, NULL);

    if (!NT_SUCCESS(Status))
    {
        KdPrint(("Failed writing file %wZ: error %d\n",
            &fileName, Status));
    }

    ExFreePool(fileName.Buffer);
    FltClose(hFile);
out:
    if (buff != NULL) {
        ExFreePoolWithTag(buff, PPFILTER_FILE_POOLTAG);
    }

    return Status;
}

VOID WorkRoutine(
    _In_ PFLT_DEFERRED_IO_WORKITEM WorkItem,
    _In_ PFLT_CALLBACK_DATA Data,
    _In_opt_ PVOID Context)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID PostContext = NULL;

    UNREFERENCED_PARAMETER(Context);

    switch (Data->Iopb->MajorFunction) {
    case IRP_MJ_CLEANUP:
        Status = HandlePreCleanup(Data);
        break;
    default:
        NT_ASSERTMSG("Unexpected deferred pre callback operation",
            FALSE);
        break;
    }

    FltCompletePendedPreOperation(Data,
        FLT_PREOP_SUCCESS_WITH_CALLBACK,
        PostContext);
    FltFreeDeferredIoWorkItem(WorkItem);
}

FLT_PREOP_CALLBACK_STATUS PfltPreOperationCallbackCleanup(
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Out_ PVOID* CompletionContext
)
{
    UNREFERENCED_PARAMETER(FltObjects);

    NTSTATUS Status = STATUS_SUCCESS;
    PFLT_DEFERRED_IO_WORKITEM WorkItem = NULL;
    CompletionContext = NULL;

    if (!NT_SUCCESS(CheckDirMatch(Data)))
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    WorkItem = FltAllocateDeferredIoWorkItem();
    if (WorkItem == NULL) {
        Status = STATUS_MEMORY_NOT_ALLOCATED;
        KdPrint(("Failed allocating work item\n"));
        goto failed;
    }

    Status = FltQueueDeferredIoWorkItem(WorkItem, Data, WorkRoutine,
        CriticalWorkQueue, NULL);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("Failed queuing work item to queue: error %d\n",
            Status));
        goto failed;
    }

    return FLT_PREOP_PENDING;

failed:
    if (WorkItem != NULL) {
        FltFreeDeferredIoWorkItem(WorkItem);
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}