#pragma once

typedef struct OBJECT_DUMP_CONTROL {
	PVOID Stream;
	ULONG Detail;
} *POB_DUMP_CONTROL;

struct OB_EXTENDED_PARSE_PARAMETERS {
	USHORT Length;
	ULONG RestrictedAccessMask;
	PEJOB Silo;
};

enum OB_OPEN_REASON {
	ObCreateHandle,
	ObOpenHandle,
	ObDuplicateHandle,
	ObInheritHandle,
	ObMaxOpenReason
};

//
// Object Type Structure
//

typedef struct OBJECT_TYPE_INITIALIZER {
	USHORT Length;

	union {

		USHORT ObjectTypeFlags;

		struct {
			UCHAR CaseInsensitive : 01; // 0x01;
			UCHAR UnnamedObjectsOnly : 01; // 0x02;
			UCHAR UseDefaultObject : 01; // 0x04;
			UCHAR SecurityRequired : 01; // 0x08;
			UCHAR MaintainHandleCount : 01; // 0x10;
			UCHAR MaintainTypeList : 01; // 0x20;
			UCHAR SupportsObjectCallbacks : 01; // 0x40;
			UCHAR CacheAligned : 01; // 0x80;
			UCHAR UseExtendedParameters : 01; // 0x01;
			UCHAR Reserved : 07; // 0xfe;
		};
	};

	ULONG ObjectTypeCode;
	ULONG InvalidAttributes;// FFFEE00D
	GENERIC_MAPPING GenericMapping;
	ULONG ValidAccessMask;
	ULONG RetainAccess;
	POOL_TYPE PoolType;
	ULONG DefaultPagedPoolCharge;
	ULONG DefaultNonPagedPoolCharge;

	void (NTAPI * DumpProcedure)( 
		_In_ PVOID Object, 
		_In_opt_ OBJECT_DUMP_CONTROL * Control
		);

	NTSTATUS (NTAPI * OpenProcedure)( 
		_In_ OB_OPEN_REASON OpenReason,
		_In_ KPROCESSOR_MODE AccessMode,
		_In_opt_ PEPROCESS Process,
		_In_ PVOID Object,
		_Inout_ ACCESS_MASK* GrantedAccess,
		_In_ ULONG HandleCount
		);

	void (NTAPI * CloseProcedure)( 
		_In_ PEPROCESS Process, 
		_In_ PVOID Object, 
		_In_ ULONG_PTR ProcessHandleCount,
		_In_ ULONG_PTR SystemHandleCount );

	void (NTAPI * DeleteProcedure)( _In_ PVOID Object );

	union {

		NTSTATUS (NTAPI * ParseProcedure)( 
			_In_ PVOID ParseObject, 
			_In_ PVOID ObjectType, 
			_Inout_ PACCESS_STATE AccessState, 
			_In_ KPROCESSOR_MODE AccessMode, 
			_In_ ULONG Attributes, 
			_Inout_ PUNICODE_STRING CompleteName, 
			_Inout_ PUNICODE_STRING RemainingName, 
			_In_opt_ PVOID Context , 
			_In_opt_ PSECURITY_QUALITY_OF_SERVICE SecurityQos, 
			_Out_ void** Object);

		NTSTATUS (NTAPI * ParseProcedureEx)( 
			_In_ PVOID ParseObject, 
			_In_ PVOID ObjectType, 
			_Inout_ PACCESS_STATE AccessState, 
			_In_ KPROCESSOR_MODE PreviousMode, 
			_In_ ULONG Attributes, 
			_Inout_ PUNICODE_STRING CompleteName, 
			_Inout_ PUNICODE_STRING RemainingName, 
			_In_opt_ PVOID Context, 
			_In_opt_ PSECURITY_QUALITY_OF_SERVICE SecurityQos, 
			_In_opt_ OB_EXTENDED_PARSE_PARAMETERS * Parameters, 
			_Out_ void** Object);
	};

	NTSTATUS (NTAPI * SecurityProcedure)( 
		_In_ PVOID Object, 
		_In_ SECURITY_OPERATION_CODE OperationCode, 
		_In_ PSECURITY_INFORMATION SecurityInformation, 
		_Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor, 
		_Inout_ PULONG CapturedLength, 
		_Inout_ PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor, 
		_In_ POOL_TYPE PoolType, 
		_In_ PGENERIC_MAPPING GenericMapping, 
		_In_ KPROCESSOR_MODE PreviousMode);

	NTSTATUS (NTAPI * QueryNameProcedure)( 
		_In_ PVOID Object, 
		_In_ BOOLEAN HasObjectName, 
		_Out_ POBJECT_NAME_INFORMATION ObjectNameInfo, 
		_In_ ULONG Length, 
		_Out_ PULONG ReturnLength, 
		_In_ KPROCESSOR_MODE PreviousMode);

	BOOLEAN (NTAPI * OkayToCloseProcedure)( 
		_In_opt_ PEPROCESS Process, 
		_In_ PVOID Object, 
		_In_ HANDLE Handle, 
		_In_ KPROCESSOR_MODE PreviousMode);

	/*0070*/ ULONG WaitObjectFlagMask;
	/*0074*/ USHORT WaitObjectFlagOffset;
	/*0076*/ USHORT WaitObjectPointerOffset;
	/*0078*/
} *POBJECT_TYPE_INITIALIZER;

C_ASSERT(FIELD_OFFSET(OBJECT_TYPE_INITIALIZER,WaitObjectFlagMask) == 0x70);

EXTERN_C
NTSYSAPI
NTSTATUS 
NTAPI 
ObCreateObjectType(
				   _In_ PUNICODE_STRING TypeName,
				   _In_ POBJECT_TYPE_INITIALIZER ObjectTypeInitializer,
				   _In_opt_ PSECURITY_DESCRIPTOR sd,
				   _Deref_out_ POBJECT_TYPE* ObjectType);

EXTERN_C
NTSYSAPI
NTSTATUS
NTAPI
ObCreateObject (
				_In_ KPROCESSOR_MODE ProbeMode,
				_In_ POBJECT_TYPE ObjectType,
				_In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
				_In_ KPROCESSOR_MODE OwnershipMode,
				_Inout_opt_ PVOID ParseContext,
				_In_ ULONG ObjectBodySize,
				_In_ ULONG PagedPoolCharge,
				_In_ ULONG NonPagedPoolCharge,
				_Out_ PVOID *Object
				);

#ifndef SDDL_REVISION
#define SDDL_REVISION 1
#endif

EXTERN_C
NTSYSAPI
NTSTATUS
NTAPI
SeConvertSecurityDescriptorToStringSecurityDescriptor(
	_In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
	_In_ ULONG RequestedStringSDRevision,
	_In_ SECURITY_INFORMATION SecurityInformation,
	_Outptr_ PWSTR* StringSecurityDescriptor,
	_Out_opt_ PULONG StringSecurityDescriptorLen
	);
