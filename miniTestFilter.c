#include "Global.h"

DRIVER_INITIALIZE DriverEntry;

MINITEST_FILTER_DATA MinitestFilterData;

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS
MinitestUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
MinitestQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

NTSTATUS MinitestInstanceSetup(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, MinitestUnload)
#pragma alloc_text(PAGE, MinitestQueryTeardown)
#endif

//
//  This defines operations we want to procceed
//

CONST FLT_OPERATION_REGISTRATION operationRegistration[] =
{
    {
        IRP_MJ_CLEANUP,
        0,
        PfltPreOperationCallbackCleanup,
        NULL,
        NULL
    },

    {IRP_MJ_OPERATION_END}
};

//
//  This defines what we want to filter with FltMgr
//


CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    NULL,                               //  Context
    operationRegistration,              //  Operation callbacks

    MinitestUnload,                     //  FilterUnload

    MinitestInstanceSetup,              //  InstanceSetup
    MinitestQueryTeardown,              //  InstanceQueryTeardown
    NULL,                               //  InstanceTeardownStart
    NULL,                               //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};


/*************************************************************************
    Filter initialization and unload routines.
*************************************************************************/

/*++

Routine Description:

    This is the initialization routine for this miniFilter driver. This
    registers the miniFilter with FltMgr and initializes all
    its global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.
    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Returns STATUS_SUCCESS.

--*/
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )

{
    KdPrint(("[MF_TEST] Start Driver Entry\n"));
    NTSTATUS status;

    UNREFERENCED_PARAMETER( RegistryPath );

    //
    //  Register with FltMgr
    //

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &MinitestFilterData.FilterHandle );


    FLT_ASSERT( NT_SUCCESS(status) );

    if (NT_SUCCESS(status))
    {

        status = FltStartFiltering( MinitestFilterData.FilterHandle );

        if (!NT_SUCCESS(status))
        {
            FltUnregisterFilter( MinitestFilterData.FilterHandle );
        }
    }

    KdPrint(("[MF_TEST] End Driver Entry\n"));
    return status;
}


/*++

Routine Description:

    This is the unload routine for this miniFilter driver. This is called
    when the minifilter is about to be unloaded. We can fail this unload
    request if this is not a mandatory unloaded indicated by the Flags
    parameter.

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns the final status of this operation.

--*/
NTSTATUS
MinitestUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )

{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    FltUnregisterFilter( MinitestFilterData.FilterHandle );

    return STATUS_SUCCESS;
}


/*++

Routine Description:

    This is the instance detach routine for this miniFilter driver.
    This is called when an instance is being manually deleted by a
    call to FltDetachVolume or FilterDetach thereby giving us a
    chance to fail that detach request.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Indicating where this detach request came from.

Return Value:

    Returns the status of this operation.

--*/
NTSTATUS
MinitestQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    KdPrint(("[MF_TEST] MinitestQueryTeardown\n"));
    PAGED_CODE();

    return STATUS_SUCCESS;
}


/*++

Routine Description:

    This is the instance setup routine for minifilter driver.
    The filter manager calls this routine to allow the minifilter driver 
    to respond to an automatic or manual attachment request. If this routine 
    returns an error or warning NTSTATUS code, the minifilter driver instance 
    is not attached to the given volume. Otherwise, the minifilter driver instance 
    is attached to the given volume.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Bitmask of flags that indicate why the instance is being attached.

    VolumeDeviceType - Device type of the file system volume

    VolumeFilesystemType - File system type of the volume

Return Value:

    Returns the status of this operation.

--*/
NTSTATUS MinitestInstanceSetup(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(VolumeDeviceType);
    UNREFERENCED_PARAMETER(VolumeFilesystemType);

    PAGED_CODE();
    
    if (VolumeDeviceType != FILE_DEVICE_DISK_FILE_SYSTEM)
        return STATUS_FLT_DO_NOT_ATTACH;

    KdPrint(("[MF_TEST] MinitestInstanceAttach\n"));

    return STATUS_SUCCESS;
}
