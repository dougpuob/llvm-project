#include <stddef.h>
#include <stdint.h>

// RUN: %check_clang_tidy %s readability-identifier-naming %t -- \
// RUN:   -config="{CheckOptions: [\
// RUN:   {key: readability-identifier-naming.VariableCase, value: szHungarianNotation}, \
// RUN:   ]}"

class UnlistedClass {};
UnlistedClass Unlisted1;
// CHECK-NOT: :[[@LINE-2]]

UnlistedClass cUnlisted2;
// CHECK-MESSAGES: :[[@LINE-1]]:15: warning: invalid case style for variable 'cUnlisted2' [readability-identifier-naming]
// CHECK-FIXES: {{^}}UnlistedClass Unlisted2;

UnlistedClass objUnlistedClass3;
// CHECK-MESSAGES: :[[@LINE-1]]:15: warning: invalid case style for variable 'objUnlistedClass3' [readability-identifier-naming]
// CHECK-FIXES: {{^}}UnlistedClass UnlistedClass3;

typedef int INDEX;
INDEX iIndex = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: invalid case style for variable 'iIndex' [readability-identifier-naming]
// CHECK-FIXES: {{^}}INDEX Index = 0;

const char *NamePtr = "Name";
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: invalid case style for variable 'NamePtr' [readability-identifier-naming]
// CHECK-FIXES: {{^}}const char *szNamePtr = "Name";

const char NameArray[] = "Name";
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: invalid case style for variable 'NameArray' [readability-identifier-naming]
// CHECK-FIXES: {{^}}const char szNameArray[] = "Name";

void *BufferPtr1 = NULL;
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: invalid case style for variable 'BufferPtr1' [readability-identifier-naming]
// CHECK-FIXES: {{^}}void *pBufferPtr1 = NULL;

void **BufferPtr2 = NULL;
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: invalid case style for variable 'BufferPtr2' [readability-identifier-naming]
// CHECK-FIXES: {{^}}void **ppBufferPtr2 = NULL;

void **pBufferPtr3 = NULL;
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: invalid case style for variable 'pBufferPtr3' [readability-identifier-naming]
// CHECK-FIXES: {{^}}void **ppBufferPtr3 = NULL;

int8_t ValueI8 = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: invalid case style for variable 'ValueI8' [readability-identifier-naming]
// CHECK-FIXES: {{^}}int8_t i8ValueI8 = 0;

int16_t ValueI16 = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: invalid case style for variable 'ValueI16' [readability-identifier-naming]
// CHECK-FIXES: {{^}}int16_t i16ValueI16 = 0;

int32_t ValueI32 = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: invalid case style for variable 'ValueI32' [readability-identifier-naming]
// CHECK-FIXES: {{^}}int32_t i32ValueI32 = 0;

int64_t ValueI64 = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: invalid case style for variable 'ValueI64' [readability-identifier-naming]
// CHECK-FIXES: {{^}}int64_t i64ValueI64 = 0;

uint8_t ValueU8 = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: invalid case style for variable 'ValueU8' [readability-identifier-naming]
// CHECK-FIXES: {{^}}uint8_t u8ValueU8 = 0;

uint16_t ValueU16 = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:10: warning: invalid case style for variable 'ValueU16' [readability-identifier-naming]
// CHECK-FIXES: {{^}}uint16_t u16ValueU16 = 0;

uint32_t ValueU32 = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:10: warning: invalid case style for variable 'ValueU32' [readability-identifier-naming]
// CHECK-FIXES: {{^}}uint32_t u32ValueU32 = 0;

uint64_t ValueU64 = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:10: warning: invalid case style for variable 'ValueU64' [readability-identifier-naming]
// CHECK-FIXES: {{^}}uint64_t u64ValueU64 = 0;

float ValueFloat = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: invalid case style for variable 'ValueFloat' [readability-identifier-naming]
// CHECK-FIXES: {{^}}float fValueFloat = 0;

double ValueDouble = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: invalid case style for variable 'ValueDouble' [readability-identifier-naming]
// CHECK-FIXES: {{^}}double dValueDouble = 0;

char ValueChar = 'c';
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: invalid case style for variable 'ValueChar' [readability-identifier-naming]
// CHECK-FIXES: {{^}}char cValueChar = 'c';

bool ValueBool = true;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: invalid case style for variable 'ValueBool' [readability-identifier-naming]
// CHECK-FIXES: {{^}}bool bValueBool = true;

int ValueInt = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: invalid case style for variable 'ValueInt' [readability-identifier-naming]
// CHECK-FIXES: {{^}}int iValueInt = 0;

size_t ValueSize = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: invalid case style for variable 'ValueSize' [readability-identifier-naming]
// CHECK-FIXES: {{^}}size_t nValueSize = 0;

wchar_t ValueWchar = 'w';
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: invalid case style for variable 'ValueWchar' [readability-identifier-naming]
// CHECK-FIXES: {{^}}wchar_t wcValueWchar = 'w';

short ValueShort = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: invalid case style for variable 'ValueShort' [readability-identifier-naming]
// CHECK-FIXES: {{^}}short sValueShort = 0;

unsigned ValueUnsigned = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:10: warning: invalid case style for variable 'ValueUnsigned' [readability-identifier-naming]
// CHECK-FIXES: {{^}}unsigned uValueUnsigned = 0;

signed ValueSigned = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: invalid case style for variable 'ValueSigned' [readability-identifier-naming]
// CHECK-FIXES: {{^}}signed iValueSigned = 0;

long ValueLong = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: invalid case style for variable 'ValueLong' [readability-identifier-naming]
// CHECK-FIXES: {{^}}long lValueLong = 0;
