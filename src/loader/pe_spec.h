#ifndef pe_spec_HEADER
#define pe_spec_HEADER

#include <stdint.h>

#define PE_SIGNATURE         "PE\0\0"
#define PE_SIGNATURE_SIZE    4
#define PE_FILE_OFFSET       0x3c
// gcc sizeof(Pe_SymbolHeader) is returning values with alignment
#define PE_SYMBOL_SIZE       18
#define PE_RELOCATION_SIZE   10

#define PE_SYM_TYPE_BASE(TYPE)    (TYPE & 0x0000000F)
#define PE_SYM_TYPE_COMPLEX(TYPE) ((TYPE >> 4) & 0x0000000F)


typedef struct _Pe_Fileheader {
    uint16_t Machine;              // 2
    uint16_t NumberOfSections;     // 4
    uint32_t TimeDateStamp;        // 8
    uint32_t PointerToSymbolTable; // 12
    uint32_t NumberOfSymbols;      // 16
    uint16_t SizeOfOptionalHeader; // 18
    uint16_t Characteristics;      // 20
} __attribute__((packed)) Pe_FileHeader;


typedef struct _Pe_OptionalHeaderStandard {
    uint16_t Magic;                   // 2
    uint8_t  MajorLinkerVersion;      // 3
    uint8_t  MinorLinkerVersion;      // 4
    uint32_t SizeOfCode;              // 8
    uint32_t SizeOfInitializedData;   // 12
    uint32_t SizeOfUninitializedData; // 16
    uint32_t AddressOfEntryPoint;     // 20
    uint32_t BaseOfCode;              // 24
    uint32_t BaseOfData;              // 28
} __attribute__((packed)) Pe_OptionalHeaderStandard;


typedef struct _Pe_OptionalHeaderStandardPlus {
    uint16_t Magic;                   // 2
    uint8_t  MajorLinkerVersion;      // 3
    uint8_t  MinorLinkerVersion;      // 4
    uint32_t SizeOfCode;              // 8
    uint32_t SizeOfInitializedData;   // 12
    uint32_t SizeOfUninitializedData; // 16
    uint32_t AddressOfEntryPoint;     // 20
    uint32_t BaseOfCode;              // 24
} __attribute__((packed)) Pe_OptionalHeaderStandardPlus;


typedef struct _Pe_OptionalHeaderWindows {
    uint32_t ImageBase;                   // 32
    uint32_t SectionAlignment;            // 36
    uint32_t FileAlignment;               // 40
    uint16_t MajorOperatingSystemVersion; // 42
    uint16_t MinorOperatingSystemVersion; // 44
    uint16_t MajorImageVersion;           // 46
    uint16_t MinorImageVersion;           // 48
    uint16_t MajorSubsystemVersion;       // 50
    uint16_t MinorSubsystemVersion;       // 52
    uint32_t Win32VersionValue;           // 56
    uint32_t SizeOfImage;                 // 60
    uint32_t SizeOfHeaders;               // 64
    uint32_t CheckSum;                    // 68
    uint16_t Subsystem;                   // 70
    uint16_t DllCharacteristics;          // 72
    uint32_t SizeOfStackReserve;          // 76
    uint32_t SizeOfStackCommit;           // 80
    uint32_t SizeOfHeapReserve;           // 84
    uint32_t SizeOfHeapCommit;            // 88
    uint32_t LoaderFlags;                 // 92
    uint32_t NumberOfRvaAndSizes;         // 96
} __attribute__((packed)) Pe_OptionalHeaderWindows;


typedef struct _Pe_OptionalHeaderWindowsPlus {
    uint64_t ImageBase;                   // 32
    uint32_t SectionAlignment;            // 36
    uint32_t FileAlignment;               // 40
    uint16_t MajorOperatingSystemVersion; // 42
    uint16_t MinorOperatingSystemVersion; // 44
    uint16_t MajorImageVersion;           // 46
    uint16_t MinorImageVersion;           // 48
    uint16_t MajorSubsystemVersion;       // 50
    uint16_t MinorSubsystemVersion;       // 52
    uint32_t Win32VersionValue;           // 56
    uint32_t SizeOfImage;                 // 60
    uint32_t SizeOfHeaders;               // 64
    uint32_t CheckSum;                    // 68
    uint16_t Subsystem;                   // 70
    uint16_t DllCharacteristics;          // 72
    uint64_t SizeOfStackReserve;          // 80
    uint64_t SizeOfStackCommit;           // 88
    uint64_t SizeOfHeapReserve;           // 96
    uint64_t SizeOfHeapCommit;            // 104
    uint32_t LoaderFlags;                 // 108
    uint32_t NumberOfRvaAndSizes;         // 112
} __attribute__((packed)) Pe_OptionalHeaderWindowsPlus;


typedef struct _IMAGE_DATA_DIRECTORY {
    uint32_t VirtualAddress;
    uint32_t Size;
} __attribute__((packed)) IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

typedef struct _Pe_DataDirectory {
    IMAGE_DATA_DIRECTORY ExportTable;           // 104/120
    IMAGE_DATA_DIRECTORY ImportTable;           // 112/128
    IMAGE_DATA_DIRECTORY ResourceTable;         // 120/136
    IMAGE_DATA_DIRECTORY ExceptionTable;        // 128/144
    IMAGE_DATA_DIRECTORY CertificateTable;      // 136/152
    IMAGE_DATA_DIRECTORY BaseRelocationTable;   // 144/160
    IMAGE_DATA_DIRECTORY Debug;                 // 152/168
    IMAGE_DATA_DIRECTORY Architecture;          // 160/176
    IMAGE_DATA_DIRECTORY GlobalPtr;             // 168/184
    IMAGE_DATA_DIRECTORY TLSTable;              // 176/192
    IMAGE_DATA_DIRECTORY LoadConfigTable;       // 184/200
    IMAGE_DATA_DIRECTORY BoundImport;           // 192/208
    IMAGE_DATA_DIRECTORY IAT;                   // 200/216
    IMAGE_DATA_DIRECTORY DelayImportDescriptor; // 208/224
    IMAGE_DATA_DIRECTORY CLRRuntimeHeader;      // 216/232
    IMAGE_DATA_DIRECTORY Reserved;              // 224/240
} __attribute__((packed)) PE_Data_Directory;


typedef struct _Pe_SectionHeader {
    char Name[8];                  // 8
    uint32_t VirtualSize;          // 12
    uint32_t VirtualAddress;       // 16
    uint32_t SizeOfRawData;        // 20
    uint32_t PointerToRawData;     // 24
    uint32_t PointerToRelocations; // 28
    uint32_t PointerToLinenumbers; // 32
    uint16_t NumberOfRelocations;  // 34
    uint16_t NumberOfLinenumbers;  // 36
    uint32_t Characteristics;      // 40
} __attribute__((packed)) Pe_SectionHeader;


// value is defined as SIGNED in the PE SPEC, but we're treating it as unsigned
// here. We will almost always use it unsigned. If you need it signed, cast it
// to a signed type
typedef struct _Pe_Symbol {
    char Name[8];                // 8
    uint32_t Value;              // 12
    uint16_t SectionNumber;      // 14
    uint16_t Type;               // 16
    uint8_t  StorageClass;       // 17
    uint8_t  NumberOfAuxSymbols; // 18
} __attribute__((packed)) Pe_Symbol;


typedef struct _Pe_SymbolFunctionDefiniton {
    uint32_t TagIndex;              // 4
    uint32_t TotalSize;             // 8
    uint32_t PointerToLinenumber;   // 12
    uint32_t PointerToNextFunction; // 16
    uint16_t Unused;                // 18
} __attribute__((packed)) Pe_SymbolFunctionDefinition;


typedef struct _Pe_Relocation {
    uint32_t VirtualAddress;   // 4
    uint32_t SymbolTableIndex; // 8
    uint16_t Type;             // 10
} __attribute__((packed)) Pe_Relocation;

#define PE_TYPE_PE32     0x10b
#define PE_TYPE_PE32PLUS 0x20b

#define IMAGE_FILE_MACHINE_UNKNOWN   0x0
#define IMAGE_FILE_MACHINE_AM33      0x1d3
#define IMAGE_FILE_MACHINE_ALPHA     0x184
#define IMAGE_FILE_MACHINE_AMD64     0x8664
#define IMAGE_FILE_MACHINE_ARM       0x1c0
#define IMAGE_FILE_MACHINE_ARMV7     0x1c4
#define IMAGE_FILE_MACHINE_ALPHA64   0x284
#define IMAGE_FILE_MACHINE_I386      0x14c
#define IMAGE_FILE_MACHINE_IA64      0x200
#define IMAGE_FILE_MACHINE_M68K      0x268
#define IMAGE_FILE_MACHINE_MIPS16    0x266
#define IMAGE_FILE_MACHINE_MIPSFPU   0x366
#define IMAGE_FILE_MACHINE_MIPSFPU16 0x266
#define IMAGE_FILE_MACHINE_POWERPC   0x1f0
#define IMAGE_FILE_MACHINE_R3000     0x162
#define IMAGE_FILE_MACHINE_R4000     0x166
#define IMAGE_FILE_MACHINE_R10000    0x168
#define IMAGE_FILE_MACHINE_SH3       0x1a2
#define IMAGE_FILE_MACHINE_SH4       0x1a6
#define IMAGE_FILE_MACHINE_THUMB     0x1c2

#define IMAGE_FILE_RELOCS_STRIPPED         0x0001
#define IMAGE_FILE_EXECUTABLE_IMAGE        0x0002
#define IMAGE_FILE_LINE_NUMS_STRIPPED      0x0004
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED     0x0008
#define IMAGE_FILE_AGRESSIVE_WS_TRIM       0x0010
#define IMAGE_FILE_LARGE_ADDRESS_AWARE     0x0020
#define IMAGE_FILE_16BIT_MACHINE           0x0040
#define IMAGE_FILE_BYTES_REVERSED_LO       0x0080
#define IMAGE_FILE_32BIT_MACHINE           0x0100
#define IMAGE_FILE_DEBUG_STRIPPED          0x0200
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP 0x0400
#define IMAGE_FILE_SYSTEM                  0x1000
#define IMAGE_FILE_DLL                     0x2000
#define IMAGE_FILE_UP_SYSTEM_ONLY          0x4000
#define IMAGE_FILE_BYTES_REVERSE_HI        0x8000

#define IMAGE_SUBSYSTEM_UNKNOWN                 0
#define IMAGE_SUBSYSTEM_NATIVE                  1
#define IMAGE_SUBSYSTEM_WINDOWS_GUI             2
#define IMAGE_SUBSYSTEM_WINDOWS_CUI             3
#define IMAGE_SUBSYSTEM_POSIX_CUI               7
#define IMAGE_SUBSYSTEM_WINDOWS_CE_GUI          9
#define IMAGE_SUBSYSTEM_EFI_APPLICATION         10
#define IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER 11
#define IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER      12
#define IMAGE_SUBSYSTEM_EFI_ROM                 13
#define IMAGE_SUBSYSTEM_XBOX                    14

#define IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE          0x0040
#define IMAGE_DLL_CHARACTERISTICS_FORCE_INTEGRITY       0x0080
#define IMAGE_DLL_CHARACTERISTICS_NX_COMPAT             0x0100
#define IMAGE_DLL_CHARACTERISTICS_NO_ISOLATION          0x0200
#define IMAGE_DLL_CHARACTERISTICS_NO_SEH                0x0400
#define IMAGE_DLL_CHARACTERISTICS_NO_BIND               0x0800
#define IMAGE_DLL_CHARACTERISTICS_WDM_DRIVER            0x2000
#define IMAGE_DLL_CHARACTERISTICS_TERMINAL_SERVER_AWARE 0x8000

#define IMAGE_SCN_TYPE_REG               0x00000000
#define IMAGE_SCN_TYPE_DSECT             0x00000001
#define IMAGE_SCN_TYPE_NOLOAD            0x00000002
#define IMAGE_SCN_TYPE_GROUP             0x00000004
#define IMAGE_SCN_TYPE_NO_PAD            0x00000008
#define IMAGE_SCN_TYPE_COPY              0x00000010
#define IMAGE_SCN_CNT_CODE               0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA   0x00000040
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA 0x00000080
#define IMAGE_SCN_LNK_OTHER              0x00000100
#define IMAGE_SCN_LNK_INFO               0x00000200
#define IMAGE_SCN_TYPE_OVER              0x00000400
#define IMAGE_SCN_LNK_REMOVE             0x00000800
#define IMAGE_SCN_LNK_COMDAT             0x00001000
#define IMAGE_SCN_MEM_FARDATA            0x00008000
#define IMAGE_SCN_MEM_PURGEABLE          0x00020000
#define IMAGE_SCN_MEM_16BIT              0x00020000
#define IMAGE_SCN_MEM_LOCKED             0x00040000
#define IMAGE_SCN_MEM_PRELOAD            0x00080000
#define IMAGE_SCN_ALIGN_1BYTES           0x00100000
#define IMAGE_SCN_ALIGN_2BYTES           0x00200000
#define IMAGE_SCN_ALIGN_4BYTES           0x00300000
#define IMAGE_SCN_ALIGN_8BYTES           0x00400000
#define IMAGE_SCN_ALIGN_16BYTES          0x00500000
#define IMAGE_SCN_ALIGN_32BYTES          0x00600000
#define IMAGE_SCN_ALIGN_64BYTES          0x00700000
#define IMAGE_SCN_ALIGN_128BYTES         0x00800000
#define IMAGE_SCN_ALIGN_256BYTES         0x00900000
#define IMAGE_SCN_ALIGN_512BYTES         0x00A00000
#define IMAGE_SCN_ALIGN_1024BYTES        0x00B00000
#define IMAGE_SCN_ALIGN_2048BYTES        0x00C00000
#define IMAGE_SCN_ALIGN_4096BYTES        0x00D00000
#define IMAGE_SCN_ALIGN_8192BYTES        0x00E00000
#define IMAGE_SCN_LNK_NRELOC_OVFL        0x01000000
#define IMAGE_SCN_MEM_DISCARDABLE        0x02000000
#define IMAGE_SCN_MEM_NOT_CACHED         0x04000000
#define IMAGE_SCN_MEM_NOT_PAGED          0x08000000
#define IMAGE_SCN_MEM_SHARED             0x10000000
#define IMAGE_SCN_MEM_EXECUTE            0x20000000
#define IMAGE_SCN_MEM_READ               0x40000000
#define IMAGE_SCN_MEM_WRITE              0x80000000

#define IMAGE_SYM_CLASS_END_OF_FUNCTION -1
#define IMAGE_SYM_CLASS_NULL             0
#define IMAGE_SYM_CLASS_AUTOMATIC        1
#define IMAGE_SYM_CLASS_EXTERNAL         2
#define IMAGE_SYM_CLASS_STATIC           3
#define IMAGE_SYM_CLASS_REGISTER         4
#define IMAGE_SYM_CLASS_EXTERNAL_DEF     5
#define IMAGE_SYM_CLASS_LABEL            6
#define IMAGE_SYM_CLASS_UNDEFINED_LABEL  7
#define IMAGE_SYM_CLASS_MEMBER_OF_STRUCT 8
#define IMAGE_SYM_CLASS_ARGUMENT         9
#define IMAGE_SYM_CLASS_STRUCT_TAG       10
#define IMAGE_SYM_CLASS_MEMBER_OF_UNION  11
#define IMAGE_SYM_CLASS_UNION_TAG        12
#define IMAGE_SYM_CLASS_TYPE_DEFINITION  13
#define IMAGE_SYM_CLASS_UNDEFINED_STATIC 14
#define IMAGE_SYM_CLASS_ENUM_TAG         15
#define IMAGE_SYM_CLASS_MEMBER_OF_ENUM   16
#define IMAGE_SYM_CLASS_REGISTER_PARAM   17
#define IMAGE_SYM_CLASS_BIT_FIELD        18
#define IMAGE_SYM_CLASS_BLOCK            100
#define IMAGE_SYM_CLASS_FUNCTION         101
#define IMAGE_SYM_CLASS_END_OF_STRUCT    102
#define IMAGE_SYM_CLASS_FILE             103
#define IMAGE_SYM_CLASS_SECTION          104
#define IMAGE_SYM_CLASS_WEAK_EXTERNAL    105

#define IMAGE_SYM_UNDEFINED  0
#define IMAGE_SYM_ABSOLUTE  -1
#define IMAGE_SYM_DEBUG     -2

#define IMAGE_SYM_TYPE_NULL   0
#define IMAGE_SYM_TYPE_VOID   1
#define IMAGE_SYM_TYPE_CHAR   2
#define IMAGE_SYM_TYPE_SHORT  3
#define IMAGE_SYM_TYPE_INT    4
#define IMAGE_SYM_TYPE_LONG   5
#define IMAGE_SYM_TYPE_FLOAT  6
#define IMAGE_SYM_TYPE_DOUBLE 8
#define IMAGE_SYM_TYPE_UNION  9
#define IMAGE_SYM_TYPE_ENUM   10
#define IMAGE_SYM_TYPE_MOE    11
#define IMAGE_SYM_TYPE_BYTE   12
#define IMAGE_SYM_TYPE_WORD   13
#define IMAGE_SYM_TYPE_UINT   14
#define IMAGE_SYM_TYPE_DWORD  15

#define IMAGE_SYM_DTYPE_NULL     0
#define IMAGE_SYM_DTYPE_POINTER  1
#define IMAGE_SYM_DTYPE_FUNCTION 2
#define IMAGE_SYM_DTYPE_ARRAY    3
#define IMAGE_SYM_MSFT_FUNCTION  0x20

#endif