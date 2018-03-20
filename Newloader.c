#include "uefibootloader.h"

const char timeout = 5;
const unsigned char no_of_os_loaders = 6;
const OS_LOADER_ENTRY_POINT loader_entry_point[] =
        {
			{ 
				.path=L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi", 
				.label=L"Microsoft Windows" 
			}, 
        	{ 
				.path=L"\\EFI\\ubuntu\\grubx64.efi", 
				.label=L"Linux Ubuntu" 
			}, 
        	{ 
				.path=L"\\EFI\\debian\\grubx64.efi", 
				.label=L"Linux Debian" 
			}, 
        	{ 
				.path=L"\\EFI\\fedora\\grubx64.efi", 
				.label=L"Linux Fedora" 
			},
        	{ 
				.path=L"\\System\\Library\\CoreServices\\boot.efi", 
				.label=L"Mac OS" 
			},
        	{ 
				.path=L"\\shellx64.efi", 
				.label=L"EFI Shell" 
			}
		};
        

unsigned int FETCH_ENTRIES(OS_ENTRY_RECORD* os, unsigned char firstKey);
EFI_STATUS FETCH_KEY_PRESS(UINT64* key);
EFI_STATUS OS_MENU_ENTRY_CALL(OS_ENTRY_RECORD* os, unsigned int index);
EFI_STATUS LOAD_OPERATING_SYSTEM(OS_ENTRY_RECORD sys);
EFI_STATUS CLOSE_BOOTLOADER();
EFI_STATUS CLEAR_CONSOLE();
VOID EFIAPI CALL_BACK_TIMER(EFI_EVENT event, void* context);


EFI_STATUS EFIAPI UefiMain (IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable)
{
    IMAGE_HANDLER_GLOBAL = ImageHandle;
    SYSTEM_TABLE_GLOBAL = SystemTable;
    BOOT_SERVICE_GLOBAL = SYSTEM_TABLE_GLOBAL->BootServices;
    
    EFI_STATUS error;
    
    error = CLEAR_CONSOLE();
    if (error == EFI_SECURITY_VIOLATION || error == EFI_ACCESS_DENIED) 
    {   
        Print(L"Error Occured. Unable to Clean Console!\n");
    }
    
    const CHAR16 LOGO[][72] =
        {
			{L"\n"},
			{L"U   B   L\n"}, 
	        {L"E   O   OA\n"},
        	{L"F   O   DE\n"},
       	 	{L"I   T   R\n"},
        	{L"\n"}
	};
    unsigned char ch;
    for(ch = 0; ch < 6; ++ch)
    {
        Print(LOGO[ch]);
    }
    Print(L"\tUEFI Bootloader\n");
            
    OS_ENTRY_RECORD os[20];
    unsigned int num_of_menu_entries = 1;
    
    unsigned int numOfLoaders = FETCH_ENTRIES(os, num_of_menu_entries);
    num_of_menu_entries += numOfLoaders;
    
    unsigned short int i;
    Print(L"0 - exit\n");
    for (i = 1; i< num_of_menu_entries; i++)
    {
        Print(L"%d - %s on filesystem with label \"%s\" and size %dMB \n", i, os[i].name, os[i].label, os[i].size>>20);
    }
    
    EFI_EVENT timer;
    if(num_of_menu_entries > 1)
    {
        CONTEXT_CALLBACK context = {timeout, num_of_menu_entries, os};
        
        error = BOOT_SERVICE_GLOBAL->CreateEventEx(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, &CALL_BACK_TIMER, &context, NULL, &timer);
        if(error == EFI_INVALID_PARAMETER)    
            Print(L"Invalid parameters for timer creation\n");        
         
        error = BOOT_SERVICE_GLOBAL->SetTimer(timer, TimerPeriodic, EFI_TIMER_DURATION(1));
        if (error == EFI_INVALID_PARAMETER)
            Print(L"Invalid parameters for SetTimer\n");
    }
    else
    {
        Print(L"No operating systems found!\n");
    }
    
    UINT64 key;
    do 
    {
        error = FETCH_KEY_PRESS(&key);
        BOOT_SERVICE_GLOBAL->CloseEvent(timer);
        if (error == EFI_SECURITY_VIOLATION || error == EFI_ACCESS_DENIED) 
        {   
            Print(L"Key read error!\n");    
        }
        
    } while (!(key-48 >=0 && key-48 < num_of_menu_entries));
    
    if(key-48 == 0)
        error = CLOSE_BOOTLOADER();
    else
        error = OS_MENU_ENTRY_CALL(os, key-48);
    
    if ( error == EFI_SECURITY_VIOLATION || error == EFI_ACCESS_DENIED) 
    {   
        Print(L"Operating System Loader file Access Error!\n");  
    }

    return EFI_SUCCESS;

}
EFI_STATUS CLEAR_CONSOLE()
{
    return SYSTEM_TABLE_GLOBAL->ConOut->ClearScreen(SYSTEM_TABLE_GLOBAL->ConOut);
}

EFI_STATUS FETCH_KEY_PRESS(UINT64* key)
{
    UINTN index;
    EFI_INPUT_KEY ikey;
    EFI_STATUS error;
    BOOT_SERVICE_GLOBAL->WaitForEvent(1, &SYSTEM_TABLE_GLOBAL->ConIn->WaitForKey, &index);

    error = SYSTEM_TABLE_GLOBAL->ConIn->ReadKeyStroke(SYSTEM_TABLE_GLOBAL->ConIn, &ikey);
    if (EFI_ERROR(error))
        return error;

    *key = KEY_INTERRUPT(0, ikey.ScanCode, ikey.UnicodeChar);
    return EFI_SUCCESS;
}

VOID EFIAPI CALL_BACK_TIMER(EFI_EVENT event, void* context)
{
    CONTEXT_CALLBACK* ctxt = (CONTEXT_CALLBACK*)context;
    Print(L"%d...", ctxt->timeout--);
    if(ctxt->timeout < 0)
    {
        EFI_STATUS error;
        Print(L"\n");
        if(ctxt->entries > 1)
        {
            error = OS_MENU_ENTRY_CALL(ctxt->systems, 1);
            if (error == EFI_SECURITY_VIOLATION || error == EFI_ACCESS_DENIED) 
            {   
                Print(L"Loader file access error!\n");  
            }
        }
        else
        {
            error = CLOSE_BOOTLOADER();
        }
    }
}

EFI_STATUS OS_MENU_ENTRY_CALL(OS_ENTRY_RECORD* os, unsigned int index)
{
    Print(L"\nLoading %s", os[index].name);
    return LOAD_OPERATING_SYSTEM(os[index]);
}

EFI_STATUS CLOSE_BOOTLOADER()
{
    Print(L"Exiting EFI BootLoader\n");
    return EFI_SUCCESS;
}

EFI_STATUS LOAD_OPERATING_SYSTEM(OS_ENTRY_RECORD system)
{
    EFI_HANDLE image;
    EFI_DEVICE_PATH* path;
    EFI_STATUS error;
    path = FileDevicePath(system.device, system.path); 
	/**
		typedef EFI_STATUS LoadImage 
 		(
  			IN BOOLEAN BootPolicy,
  			IN EFI_HANDLE ParentImageHandle,
  			IN EFI_DEVICE_PATH_PROTOCOL *DevicePath,
  			IN VOID *SourceBuffer OPTIONAL,
  			IN UINTN SourceSize,
  			OUT EFI_HANDLE *ImageHandle
  		);
	**/
    BOOT_SERVICE_GLOBAL->LoadImage(FALSE, IMAGE_HANDLER_GLOBAL, path, NULL, 0, &image);
    
    error = CLEAR_CONSOLE();
    if (error == EFI_SECURITY_VIOLATION || error == EFI_ACCESS_DENIED) 
    {   
        Print(L"Unable to clear screen!\n");
    }
    /**
		typedef EFI_STATUS StartImage 
		(
  			IN  EFI_HANDLE ImageHandle,
  			OUT UINTN      *ExitDataSize,
  			OUT CHAR16     **ExitData OPTIONAL
  		);
   **/
    error = BOOT_SERVICE_GLOBAL->StartImage(image, NULL, NULL);  

    return error;
}

unsigned int FETCH_ENTRIES(OS_ENTRY_RECORD *os, unsigned char first_index)
{
    unsigned int noe = 0;
    EFI_GUID efi_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_GUID fsi_guid = EFI_FILE_SYSTEM_INFO_ID;
    UINTN length = 1000;
    EFI_HANDLE devices[length];

  	 /**
		typedef EFI_STATUS LocateHandle 
		(
  			IN     EFI_LOCATE_SEARCH_TYPE SearchType,
  			IN     EFI_GUID               *Protocol OPTIONAL,
  			IN     VOID                   *SearchKey OPTIONAL,
  			IN OUT UINTN                  *BufferSize,
  			OUT    EFI_HANDLE             *Buffer
  		);
  	 **/
    BOOT_SERVICE_GLOBAL->LocateHandle(2, &efi_guid, 0, &length, devices);
    length = length/sizeof(EFI_HANDLE);
    unsigned int i;
    for (i = 0; i < length; ++i)
    {
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* filesystem;
        EFI_FILE_PROTOCOL* root;
	
		UINTN buffer_size = 200;
		UINTN *buffer[buffer_size];
    	/**
   			typedef EFI_STATUS HandleProtocol 
			(
  				IN  EFI_HANDLE Handle,
  				IN  EFI_GUID   *Protocol,
  				OUT VOID       **Interface
  			);
  		 **/    
		BOOT_SERVICE_GLOBAL->HandleProtocol(devices[i], &efi_guid, (void **) &filesystem);
        filesystem->OpenVolume(filesystem, &root); /** To open a volume of media device **/
		
		EFI_STATUS recorded_device_info = root->GetInfo(root,&fsi_guid, &buffer_size,(void *) buffer);
		EFI_FILE_PROTOCOL* file;
        
		unsigned int j;
        for(j = 0; j < no_of_os_loaders; ++j)
        {
            if(root->Open(root,&file, loader_entry_point[j].path, EFI_FILE_MODE_READ, 0ULL) == EFI_SUCCESS)
            {
                file->Close(file);
				CHAR16 * label=L"<no label>";
				UINT64 size=0;
				if (recorded_device_info == EFI_SUCCESS)
				{
					EFI_FILE_SYSTEM_INFO *fi = (EFI_FILE_SYSTEM_INFO *) buffer;
					size = fi->VolumeSize;
					label = fi->VolumeLabel;
				}
                
				OS_ENTRY_RECORD sys = 	{
											.device = devices[i], 
											.path = loader_entry_point[j].path, 
											.name = loader_entry_point[j].label, 
											.label = label, 
											.size = size
									 	};
                os[first_index + noe] = sys;
                noe++;
            }
        }
    }
    return noe;
}
