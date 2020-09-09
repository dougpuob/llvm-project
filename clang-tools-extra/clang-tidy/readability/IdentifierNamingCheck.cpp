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
           "camel_Snake_Back"},
          {readability::IdentifierNamingCheck::CT_HungarianNotation,
           "szHungarianNotation"}};
  return llvm::makeArrayRef(Mapping);
}

namespace readability {

// clang-format off
#define NAMING_KEYS(m) \
    m(Namespace) \
    m(InlineNamespace) \
    m(EnumConstant) \
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

#undef NAMING_KEYS
// clang-format on

static std::vector<llvm::Optional<IdentifierNamingCheck::NamingStyle>>
getNamingStyles(const ClangTidyCheck::OptionsView &Options) {
  std::vector<llvm::Optional<IdentifierNamingCheck::NamingStyle>> Styles;
  Styles.reserve(array_lengthof(StyleNames));
  for (auto const &StyleName : StyleNames) {
    auto CaseOptional = Options.getOptional<IdentifierNamingCheck::CaseType>(
        (StyleName + "Case").str());
    auto Prefix = Options.get((StyleName + "Prefix").str(), "");
    auto Postfix = Options.get((StyleName + "Suffix").str(), "");

    if (CaseOptional || !Prefix.empty() || !Postfix.empty())
      Styles.emplace_back(IdentifierNamingCheck::NamingStyle{
          std::move(CaseOptional), std::move(Prefix), std::move(Postfix)});
    else
      Styles.emplace_back(llvm::None);
  }
  return Styles;
}

IdentifierNamingCheck::IdentifierNamingCheck(StringRef Name,
                                             ClangTidyContext *Context)
    : RenamerClangTidyCheck(Name, Context), Context(Context), CheckName(Name),
      GetConfigPerFile(Options.get("GetConfigPerFile", true)),
      IgnoreFailedSplit(Options.get("IgnoreFailedSplit", false)),
      IgnoreMainLikeFunctions(Options.get("IgnoreMainLikeFunctions", false)) {

  auto IterAndInserted = NamingStylesCache.try_emplace(
      llvm::sys::path::parent_path(Context->getCurrentFile()),
      getNamingStyles(Options));
  assert(IterAndInserted.second && "Couldn't insert Style");
  // Holding a reference to the data in the vector is safe as it should never
  // move.
  MainFileStyle = IterAndInserted.first->getValue();
}

IdentifierNamingCheck::~IdentifierNamingCheck() = default;

void IdentifierNamingCheck::storeOptions(ClangTidyOptions::OptionMap &Opts) {
  RenamerClangTidyCheck::storeOptions(Opts);
  ArrayRef<llvm::Optional<NamingStyle>> NamingStyles =
      getStyleForFile(Context->getCurrentFile());
  for (size_t I = 0; I < SK_Count; ++I) {
    if (!NamingStyles[I])
      continue;
    if (NamingStyles[I]->Case)
      Options.store(Opts, (StyleNames[I] + "Case").str(),
                    *NamingStyles[I]->Case);
    Options.store(Opts, (StyleNames[I] + "Prefix").str(),
                  NamingStyles[I]->Prefix);
    Options.store(Opts, (StyleNames[I] + "Suffix").str(),
                  NamingStyles[I]->Suffix);
  }
  Options.store(Opts, "GetConfigPerFile", GetConfigPerFile);
  Options.store(Opts, "IgnoreFailedSplit", IgnoreFailedSplit);
  Options.store(Opts, "IgnoreMainLikeFunctions", IgnoreMainLikeFunctions);
}

static const std::string
getHungarianNotationTypePrefix(const std::string &TypeName,
                               const NamedDecl *InputDecl) {
  if (!InputDecl || TypeName.empty()) {
    return TypeName;
  }

  // clang-format off
  const static llvm::StringMap<StringRef> HungarianNotationTable = {
        // Primitive types
        {"int8_t",          "i8"},
        {"int16_t",         "i16"},
        {"int32_t",         "i32"},
        {"int64_t",         "i64"},
        {"uint8_t",         "u8"},
        {"uint16_t",        "u16"},
        {"uint32_t",        "u32"},
        {"uint64_t",        "u64"},
        {"char8_t",         "c8"},
        {"char16_t",        "c16"},
        {"char32_t",        "c32"},
        {"float",           "f"},
        {"double",          "d"},
        {"char",            "c"},
        {"bool",            "b"},
        {"_Bool",           "b"},
        {"int",             "i"},
        {"size_t",          "n"},
        {"wchar_t",         "wc"},
        {"short",           "s"},
        {"signed",          "i"},
        {"unsigned",        "u"},
        {"long",            "l"},
        {"long long",       "ll"},
        {"unsigned long",   "ul"},
        {"long double",     "ld"},
        {"ptrdiff_t",       "p"},
        // Windows data types
        {"BOOL",            "b"},
        {"BOOLEAN",         "b"},
        {"BYTE",            "by"},
        {"WORD",            "w"},
        {"DWORD",           "dw"}};
  // clang-format on

  std::string ClonedTypeName = TypeName;

  // Handle null string
  std::string PrefixStr;
  if (const auto *TD = dyn_cast<ValueDecl>(InputDecl)) {
    auto QT = TD->getType();
    if (QT->isFunctionPointerType()) {
      PrefixStr = "fn"; // Function Pointer
    } else if (QT->isPointerType()) {
      // clang-format off
      const static llvm::StringMap<StringRef> NullString = {
        {"char*",     "sz"},
        {"wchar_t*",  "wsz"}};
      // clang-format on
      for (const auto &Type : NullString) {
        const auto &Key = Type.getKey();
        if (ClonedTypeName.find(Key.str()) == 0) {
          PrefixStr = Type.getValue().str();
          ClonedTypeName = ClonedTypeName.substr(
              Key.size(), ClonedTypeName.size() - Key.size());
          break;
        }
      }
    } else if (QT->isArrayType()) {
      // clang-format off
      const static llvm::StringMap<StringRef> NullString = {
        {"char",     "sz"},
        {"wchar_t",  "wsz"}};
      // clang-format on
      for (const auto &Type : NullString) {
        const auto &Key = Type.getKey();
        if (ClonedTypeName.find(Key.str()) == 0) {
          PrefixStr = Type.getValue().str();
          ClonedTypeName = ClonedTypeName.substr(
              Key.size(), ClonedTypeName.size() - Key.size());
          break;
        }
      }
      if (PrefixStr.empty()) {
        PrefixStr = 'a'; // Array
      }
    } else if (QT->isReferenceType()) {
      size_t Pos = ClonedTypeName.find_last_of("&");
      if (Pos != std::string::npos) {
        ClonedTypeName = ClonedTypeName.substr(0, Pos);
      }
    }
  }

  // Handle pointers
  size_t PtrCount = [&](std::string TypeName) -> size_t {
    size_t Pos = TypeName.find('*');
    size_t Count = 0;
    for (; Pos < TypeName.length(); Pos++, Count++) {
      if ('*' != TypeName[Pos])
        break;
    }
    return Count;
  }(ClonedTypeName);
  if (PtrCount > 0) {
    ClonedTypeName = [&](std::string Str, const std::string &From,
                         const std::string &To) {
      size_t StartPos = 0;
      while ((StartPos = Str.find(From, StartPos)) != std::string::npos) {
        Str.replace(StartPos, From.length(), To);
        StartPos += To.length();
      }
      return Str;
    }(ClonedTypeName, "*", "");
  }

  if (PrefixStr.empty()) {
    for (const auto &Type : HungarianNotationTable) {
      const auto &Key = Type.getKey();
      if (ClonedTypeName == Key) {
        PrefixStr = Type.getValue().str();
        break;
      }
    }
  }

  if (PtrCount > 0) {
    for (size_t Idx = 0; Idx < PtrCount; Idx++) {
      PrefixStr.insert(PrefixStr.begin(), 'p');
    }
  }

  return PrefixStr;
}

std::string
IdentifierNamingCheck::getDeclTypeName(const clang::NamedDecl *Decl) const {
  const ValueDecl *ValDecl = dyn_cast<ValueDecl>(Decl);
  if (!ValDecl) {
    return "";
  }

  // Get type text of variable declarations.
  const auto &SM = ValDecl->getASTContext().getSourceManager();
  const char *Begin = SM.getCharacterData(ValDecl->getBeginLoc());
  const char *Curr = SM.getCharacterData(ValDecl->getLocation());
  const intptr_t StrLen = Curr - Begin;
  std::string TypeName;
  if (StrLen > 0) {
    std::string Type(Begin, StrLen);

    const static std::list<std::string> Keywords = {
        // Constexpr specifiers
        "constexpr", "constinit", "consteval",
        // Qualifier
        "const", "volatile", "restrict", "mutable",
        // Storage class specifiers
        "auto", "register", "static", "extern", "thread_local"};

    // Remove keywords
    for (const std::string &Kw : Keywords) {
      for (size_t Pos = 0; (Pos = Type.find(Kw, Pos)) != std::string::npos;) {
        Type.replace(Pos, Kw.length(), "");
      }
    }

    // Replace spaces with single space
    for (size_t Pos = 0; (Pos = Type.find("  ", Pos)) != std::string::npos;
         Pos += strlen(" ")) {
      Type.replace(Pos, strlen("  "), " ");
    }

    // Replace " *" with "*"
    for (size_t Pos = 0; (Pos = Type.find(" *", Pos)) != std::string::npos;
         Pos += strlen("*")) {
      Type.replace(Pos, strlen(" *"), "*");
    }

    // Replace " &" with "&"
    for (size_t Pos = 0; (Pos = Type.find(" &", Pos)) != std::string::npos;
         Pos += strlen("&")) {
      Type.replace(Pos, strlen(" &"), "&");
    }

    Type = Type.erase(Type.find_last_not_of(" ") + 1);
    Type = Type.erase(0, Type.find_first_not_of(" "));
    TypeName = Type;
  }

  return TypeName;
}

static bool matchesStyle(StringRef Type, StringRef Name,
                         IdentifierNamingCheck::NamingStyle Style,
                         const NamedDecl *Decl) {
  static llvm::Regex Matchers[] = {
      llvm::Regex("^.*$"),
      llvm::Regex("^[a-z][a-z0-9_]*$"),
      llvm::Regex("^[a-z][a-zA-Z0-9]*$"),
      llvm::Regex("^[A-Z][A-Z0-9_]*$"),
      llvm::Regex("^[A-Z][a-zA-Z0-9]*$"),
      llvm::Regex("^[A-Z]([a-z0-9]*(_[A-Z])?)*"),
      llvm::Regex("^[a-z]([a-z0-9]*(_[A-Z])?)*"),
      llvm::Regex("^[A-Z][a-zA-Z0-9]*$"),
  };

  if (!Name.consume_front(Style.Prefix))
    return false;
  if (!Name.consume_back(Style.Suffix))
    return false;

  // Ensure the name doesn't have any extra underscores beyond those specified
  // in the prefix and suffix.
  if (Name.startswith("_") || Name.endswith("_"))
    return false;

  if (Style.Case == IdentifierNamingCheck::CaseType::CT_HungarianNotation) {
    const std::string TypePrefix =
        getHungarianNotationTypePrefix(Type.str(), Decl);
    if (TypePrefix.length() > 0) {
      if (!Name.startswith(TypePrefix))
        return false;
      Name = Name.drop_front(TypePrefix.size());
    }
  }

  if (Style.Case && !Matchers[static_cast<size_t>(*Style.Case)].match(Name))
    return false;

  return true;
}

static std::string fixupWithCase(const StringRef &Type, const StringRef &Name,
                                 const Decl *InputDecl,
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

  case IdentifierNamingCheck::CT_HungarianNotation: {
    const NamedDecl *ND = dyn_cast<NamedDecl>(InputDecl);
    const std::string TypePrefix =
        getHungarianNotationTypePrefix(Type.str(), ND);
    Fixup = TypePrefix;
    for (size_t Idx = 0; Idx < Words.size(); Idx++) {
      // Skip first part if it's a lowercase string.
      if (Idx == 0) {
        bool LowerAlnum =
            std::all_of(Words[Idx].begin(), Words[Idx].end(),
                        [](const char C) { return isdigit(C) || islower(C); });
        if (LowerAlnum)
          continue;
      }
      Fixup += Words[Idx];
    }
    break;
  }
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
fixupWithStyle(const StringRef &Type, const StringRef &Name,
               const IdentifierNamingCheck::NamingStyle &Style,
               const Decl *InputDecl) {
  const std::string Fixed = fixupWithCase(
      Type, Name, InputDecl,
      Style.Case.getValueOr(IdentifierNamingCheck::CaseType::CT_AnyCase));
  StringRef Mid = StringRef(Fixed).trim("_");
  if (Mid.empty())
    Mid = "_";
  return (Style.Prefix + Mid + Style.Suffix).str();
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

  if (isa<EnumConstantDecl>(D)) {
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
    auto FindHidden = [&](const CXXBaseSpecifier *S, clang::CXXBasePath &P) {
      return CXXRecordDecl::FindOrdinaryMember(S, P, Decl->getDeclName());
    };
    CXXBasePaths UnusedPaths;
    if (Decl->getParent()->lookupInBases(FindHidden, UnusedPaths))
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
    const StringRef &Type, const StringRef &Name, const NamedDecl *Decl,
    SourceLocation Location,
    ArrayRef<llvm::Optional<IdentifierNamingCheck::NamingStyle>> NamingStyles,
    StyleKind SK, const SourceManager &SM, bool IgnoreFailedSplit) {
  if (SK == SK_Invalid || !NamingStyles[SK])
    return None;

  const IdentifierNamingCheck::NamingStyle &Style = *NamingStyles[SK];
  if (matchesStyle(Type, Name, Style, Decl))
    return None;

  std::string KindName = fixupWithCase(Type, StyleNames[SK], Decl,
                                       IdentifierNamingCheck::CT_LowerCase);
  std::replace(KindName.begin(), KindName.end(), '_', ' ');

  std::string Fixup = fixupWithStyle(Type, Name, Style, Decl);
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
  ArrayRef<llvm::Optional<NamingStyle>> NamingStyles =
      getStyleForFile(SM.getFilename(Loc));

  std::string TypeName = getDeclTypeName(Decl);
  return getFailureInfo(
      TypeName, Decl->getName(), Decl, Loc, NamingStyles,
      findStyleKind(Decl, NamingStyles, IgnoreMainLikeFunctions), SM,
      IgnoreFailedSplit);
}

llvm::Optional<RenamerClangTidyCheck::FailureInfo>
IdentifierNamingCheck::GetMacroFailureInfo(const Token &MacroNameTok,
                                           const SourceManager &SM) const {
  SourceLocation Loc = MacroNameTok.getLocation();

  return getFailureInfo("", MacroNameTok.getIdentifierInfo()->getName(), NULL,
                        Loc, getStyleForFile(SM.getFilename(Loc)),
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

ArrayRef<llvm::Optional<IdentifierNamingCheck::NamingStyle>>
IdentifierNamingCheck::getStyleForFile(StringRef FileName) const {
  if (!GetConfigPerFile)
    return MainFileStyle;
  auto &Styles = NamingStylesCache[llvm::sys::path::parent_path(FileName)];
  if (Styles.empty()) {
    ClangTidyOptions Options = Context->getOptionsForFile(FileName);
    if (Options.Checks && GlobList(*Options.Checks).contains(CheckName))
      Styles = getNamingStyles({CheckName, Options.CheckOptions});
    else
      Styles.resize(SK_Count, None);
  }
  return Styles;
}

} // namespace readability
} // namespace tidy
} // namespace clang
