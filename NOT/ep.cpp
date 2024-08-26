#include "stdafx.h"

#include "ot.h"

#define DbgPrint(...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, __VA_ARGS__)

POBJECT_TYPE _G_ObjectType;
PVOID _G_pRootObj;

struct MyObject 
{
	ULONG SomeData = 0x12345678;
	USHORT NameLength;
	WCHAR Name[];

	void* operator new(size_t , PVOID pv)
	{
		return pv;
	}

	void operator delete(void* )
	{
	}

	MyObject(PCUNICODE_STRING name) : NameLength(name->Length)
	{
		if (PWSTR Buffer = name->Buffer)
		{
			memcpy(Name, Buffer, NameLength);
		}

		DbgPrint("%s<%p>\n", __FUNCTION__, this);
	}

	~MyObject()
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
	}
};

void NTAPI DumpProcedure( 
						 _In_ PVOID Object, 
						 _In_opt_ OBJECT_DUMP_CONTROL * Control
						 )
{
	DbgPrint("%s(%p, %p)\n", __FUNCTION__, Object, Control);
}

NTSTATUS NTAPI OpenProcedure( 
						   _In_ OB_OPEN_REASON OpenReason,
						   _In_ KPROCESSOR_MODE AccessMode,
						   _In_opt_ PEPROCESS Process,
						   _In_ PVOID Object,
						   _Inout_ ACCESS_MASK* GrantedAccess,
						   _In_ ULONG HandleCount
						   )
{
	DbgPrint("%s(%x, %x, %p, %p, %x, %x)\n", __FUNCTION__, OpenReason, AccessMode, Process, Object, *GrantedAccess, HandleCount);

	return STATUS_SUCCESS;
}

void NTAPI CloseProcedure( 
						  _In_ PEPROCESS Process, 
						  _In_ PVOID Object, 
						  _In_ ULONG_PTR ProcessHandleCount,
						  _In_ ULONG_PTR SystemHandleCount )
{
	DbgPrint("%s(%p, %p, %p, %p)\n", __FUNCTION__, Process, Object, ProcessHandleCount, SystemHandleCount);

}

void NTAPI DeleteProcedure( _In_ PVOID Object )
{
	DbgPrint("%s(%p)\n", __FUNCTION__, Object);

	delete reinterpret_cast<MyObject*>(Object);
}

NTSTATUS NTAPI ParseProcedure( 
							  _In_ PVOID ParseObject, 
							  _In_ PVOID ObjectType, 
							  _Inout_ PACCESS_STATE AccessState, 
							  _In_ KPROCESSOR_MODE AccessMode, 
							  _In_ ULONG Attributes, 
							  _Inout_ PUNICODE_STRING CompleteName, 
							  _Inout_ PUNICODE_STRING RemainingName, 
							  _In_opt_ PVOID Context, 
							  _In_opt_ PSECURITY_QUALITY_OF_SERVICE SecurityQos, 
							  _Out_ void** Object)
{
	DbgPrint("%s(po=%p t=%p %p %x %x \"%wZ\" \"%wZ\" %p %p)\n", __FUNCTION__, ParseObject, ObjectType, 
		AccessState, AccessMode, Attributes, CompleteName, RemainingName, Context, SecurityQos);
	
	*Object = 0;

	PVOID pObj;
	OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, 0, Attributes, 0, SecurityQos };

	NTSTATUS status = ObCreateObject(KernelMode, _G_ObjectType, &oa, AccessMode, Context, 
		FIELD_OFFSET(MyObject, Name) + RemainingName->Length, 0, 0, &pObj);

	DbgPrint("ObCreateObject=%x, %p\n", status, pObj);

	if (0 > status)
	{
		return status;
	}

	/*MyObject* p =*/ new(pObj) MyObject(RemainingName);
	*Object = pObj;

	RemainingName->Buffer = 0;
	RemainingName->Length = 0;
	RemainingName->MaximumLength = 0;

	return STATUS_SUCCESS;	
}

NTSTATUS NTAPI QueryNameProcedure( 
								  _In_ PVOID Object, 
								  _In_ BOOLEAN HasObjectName, 
								  _Out_ POBJECT_NAME_INFORMATION ObjectNameInfo, 
								  _In_ ULONG Length, 
								  _Out_ PULONG ReturnLength, 
								  _In_ KPROCESSOR_MODE PreviousMode)
{
	DbgPrint("%s(%p %x %p %x %x)\n", __FUNCTION__, Object, HasObjectName, ObjectNameInfo, Length, PreviousMode);

	ULONG NameLength = reinterpret_cast<MyObject*>(Object)->NameLength;
	ULONG cb = sizeof(UNICODE_STRING) + NameLength + sizeof(WCHAR);

	__try
	{
		if (PreviousMode)
		{
			ProbeForWrite(ObjectNameInfo, cb, __alignof(OBJECT_NAME_INFORMATION));
		}

		if (Length < cb)
		{
			if (Length < sizeof(UNICODE_STRING))
			{
				*ReturnLength = 0;
				return STATUS_BUFFER_TOO_SMALL;
			}

			ObjectNameInfo->Name.MaximumLength = (USHORT)cb;
			*ReturnLength = sizeof(UNICODE_STRING);
			return STATUS_BUFFER_OVERFLOW;
		}

		PWSTR Buffer = (PWSTR)(1 + &ObjectNameInfo->Name);

		ObjectNameInfo->Name.Buffer = Buffer;
		ObjectNameInfo->Name.Length = (USHORT)NameLength;
		ObjectNameInfo->Name.MaximumLength = (USHORT)(NameLength + sizeof(WCHAR));

		memcpy(Buffer, reinterpret_cast<MyObject*>(Object)->Name, NameLength);
		*(WCHAR*)RtlOffsetToPointer(Buffer, NameLength) = 0;

		*ReturnLength = cb;

		return STATUS_SUCCESS;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return GetExceptionCode();
	}
}

BOOLEAN NTAPI OkayToCloseProcedure( 
								   _In_opt_ PEPROCESS Process, 
								   _In_ PVOID Object, 
								   _In_ HANDLE Handle, 
								   _In_ KPROCESSOR_MODE PreviousMode)
{
	DbgPrint("%s(%p, %p, %p, %x)\n", __FUNCTION__, Process, Object, Handle, PreviousMode);

	return TRUE;
}

NTSTATUS InitTI()
{
	OBJECT_TYPE_INITIALIZER oti = { sizeof(oti) };
	UNICODE_STRING TypeName;
	RtlInitUnicodeString(&TypeName, L"DataStack");

	oti.SecurityRequired = TRUE;
	oti.UseDefaultObject = TRUE;

	oti.ObjectTypeCode = '[()]';
	
	oti.InvalidAttributes = OBJ_OPENLINK;
	
	oti.GenericMapping.GenericAll = KEY_ALL_ACCESS;
	oti.GenericMapping.GenericRead = KEY_READ;
	oti.GenericMapping.GenericExecute = KEY_EXECUTE;
	oti.GenericMapping.GenericWrite = KEY_WRITE;
	oti.ValidAccessMask = KEY_ALL_ACCESS;
	
	oti.PoolType = PagedPool;

	oti.DumpProcedure = DumpProcedure;
	oti.OpenProcedure = OpenProcedure;
	oti.CloseProcedure = CloseProcedure;
	oti.DeleteProcedure = DeleteProcedure;
	oti.ParseProcedure = ParseProcedure;
	//oti.SecurityProcedure = SecurityProcedure;
	oti.QueryNameProcedure = QueryNameProcedure;
	oti.OkayToCloseProcedure = OkayToCloseProcedure;

	NTSTATUS status = ObCreateObjectType(&TypeName, &oti, 0, &_G_ObjectType);

	DbgPrint("ObCreateObjectType=%x, %p\n", status, _G_ObjectType);

	return status;
}

void DeleteTI()
{
	if (_G_pRootObj) 
	{
		ObMakeTemporaryObject(_G_pRootObj);
		_G_pRootObj = 0;
	}

	if (_G_ObjectType)
	{
		ObMakeTemporaryObject(_G_ObjectType);
		//ObfDereferenceObject(_G_ObjectType); // will be Bug Check: CRITICAL_OBJECT_TERMINATION
		_G_ObjectType = 0;
	}
}

NTSTATUS TestTI()
{
	DbgSetDebugFilterState(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, TRUE);

	NTSTATUS status = InitTI();

	if (0 <= status)
	{
		UNICODE_STRING ObjectName;
		RtlInitUnicodeString(&ObjectName, L"\\KernelObjects\\MyRootObj");
		OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, &ObjectName, OBJ_CASE_INSENSITIVE|OBJ_PERMANENT };

		PVOID pObj;
		status = ObCreateObject(KernelMode, _G_ObjectType, &oa, KernelMode, 0, sizeof(MyObject), 0, 0, &pObj);

		DbgPrint("ObCreateObject=%x %p\n", status, pObj);

		if (0 <= status)
		{
			UNICODE_STRING us{};
			new(pObj) MyObject(&us);

			HANDLE hObject;
			status = ObInsertObject(pObj, 0, 0, 0, &pObj, &hObject);
			
			DbgPrint("ObInsertObject=%x %p %p\n", status, pObj, hObject);

			if (0 <= status)
			{
				NtClose(hObject);
				_G_pRootObj = pObj;
			}
		}
	}

	if (0 > status)
	{
		DeleteTI();
	}

	return status;
}

void NTAPI DriverUnload(PDRIVER_OBJECT DriverObject)
{
	DeleteTI();

	DbgPrint("DriverUnload(%p)", DriverObject);
}

EXTERN_C
NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING)
{
	DriverObject->DriverUnload = DriverUnload;

	return TestTI();
}