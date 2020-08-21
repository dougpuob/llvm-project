// RUN: %check_clang_tidy %s readability-identifier-naming %t -- \
// RUN:   -config='{CheckOptions: [ \
// RUN:     {key: readability-identifier-naming.VariableCase, value: szHungarainNotion} \
// RUN:   ]}'

char* aszName2 = "Test2";
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: invalid case style for variable 'aszName2'
// CHECK-FIXES: {{^}}Hungarain Notion szName2 {{{$}}