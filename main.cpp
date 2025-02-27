#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include "Windows.h"
#include "Psapi.h"

using namespace std::string_literals;
using namespace std::chrono_literals;

bool bAutoClose = false;
bool bSilentMode = false;
bool bHasClosedOnce = false;

int ThrowMessage(const std::string& msg)
{
	if (bSilentMode)
		return 1;

	return MessageBoxA(nullptr, msg.c_str(), "Message", MB_OK);
}

bool ClosePmService()
{
	int pid = 0;
	DWORD procs[1024], bytesReturned, numProcesses;
	TCHAR szProcessName[MAX_PATH];

	if (!EnumProcesses(procs, sizeof(procs), &bytesReturned))
		ThrowMessage("ERROR: Couldn't enumerate list of process identifiers"s);

	numProcesses = bytesReturned / sizeof(DWORD);

	for (int i = 0; i < numProcesses; i++)
	{
		if (procs[i] != 0)
		{
			// Get a handle to the current process in our iteration
			HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, FALSE, procs[i]);

			if (handle == NULL)
				continue;
				
			HMODULE module;
			
			if (EnumProcessModules(handle, &module, sizeof(module), &bytesReturned))
			{
				if (!GetModuleBaseName(handle, module, szProcessName, sizeof(szProcessName) / sizeof(TCHAR)))
					ThrowMessage("ERROR: Failed to get process name");

				if (strcmp(szProcessName, "ExitLagPmService.exe") == 0)
				{
					if (!TerminateProcess(handle, NULL))
					{
						ThrowMessage("ERROR: Couldn't terminate ExitLagPmService.exe!");

						CloseHandle(handle);

						return false;
					}
					
					bHasClosedOnce = true;

					CloseHandle(handle);

					return true;
				}
			}
			
			CloseHandle(handle);
		}
	}

	return false;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
	/* Close FuckPresentMon after closing the PresentMon service 1 time, good if you never relaunch ExitLag before computer shutdown */
	if (strcmp(pCmdLine, "--autoclose") == 0 /* || IsDebuggerPresent() */ )
	{
		bAutoClose = true;
		ThrowMessage("Running with autoclose flag or debugger present");
	}

	if (strcmp(pCmdLine, "--silent") == 0)
		bSilentMode = true;

	/* Give ExitLag and the gay ahh service some time to start(when running this on startup) */ 
	std::this_thread::sleep_for(5s);

	while (true)
	{
		if (ClosePmService())
			ThrowMessage("Terminated ExitLagPmService.exe");

		if (bAutoClose && bHasClosedOnce)
			break;

		std::this_thread::sleep_for(5s);
	}

	return EXIT_SUCCESS;
}