// RUN: %check_clang_tidy %s readability-identifier-naming %t -- \
// RUN:   -config="{CheckOptions: [\
// RUN:   {key: readability-identifier-naming.VariableCase, value: szHungarianNotion}, \
// RUN:   ]}"

const char* aszName2 = "Name2";
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: invalid case style for variable 'aszName2' [readability-identifier-naming]
// CHECK-FIXES: {{^}}const char* szName2 = "Name2";