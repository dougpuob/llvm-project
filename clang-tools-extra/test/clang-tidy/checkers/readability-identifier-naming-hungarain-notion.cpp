// RUN: %check_clang_tidy %s readability-identifier-naming %t -- \
// RUN:   -config="{CheckOptions: [\
// RUN:   {key: readability-identifier-naming.VariableCase, value: szHungarianNotion}, \
// RUN:   ]}"

const char* NamePtr = "Name";
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: invalid case style for variable 'NamePtr' [readability-identifier-naming]
// CHECK-FIXES: {{^}}const char* szNamePtr = "Name";

const char NameArray[] = "Name";
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: invalid case style for variable 'NameArray' [readability-identifier-naming]
// CHECK-FIXES: {{^}}const char szNameArray[] = "Name";