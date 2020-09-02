typedef signed char         int8_t;     // NOLINT
typedef short               int16_t;    // NOLINT
typedef long                int32_t;    // NOLINT
typedef long long           int64_t;    // NOLINT

typedef unsigned char       uint8_t;    // NOLINT
typedef unsigned short      uint16_t;   // NOLINT
typedef unsigned long       uint32_t;   // NOLINT
typedef unsigned long long  uint64_t;   // NOLINT

typedef unsigned int        size_t;     // NOLINT
typedef long                intptr_t;   // NOLINT
typedef unsigned long       uintptr_t;  // NOLINT
typedef long int            ptrdiff_t;  // NOLINT

typedef unsigned char       BYTE;       // NOLINT
typedef unsigned short      WORD;       // NOLINT
typedef unsigned long       DWORD;      // NOLINT

typedef int                 BOOL;       // NOLINT
typedef BYTE                BOOLEAN;    // NOLINT

#define NULL                (0)         // NOLINT

// RUN: %check_clang_tidy %s readability-identifier-naming %t -- \
// RUN:   -config="{CheckOptions: [\
// RUN:     {key: readability-identifier-naming.FunctionCase       , value: CamelCase },           \
// RUN:     {key: readability-identifier-naming.ClassCase          , value: szHungarianNotation }, \
// RUN:     {key: readability-identifier-naming.TypedefCase        , value: szHungarianNotation }, \
// RUN:     {key: readability-identifier-naming.MemberCase         , value: szHungarianNotation }, \
// RUN:     {key: readability-identifier-naming.ClassMemberCase    , value: szHungarianNotation }, \
// RUN:     {key: readability-identifier-naming.ConstantMemberCase , value: szHungarianNotation }, \
// RUN:     {key: readability-identifier-naming.VariableCase       , value: szHungarianNotation }, \
// RUN:     {key: readability-identifier-naming.ParameterCase      , value: szHungarianNotation }, \
// RUN:     {key: readability-identifier-naming.GlobalPointerCase  , value: szHungarianNotation }, \
// RUN:     {key: readability-identifier-naming.GlobalVariableCase , value: szHungarianNotation }, \
// RUN:     {key: readability-identifier-naming.GlobalFunctionCase , value: CamelCase }, \
// RUN:   ]}"

class UnlistedClass { public: mutable int ValInt; };
// CHECK-MESSAGES: :[[@LINE-1]]:43: warning: invalid case style for member 'ValInt' [readability-identifier-naming]
// CHECK-FIXES: {{^}}class UnlistedClass { public: mutable int iValInt; };

UnlistedClass cUnlisted2;
// CHECK-MESSAGES: :[[@LINE-1]]:15: warning: invalid case style for global variable 'cUnlisted2' [readability-identifier-naming]
// CHECK-FIXES: {{^}}UnlistedClass Unlisted2;

UnlistedClass objUnlistedClass3;
// CHECK-MESSAGES: :[[@LINE-1]]:15: warning: invalid case style for global variable 'objUnlistedClass3' [readability-identifier-naming]
// CHECK-FIXES: {{^}}UnlistedClass UnlistedClass3;

typedef int INDEX;
INDEX iIndex = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: invalid case style for global variable 'iIndex' [readability-identifier-naming]
// CHECK-FIXES: {{^}}INDEX Index = 0;

struct DataBuffer {
    mutable size_t Size;
};
// CHECK-MESSAGES: :[[@LINE-2]]:20: warning: invalid case style for member 'Size' [readability-identifier-naming]
// CHECK-FIXES: {{^}}    mutable size_t nSize;

int &RefValueIndex = iIndex;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: invalid case style for global variable 'RefValueIndex' [readability-identifier-naming]
// CHECK-FIXES: {{^}}int &iRefValueIndex = Index;

typedef void (*FUNC_PTR_HELLO)();
FUNC_PTR_HELLO Hello = NULL;
// CHECK-MESSAGES: :[[@LINE-1]]:16: warning: invalid case style for global pointer 'Hello' [readability-identifier-naming]
// CHECK-FIXES: {{^}}FUNC_PTR_HELLO fnHello = NULL;

void *ValueVoidPtr = NULL;
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: invalid case style for global pointer 'ValueVoidPtr' [readability-identifier-naming]
// CHECK-FIXES: {{^}}void *pValueVoidPtr = NULL;

ptrdiff_t PtrDiff = NULL;
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: invalid case style for global variable 'PtrDiff' [readability-identifier-naming]
// CHECK-FIXES: {{^}}ptrdiff_t pPtrDiff = NULL;

const char *NamePtr = "Name";
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: invalid case style for global pointer 'NamePtr' [readability-identifier-naming]
// CHECK-FIXES: {{^}}const char *szNamePtr = "Name";

const char NameArray[] = "Name";
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: invalid case style for global variable 'NameArray' [readability-identifier-naming]
// CHECK-FIXES: {{^}}const char szNameArray[] = "Name";

const char *NamePtrArray[] = {"AA", "BB"};
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: invalid case style for global variable 'NamePtrArray' [readability-identifier-naming]
// CHECK-FIXES: {{^}}const char *pszNamePtrArray[] = {"AA", "BB"};

int DataInt[1] = {0};
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: invalid case style for global variable 'DataInt' [readability-identifier-naming]
// CHECK-FIXES: {{^}}int aDataInt[1] = {0};

int *DataIntPtr[1] = {0};
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: invalid case style for global variable 'DataIntPtr' [readability-identifier-naming]
// CHECK-FIXES: {{^}}int *paDataIntPtr[1] = {0};

void *BufferPtr1;
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: invalid case style for global pointer 'BufferPtr1' [readability-identifier-naming]
// CHECK-FIXES: {{^}}void *pBufferPtr1;

void **BufferPtr2;
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: invalid case style for global pointer 'BufferPtr2' [readability-identifier-naming]
// CHECK-FIXES: {{^}}void **ppBufferPtr2;

void **pBufferPtr3;
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: invalid case style for global pointer 'pBufferPtr3' [readability-identifier-naming]
// CHECK-FIXES: {{^}}void **ppBufferPtr3;

int *pBufferPtr4;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: invalid case style for global pointer 'pBufferPtr4' [readability-identifier-naming]
// CHECK-FIXES: {{^}}int *piBufferPtr4;

int DataArray[2] = {0};
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: invalid case style for global variable 'DataArray' [readability-identifier-naming]
// CHECK-FIXES: {{^}}int aDataArray[2] = {0};

int8_t *ValueI8Ptr;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: invalid case style for global pointer 'ValueI8Ptr' [readability-identifier-naming]
// CHECK-FIXES: {{^}}int8_t *pi8ValueI8Ptr;

uint8_t *ValueU8Ptr;
// CHECK-MESSAGES: :[[@LINE-1]]:10: warning: invalid case style for global pointer 'ValueU8Ptr' [readability-identifier-naming]
// CHECK-FIXES: {{^}}uint8_t *pu8ValueU8Ptr;

int8_t ValueI8;
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: invalid case style for global variable 'ValueI8' [readability-identifier-naming]
// CHECK-FIXES: {{^}}int8_t i8ValueI8;

int16_t ValueI16 = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: invalid case style for global variable 'ValueI16' [readability-identifier-naming]
// CHECK-FIXES: {{^}}int16_t i16ValueI16 = 0;

int32_t ValueI32 = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: invalid case style for global variable 'ValueI32' [readability-identifier-naming]
// CHECK-FIXES: {{^}}int32_t i32ValueI32 = 0;

int64_t ValueI64 = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: invalid case style for global variable 'ValueI64' [readability-identifier-naming]
// CHECK-FIXES: {{^}}int64_t i64ValueI64 = 0;

uint8_t ValueU8 = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: invalid case style for global variable 'ValueU8' [readability-identifier-naming]
// CHECK-FIXES: {{^}}uint8_t u8ValueU8 = 0;

uint16_t ValueU16 = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:10: warning: invalid case style for global variable 'ValueU16' [readability-identifier-naming]
// CHECK-FIXES: {{^}}uint16_t u16ValueU16 = 0;

uint32_t ValueU32 = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:10: warning: invalid case style for global variable 'ValueU32' [readability-identifier-naming]
// CHECK-FIXES: {{^}}uint32_t u32ValueU32 = 0;

uint64_t ValueU64 = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:10: warning: invalid case style for global variable 'ValueU64' [readability-identifier-naming]
// CHECK-FIXES: {{^}}uint64_t u64ValueU64 = 0;

float ValueFloat = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: invalid case style for global variable 'ValueFloat' [readability-identifier-naming]
// CHECK-FIXES: {{^}}float fValueFloat = 0;

double ValueDouble = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: invalid case style for global variable 'ValueDouble' [readability-identifier-naming]
// CHECK-FIXES: {{^}}double dValueDouble = 0;

char ValueChar = 'c';
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: invalid case style for global variable 'ValueChar' [readability-identifier-naming]
// CHECK-FIXES: {{^}}char cValueChar = 'c';

bool ValueBool = true;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: invalid case style for global variable 'ValueBool' [readability-identifier-naming]
// CHECK-FIXES: {{^}}bool bValueBool = true;

int ValueInt = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: invalid case style for global variable 'ValueInt' [readability-identifier-naming]
// CHECK-FIXES: {{^}}int iValueInt = 0;

int &RefValueInt = ValueInt;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: invalid case style for global variable 'RefValueInt' [readability-identifier-naming]
// CHECK-FIXES: {{^}}int &iRefValueInt = iValueInt;

size_t ValueSize = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: invalid case style for global variable 'ValueSize' [readability-identifier-naming]
// CHECK-FIXES: {{^}}size_t nValueSize = 0;

wchar_t ValueWchar = 'w';
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: invalid case style for global variable 'ValueWchar' [readability-identifier-naming]
// CHECK-FIXES: {{^}}wchar_t wcValueWchar = 'w';

short ValueShort = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: invalid case style for global variable 'ValueShort' [readability-identifier-naming]
// CHECK-FIXES: {{^}}short sValueShort = 0;

unsigned ValueUnsigned = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:10: warning: invalid case style for global variable 'ValueUnsigned' [readability-identifier-naming]
// CHECK-FIXES: {{^}}unsigned uValueUnsigned = 0;

signed ValueSigned = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: invalid case style for global variable 'ValueSigned' [readability-identifier-naming]
// CHECK-FIXES: {{^}}signed iValueSigned = 0;

long ValueLong = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: invalid case style for global variable 'ValueLong' [readability-identifier-naming]
// CHECK-FIXES: {{^}}long lValueLong = 0;

long long ValueLongLong = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: invalid case style for global variable 'ValueLongLong' [readability-identifier-naming]
// CHECK-FIXES: {{^}}long long llValueLongLong = 0;

long long &RefValueLongLong = ValueLongLong;
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: invalid case style for global variable 'RefValueLongLong' [readability-identifier-naming]
// CHECK-FIXES: {{^}}long long &llRefValueLongLong = llValueLongLong;

long double ValueLongDouble = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: invalid case style for global variable 'ValueLongDouble' [readability-identifier-naming]
// CHECK-FIXES: {{^}}long double ldValueLongDouble = 0;

volatile int VolatileInt = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:14: warning: invalid case style for global variable 'VolatileInt' [readability-identifier-naming]
// CHECK-FIXES: {{^}}volatile int iVolatileInt = 0;

const int &ConstRefValue = ValueInt;
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: invalid case style for global variable 'ConstRefValue' [readability-identifier-naming]
// CHECK-FIXES: {{^}}const int &iConstRefValue = iValueInt;

thread_local int ThreadLocalValueInt = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:18: warning: invalid case style for global variable 'ThreadLocalValueInt' [readability-identifier-naming]
// CHECK-FIXES: {{^}}thread_local int iThreadLocalValueInt = 0;

extern int ExternValueInt;
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: invalid case style for global variable 'ExternValueInt' [readability-identifier-naming]
// CHECK-FIXES: {{^}}extern int iExternValueInt;

void MyFunc1(int Val){}
// CHECK-MESSAGES: :[[@LINE-1]]:18: warning: invalid case style for parameter 'Val' [readability-identifier-naming]
// CHECK-FIXES: {{^}}void MyFunc1(int iVal){}

void MyFunc2(void* Val){}
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: invalid case style for parameter 'Val' [readability-identifier-naming]
// CHECK-FIXES: {{^}}void MyFunc2(void* pVal){}

static constexpr int const &ConstExprInt = 42;
// CHECK-MESSAGES: :[[@LINE-1]]:29: warning: invalid case style for global variable 'ConstExprInt' [readability-identifier-naming]
// CHECK-FIXES: {{^}}static constexpr int const &iConstExprInt = 42;

DWORD MsDword = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: invalid case style for global variable 'MsDword' [readability-identifier-naming]
// CHECK-FIXES: {{^}}DWORD dwMsDword = 0;

BYTE MsByte = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: invalid case style for global variable 'MsByte' [readability-identifier-naming]
// CHECK-FIXES: {{^}}BYTE byMsByte = 0;

WORD MsWord = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: invalid case style for global variable 'MsWord' [readability-identifier-naming]
// CHECK-FIXES: {{^}}WORD wMsWord = 0;

BOOL MsBool = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: invalid case style for global variable 'MsBool' [readability-identifier-naming]
// CHECK-FIXES: {{^}}BOOL bMsBool = 0;

BOOLEAN MsBoolean = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: invalid case style for global variable 'MsBoolean' [readability-identifier-naming]
// CHECK-FIXES: {{^}}BOOLEAN bMsBoolean = 0;
