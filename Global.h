#pragma once
#include <fltKernel.h>
#include <suppress.h>

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

#define MINIFILTER_FILTER_NAME     L"MiniTestFilter"
#define PPFILTER_FILE_POOLTAG      'mftg'

typedef struct _MINITEST_FILTER_DATA {

    //
    //  The filter handle that results from a call to
    //  FltRegisterFilter.
    //

    PFLT_FILTER FilterHandle;

} MINITEST_FILTER_DATA, * PMINITEST_FILTER_DATA;

//
//  Structure that contains all the global data structures
//  used throughout MinitestFilter.
//

extern MINITEST_FILTER_DATA MinitestFilterData;

FLT_PREOP_CALLBACK_STATUS PfltPreOperationCallbackCleanup(
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Out_ PVOID* CompletionContext
);