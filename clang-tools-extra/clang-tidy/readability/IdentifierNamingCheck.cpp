//===--- IdentifierNamingCheck.cpp - clang-tidy ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "IdentifierNamingCheck.h"

#include "../GlobList.h"
#include "clang/AST/CXXInheritance.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Regex.h"

#define DEBUG_TYPE "clang-tidy"

// FixItHint

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {

llvm::ArrayRef<
    std::pair<readability::IdentifierNamingCheck::CaseType, StringRef>>
OptionEnumMapping<
    readability::IdentifierNamingCheck::CaseType>::getEnumMapping() {
  static constexpr std::pair<readability::IdentifierNamingCheck::CaseType,
                             StringRef>
      Mapping[] = {
          {readability::IdentifierNamingCheck::CT_AnyCase, "aNy_CasE"},
          {readability::IdentifierNamingCheck::CT_LowerCase, "lower_case"},
          {readability::IdentifierNamingCheck::CT_UpperCase, "UPPER_CASE"},
          {readability::IdentifierNamingCheck::CT_CamelBack, "camelBack"},
          {readability::IdentifierNamingCheck::CT_CamelCase, "CamelCase"},
          {readability::IdentifierNamingCheck::CT_CamelSnakeCase,
           "Camel_Snake_Case"},
          {readability::IdentifierNamingCheck::CT_CamelSnakeBack,
           "camel_Snake_Back"}};
  return llvm::makeArrayRef(Mapping);
}

template <>
struct OptionEnumMapping<
    readability::IdentifierNamingCheck::HungarianPrefixType> {
  using HungarianPrefixType =
      readability::IdentifierNamingCheck::HungarianPrefixType;
  static llvm::ArrayRef<std::pair<HungarianPrefixType, StringRef>>
  getEnumMapping() {
    static constexpr std::pair<HungarianPrefixType, StringRef> Mapping[] = {
        {HungarianPrefixType::HPT_Off, "Off"},
        {HungarianPrefixType::HPT_On, "On"},
        {HungarianPrefixType::HPT_LowerCase, "LowerCase"},
        {HungarianPrefixType::HPT_CamelCase, "CamelCase"}};
    return llvm::makeArrayRef(Mapping);
  }
};

namespace readability {

// clang-format off
#define NAMING_KEYS(m) \
    m(Namespace) \
    m(InlineNamespace) \
    m(EnumConstant) \
    m(ScopedEnumConstant) \
    m(ConstexprVariable) \
    m(ConstantMember) \
    m(PrivateMember) \
    m(ProtectedMember) \
    m(PublicMember) \
    m(Member) \
    m(ClassConstant) \
    m(ClassMember) \
    m(GlobalConstant) \
    m(GlobalConstantPointer) \
    m(GlobalPointer) \
    m(GlobalVariable) \
    m(LocalConstant) \
    m(LocalConstantPointer) \
    m(LocalPointer) \
    m(LocalVariable) \
    m(StaticConstant) \
    m(StaticVariable) \
    m(Constant) \
    m(Variable) \
    m(ConstantParameter) \
    m(ParameterPack) \
    m(Parameter) \
    m(PointerParameter) \
    m(ConstantPointerParameter) \
    m(AbstractClass) \
    m(Struct) \
    m(Class) \
    m(Union) \
    m(Enum) \
    m(GlobalFunction) \
    m(ConstexprFunction) \
    m(Function) \
    m(ConstexprMethod) \
    m(VirtualMethod) \
    m(ClassMethod) \
    m(PrivateMethod) \
    m(ProtectedMethod) \
    m(PublicMethod) \
    m(Method) \
    m(Typedef) \
    m(TypeTemplateParameter) \
    m(ValueTemplateParameter) \
    m(TemplateTemplateParameter) \
    m(TemplateParameter) \
    m(TypeAlias) \
    m(MacroDefinition) \
    m(ObjcIvar) \

enum StyleKind {
#define ENUMERATE(v) SK_ ## v,
  NAMING_KEYS(ENUMERATE)
#undef ENUMERATE
  SK_Count,
  SK_Invalid
};

static StringRef const StyleNames[] = {
#define STRINGIZE(v) #v,
  NAMING_KEYS(STRINGIZE)
#undef STRINGIZE
};

#define HUNGARIAN_NOTATION_PRIMITIVE_TYPES(m) \
     m(int8_t) \
     m(int16_t) \
     m(int32_t) \
     m(int64_t) \
     m(uint8_t) \
     m(uint16_t) \
     m(uint32_t) \
     m(uint64_t) \
     m(char8_t) \
     m(char16_t) \
     m(char32_t) \
     m(float) \
     m(double) \
     m(char) \
     m(bool) \
     m(_Bool) \
     m(int) \
     m(size_t) \
     m(wchar_t) \
     m(short-int) \
     m(short) \
     m(signed-int) \
     m(signed-short) \
     m(signed-short-int) \
     m(signed-long-long-int) \
     m(signed-long-long) \
     m(signed-long-int) \
     m(signed-long) \
     m(signed) \
     m(unsigned-long-long-int) \
     m(unsigned-long-long) \
     m(unsigned-long-int) \
     m(unsigned-long) \
     m(unsigned-short-int) \
     m(unsigned-short) \
     m(unsigned-int) \
     m(unsigned) \
     m(long-long-int) \
     m(long-double) \
     m(long-long) \
     m(long-int) \
     m(long) \
     m(ptrdiff_t) \

static StringRef const HungarainNotationPrimitiveTypes[] = {
#define STRINGIZE(v) #v,
  HUNGARIAN_NOTATION_PRIMITIVE_TYPES(STRINGIZE)
#undef STRINGIZE
};

#define HUNGARIAN_NOTATION_USER_DEFINED_TYPES(m) \
     m(BOOL) \
     m(BOOLEAN) \
     m(BYTE) \
     m(CHAR) \
     m(UCHAR) \
     m(SHORT) \
     m(USHORT) \
     m(WORD) \
     m(DWORD) \
     m(DWORD32) \
     m(DWORD64) \
     m(LONG) \
     m(ULONG) \
     m(ULONG32) \
     m(ULONG64) \
     m(ULONGLONG) \
     m(HANDLE) \
     m(INT) \
     m(INT8) \
     m(INT16) \
     m(INT32) \
     m(INT64) \
     m(UINT) \
     m(UINT8) \
     m(UINT16) \
     m(UINT32) \
     m(UINT64) \
     m(PVOID) \

static StringRef const HungarainNotationUserDefinedTypes[] = {
#define STRINGIZE(v) #v,
  HUNGARIAN_NOTATION_USER_DEFINED_TYPES(STRINGIZE)
#undef STRINGIZE
};


#undef NAMING_KEYS
// clang-format on

static void getHungarianNotationDefaultConfig(
    IdentifierNamingCheck::HungarianNotationOption &HNOption) {

  // Options
  static constexpr std::pair<StringRef, StringRef> General[] = {
      {"TreatStructAsClass", "false"}};
  for (const auto &G : General)
    HNOption.General.try_emplace(G.first, G.second);

  // Derived types
  static constexpr std::pair<StringRef, StringRef> DerivedTypes[] = {
      {"Array", "a"}, {"Pointer", "p"}, {"FunctionPointer", "fn"}};
  for (const auto &DT : DerivedTypes)
    HNOption.DerivedType.try_emplace(DT.first, DT.second);

  // C strings
  static constexpr std::pair<StringRef, StringRef> CStrings[] = {
      {"char*", "sz"},
      {"char[]", "sz"},
      {"wchar_t*", "wsz"},
      {"wchar_t[]", "wsz"}};
  for (const auto &CStr : CStrings)
    HNOption.CString.try_emplace(CStr.first, CStr.second);

  // clang-format off
  static constexpr std::pair<StringRef, StringRef> PrimitiveTypes[] = {
        {"int8_t",                  "i8"  },
        {"int16_t",                 "i16" },
        {"int32_t",                 "i32" },
        {"int64_t",                 "i64" },
        {"uint8_t",                 "u8"  },
        {"uint16_t",                "u16" },
        {"uint32_t",                "u32" },
        {"uint64_t",                "u64" },
        {"char8_t",                 "c8"  },
        {"char16_t",                "c16" },
        {"char32_t",                "c32" },
        {"float",                   "f"   },
        {"double",                  "d"   },
        {"char",                    "c"   },
        {"bool",                    "b"   },
        {"_Bool",                   "b"   },
        {"int",                     "i"   },
        {"size_t",                  "n"   },
        {"wchar_t",                 "wc"  },
        {"short int",               "si"  },
        {"short",                   "s"   },
        {"signed int",              "si"  },
        {"signed short",            "ss"  },
        {"signed short int",        "ssi" },
        {"signed long long int",    "slli"},
        {"signed long long",        "sll" },
        {"signed long int",         "sli" },
        {"signed long",             "sl"  },
        {"signed",                  "s"   },
        {"unsigned long long int",  "ulli"},
        {"unsigned long long",      "ull" },
        {"unsigned long int",       "uli" },
        {"unsigned long",           "ul"  },
        {"unsigned short int",      "usi" },
        {"unsigned short",          "us"  },
        {"unsigned int",            "ui"  },
        {"unsigned",                "u"   },
        {"long long int",           "lli" },
        {"long double",             "ld"  },
        {"long long",               "ll"  },
        {"long int",                "li"  },
        {"long",                    "l"   },
        {"ptrdiff_t",               "p"   }};
  // clang-format on
  for (const auto &PT : PrimitiveTypes)
    HNOption.PrimitiveType.try_emplace(PT.first, PT.second);

  // clang-format off
  static constexpr std::pair<StringRef, StringRef> UserDefinedTypes[] = {
      // Windows data types
      {"BOOL",                    "b"   },
      {"BOOLEAN",                 "b"   },
      {"BYTE",                    "by"  },
      {"CHAR",                    "c"   },
      {"UCHAR",                   "uc"  },
      {"SHORT",                   "s"   },
      {"USHORT",                  "us"  },
      {"WORD",                    "w"   },
      {"DWORD",                   "dw"  },
      {"DWORD32",                 "dw32"},
      {"DWORD64",                 "dw64"},
      {"LONG",                    "l"   },
      {"ULONG",                   "ul"  },
      {"ULONG32",                 "ul32"},
      {"ULONG64",                 "ul64"},
      {"ULONGLONG",               "ull" },
      {"HANDLE",                  "h"   },
      {"INT",                     "i"   },
      {"INT8",                    "i8"  },
      {"INT16",                   "i16" },
      {"INT32",                   "i32" },
      {"INT64",                   "i64" },
      {"UINT",                    "ui"  },
      {"UINT8",                   "u8"  },
      {"UINT16",                  "u16" },
      {"UINT32",                  "u32" },
      {"UINT64",                  "u64" },
      {"PVOID",                   "p"   } };
  // clang-format on
  for (const auto &UDT : UserDefinedTypes)
    HNOption.UserDefinedType.try_emplace(UDT.first, UDT.second);
}

static constexpr StringRef HNOpts[] = {"TreatStructAsClass"};
static constexpr StringRef HNDerivedTypes[] = {"Array", "Pointer",
                                               "FunctionPointer"};
static void getHungarianNotationFileConfig(
    const ClangTidyCheck::OptionsView &Options,
    IdentifierNamingCheck::HungarianNotationOption &HNOption) {
  StringRef Section = "HungarianNotation.";

  SmallString<128> Buffer;
  for (const auto &Opt : HNOpts) {
    Buffer.assign({Section, "General.", Opt});
    std::string Val = Options.get(Buffer, "");
    if (!Val.empty())
      HNOption.General[Opt] = std::move(Val);
  }

  for (const auto &Type : HNDerivedTypes) {
    Buffer.assign({Section, "DerivedType.", Type});
    std::string Val = Options.get(Buffer, "");
    if (!Val.empty())
      HNOption.DerivedType[Type] = std::move(Val);
  }

  static constexpr std::pair<StringRef, StringRef> HNCStrings[] = {
      {"CharPrinter", "char*"},
      {"CharArray", "char[]"},
      {"WideCharPrinter", "wchar_t*"},
      {"WideCharArray", "wchar_t[]"}};

  for (const auto &CStr : HNCStrings) {
    Buffer.assign({Section, "CString.", CStr.first});
    std::string Val = Options.get(Buffer, "");
    if (!Val.empty())
      HNOption.CString[CStr.first] = std::move(Val);
  }

  for (const auto &PrimType : HungarainNotationPrimitiveTypes) {
    Buffer.assign({Section, "PrimitiveType.", PrimType});
    std::string Val = Options.get(Buffer, "");
    if (!Val.empty()) {
      std::string Type = PrimType.str();
      std::replace(Type.begin(), Type.end(), '-', ' ');
      HNOption.PrimitiveType[Type] = std::move(Val);
    }
  }

  for (const auto &Type : HungarainNotationUserDefinedTypes) {
    Buffer.assign({Section, "UserDefinedType.", Type});
    std::string Val = Options.get(Buffer, "");
    if (!Val.empty())
      HNOption.UserDefinedType[Type] = std::move(Val);
  }
}

static bool
isHungarianNotationOptionEnabled(StringRef OptionKey,
                                 const llvm::StringMap<std::string> &StrMap) {
  if (OptionKey.empty())
    return false;

  std::string OptionVal = StrMap.lookup(OptionKey);
  llvm::transform(OptionVal, OptionVal.begin(), toupper);

  if (OptionVal == "1" || OptionVal == "TRUE" || OptionVal == "ON")
    return true;

  return false;
}

IdentifierNamingCheck::NamingStyle::NamingStyle(
    llvm::Optional<IdentifierNamingCheck::CaseType> Case,
    const std::string &Prefix, const std::string &Suffix,
    const std::string &IgnoredRegexpStr, HungarianPrefixType HPType)
    : Case(Case), Prefix(Prefix), Suffix(Suffix),
      IgnoredRegexpStr(IgnoredRegexpStr), HPType(HPType) {
  if (!IgnoredRegexpStr.empty()) {
    IgnoredRegexp =
        llvm::Regex(llvm::SmallString<128>({"^", IgnoredRegexpStr, "$"}));
    if (!IgnoredRegexp.isValid())
      llvm::errs() << "Invalid IgnoredRegexp regular expression: "
                   << IgnoredRegexpStr;
  }
}

static IdentifierNamingCheck::FileStyle
getFileStyleFromOptions(const ClangTidyCheck::OptionsView &Options) {
  IdentifierNamingCheck::HungarianNotationOption HNOption;
  getHungarianNotationDefaultConfig(HNOption);
  getHungarianNotationFileConfig(Options, HNOption);
  SmallVector<llvm::Optional<IdentifierNamingCheck::NamingStyle>, 0> Styles;
  Styles.resize(SK_Count);
  SmallString<64> StyleString;
  for (unsigned I = 0; I < SK_Count; ++I) {
    StyleString = StyleNames[I];
    size_t StyleSize = StyleString.size();

    std::string HPrefixKey = (StyleString + "HungarianPrefix").str();
    using HungarianPrefixType = IdentifierNamingCheck::HungarianPrefixType;
    auto HPTVal = HungarianPrefixType::HPT_Off;
    std::string HPrefixVal = Options.get(HPrefixKey, "");
    if (!HPrefixVal.empty()) {
      if (auto HPTypeVal = Options.get<HungarianPrefixType>(HPrefixKey))
        HPTVal = HPTypeVal.get();
    }
	
    StyleString.append("IgnoredRegexp");
    std::string IgnoredRegexpStr = Options.get(StyleString, "");
    StyleString.resize(StyleSize);
    StyleString.append("Prefix");
    std::string Prefix(Options.get(StyleString, ""));
    // Fast replacement of [Pre]fix -> [Suf]fix.
    memcpy(&StyleString[StyleSize], "Suf", 3);
    std::string Postfix(Options.get(StyleString, ""));
    memcpy(&StyleString[StyleSize], "Case", 4);
    StyleString.pop_back();
    StyleString.pop_back();
    auto CaseOptional =
        Options.getOptional<IdentifierNamingCheck::CaseType>(StyleString);

    if (CaseOptional || !Prefix.empty() || !Postfix.empty() ||
        !IgnoredRegexpStr.empty() || !HPrefixVal.empty())
      Styles[I].emplace(std::move(CaseOptional), std::move(Prefix),
                        std::move(Postfix), std::move(IgnoredRegexpStr),
                        HPTVal);
  }
  bool IgnoreMainLike = Options.get("IgnoreMainLikeFunctions", false);
  return {std::move(Styles), std::move(HNOption), IgnoreMainLike};
}

IdentifierNamingCheck::IdentifierNamingCheck(StringRef Name,
                                             ClangTidyContext *Context)
    : RenamerClangTidyCheck(Name, Context), Context(Context), CheckName(Name),
      GetConfigPerFile(Options.get("GetConfigPerFile", true)),
      IgnoreFailedSplit(Options.get("IgnoreFailedSplit", false)) {
  auto IterAndInserted = NamingStylesCache.try_emplace(
      llvm::sys::path::parent_path(Context->getCurrentFile()),
      getFileStyleFromOptions(Options));
  assert(IterAndInserted.second && "Couldn't insert Style");
  // Holding a reference to the data in the vector is safe as it should never
  // move.
  MainFileStyle = &IterAndInserted.first->getValue();
}

IdentifierNamingCheck::~IdentifierNamingCheck() = default;

void IdentifierNamingCheck::storeOptions(ClangTidyOptions::OptionMap &Opts) {
  RenamerClangTidyCheck::storeOptions(Opts);
  SmallString<64> StyleString;
  ArrayRef<llvm::Optional<NamingStyle>> Styles = MainFileStyle->getStyles();
  for (size_t I = 0; I < SK_Count; ++I) {
    if (!Styles[I])
      continue;
    StyleString = StyleNames[I];
    size_t StyleSize = StyleString.size();
	
    Options.store(Opts, (StyleString + "HungarianPrefix").str(),
                  Styles[I]->HPType);

    StyleString.append("IgnoredRegexp");
    Options.store(Opts, StyleString, Styles[I]->IgnoredRegexpStr);
    StyleString.resize(StyleSize);
    StyleString.append("Prefix");
    Options.store(Opts, StyleString, Styles[I]->Prefix);
    // Fast replacement of [Pre]fix -> [Suf]fix.
    memcpy(&StyleString[StyleSize], "Suf", 3);
    Options.store(Opts, StyleString, Styles[I]->Suffix);
    if (Styles[I]->Case) {
      memcpy(&StyleString[StyleSize], "Case", 4);
      StyleString.pop_back();
      StyleString.pop_back();
      Options.store(Opts, StyleString, *Styles[I]->Case);
    }
  }
  Options.store(Opts, "GetConfigPerFile", GetConfigPerFile);
  Options.store(Opts, "IgnoreFailedSplit", IgnoreFailedSplit);
  Options.store(Opts, "IgnoreMainLikeFunctions",
                MainFileStyle->isIgnoringMainLikeFunction());
}

static const std::string getHungarianNotationDataTypePrefix(
    StringRef TypeName, const NamedDecl *ND,
    const IdentifierNamingCheck::HungarianNotationOption &HNOption) {
  if (!ND || TypeName.empty())
    return TypeName.str();

  std::string ModifiedTypeName(TypeName);

  // Derived types
  std::string PrefixStr;
  if (const auto *TD = dyn_cast<ValueDecl>(ND)) {
    QualType QT = TD->getType();
    if (QT->isFunctionPointerType()) {
      PrefixStr = HNOption.DerivedType.lookup("FunctionPointer");
    } else if (QT->isPointerType()) {
      for (const auto &CStr : HNOption.CString) {
        std::string Key = CStr.getKey().str();
        if (ModifiedTypeName.find(Key) == 0) {
          PrefixStr = CStr.getValue();
          ModifiedTypeName = ModifiedTypeName.substr(
              Key.size(), ModifiedTypeName.size() - Key.size());
          break;
        }
      }
    } else if (QT->isArrayType()) {
      for (const auto &CStr : HNOption.CString) {
        std::string Key = CStr.getKey().str();
        if (ModifiedTypeName.find(Key) == 0) {
          PrefixStr = CStr.getValue();
          break;
        }
      }
      if (PrefixStr.empty())
        PrefixStr = HNOption.DerivedType.lookup("Array");
    } else if (QT->isReferenceType()) {
      size_t Pos = ModifiedTypeName.find_last_of("&");
      if (Pos != std::string::npos)
        ModifiedTypeName = ModifiedTypeName.substr(0, Pos);
    }
  }

  // Pointers
  size_t PtrCount = [&](std::string TypeName) -> size_t {
    size_t Pos = TypeName.find('*');
    size_t Count = 0;
    for (; Pos < TypeName.length(); Pos++, Count++) {
      if ('*' != TypeName[Pos])
        break;
    }
    return Count;
  }(ModifiedTypeName);
  if (PtrCount > 0) {
    ModifiedTypeName = [&](std::string Str, StringRef From, StringRef To) {
      size_t StartPos = 0;
      while ((StartPos = Str.find(From.data(), StartPos)) !=
             std::string::npos) {
        Str.replace(StartPos, From.size(), To.data());
        StartPos += To.size();
      }
      return Str;
    }(ModifiedTypeName, "*", "");
  }

  // Primitive types
  if (PrefixStr.empty()) {
    for (const auto &Type : HNOption.PrimitiveType) {
      if (ModifiedTypeName == Type.getKey()) {
        PrefixStr = Type.getValue();
        break;
      }
    }
  }

  // User-Defined types
  if (PrefixStr.empty()) {
    for (const auto &Type : HNOption.UserDefinedType) {
      if (ModifiedTypeName == Type.getKey()) {
        PrefixStr = Type.getValue();
        break;
      }
    }
  }

  for (size_t Idx = 0; Idx < PtrCount; Idx++)
    PrefixStr.insert(0, HNOption.DerivedType.lookup("Pointer"));

  return PrefixStr;
}

static std::string getHungarianNotationClassPrefix(
    const CXXRecordDecl *CRD,
    const IdentifierNamingCheck::HungarianNotationOption &HNOption) {

  if (CRD->isUnion())
    return {};

  if (CRD->isStruct() &&
      !isHungarianNotationOptionEnabled("TreatStructAsClass", HNOption.General))
    return {};

  return CRD->isAbstract() ? "I" : "C";
}

static std::string getHungarianNotationEnumPrefix(const EnumConstantDecl *ECD) {
  std::string Name = ECD->getType().getAsString();
  if (std::string::npos != Name.find("enum")) {
    Name = Name.substr(strlen("enum"), Name.length() - strlen("enum"));
    Name = Name.erase(0, Name.find_first_not_of(" "));
  }

  static llvm::Regex Splitter(
      "([a-z0-9A-Z]*)(_+)|([A-Z]?[a-z0-9]+)([A-Z]|$)|([A-Z]+)([A-Z]|$)");

  StringRef EnumName(Name);
  SmallVector<StringRef, 8> Substrs;
  EnumName.split(Substrs, "_", -1, false);

  SmallVector<StringRef, 8> Words;
  SmallVector<StringRef, 8> Groups;
  for (auto Substr : Substrs) {
    while (!Substr.empty()) {
      Groups.clear();
      if (!Splitter.match(Substr, &Groups))
        break;

      if (Groups[2].size() > 0) {
        Words.push_back(Groups[1]);
        Substr = Substr.substr(Groups[0].size());
      } else if (Groups[3].size() > 0) {
        Words.push_back(Groups[3]);
        Substr = Substr.substr(Groups[0].size() - Groups[4].size());
      } else if (Groups[5].size() > 0) {
        Words.push_back(Groups[5]);
        Substr = Substr.substr(Groups[0].size() - Groups[6].size());
      }
    }
  }

  std::string Initial;
  for (StringRef Word : Words)
    Initial += tolower(Word[0]);

  return Initial;
}

static std::string getDeclTypeName(const NamedDecl *ND) {
  const auto *VD = dyn_cast<ValueDecl>(ND);
  if (!VD)
    return {};

  if (isa<FunctionDecl, EnumConstantDecl>(ND))
    return {};

  // Get type text of variable declarations.
  auto &SM = VD->getASTContext().getSourceManager();
  const char *Begin = SM.getCharacterData(VD->getBeginLoc());
  const char *End = SM.getCharacterData(VD->getEndLoc());
  intptr_t StrLen = End - Begin;

  // FIXME: Sometimes the value that returns from ValDecl->getEndLoc()
  // is wrong(out of location of Decl). This causes `StrLen` will be assigned
  // an unexpected large value. Current workaround to find the terminated
  // character instead of the `getEndLoc()` function.
  const char *EOL = strchr(Begin, '\n');
  if (!EOL)
    EOL = Begin + strlen(Begin);

  const std::vector<const char *> PosList = {
      strchr(Begin, '='), strchr(Begin, ';'), strchr(Begin, ','),
      strchr(Begin, ')'), EOL};
  for (const auto &Pos : PosList) {
    if (Pos > Begin)
      EOL = std::min(EOL, Pos);
  }

  StrLen = EOL - Begin;
  std::string TypeName;
  if (StrLen > 0) {
    std::string Type(Begin, StrLen);

    static constexpr StringRef Keywords[] = {
        // Constexpr specifiers
        "constexpr", "constinit", "consteval",
        // Qualifier
        "const", "volatile", "restrict", "mutable",
        // Storage class specifiers
        "register", "static", "extern", "thread_local",
        // Other keywords
        "virtual"};

    // Remove keywords
    for (StringRef Kw : Keywords) {
      for (size_t Pos = 0;
           (Pos = Type.find(Kw.data(), Pos)) != std::string::npos;) {
        Type.replace(Pos, Kw.size(), "");
      }
    }
    TypeName = Type.erase(0, Type.find_first_not_of(" "));

    // Replace spaces with single space.
    for (size_t Pos = 0; (Pos = Type.find("  ", Pos)) != std::string::npos;
         Pos += strlen(" ")) {
      Type.replace(Pos, strlen("  "), " ");
    }

    // Replace " &" with "&".
    for (size_t Pos = 0; (Pos = Type.find(" &", Pos)) != std::string::npos;
         Pos += strlen("&")) {
      Type.replace(Pos, strlen(" &"), "&");
    }

    // Replace " *" with "* ".
    for (size_t Pos = 0; (Pos = Type.find(" *", Pos)) != std::string::npos;
         Pos += strlen("*")) {
      Type.replace(Pos, strlen(" *"), "* ");
    }

    // Remove redundant tailing.
    static constexpr StringRef TailsOfMultiWordType[] = {
        " int", " char", " double", " long", " short"};
    bool RedundantRemoved = false;
    for (auto Kw : TailsOfMultiWordType) {
      size_t Pos = Type.rfind(Kw.data());
      if (Pos != std::string::npos) {
        Type = Type.substr(0, Pos + Kw.size());
        RedundantRemoved = true;
        break;
      }
    }
    TypeName = Type.erase(0, Type.find_first_not_of(" "));
    if (!RedundantRemoved) {
      std::size_t FoundSpace = Type.find(" ");
      if (FoundSpace != std::string::npos)
        Type = Type.substr(0, FoundSpace);
    }

    TypeName = Type.erase(0, Type.find_first_not_of(" "));

    QualType QT = VD->getType();
    if (!QT.isNull() && QT->isArrayType())
      TypeName.append("[]");
  }

  return TypeName;
}

static std::string getHungarianNotationPrefix(
    const Decl *D,
    const IdentifierNamingCheck::HungarianNotationOption &HNOption) {
  const auto *ND = dyn_cast<NamedDecl>(D);
  if (!ND)
    return {};

  std::string Prefix;
  if (const auto *ECD = dyn_cast<EnumConstantDecl>(ND)) {
    Prefix = getHungarianNotationEnumPrefix(ECD);
  } else if (const auto *CRD = dyn_cast<CXXRecordDecl>(ND)) {
    Prefix = getHungarianNotationClassPrefix(CRD, HNOption);
  } else if (isa<VarDecl, FieldDecl, RecordDecl>(ND)) {
    std::string TypeName = getDeclTypeName(ND);
    if (!TypeName.empty())
      Prefix = getHungarianNotationDataTypePrefix(TypeName, ND, HNOption);
  }

  return Prefix;
}

static bool removeDuplicatedHungarianNotationPrefix(
    SmallVector<StringRef, 8> &Words,
    const IdentifierNamingCheck::HungarianNotationOption &HNOption) {
  if (Words.size() <= 1)
    return true;

  std::string CorrectName = Words[0].str();
  std::vector<llvm::StringMap<std::string>> MapList = {
      HNOption.CString, HNOption.DerivedType, HNOption.PrimitiveType,
      HNOption.UserDefinedType};

  for (const auto &Map : MapList) {
    for (const auto &Str : Map) {
      if (Str.getValue() == CorrectName) {
        Words.erase(Words.begin(), Words.begin() + 1);
        return true;
      }
    }
  }

  return false;
}

static bool
matchesStyle(StringRef Type, StringRef Name,
             const IdentifierNamingCheck::NamingStyle &Style,
             const IdentifierNamingCheck::HungarianNotationOption &HNOption,
             const NamedDecl *Decl) {
  static llvm::Regex Matchers[] = {
      llvm::Regex("^.*$"),
      llvm::Regex("^[a-z][a-z0-9_]*$"),
      llvm::Regex("^[a-z][a-zA-Z0-9]*$"),
      llvm::Regex("^[A-Z][A-Z0-9_]*$"),
      llvm::Regex("^[A-Z][a-zA-Z0-9]*$"),
      llvm::Regex("^[A-Z]([a-z0-9]*(_[A-Z])?)*"),
      llvm::Regex("^[a-z]([a-z0-9]*(_[A-Z])?)*"),
  };

  if (!Name.consume_front(Style.Prefix))
    return false;
  if (!Name.consume_back(Style.Suffix))
    return false;
  if (IdentifierNamingCheck::HungarianPrefixType::HPT_Off != Style.HPType) {
    std::string HNPrefix = getHungarianNotationPrefix(Decl, HNOption);
    if (!Name.consume_front(HNPrefix))
      return false;
  }

  // Ensure the name doesn't have any extra underscores beyond those specified
  // in the prefix and suffix.
  if (Name.startswith("_") || Name.endswith("_"))
    return false;

  if (Style.Case && !Matchers[static_cast<size_t>(*Style.Case)].match(Name))
    return false;

  return true;
}

static std::string
fixupWithCase(StringRef Type, StringRef Name, const Decl *D,
              const IdentifierNamingCheck::NamingStyle &Style,
              const IdentifierNamingCheck::HungarianNotationOption &HNOption,
              IdentifierNamingCheck::CaseType Case) {
  static llvm::Regex Splitter(
      "([a-z0-9A-Z]*)(_+)|([A-Z]?[a-z0-9]+)([A-Z]|$)|([A-Z]+)([A-Z]|$)");

  SmallVector<StringRef, 8> Substrs;
  Name.split(Substrs, "_", -1, false);

  SmallVector<StringRef, 8> Words;
  SmallVector<StringRef, 8> Groups;
  for (auto Substr : Substrs) {
    while (!Substr.empty()) {
      Groups.clear();
      if (!Splitter.match(Substr, &Groups))
        break;

      if (Groups[2].size() > 0) {
        Words.push_back(Groups[1]);
        Substr = Substr.substr(Groups[0].size());
      } else if (Groups[3].size() > 0) {
        Words.push_back(Groups[3]);
        Substr = Substr.substr(Groups[0].size() - Groups[4].size());
      } else if (Groups[5].size() > 0) {
        Words.push_back(Groups[5]);
        Substr = Substr.substr(Groups[0].size() - Groups[6].size());
      }
    }
  }

  if (Words.empty())
    return Name.str();

  if (IdentifierNamingCheck::HungarianPrefixType::HPT_Off != Style.HPType)
    removeDuplicatedHungarianNotationPrefix(Words, HNOption);

  SmallString<128> Fixup;
  switch (Case) {
  case IdentifierNamingCheck::CT_AnyCase:
    return Name.str();
    break;

  case IdentifierNamingCheck::CT_LowerCase:
    for (auto const &Word : Words) {
      if (&Word != &Words.front())
        Fixup += "_";
      Fixup += Word.lower();
    }
    break;

  case IdentifierNamingCheck::CT_UpperCase:
    for (auto const &Word : Words) {
      if (&Word != &Words.front())
        Fixup += "_";
      Fixup += Word.upper();
    }
    break;

  case IdentifierNamingCheck::CT_CamelCase:
    for (auto const &Word : Words) {
      Fixup += toupper(Word.front());
      Fixup += Word.substr(1).lower();
    }
    break;

  case IdentifierNamingCheck::CT_CamelBack:
    for (auto const &Word : Words) {
      if (&Word == &Words.front()) {
        Fixup += Word.lower();
      } else {
        Fixup += toupper(Word.front());
        Fixup += Word.substr(1).lower();
      }
    }
    break;

  case IdentifierNamingCheck::CT_CamelSnakeCase:
    for (auto const &Word : Words) {
      if (&Word != &Words.front())
        Fixup += "_";
      Fixup += toupper(Word.front());
      Fixup += Word.substr(1).lower();
    }
    break;

  case IdentifierNamingCheck::CT_CamelSnakeBack:
    for (auto const &Word : Words) {
      if (&Word != &Words.front()) {
        Fixup += "_";
        Fixup += toupper(Word.front());
      } else {
        Fixup += tolower(Word.front());
      }
      Fixup += Word.substr(1).lower();
    }
    break;
  }

  return Fixup.str().str();
}

static bool isParamInMainLikeFunction(const ParmVarDecl &ParmDecl,
                                      bool IncludeMainLike) {
  const auto *FDecl =
      dyn_cast_or_null<FunctionDecl>(ParmDecl.getParentFunctionOrMethod());
  if (!FDecl)
    return false;
  if (FDecl->isMain())
    return true;
  if (!IncludeMainLike)
    return false;
  if (FDecl->getAccess() != AS_public && FDecl->getAccess() != AS_none)
    return false;
  enum MainType { None, Main, WMain };
  auto IsCharPtrPtr = [](QualType QType) -> MainType {
    if (QType.isNull())
      return None;
    if (QType = QType->getPointeeType(), QType.isNull())
      return None;
    if (QType = QType->getPointeeType(), QType.isNull())
      return None;
    if (QType->isCharType())
      return Main;
    if (QType->isWideCharType())
      return WMain;
    return None;
  };
  auto IsIntType = [](QualType QType) {
    if (QType.isNull())
      return false;
    if (const auto *Builtin =
            dyn_cast<BuiltinType>(QType->getUnqualifiedDesugaredType())) {
      return Builtin->getKind() == BuiltinType::Int;
    }
    return false;
  };
  if (!IsIntType(FDecl->getReturnType()))
    return false;
  if (FDecl->getNumParams() < 2 || FDecl->getNumParams() > 3)
    return false;
  if (!IsIntType(FDecl->parameters()[0]->getType()))
    return false;
  MainType Type = IsCharPtrPtr(FDecl->parameters()[1]->getType());
  if (Type == None)
    return false;
  if (FDecl->getNumParams() == 3 &&
      IsCharPtrPtr(FDecl->parameters()[2]->getType()) != Type)
    return false;

  if (Type == Main) {
    static llvm::Regex Matcher(
        "(^[Mm]ain([_A-Z]|$))|([a-z0-9_]Main([_A-Z]|$))|(_main(_|$))");
    assert(Matcher.isValid() && "Invalid Matcher for main like functions.");
    return Matcher.match(FDecl->getName());
  } else {
    static llvm::Regex Matcher("(^((W[Mm])|(wm))ain([_A-Z]|$))|([a-z0-9_]W[Mm]"
                               "ain([_A-Z]|$))|(_wmain(_|$))");
    assert(Matcher.isValid() && "Invalid Matcher for wmain like functions.");
    return Matcher.match(FDecl->getName());
  }
}

static std::string
fixupWithStyle(StringRef Type, StringRef Name,
               const IdentifierNamingCheck::NamingStyle &Style,
               const IdentifierNamingCheck::HungarianNotationOption &HNOption,
               const Decl *D) {
  std::string Fixed = fixupWithCase(
      Type, Name, D, Style, HNOption,
      Style.Case.getValueOr(IdentifierNamingCheck::CaseType::CT_AnyCase));
  std::string HungarianPrefix;
  using HungarianPrefixType = IdentifierNamingCheck::HungarianPrefixType;
  if (HungarianPrefixType::HPT_Off != Style.HPType) {
    HungarianPrefix = getHungarianNotationPrefix(D, HNOption);
    if (!HungarianPrefix.empty()) {
      if (Style.HPType == HungarianPrefixType::HPT_LowerCase)
        HungarianPrefix += "_";

      if (Style.HPType == HungarianPrefixType::HPT_CamelCase)
        Fixed[0] = toupper(Fixed[0]);
    }
  }
  StringRef Mid = StringRef(Fixed).trim("_");
  if (Mid.empty())
    Mid = "_";

  return (Style.Prefix + HungarianPrefix + Mid + Style.Suffix).str();
}

static StyleKind findStyleKind(
    const NamedDecl *D,
    ArrayRef<llvm::Optional<IdentifierNamingCheck::NamingStyle>> NamingStyles,
    bool IgnoreMainLikeFunctions) {
  assert(D && D->getIdentifier() && !D->getName().empty() && !D->isImplicit() &&
         "Decl must be an explicit identifier with a name.");

  if (isa<ObjCIvarDecl>(D) && NamingStyles[SK_ObjcIvar])
    return SK_ObjcIvar;
  
  if (isa<TypedefDecl>(D) && NamingStyles[SK_Typedef])
    return SK_Typedef;

  if (isa<TypeAliasDecl>(D) && NamingStyles[SK_TypeAlias])
    return SK_TypeAlias;

  if (const auto *Decl = dyn_cast<NamespaceDecl>(D)) {
    if (Decl->isAnonymousNamespace())
      return SK_Invalid;

    if (Decl->isInline() && NamingStyles[SK_InlineNamespace])
      return SK_InlineNamespace;

    if (NamingStyles[SK_Namespace])
      return SK_Namespace;
  }

  if (isa<EnumDecl>(D) && NamingStyles[SK_Enum])
    return SK_Enum;

  if (const auto *EnumConst = dyn_cast<EnumConstantDecl>(D)) {
    if (cast<EnumDecl>(EnumConst->getDeclContext())->isScoped() &&
        NamingStyles[SK_ScopedEnumConstant])
      return SK_ScopedEnumConstant;

    if (NamingStyles[SK_EnumConstant])
      return SK_EnumConstant;

    if (NamingStyles[SK_Constant])
      return SK_Constant;

    return SK_Invalid;
  }

  if (const auto *Decl = dyn_cast<CXXRecordDecl>(D)) {
    if (Decl->isAnonymousStructOrUnion())
      return SK_Invalid;

    if (!Decl->getCanonicalDecl()->isThisDeclarationADefinition())
      return SK_Invalid;

    if (Decl->hasDefinition() && Decl->isAbstract() &&
        NamingStyles[SK_AbstractClass])
      return SK_AbstractClass;

    if (Decl->isStruct() && NamingStyles[SK_Struct])
      return SK_Struct;

    if (Decl->isStruct() && NamingStyles[SK_Class])
      return SK_Class;

    if (Decl->isClass() && NamingStyles[SK_Class])
      return SK_Class;

    if (Decl->isClass() && NamingStyles[SK_Struct])
      return SK_Struct;

    if (Decl->isUnion() && NamingStyles[SK_Union])
      return SK_Union;

    if (Decl->isEnum() && NamingStyles[SK_Enum])
      return SK_Enum;

    return SK_Invalid;
  }

  if (const auto *Decl = dyn_cast<FieldDecl>(D)) {
    QualType Type = Decl->getType();

    if (!Type.isNull() && Type.isConstQualified()) {
      if (NamingStyles[SK_ConstantMember])
        return SK_ConstantMember;

      if (NamingStyles[SK_Constant])
        return SK_Constant;
    }

    if (Decl->getAccess() == AS_private && NamingStyles[SK_PrivateMember])
      return SK_PrivateMember;

    if (Decl->getAccess() == AS_protected && NamingStyles[SK_ProtectedMember])
      return SK_ProtectedMember;

    if (Decl->getAccess() == AS_public && NamingStyles[SK_PublicMember])
      return SK_PublicMember;

    if (NamingStyles[SK_Member])
      return SK_Member;

    return SK_Invalid;
  }

  if (const auto *Decl = dyn_cast<ParmVarDecl>(D)) {
    if (isParamInMainLikeFunction(*Decl, IgnoreMainLikeFunctions))
      return SK_Invalid;
    QualType Type = Decl->getType();

    if (Decl->isConstexpr() && NamingStyles[SK_ConstexprVariable])
      return SK_ConstexprVariable;

    if (!Type.isNull() && Type.isConstQualified()) {
      if (Type.getTypePtr()->isAnyPointerType() && NamingStyles[SK_ConstantPointerParameter])
        return SK_ConstantPointerParameter;

      if (NamingStyles[SK_ConstantParameter])
        return SK_ConstantParameter;

      if (NamingStyles[SK_Constant])
        return SK_Constant;
    }

    if (Decl->isParameterPack() && NamingStyles[SK_ParameterPack])
      return SK_ParameterPack;

    if (!Type.isNull() && Type.getTypePtr()->isAnyPointerType() && NamingStyles[SK_PointerParameter])
        return SK_PointerParameter;

    if (NamingStyles[SK_Parameter])
      return SK_Parameter;

    return SK_Invalid;
  }

  if (const auto *Decl = dyn_cast<VarDecl>(D)) {
    QualType Type = Decl->getType();

    if (Decl->isConstexpr() && NamingStyles[SK_ConstexprVariable])
      return SK_ConstexprVariable;

    if (!Type.isNull() && Type.isConstQualified()) {
      if (Decl->isStaticDataMember() && NamingStyles[SK_ClassConstant])
        return SK_ClassConstant;

      if (Decl->isFileVarDecl() && Type.getTypePtr()->isAnyPointerType() && NamingStyles[SK_GlobalConstantPointer])
        return SK_GlobalConstantPointer;

      if (Decl->isFileVarDecl() && NamingStyles[SK_GlobalConstant])
        return SK_GlobalConstant;

      if (Decl->isStaticLocal() && NamingStyles[SK_StaticConstant])
        return SK_StaticConstant;

      if (Decl->isLocalVarDecl() && Type.getTypePtr()->isAnyPointerType() && NamingStyles[SK_LocalConstantPointer])
        return SK_LocalConstantPointer;

      if (Decl->isLocalVarDecl() && NamingStyles[SK_LocalConstant])
        return SK_LocalConstant;

      if (Decl->isFunctionOrMethodVarDecl() && NamingStyles[SK_LocalConstant])
        return SK_LocalConstant;

      if (NamingStyles[SK_Constant])
        return SK_Constant;
    }

    if (Decl->isStaticDataMember() && NamingStyles[SK_ClassMember])
      return SK_ClassMember;

    if (Decl->isFileVarDecl() && Type.getTypePtr()->isAnyPointerType() && NamingStyles[SK_GlobalPointer])
      return SK_GlobalPointer;

    if (Decl->isFileVarDecl() && NamingStyles[SK_GlobalVariable])
      return SK_GlobalVariable;

    if (Decl->isStaticLocal() && NamingStyles[SK_StaticVariable])
      return SK_StaticVariable;
 
    if (Decl->isLocalVarDecl() && Type.getTypePtr()->isAnyPointerType() && NamingStyles[SK_LocalPointer])
      return SK_LocalPointer;

    if (Decl->isLocalVarDecl() && NamingStyles[SK_LocalVariable])
      return SK_LocalVariable;

    if (Decl->isFunctionOrMethodVarDecl() && NamingStyles[SK_LocalVariable])
      return SK_LocalVariable;

    if (NamingStyles[SK_Variable])
      return SK_Variable;

    return SK_Invalid;
  }

  if (const auto *Decl = dyn_cast<CXXMethodDecl>(D)) {
    if (Decl->isMain() || !Decl->isUserProvided() ||
        Decl->size_overridden_methods() > 0)
      return SK_Invalid;

    // If this method has the same name as any base method, this is likely
    // necessary even if it's not an override. e.g. CRTP.
    for (const CXXBaseSpecifier &Base : Decl->getParent()->bases())
      if (const auto *RD = Base.getType()->getAsCXXRecordDecl())
        if (RD->hasMemberName(Decl->getDeclName()))
          return SK_Invalid;

    if (Decl->isConstexpr() && NamingStyles[SK_ConstexprMethod])
      return SK_ConstexprMethod;

    if (Decl->isConstexpr() && NamingStyles[SK_ConstexprFunction])
      return SK_ConstexprFunction;

    if (Decl->isStatic() && NamingStyles[SK_ClassMethod])
      return SK_ClassMethod;

    if (Decl->isVirtual() && NamingStyles[SK_VirtualMethod])
      return SK_VirtualMethod;

    if (Decl->getAccess() == AS_private && NamingStyles[SK_PrivateMethod])
      return SK_PrivateMethod;

    if (Decl->getAccess() == AS_protected && NamingStyles[SK_ProtectedMethod])
      return SK_ProtectedMethod;

    if (Decl->getAccess() == AS_public && NamingStyles[SK_PublicMethod])
      return SK_PublicMethod;

    if (NamingStyles[SK_Method])
      return SK_Method;

    if (NamingStyles[SK_Function])
      return SK_Function;

    return SK_Invalid;
  }

  if (const auto *Decl = dyn_cast<FunctionDecl>(D)) {
    if (Decl->isMain())
      return SK_Invalid;

    if (Decl->isConstexpr() && NamingStyles[SK_ConstexprFunction])
      return SK_ConstexprFunction;

    if (Decl->isGlobal() && NamingStyles[SK_GlobalFunction])
      return SK_GlobalFunction;

    if (NamingStyles[SK_Function])
      return SK_Function;
  }

  if (isa<TemplateTypeParmDecl>(D)) {
    if (NamingStyles[SK_TypeTemplateParameter])
      return SK_TypeTemplateParameter;

    if (NamingStyles[SK_TemplateParameter])
      return SK_TemplateParameter;

    return SK_Invalid;
  }

  if (isa<NonTypeTemplateParmDecl>(D)) {
    if (NamingStyles[SK_ValueTemplateParameter])
      return SK_ValueTemplateParameter;

    if (NamingStyles[SK_TemplateParameter])
      return SK_TemplateParameter;

    return SK_Invalid;
  }

  if (isa<TemplateTemplateParmDecl>(D)) {
    if (NamingStyles[SK_TemplateTemplateParameter])
      return SK_TemplateTemplateParameter;

    if (NamingStyles[SK_TemplateParameter])
      return SK_TemplateParameter;

    return SK_Invalid;
  }

  return SK_Invalid;
}

static llvm::Optional<RenamerClangTidyCheck::FailureInfo> getFailureInfo(
    StringRef Type, StringRef Name, const NamedDecl *ND,
    SourceLocation Location,
    ArrayRef<llvm::Optional<IdentifierNamingCheck::NamingStyle>> NamingStyles,
    const IdentifierNamingCheck::HungarianNotationOption &HNOption,
    StyleKind SK, const SourceManager &SM, bool IgnoreFailedSplit) {
  if (SK == SK_Invalid || !NamingStyles[SK])
    return None;

  const IdentifierNamingCheck::NamingStyle &Style = *NamingStyles[SK];
  if (Style.IgnoredRegexp.isValid() && Style.IgnoredRegexp.match(Name))
    return None;

  if (matchesStyle(Type, Name, Style, HNOption, ND))
    return None;

  std::string KindName =
      fixupWithCase(Type, StyleNames[SK], ND, Style, HNOption,
                    IdentifierNamingCheck::CT_LowerCase);
  std::replace(KindName.begin(), KindName.end(), '_', ' ');

  std::string Fixup = fixupWithStyle(Type, Name, Style, HNOption, ND);
  if (StringRef(Fixup).equals(Name)) {
    if (!IgnoreFailedSplit) {
      LLVM_DEBUG(Location.print(llvm::dbgs(), SM);
                 llvm::dbgs()
                 << llvm::formatv(": unable to split words for {0} '{1}'\n",
                                  KindName, Name));
    }
    return None;
  }
  return RenamerClangTidyCheck::FailureInfo{std::move(KindName),
                                            std::move(Fixup)};
}

llvm::Optional<RenamerClangTidyCheck::FailureInfo>
IdentifierNamingCheck::GetDeclFailureInfo(const NamedDecl *Decl,
                                          const SourceManager &SM) const {
  SourceLocation Loc = Decl->getLocation();
  const FileStyle &FileStyle = getStyleForFile(SM.getFilename(Loc));
  if (!FileStyle.isActive())
    return llvm::None;

  return getFailureInfo(getDeclTypeName(Decl), Decl->getName(), Decl, Loc,
                        FileStyle.getStyles(), FileStyle.getHNOption(),
                        findStyleKind(Decl, FileStyle.getStyles(),
                                      FileStyle.isIgnoringMainLikeFunction()),
                        SM, IgnoreFailedSplit);
}

llvm::Optional<RenamerClangTidyCheck::FailureInfo>
IdentifierNamingCheck::GetMacroFailureInfo(const Token &MacroNameTok,
                                           const SourceManager &SM) const {
  SourceLocation Loc = MacroNameTok.getLocation();
  const FileStyle &Style = getStyleForFile(SM.getFilename(Loc));
  if (!Style.isActive())
    return llvm::None;

  return getFailureInfo("", MacroNameTok.getIdentifierInfo()->getName(), NULL,
                        Loc, Style.getStyles(), Style.getHNOption(),
                        SK_MacroDefinition, SM, IgnoreFailedSplit);
}

RenamerClangTidyCheck::DiagInfo
IdentifierNamingCheck::GetDiagInfo(const NamingCheckId &ID,
                                   const NamingCheckFailure &Failure) const {
  return DiagInfo{"invalid case style for %0 '%1'",
                  [&](DiagnosticBuilder &Diag) {
                    Diag << Failure.Info.KindName << ID.second;
                  }};
}

const IdentifierNamingCheck::FileStyle &
IdentifierNamingCheck::getStyleForFile(StringRef FileName) const {
  if (!GetConfigPerFile)
    return *MainFileStyle;
  StringRef Parent = llvm::sys::path::parent_path(FileName);
  auto Iter = NamingStylesCache.find(Parent);
  if (Iter != NamingStylesCache.end())
    return Iter->getValue();

  ClangTidyOptions Options = Context->getOptionsForFile(FileName);
  if (Options.Checks && GlobList(*Options.Checks).contains(CheckName)) {
    auto It = NamingStylesCache.try_emplace(
        Parent, getFileStyleFromOptions({CheckName, Options.CheckOptions}));
    assert(It.second);
    return It.first->getValue();
  }
  // Default construction gives an empty style.
  auto It = NamingStylesCache.try_emplace(Parent);
  assert(It.second);
  return It.first->getValue();
}

} // namespace readability
} // namespace tidy
} // namespace clang
