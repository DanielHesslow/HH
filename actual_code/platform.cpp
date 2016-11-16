#include "platform.h"
#include "header.h"

BackingBuffer *platform_execute_command(char *command, int command_len)
{
	//check for leakages... This is just thrown together!
	SECURITY_ATTRIBUTES atributes = {};
	atributes.nLength = sizeof(atributes);
	atributes.bInheritHandle = true;
	atributes.lpSecurityDescriptor = 0; // makes sure this is OK!
	HANDLE readPipe;
	HANDLE writePipe;

	BackingBuffer *backingBuffer = allocBackingBuffer(200, 200);


	if (CreatePipe(&readPipe, &writePipe, &atributes, 0))
	{
		STARTUPINFO si = {};
		si.cb = sizeof(si); //like why do we need to do this microsoft.. why? is this sturct variable size or what?
		si.hStdOutput = writePipe;										//appearently because they like to change the struct without it breaking shit
	
		si.hStdError = writePipe;										//all right i guess...
		si.dwFlags = STARTF_USESTDHANDLES;

		PROCESS_INFORMATION pi; // I like the naming consistency, why isn't this called PROCESSINFO

		char buffer[500];
		snprintf(buffer, sizeof(buffer), "cmd.exe /C %.*s", command_len, command);

		if (CreateProcess(0, buffer, 0, 0, true, CREATE_UNICODE_ENVIRONMENT, 0, 0, &si, &pi))
		{
			//we should maybe not wait here, just lock this buffer?
			//this might take some time and should freeze us!

			//from msdn, do it:
			//Use caution when calling the wait functions and code that directly or indirectly creates windows.
			//If a thread creates any windows, it must process messages.Message broadcasts are sent to all windows in the system.
			//A thread that uses a wait function with no time - out interval may cause the system to become deadlocked.
			//Two examples of code that indirectly creates windows are DDE and the CoInitialize function.
			//Therefore, if you have a thread that creates windows, use MsgWaitForMultipleObjects or MsgWaitForMultipleObjectsEx, rather than WaitForSingleObject.

			while (WaitForSingleObject(pi.hProcess, 1) == WAIT_TIMEOUT)
			{
				DWORD len;
				ReadFile(readPipe, buffer, sizeof(buffer), &len, 0);
				for (int i = 0; i < len; i++)
				{
					appendCharacter(backingBuffer->buffer, backingBuffer->buffer->cursor_ids.start[0].id, buffer[i]);
				}
			}

			CloseHandle(readPipe);
			CloseHandle(writePipe);

			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);

			initial_mgb_process(backingBuffer->buffer);
			return backingBuffer;
		}
		else
		{
			//todo handle and log
			assert(false);
		}
	}
	else
	{
		//todo handle and log
		assert(false);
	}

	return 0;
}