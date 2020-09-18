// RUN: %check_clang_tidy %s readability-identifier-naming %t -- \
// RUN:   -config='{ CheckOptions: [ \
// RUN:     { key: readability-identifier-naming.ClassMemberCase                                   , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.ConstantCase                                      , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.ConstantMemberCase                                , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.ConstantParameterCase                             , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.ConstantPointerParameterCase                      , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.ConstexprVariableCase                             , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.GlobalConstantCase                                , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.GlobalConstantPointerCase                         , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.GlobalVariableCase                                , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.LocalConstantCase                                 , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.LocalConstantPointerCase                          , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.LocalPointerCase                                  , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.LocalVariableCase                                 , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.MemberCase                                        , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.ParameterCase                                     , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.PointerParameterCase                              , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.PrivateMemberCase                                 , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.StaticConstantCase                                , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.StaticVariableCase                                , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.StructCase                                        , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.UnionCase                                         , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.VariableCase                                      , value: szHungarianNotation }, \
// RUN:     { key: readability-identifier-naming.HungarianNotation.Options.ClassPrefixWithC        , value: 1    }, \
// RUN:     { key: readability-identifier-naming.HungarianNotation.Options.VirtualClassPrefixWithI , value: 1    }, \
// RUN:   ]}'

// clang-format off
//===----------------------------------------------------------------------===//
// Class
//===----------------------------------------------------------------------===//
class MyClass { int Func(); };
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: invalid case style for struct 'MyClass' [readability-identifier-naming]
// CHECK-FIXES: {{^}}class CMyClass { int Func(); };

class AbstractClass { virtual int Func() = 0; };
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: invalid case style for struct 'AbstractClass' [readability-identifier-naming]
// CHECK-FIXES: {{^}}class IAbstractClass { virtual int Func() = 0; };

class AbstractClass1 { virtual int Func1() = 0; int Func2(); };
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: invalid case style for struct 'AbstractClass1' [readability-identifier-naming]
// CHECK-FIXES: {{^}}class IAbstractClass1 { virtual int Func1() = 0; int Func2(); };
// clang-format on
