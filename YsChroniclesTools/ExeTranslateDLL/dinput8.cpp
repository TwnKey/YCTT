#undef max
#define DIRECTINPUT_VERSION 0x0800
#include <cstdint>
#include "Console.h"
#include "Patch.h"
#include <dinput.h>
#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>
#include <windows.h>
#include <psapi.h>
#include <tchar.h>

#define DLL_NAME dinput8
#define API_NAME DirectInput8Create
#define HOOKED_API Hooked_DirectInput8Create
#define PARAMS_DCL (HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID * ppvOut, LPUNKNOWN punkOuter)
#define PARAMS_CALL (hinst, dwVersion, riidltf, ppvOut, punkOuter)
#define ERR_CODE 0x80070057

typedef int __cdecl EdVoiceAPIType();
HMODULE base_address;

typedef HRESULT(WINAPI* IDirectInputDevice_GetDeviceData_t)(IDirectInputDevice8*, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);
IDirectInputDevice_GetDeviceData_t fnGetDeviceData = NULL;

FARPROC p;
HINSTANCE hL = 0;
Patch* P = NULL;
bool Patched = false;
typedef int __cdecl EdVoiceAPIType();
typedef HRESULT WINAPI HookedAPIType PARAMS_DCL;
static HookedAPIType* api_ori = NULL;


BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID v)
{
	
	if (reason == DLL_PROCESS_ATTACH)
	{

		if (!api_ori) {
			base_address = GetModuleHandle(NULL);
			//Console *c = new Console();
			CHAR szPath[MAX_PATH];
			if (!GetSystemDirectoryA(szPath, sizeof(szPath) - 20))
				return FALSE;
			strcat_s(szPath, "\\dinput8.dll");

			HMODULE dll = LoadLibraryA(szPath);
			if (dll) {
				api_ori = (HookedAPIType*)GetProcAddress(dll, "DirectInput8Create");
				
				if (!api_ori) {
					FreeLibrary(dll);
				}
				char name[MAX_PATH + 1];
				GetModuleFileNameA(base_address, name, MAX_PATH);
				P = new Patch(base_address, name);
				P->Patching();

			}
		}
		
		
	}

	if (reason == DLL_PROCESS_DETACH)
	{
		delete P;
		FreeLibrary(hL);
	}
	return TRUE;
}

// DirectInput8Create
//Exécutée APRES le dllmain
HRESULT __stdcall  __E__0__(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID * ppvOut, LPUNKNOWN punkOuter)
{
	typedef HRESULT(__stdcall* pCreate)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
	pCreate ppC = (pCreate)p;
	
	
	
	
	
	static int tried = 0;
	
	if (!tried) {
		tried = 1;
		HMODULE ed_voice_handle = LoadLibraryA("voice/ed_voice.dll");
		if (ed_voice_handle) {
			EdVoiceAPIType* start_up = (EdVoiceAPIType*)GetProcAddress(ed_voice_handle, "StartUp");
			if (!start_up || !start_up()) {
				FreeLibrary(ed_voice_handle);
			}
		}
	}

	HRESULT ret = api_ori(hinst, dwVersion, riidltf, ppvOut, punkOuter);
	 

	return ret;
}