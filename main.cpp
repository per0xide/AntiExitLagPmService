#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include "Windows.h"
#include "TlHelp32.h"

using namespace std::string_literals;
using namespace std::chrono_literals;

bool bAutoClose = false;
bool bDebugMode = false;
bool bHasClosedOnce = false;

int DebugMessage(const std::string& msg)
{
	if (bDebugMode)
        return MessageBoxA(nullptr, msg.c_str(), "Debug", MB_OK);
}

bool ClosePmService()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return false;

    PROCESSENTRY32W procEntry{};
    procEntry.dwSize = sizeof(procEntry);

    if (!Process32FirstW(snapshot, &procEntry))
    {
        CloseHandle(snapshot);
        return false;
    }

    do
    {
        if (L"ExitLagPmService.exe"s == procEntry.szExeFile)
        {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, procEntry.th32ProcessID);

            if (hProcess)
            {
                if (!TerminateProcess(hProcess, 0))
                    return false;

                bHasClosedOnce = true;

                CloseHandle(hProcess);
                CloseHandle(snapshot);

                DebugMessage("Terminated ExitLagPmService.exe.");
                return true;
            }
            else
            {
                DebugMessage("Couldn't aquire handle to target process");
            }
        }
    } while (Process32NextW(snapshot, &procEntry));

    DebugMessage("Couldn't locate ExitLagPmService.exe.");

    CloseHandle(snapshot);

	return false;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
	/* Close FuckPresentMon after closing the PresentMon service 1 time */
	if (strcmp(pCmdLine, "--autoclose") == 0)
	{
		bAutoClose = true;
		DebugMessage("Running with autoclose flag or debugger present");
	}

	if (strcmp(pCmdLine, "--debug") == 0)
		bDebugMode = true;

	/* Give ExitLag and the gay ahh service some time to start(when running this on startup) */ 
	std::this_thread::sleep_for(5s);

	while (true)
	{
		if (ClosePmService())
			DebugMessage("Terminated ExitLagPmService.exe");

		if (bAutoClose && bHasClosedOnce)
			break;

		std::this_thread::sleep_for(3s);
	}

	return EXIT_SUCCESS;
}