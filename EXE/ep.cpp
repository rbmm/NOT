#include "stdafx.h"

void WINAPI ep(void*)
{
	if (IsDebuggerPresent()) __debugbreak();

	HANDLE hEvent;
	UNICODE_STRING ObjectName;
	OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, &ObjectName, OBJ_CASE_INSENSITIVE };
	RtlInitUnicodeString(&ObjectName, L"\\KernelObjects\\MyRootObj\\aaa\\rrr\\888");
	
	NTSTATUS status;
	if (0 <= (status = NtOpenEvent(&hEvent, KEY_WRITE, &oa)))
	{
		ULONG len;
		POBJECT_NAME_INFORMATION poni = (POBJECT_NAME_INFORMATION)alloca(0x100);

		if (0 <= (status = NtQueryObject(hEvent, ObjectNameInformation, poni, 0x100, &len)))
		{
			DbgPrint(": \"%wZ\"\n", &poni->Name);
		}

		NtClose(hEvent);
	}
	ExitProcess(status);
}

