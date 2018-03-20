#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileSystemInfo.h>
#include <Library/DevicePathLib.h>

#define KEY_INTERRUPT(keys, scan, uni) ((((UINT64)keys) << 32) | ((scan) << 16) | (uni))
#define EFI_TIMER_DURATION(Seconds) MultU64x32((UINT64)(Seconds), 10000000)


typedef struct tempname1
{
    CHAR16* path;
    CHAR16* label;
} OS_LOADER_ENTRY_POINT;

typedef struct tempname2
{
    EFI_HANDLE device;
    CHAR16* path;
    CHAR16* name;
    CHAR16* label;
    UINT64 size;
} OS_ENTRY_RECORD;

typedef struct tempname3
{
    char timeout;
    unsigned int entries;
    OS_ENTRY_RECORD* systems;
} CONTEXT_CALLBACK;

EFI_SYSTEM_TABLE *SYSTEM_TABLE_GLOBAL;
EFI_BOOT_SERVICES *BOOT_SERVICE_GLOBAL;
EFI_HANDLE IMAGE_HANDLER_GLOBAL;
