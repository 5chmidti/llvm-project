// unittests/ASTMatchers/ASTMatchersNarrowingTest.cpp - AST matcher unit tests//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ASTMatchersTest.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Testing/TestClangConfig.h"
#include "gtest/gtest.h"
#include <string>

namespace clang {
namespace ast_matchers {
// doc_test_begin
TEST_P(ASTMatchersDocTest, docs_162) {
    const StringRef Code = R"cpp(
	  int* p;
	  void f();
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_162 originates here.");
    using Verifier = VerifyBoundNodeMatch<Decl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int* p)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(void f())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        decl(anything()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_174) {
    const StringRef Code = R"cpp(
	  int X;
	  namespace NS {
	    int Y;
	  }  // namespace NS
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_174 originates here.");
    using Verifier = VerifyBoundNodeMatch<NamedDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(X)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(NS)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        namedDecl(hasDeclContext(translationUnitDecl())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_190) {
    const StringRef Code = R"cpp(
	  typedef int X;
	  using Y = int;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_190 originates here.");
    using Verifier = VerifyBoundNodeMatch<TypedefDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(typedef int X)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        typedefDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_204) {
    const StringRef Code = R"cpp(
	  typedef int X;
	  using Y = int;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_204 originates here.");
    using Verifier = VerifyBoundNodeMatch<TypedefNameDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(typedef int X)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(using Y = int)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        typedefNameDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_217) {
    const StringRef Code = R"cpp(
	  typedef int X;
	  using Y = int;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_217 originates here.");
    using Verifier = VerifyBoundNodeMatch<TypeAliasDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(using Y = int)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        typeAliasDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_231) {
    const StringRef Code = R"cpp(
	  template <typename T> struct X {};
	  template <typename T> using Y = X<T>;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_231 originates here.");
    using Verifier = VerifyBoundNodeMatch<TypeAliasTemplateDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(template <typename T> using Y = X<T>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        typeAliasTemplateDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_244) {
    const StringRef Code = R"cpp(
	  #include "Y.h"
	  typedef int my_main_file_int;
	  my_main_file_int a = 0;
	  my_header_int b = 1;
	)cpp";
	const FileContentMappings VirtualMappedFiles = {std::pair{"Y.h", 
	R"cpp(
	  #pragma once
	  typedef int my_header_int;
	)cpp"},};
    {
    SCOPED_TRACE("Test failure from docs_244 originates here.");
    using Verifier = VerifyBoundNodeMatch<TypedefDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(typedef int my_main_file_int)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        typedefDecl(isExpansionInMainFile()).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{},
		VirtualMappedFiles));
	}
}

TEST_P(ASTMatchersDocTest, docs_370) {
    const StringRef Code = R"cpp(
	  void X();
	  class C {
	    friend void X();
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_370 originates here.");
    using Verifier = VerifyBoundNodeMatch<Decl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(void X())cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(C)cpp", 2);
	Matches.emplace_back(MatchKind::Code, R"cpp(friend void X())cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        decl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_385) {
    const StringRef Code = R"cpp(
	  struct pair { int x; int y; };
	  pair make(int, int);
	  int number = 42;
	  auto [foo, bar] = make(42, 42);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(17);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_385 originates here.");
    using Verifier = VerifyBoundNodeMatch<DecompositionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(auto [foo, bar] = make(42, 42))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        decompositionDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_401) {
    const StringRef Code = R"cpp(
	  struct pair { int x; int y; };
	  pair make(int, int);
	  auto [foo, bar] = make(42, 42);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(17);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_401 originates here.");
    using Verifier = VerifyBoundNodeMatch<BindingDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(foo)cpp", 2);
	Matches.emplace_back(MatchKind::Name, R"cpp(bar)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        bindingDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_415) {
    const StringRef Code = R"cpp(
	  extern "C" {}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_415 originates here.");
    using Verifier = VerifyBoundNodeMatch<LinkageSpecDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(extern "C" {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        linkageSpecDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_428) {
    const StringRef Code = R"cpp(
	  typedef int X;
	  struct S {
	    union {
	      int i;
	    } U;
	  };
	)cpp";

	const TestClangConfig& Conf = GetParam();

    {
    SCOPED_TRACE("Test failure from docs_428 originates here.");
    using Verifier = VerifyBoundNodeMatch<NamedDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(typedef int X)cpp", 1);
	if (Conf.isC())
		Matches.emplace_back(MatchKind::Name, R"cpp(S)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(int i)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp()cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(U)cpp", 1);
	if (Conf.isCXX())
		Matches.emplace_back(MatchKind::Name, R"cpp(S)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        namedDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_460) {
    const StringRef Code = R"cpp(
	  namespace {}
	  namespace test {}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_460 originates here.");
    using Verifier = VerifyBoundNodeMatch<NamespaceDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(namespace {})cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(namespace test {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        namespaceDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_473) {
    const StringRef Code = R"cpp(
	  namespace test {}
	  namespace alias = ::test;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_473 originates here.");
    using Verifier = VerifyBoundNodeMatch<NamespaceAliasDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(alias)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        namespaceAliasDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_487) {
    const StringRef Code = R"cpp(
	  class X;
	  template<class T> class Z {};
	  struct S {};
	  union U {};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_487 originates here.");
    using Verifier = VerifyBoundNodeMatch<RecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(X)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(Z)cpp", 2);
	Matches.emplace_back(MatchKind::Name, R"cpp(S)cpp", 2);
	Matches.emplace_back(MatchKind::Name, R"cpp(U)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        recordDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_502) {
    const StringRef Code = R"cpp(
	  class X;
	  template<class T> class Z {};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_502 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(X)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(Z)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_515) {
    const StringRef Code = R"cpp(
	  template<class T> class Z {};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_515 originates here.");
    using Verifier = VerifyBoundNodeMatch<ClassTemplateDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(Z)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        classTemplateDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_527) {
    const StringRef Code = R"cpp(
	  template<typename T> class A {};
	  template<> class A<double> {};
	  A<int> a;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_527 originates here.");
    using Verifier = VerifyBoundNodeMatch<ClassTemplateSpecializationDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(class A<int>)cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(class A<double>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        classTemplateSpecializationDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_543) {
    const StringRef Code = R"cpp(
	  template<class T1, class T2, int I>
	  class A {};
	
	  template<class T, int I> class A<T, T*, I> {};
	
	  template<>
	  class A<int, int, 1> {};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_543 originates here.");
    using Verifier = VerifyBoundNodeMatch<ClassTemplatePartialSpecializationDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(template<class T, int I> class A<T, T*, I> {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        classTemplatePartialSpecializationDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_564) {
    const StringRef Code = R"cpp(
	  class X { int y; };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_564 originates here.");
    using Verifier = VerifyBoundNodeMatch<DeclaratorDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int y)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        declaratorDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_576) {
    const StringRef Code = R"cpp(
	  void f(int x);
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_576 originates here.");
    using Verifier = VerifyBoundNodeMatch<ParmVarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int x)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        parmVarDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_587) {
    const StringRef Code = R"cpp(
	  class C {
	  public:
	    int a;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_587 originates here.");
    using Verifier = VerifyBoundNodeMatch<AccessSpecDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(public:)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        accessSpecDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_602) {
    const StringRef Code = R"cpp(
	  class B {};
	  class C : public virtual B {};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_602 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(C)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasDirectBase(cxxBaseSpecifier())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_614) {
    const StringRef Code = R"cpp(
	  class C {
	    C() : i(42) {}
	    int i;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_614 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXCtorInitializer>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(i(42))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxCtorInitializer().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_629) {
    const StringRef Code = R"cpp(
	  template <typename T> struct C {};
	  C<int> c;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_629 originates here.");
    using Verifier = VerifyBoundNodeMatch<TemplateSpecializationType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(C<int>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        templateSpecializationType(hasAnyTemplateArgument(templateArgument())).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_642) {
    const StringRef Code = R"cpp(
	  template <typename T> struct C {};
	  C<int> c;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_642 originates here.");
    using Verifier = VerifyBoundNodeMatch<TemplateArgumentLoc>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        templateArgumentLoc().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_655) {
    const StringRef Code = R"cpp(
	  template<template <typename> class S> class X {};
	  template<typename T> class Y {};
	  X<Y> xi;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_655 originates here.");
    using Verifier = VerifyBoundNodeMatch<ClassTemplateSpecializationDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(class X<Y>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        classTemplateSpecializationDecl(hasAnyTemplateArgument(
              refersToTemplate(templateName()))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_670) {
    const StringRef Code = R"cpp(
	  template <typename T, int N> struct C {};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_670 originates here.");
    using Verifier = VerifyBoundNodeMatch<NonTypeTemplateParmDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int N)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        nonTypeTemplateParmDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_684) {
    const StringRef Code = R"cpp(
	  template <typename T, int N> struct C {};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_684 originates here.");
    using Verifier = VerifyBoundNodeMatch<TemplateTypeParmDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(typename T)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        templateTypeParmDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_697) {
    const StringRef Code = R"cpp(
	  template <template <typename> class Z, int N> struct C {};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_697 originates here.");
    using Verifier = VerifyBoundNodeMatch<TemplateTemplateParmDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(template <typename> class Z)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        templateTemplateParmDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_712) {
    const StringRef Code = R"cpp(
	  class C {
	  public:    int a;
	  protected: int b;
	  private:   int c;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_712 originates here.");
    using Verifier = VerifyBoundNodeMatch<FieldDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(a)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        fieldDecl(isPublic()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_723) {
    const StringRef Code = R"cpp(
	  class Base {};
	  class Derived1 : public Base {};
	  struct Derived2 : Base {};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_723 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(Derived1)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(Derived2)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasAnyBase(cxxBaseSpecifier(isPublic()).bind("base"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_745) {
    const StringRef Code = R"cpp(
	  class C {
	  public:    int a;
	  protected: int b;
	  private:   int c;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_745 originates here.");
    using Verifier = VerifyBoundNodeMatch<FieldDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        fieldDecl(isProtected()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_755) {
    const StringRef Code = R"cpp(
	  class Base {};
	  class Derived : protected Base {};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_755 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(Derived)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasAnyBase(cxxBaseSpecifier(isProtected()).bind("base"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_776) {
    const StringRef Code = R"cpp(
	  class C {
	  public:    int a;
	  protected: int b;
	  private:   int c;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_776 originates here.");
    using Verifier = VerifyBoundNodeMatch<FieldDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(c)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        fieldDecl(isPrivate()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_786) {
    const StringRef Code = R"cpp(
	  struct Base {};
	  struct Derived1 : private Base {}; // \match{Base}
	  class Derived2 : Base {}; // \match{Base}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_786 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(Derived1)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(Derived2)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasAnyBase(cxxBaseSpecifier(isPrivate()).bind("base"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_807) {
    const StringRef Code = R"cpp(
	  class C {
	    int a : 2;
	    int b;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_807 originates here.");
    using Verifier = VerifyBoundNodeMatch<FieldDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(a)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        fieldDecl(isBitField()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_823) {
    const StringRef Code = R"cpp(
	  class C {
	    int a : 2;
	    int b : 4;
	    int c : 2;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_823 originates here.");
    using Verifier = VerifyBoundNodeMatch<FieldDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(a)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(c)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        fieldDecl(hasBitWidth(2)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_842) {
    const StringRef Code = R"cpp(
	  class C {
	    int a = 2;
	    int b = 3;
	    int c;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_842 originates here.");
    using Verifier = VerifyBoundNodeMatch<FieldDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(a)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        fieldDecl(hasInClassInitializer(integerLiteral(equals(2)))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_842 originates here.");
    using Verifier = VerifyBoundNodeMatch<FieldDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(a)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        fieldDecl(hasInClassInitializer(anything())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_871) {
    const StringRef Code = R"cpp(
	  template<typename T> class A {}; // #1
	  template<> class A<int> {}; // #2
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_871 originates here.");
    using Verifier = VerifyBoundNodeMatch<ClassTemplateSpecializationDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(template<> class A<int> {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        classTemplateSpecializationDecl(hasSpecializedTemplate(classTemplateDecl().bind("ctd"))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_900) {
    const StringRef Code = R"cpp(
	  template<typename T> class A {};
	  template<> class A<double> {};
	  A<int> a;
	
	  template<typename T> void f() {};
	  void func() { f<int>(); };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_900 originates here.");
    using Verifier = VerifyBoundNodeMatch<ClassTemplateSpecializationDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(class A<int>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        classTemplateSpecializationDecl(
                        hasAnyTemplateArgument(
                          refersToType(asString("int")))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}

    {
    SCOPED_TRACE("Test failure from docs_900 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(f)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(hasAnyTemplateArgument(
              refersToType(asString("int")))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_934) {
    const StringRef Code = R"cpp(
	  void foo()
	  {
	      int i = 3.0;
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_934 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int i = 3.0)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        traverse(TK_IgnoreUnlessSpelledInSource,
    varDecl(hasInitializer(floatLiteral().bind("init")))
  ).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1014) {
    const StringRef Code = R"cpp(
	  int arr[5];
	  const int a = 0;
	  char b = 0;
	  const int c = a;
	  int *d = arr;
	  long e = (long) 0l;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_1014 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(a)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasInitializer(ignoringImpCasts(integerLiteral()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_1014 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(c)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(d)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasInitializer(ignoringImpCasts(declRefExpr()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_1014 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(a)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasInitializer(integerLiteral())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_1014 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;


    EXPECT_TRUE(notMatches(
        Code,
        varDecl(hasInitializer(declRefExpr())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1045) {
    const StringRef Code = R"cpp(
	  int a = 0;
	  char b = (0);
	  void* c = reinterpret_cast<char*>(0);
	  char d = char(0);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1045 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(a)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(b)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(c)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(d)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasInitializer(ignoringParenCasts(integerLiteral()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_1045 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(a)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasInitializer(integerLiteral())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1068) {
    const StringRef Code = R"cpp(
	  int arr[5];
	  int a = 0;
	  char b = (0);
	  const int c = a;
	  int *d = (arr);
	  long e = ((long) 0l);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1068 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(a)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasInitializer(ignoringParenImpCasts(integerLiteral()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_1068 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(c)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(d)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasInitializer(ignoringParenImpCasts(declRefExpr()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_1068 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(a)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasInitializer(integerLiteral())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_1068 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;


    EXPECT_TRUE(notMatches(
        Code,
        varDecl(hasInitializer(declRefExpr())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1099) {
    const StringRef Code = R"cpp(
	  void (*fp)(void);
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_1099 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(fp)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasType(pointerType(pointee(ignoringParens(functionType()))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1113) {
    const StringRef Code = R"cpp(
	  const char* str = ("my-string");
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_1113 originates here.");
    using Verifier = VerifyBoundNodeMatch<ImplicitCastExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(("my-string"))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        implicitCastExpr(hasSourceExpression(ignoringParens(stringLiteral()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1134) {
    const StringRef Code = R"cpp(
	  template<typename T>
	  void f(T x, T y) { sizeof(T() + T()); }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1134 originates here.");
    using Verifier = VerifyBoundNodeMatch<Expr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(sizeof(T() + T()))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp((T() + T()))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(T() + T())cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(T())cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        expr(isInstantiationDependent()).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_1152) {
    const StringRef Code = R"cpp(
	  template<typename T>
	  void add(T x, int y) {
	    x + y;
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1152 originates here.");
    using Verifier = VerifyBoundNodeMatch<Expr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(x + y)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(x)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        expr(isTypeDependent()).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_1168) {
    const StringRef Code = R"cpp(
	  template<int Size> int f() { return Size; }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1168 originates here.");
    using Verifier = VerifyBoundNodeMatch<Expr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(Size)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        expr(isValueDependent()).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_1181) {
    const StringRef Code = R"cpp(
	  template<typename T, typename U> class A {};
	  A<double, int> b;
	  A<int, double> c;
	
	  template<typename T> void f() {}
	  void func() { f<int>(); };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1181 originates here.");
    using Verifier = VerifyBoundNodeMatch<ClassTemplateSpecializationDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(class A<double, int>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        classTemplateSpecializationDecl(hasTemplateArgument(
    1, refersToType(asString("int")))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}

    {
    SCOPED_TRACE("Test failure from docs_1181 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(f)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(hasTemplateArgument(0,
                        refersToType(asString("int")))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_1214) {
    const StringRef Code = R"cpp(
	  template<typename T> struct C {};
	  C<int> c;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1214 originates here.");
    using Verifier = VerifyBoundNodeMatch<ClassTemplateSpecializationDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(struct C<int>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        classTemplateSpecializationDecl(templateArgumentCountIs(1)).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_1233) {
    const StringRef Code = R"cpp(
	  struct X {};
	  template<typename T> struct A {};
	  A<X> a;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1233 originates here.");
    using Verifier = VerifyBoundNodeMatch<ClassTemplateSpecializationDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(struct A<struct X>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        classTemplateSpecializationDecl(hasAnyTemplateArgument(refersToType(
            recordType(hasDeclaration(recordDecl(hasName("X"))))))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_1253) {
    const StringRef Code = R"cpp(
	  template<template <typename> class S> class X {};
	  template<typename T> class Y {};
	  X<Y> xi;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1253 originates here.");
    using Verifier = VerifyBoundNodeMatch<ClassTemplateSpecializationDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(class X<Y>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        classTemplateSpecializationDecl(hasAnyTemplateArgument(
              refersToTemplate(templateName()))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_1274) {
    const StringRef Code = R"cpp(
	  struct B { int next; };
	  template<int(B::*next_ptr)> struct A {};
	  A<&B::next> a;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1274 originates here.");
    using Verifier = VerifyBoundNodeMatch<ClassTemplateSpecializationDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(struct A<&B::next>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        classTemplateSpecializationDecl(hasAnyTemplateArgument(
    refersToDeclaration(fieldDecl(hasName("next")).bind("next")))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_1296) {
    const StringRef Code = R"cpp(
	  struct B { int next; };
	  template<int(B::*next_ptr)> struct A {};
	  A<&B::next> a;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1296 originates here.");
    using Verifier = VerifyBoundNodeMatch<TemplateSpecializationType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(A<&struct B::next>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        templateSpecializationType(hasAnyTemplateArgument(
  isExpr(hasDescendant(declRefExpr(to(fieldDecl(hasName("next")).bind("next"))))))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_1317) {
    const StringRef Code = R"cpp(
	  template<int T> struct C {};
	  C<42> c;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1317 originates here.");
    using Verifier = VerifyBoundNodeMatch<ClassTemplateSpecializationDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(struct C<42>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        classTemplateSpecializationDecl(
  hasAnyTemplateArgument(isIntegral())).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_1334) {
    const StringRef Code = R"cpp(
	  template<int T> struct C {};
	  C<42> c;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1334 originates here.");
    using Verifier = VerifyBoundNodeMatch<ClassTemplateSpecializationDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(struct C<42>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        classTemplateSpecializationDecl(
  hasAnyTemplateArgument(refersToIntegralType(asString("int")))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_1358) {
    const StringRef Code = R"cpp(
	  template<int T> struct C {};
	  C<42> c;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1358 originates here.");
    using Verifier = VerifyBoundNodeMatch<ClassTemplateSpecializationDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(struct C<42>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        classTemplateSpecializationDecl(
  hasAnyTemplateArgument(equalsIntegralValue("42"))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_1392) {
    const StringRef Code = R"cpp(
	  enum X { A, B, C };
	  void F();
	  int V = 0;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_1392 originates here.");
    using Verifier = VerifyBoundNodeMatch<ValueDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(A)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(B)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(C)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(void F())cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(int V = 0)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        valueDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1405) {
    const StringRef Code = R"cpp(
	  class Foo {
	   public:
	    Foo();
	    Foo(int);
	    int DoSomething();
	  };
	
	  struct Bar {};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1405 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXConstructorDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(Foo())cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(Foo(int))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxConstructorDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1425) {
    const StringRef Code = R"cpp(
	  class Foo {
	   public:
	    virtual ~Foo();
	  };
	
	  struct Bar {};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1425 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXDestructorDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(virtual ~Foo())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxDestructorDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1443) {
    const StringRef Code = R"cpp(
	  enum X {
	    A, B, C
	  };
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_1443 originates here.");
    using Verifier = VerifyBoundNodeMatch<EnumDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(X)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        enumDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1456) {
    const StringRef Code = R"cpp(
	  enum X {
	    A, B, C
	  };
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_1456 originates here.");
    using Verifier = VerifyBoundNodeMatch<EnumConstantDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(A)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(B)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(C)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        enumConstantDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1469) {
    const StringRef Code = R"cpp(
	  class X;
	  template<class T> class Z {};
	  struct S {};
	  union U {};
	  enum E { A, B, C };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1469 originates here.");
    using Verifier = VerifyBoundNodeMatch<TagDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(class X)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(class Z {})cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(class Z)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(struct S {})cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(struct S)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(union U {})cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(union U)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(enum E { A, B, C })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        tagDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_1489) {
    const StringRef Code = R"cpp(
	  class X { void y(); };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1489 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMethodDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(void y())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMethodDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1502) {
    const StringRef Code = R"cpp(
	  class X { operator int() const; };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1502 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXConversionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(operator int() const)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxConversionDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1515) {
    const StringRef Code = R"cpp(
	  template<typename T>
	  class X { X(int); };
	  X(int) -> X<int>;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(17);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1515 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXDeductionGuideDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeOfStr, R"cpp(auto (int) -> X<int>)cpp", 1);
	Matches.emplace_back(MatchKind::TypeOfStr, R"cpp(auto (int) -> X<T>)cpp", 1);
	Matches.emplace_back(MatchKind::TypeOfStr, R"cpp(auto (X<T>) -> X<T>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxDeductionGuideDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_1534) {
    const StringRef Code = R"cpp(
	  template<typename T> concept my_concept = true;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(20);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1534 originates here.");
    using Verifier = VerifyBoundNodeMatch<ConceptDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(template<typename T> concept my_concept = true)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        conceptDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_1551) {
    const StringRef Code = R"cpp(
	  int a;
	  struct Foo {
	    int x;
	  };
	  void bar(int val);
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_1551 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int a)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(int val)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1566) {
    const StringRef Code = R"cpp(
	  int a;
	  struct Foo {
	    int x;
	  };
	  void bar(int val);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1566 originates here.");
    using Verifier = VerifyBoundNodeMatch<FieldDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int x)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        fieldDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1581) {
    const StringRef Code = R"cpp(
	  struct X { struct { int a; }; };
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_1581 originates here.");
    using Verifier = VerifyBoundNodeMatch<IndirectFieldDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(a)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        indirectFieldDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1592) {
    const StringRef Code = R"cpp(
	  void f();
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_1592 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(void f())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1604) {
    const StringRef Code = R"cpp(
	  template<class T> void f(T t) {}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1604 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionTemplateDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(template<class T> void f(T t) {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionTemplateDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_1617) {
    const StringRef Code = R"cpp(
	  class X { friend void foo(); };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1617 originates here.");
    using Verifier = VerifyBoundNodeMatch<FriendDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(friend void foo())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        friendDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1628) {
    const StringRef Code = R"cpp(
	  void foo(int a) { { ++a; } }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_1628 originates here.");
    using Verifier = VerifyBoundNodeMatch<Stmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp({ { ++a; } })cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp({ ++a; })cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(++a)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(a)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        stmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1639) {
    const StringRef Code = R"cpp(
	  void foo() {
	    int a;
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_1639 originates here.");
    using Verifier = VerifyBoundNodeMatch<DeclStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int a;)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        declStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1651) {
    const StringRef Code = R"cpp(
	  class Y {
	    void x() { this->x(); x(); Y y; y.x(); a; this->b; Y::b; }
	    int a; static int b;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1651 originates here.");
    using Verifier = VerifyBoundNodeMatch<MemberExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(this->x)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(x)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(y.x)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(a)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(this->b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        memberExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1665) {
    const StringRef Code = R"cpp(
	  struct X {
	    template <class T> void f();
	    void g();
	  };
	  template <class T> void h() { X x; x.f<T>(); x.g(); }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1665 originates here.");
    using Verifier = VerifyBoundNodeMatch<UnresolvedMemberExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(x.f<T>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        unresolvedMemberExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_1682) {
    const StringRef Code = R"cpp(
	  template <class T> void f() { T t; t.g(); }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1682 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXDependentScopeMemberExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(t.g)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxDependentScopeMemberExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_1695) {
    const StringRef Code = R"cpp(
	  struct X { void foo(); };
	  void bar();
	  void foobar() {
	    X x;
	    x.foo();
	    bar();
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1695 originates here.");
    using Verifier = VerifyBoundNodeMatch<CallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(x.foo())cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(bar())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        callExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1712) {
    const StringRef Code = R"cpp(
	  namespace NS {
	    struct X {};
	    void y(X);
	  }
	
	  void y(...);
	
	  void test() {
	    NS::X x;
	    y(x); // Matches
	    NS::y(x); // Doesn't match
	    y(42); // Doesn't match
	    using NS::y;
	    y(x); // Found by both unqualified lookup and ADL, doesn't match
	   }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1712 originates here.");
    using Verifier = VerifyBoundNodeMatch<CallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(y(x))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        callExpr(usesADL()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1738) {
    const StringRef Code = R"cpp(
	  void f() {
	    []() { return 5; };
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1738 originates here.");
    using Verifier = VerifyBoundNodeMatch<LambdaExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp([]() { return 5; })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        lambdaExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1751) {
    const StringRef Code = R"cpp(
	  struct X {
	    void y();
	    void m() { y(); }
	  };
	  void f();
	  void g() {
	    X x;
	    x.y();
	    f();
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1751 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMemberCallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(x.y())cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(y())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMemberCallExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1952) {
    const StringRef Code = R"cpp(
	  struct A { ~A(); };
	  void f(A);
	  void g(A&);
	  void h() {
	    A a = A{};
	    f(A{});
	    f(a);
	    g(a);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1952 originates here.");
    using Verifier = VerifyBoundNodeMatch<ExprWithCleanups>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(A{})cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(f(A{}))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(f(a))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        exprWithCleanups().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1974) {
    const StringRef Code = R"cpp(
	  int a[] = { 1, 2 };
	  struct B { int x, y; };
	  struct B b = { 5, 6 };
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_1974 originates here.");
    using Verifier = VerifyBoundNodeMatch<InitListExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp({ 1, 2 })cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp({ 5, 6 })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        initListExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_1996) {
    const StringRef Code = R"cpp(
	  namespace std {
	    template <typename T>
	    class initializer_list {
	      const T* begin;
	      const T* end;
	    };
	  }
	  template <typename T> class vector {
	    public: vector(std::initializer_list<T>) {}
	  };
	
	  vector<int> a({ 1, 2, 3 });
	  vector<int> b = { 4, 5 };
	  int c[] = { 6, 7 };
	  struct pair { int x; int y; };
	  pair d = { 8, 9 };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_1996 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXStdInitializerListExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp({ 1, 2, 3 })cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp({ 4, 5 })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxStdInitializerListExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_2024) {
    const StringRef Code = R"cpp(
	  struct point { double x; double y; };
	  struct point pt = { .x = 42.0 };
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2024 originates here.");
    using Verifier = VerifyBoundNodeMatch<InitListExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp({ .x = 42.0 })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        initListExpr(has(implicitValueInitExpr().bind("implicit"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2039) {
    const StringRef Code = R"cpp(
	  template<typename T> class X {
	    void f() {
	      X x(*this);
	      int a = 0, b = 1; int i = (a, b);
	    }
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2039 originates here.");
    using Verifier = VerifyBoundNodeMatch<ParenListExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp((*this))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        parenListExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_2059) {
    const StringRef Code = R"cpp(
	  template <int N>
	  struct A { static const int n = N; };
	  struct B : public A<42> {};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2059 originates here.");
    using Verifier = VerifyBoundNodeMatch<SubstNonTypeTemplateParmExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(N)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        substNonTypeTemplateParmExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_2074) {
    const StringRef Code = R"cpp(
	  namespace X { int x; }
	  using X::x;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2074 originates here.");
    using Verifier = VerifyBoundNodeMatch<UsingDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(using X::x)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        usingDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2086) {
    const StringRef Code = R"cpp(
	  namespace X { enum x { val1, val2 }; }
	  using enum X::x;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2086 originates here.");
    using Verifier = VerifyBoundNodeMatch<UsingEnumDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(using enum X::x)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        usingEnumDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2099) {
    const StringRef Code = R"cpp(
	  namespace X { int x; }
	  using namespace X;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2099 originates here.");
    using Verifier = VerifyBoundNodeMatch<UsingDirectiveDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(using namespace X)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        usingDirectiveDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2113) {
    const StringRef Code = R"cpp(
	  template<typename T>
	  T foo() { T a; return a; }
	  template<typename T>
	  void bar() {
	    foo<T>();
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2113 originates here.");
    using Verifier = VerifyBoundNodeMatch<UnresolvedLookupExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(foo<T>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        unresolvedLookupExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_2130) {
    const StringRef Code = R"cpp(
	  template<typename X>
	  class C : private X {
	    using X::x;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2130 originates here.");
    using Verifier = VerifyBoundNodeMatch<UnresolvedUsingValueDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(using X::x)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        unresolvedUsingValueDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_2147) {
    const StringRef Code = R"cpp(
	  template <typename T>
	  struct Base { typedef T Foo; };
	
	  template<typename T>
	  struct S : private Base<T> {
	    using typename Base<T>::Foo;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2147 originates here.");
    using Verifier = VerifyBoundNodeMatch<UnresolvedUsingTypenameDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(using typename Base<T>::Foo)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        unresolvedUsingTypenameDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_2166) {
    const StringRef Code = R"cpp(
	  void f(int a) {
	    switch (a) {
	      case 37: break;
	    }
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2166 originates here.");
    using Verifier = VerifyBoundNodeMatch<ConstantExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(37)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        constantExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2181) {
    const StringRef Code = R"cpp(
	  int foo() { return 1; }
	  int bar() {
	    int a = (foo() + 1);
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2181 originates here.");
    using Verifier = VerifyBoundNodeMatch<ParenExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp((foo() + 1))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        parenExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2194) {
    const StringRef Code = R"cpp(
	  struct string {
	    string(const char*);
	    string(const char*s, int n);
	  };
	  void f(const string &a, const string &b);
	  void foo(char *ptr, int n) {
	    f(string(ptr, n), ptr);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2194 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXConstructExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(string(ptr, n))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(ptr)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxConstructExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2214) {
    const StringRef Code = R"cpp(
	  template <typename T>
	  void f(const T& t) { return T(t); }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2214 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXUnresolvedConstructExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(T(t))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxUnresolvedConstructExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_2229) {
    const StringRef Code = R"cpp(
	  struct foo {
	    int i;
	    int f() { return i; }
	    int g() { return this->i; }
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2229 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXThisExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(this)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(i)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxThisExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2247) {
    const StringRef Code = R"cpp(
	  struct S {
	    S() { }  // User defined constructor makes S non-POD.
	    ~S() { } // User defined destructor makes it non-trivial.
	  };
	  void test() {
	    const S &s_ref = S(); // Requires a CXXBindTemporaryExpr.
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2247 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXBindTemporaryExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(S())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxBindTemporaryExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2265) {
    const StringRef Code = R"cpp(
	  struct T {void func();};
	  T f();
	  void g(T);
	  void foo() {
	    T u(f());
	    g(f());
	    f().func();
	    f(); // does not match
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2265 originates here.");
    using Verifier = VerifyBoundNodeMatch<MaterializeTemporaryExpr>;

    std::vector<Verifier::Match> Matches;
	if (Conf.isCXXOrEarlier(14))
		Matches.emplace_back(MatchKind::Code, R"cpp(f())cpp", 3);
	if (Conf.isCXXOrLater(17))
		Matches.emplace_back(MatchKind::Code, R"cpp(f())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        materializeTemporaryExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2288) {
    const StringRef Code = R"cpp(
	  void* operator new(decltype(sizeof(void*)));
	  struct X {};
	  void foo() {
	    auto* x = new X;
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2288 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXNewExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(new X)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxNewExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2304) {
    const StringRef Code = R"cpp(
	  void* operator new(decltype(sizeof(void*)));
	  void operator delete(void*);
	  struct X {};
	  void foo() {
	    auto* x = new X;
	    delete x;
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2304 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXDeleteExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(delete x)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxDeleteExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2323) {
    const StringRef Code = R"cpp(
	  bool a() noexcept;
	  bool b() noexcept(true);
	  bool c() noexcept(false);
	  bool d() noexcept(noexcept(a()));
	  bool e = noexcept(b()) || noexcept(c());
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2323 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXNoexceptExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(noexcept(a()))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(noexcept(b()))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(noexcept(c()))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxNoexceptExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2345) {
    const StringRef Code = R"cpp(
	  void testLambdaCapture() {
	    int a[10];
	    [a]() {};
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2345 originates here.");
    using Verifier = VerifyBoundNodeMatch<ArrayInitLoopExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(a)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        arrayInitLoopExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2382) {
    const StringRef Code = R"cpp(
	  void foo() {
	    int a[2] = {0, 1};
	    int i = a[1];
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2382 originates here.");
    using Verifier = VerifyBoundNodeMatch<ArraySubscriptExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(a[1])cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        arraySubscriptExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2396) {
    const StringRef Code = R"cpp(
	  void f(int x, int y = 0);
	  void g() {
	    f(42);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2396 originates here.");
    using Verifier = VerifyBoundNodeMatch<CallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(f(42))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        callExpr(has(cxxDefaultArgExpr())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2418) {
    const StringRef Code = R"cpp(
	  struct ostream;
	  ostream &operator<< (ostream &out, int i) { };
	  void f(ostream& o, int b, int c) {
	    o << b << c;
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2418 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXOperatorCallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(o << b << c)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(o << b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxOperatorCallExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2437) {
    const StringRef Code = R"cpp(
	  template <typename... Args>
	  auto sum(Args... args) {
	      return (0 + ... + args);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(17);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2437 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXFoldExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp((0 + ... + args))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxFoldExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2452) {
    const StringRef Code = R"cpp(
	  struct HasSpaceshipMem {
	    int a;
	    constexpr bool operator==(const HasSpaceshipMem&) const = default;
	  };
	  void compare() {
	    HasSpaceshipMem hs1, hs2;
	    if (hs1 != hs2)
	        return;
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(20);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2452 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRewrittenBinaryOperator>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(hs1 != hs2)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRewrittenBinaryOperator().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2477) {
    const StringRef Code = R"cpp(
	  int f(int x, int y) { return x + y; }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2477 originates here.");
    using Verifier = VerifyBoundNodeMatch<Expr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(x + y)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(x)cpp", 2);
	Matches.emplace_back(MatchKind::Code, R"cpp(y)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        expr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2490) {
    const StringRef Code = R"cpp(
	  void f(bool x) {
	    if (x) {}
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2490 originates here.");
    using Verifier = VerifyBoundNodeMatch<DeclRefExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(x)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        declRefExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2532) {
    const StringRef Code = R"cpp(
	  void foo(int x) {
	    if (x) {}
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2532 originates here.");
    using Verifier = VerifyBoundNodeMatch<IfStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(if (x) {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        ifStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2544) {
    const StringRef Code = R"cpp(
	  void foo() {
	    for (;;) {}
	    int i[] =  {1, 2, 3}; for (auto a : i);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2544 originates here.");
    using Verifier = VerifyBoundNodeMatch<ForStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(for (;;) {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        forStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2559) {
    const StringRef Code = R"cpp(
	void foo(int N) {
	    for (int x = 0; x < N; ++x) { }
	}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2559 originates here.");
    using Verifier = VerifyBoundNodeMatch<ForStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(for (int x = 0; x < N; ++x) { })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        forStmt(hasIncrement(unaryOperator(hasOperatorName("++")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2576) {
    const StringRef Code = R"cpp(
	void foo(int N) {
	    for (int x = 0; x < N; ++x) { }
	}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2576 originates here.");
    using Verifier = VerifyBoundNodeMatch<ForStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(for (int x = 0; x < N; ++x) { })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        forStmt(hasLoopInit(declStmt())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2591) {
    const StringRef Code = R"cpp(
	  void foo() {
	    int i[] =  {1, 2, 3}; for (auto a : i);
	    for(int j = 0; j < 5; ++j);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2591 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXForRangeStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(for (auto a : i);)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxForRangeStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2606) {
    const StringRef Code = R"cpp(
	  void foo() {
	    int a[42] = {};
	    for (int x : a) { }
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2606 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXForRangeStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(for (int x : a) { })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxForRangeStmt(hasLoopVariable(anything())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2624) {
    const StringRef Code = R"cpp(
	  void foo() {
	    int a[42] = {};
	    for (int x : a) { }
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2624 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXForRangeStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(for (int x : a) { })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxForRangeStmt(hasRangeInit(anything())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2642) {
    const StringRef Code = R"cpp(
	void foo() {
	  while (true) {}
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2642 originates here.");
    using Verifier = VerifyBoundNodeMatch<WhileStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(while (true) {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        whileStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2655) {
    const StringRef Code = R"cpp(
	void foo() {
	  do {} while (true);
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2655 originates here.");
    using Verifier = VerifyBoundNodeMatch<DoStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(do {} while (true))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        doStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2668) {
    const StringRef Code = R"cpp(
	void foo() {
	  while (true) { break; }
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2668 originates here.");
    using Verifier = VerifyBoundNodeMatch<BreakStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(break)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        breakStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2681) {
    const StringRef Code = R"cpp(
	void foo() {
	  while (true) { continue; }
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2681 originates here.");
    using Verifier = VerifyBoundNodeMatch<ContinueStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(continue)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        continueStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2695) {
    const StringRef Code = R"cpp(
	  namespace std {
	  template <typename T = void>
	  struct coroutine_handle {
	      static constexpr coroutine_handle from_address(void* addr) {
	        return {};
	      }
	  };
	
	  struct always_suspend {
	      bool await_ready() const noexcept;
	      bool await_resume() const noexcept;
	      template <typename T>
	      bool await_suspend(coroutine_handle<T>) const noexcept;
	  };
	
	  template <typename T>
	  struct coroutine_traits {
	      using promise_type = T::promise_type;
	  };
	  }  // namespace std
	
	  struct generator {
	      struct promise_type {
	          void return_value(int v);
	          std::always_suspend yield_value(int&&);
	          std::always_suspend initial_suspend() const noexcept;
	          std::always_suspend final_suspend() const noexcept;
	          void unhandled_exception();
	          generator get_return_object();
	      };
	  };
	
	  generator f() {
	      co_return 10;
	  }
	
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(20);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2695 originates here.");
    using Verifier = VerifyBoundNodeMatch<CoreturnStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(co_return 10)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        coreturnStmt(has(integerLiteral())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2742) {
    const StringRef Code = R"cpp(
	int foo() {
	  return 1;
	}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2742 originates here.");
    using Verifier = VerifyBoundNodeMatch<ReturnStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(return 1)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        returnStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2754) {
    const StringRef Code = R"cpp(
	void bar();
	void foo() {
	  goto FOO;
	  FOO: bar();
	}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2754 originates here.");
    using Verifier = VerifyBoundNodeMatch<GotoStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(goto FOO)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        gotoStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2768) {
    const StringRef Code = R"cpp(
	void bar();
	void foo() {
	  goto FOO;
	  FOO: bar();
	}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2768 originates here.");
    using Verifier = VerifyBoundNodeMatch<LabelStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(FOO: bar())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        labelStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2782) {
    const StringRef Code = R"cpp(
	void bar();
	void foo() {
	  FOO: bar();
	  void *ptr = &&FOO;
	  goto *ptr;
	}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2782 originates here.");
    using Verifier = VerifyBoundNodeMatch<AddrLabelExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(&&FOO)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        addrLabelExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2798) {
    const StringRef Code = R"cpp(
	void foo(int a) {
	  switch(a) { case 42: break; default: break; }
	}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2798 originates here.");
    using Verifier = VerifyBoundNodeMatch<SwitchStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(switch(a) { case 42: break; default: break; })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        switchStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2810) {
    const StringRef Code = R"cpp(
	void foo(int a) {
	  switch(a) { case 42: break; default: break; }
	}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2810 originates here.");
    using Verifier = VerifyBoundNodeMatch<SwitchCase>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(case 42: break)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(default: break)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        switchCase().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2822) {
    const StringRef Code = R"cpp(
	void foo(int a) {
	  switch(a) { case 42: break; default: break; }
	}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2822 originates here.");
    using Verifier = VerifyBoundNodeMatch<CaseStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(case 42: break)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        caseStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2834) {
    const StringRef Code = R"cpp(
	void foo(int a) {
	  switch(a) { case 42: break; default: break; }
	}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2834 originates here.");
    using Verifier = VerifyBoundNodeMatch<DefaultStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(default: break)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        defaultStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2847) {
    const StringRef Code = R"cpp(
	void foo() { for (;;) {{}} }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2847 originates here.");
    using Verifier = VerifyBoundNodeMatch<CompoundStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp({ for (;;) {{}} })cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp({{}})cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp({})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        compoundStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2858) {
    const StringRef Code = R"cpp(
	void foo() {
	  try {} catch(int i) {}
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2858 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXCatchStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(catch(int i) {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxCatchStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2871) {
    const StringRef Code = R"cpp(
	void foo() {
	  try {} catch(int i) {}
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2871 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXTryStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(try {} catch(int i) {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxTryStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2883) {
    const StringRef Code = R"cpp(
	void foo() {
	  try { throw 5; } catch(int i) {}
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2883 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXThrowExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(throw 5)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxThrowExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2896) {
    const StringRef Code = R"cpp(
	void foo() {
	  foo();;
	}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2896 originates here.");
    using Verifier = VerifyBoundNodeMatch<NullStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(;)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        nullStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2907) {
    const StringRef Code = R"cpp(
	void foo() {
	 int i = 100;
	  __asm("mov %al, 2");
	}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2907 originates here.");
    using Verifier = VerifyBoundNodeMatch<AsmStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(__asm("mov %al, 2"))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        asmStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2920) {
    const StringRef Code = R"cpp(
	  bool Flag = true;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2920 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXBoolLiteralExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(true)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxBoolLiteral().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2932) {
    const StringRef Code = R"cpp(
	  char *s = "abcd";
	  wchar_t *ws = L"abcd";
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2932 originates here.");
    using Verifier = VerifyBoundNodeMatch<StringLiteral>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp("abcd")cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(L"abcd")cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        stringLiteral().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2949) {
    const StringRef Code = R"cpp(
	  char ch = 'a';
	  wchar_t chw = L'a';
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_2949 originates here.");
    using Verifier = VerifyBoundNodeMatch<CharacterLiteral>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp('a')cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(L'a')cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        characterLiteral().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2966) {
    const StringRef Code = R"cpp(
	  int a = 1;
	  int b = 1L;
	  int c = 0x1;
	  int d = 1U;
	  int e = 1.0;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2966 originates here.");
    using Verifier = VerifyBoundNodeMatch<IntegerLiteral>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(1)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(1L)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(0x1)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(1U)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        integerLiteral().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_2984) {
    const StringRef Code = R"cpp(
	  int a = 1.0;
	  int b = 1.0F;
	  int c = 1.0L;
	  int d = 1e10;
	  int e = 1;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_2984 originates here.");
    using Verifier = VerifyBoundNodeMatch<FloatingLiteral>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(1.0)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(1.0F)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(1.0L)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(1e10)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        floatLiteral().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3002) {
    const StringRef Code = R"cpp(
	  auto a = 1i;
	  auto b = 1.0i;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3002 originates here.");
    using Verifier = VerifyBoundNodeMatch<ImaginaryLiteral>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(1i)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(1.0i)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        imaginaryLiteral().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3021) {
    const StringRef Code = R"cpp(
	  float operator ""_foo(long double);
	  float a = 1234.5_foo;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.Language == Lang_CXX11;
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3021 originates here.");
    using Verifier = VerifyBoundNodeMatch<UserDefinedLiteral>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(1234.5_foo)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        userDefinedLiteral().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3034) {
    const StringRef Code = R"cpp(
	  struct vector { int x; int y; };
	  struct vector myvec = (struct vector){ 1, 2 };
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_3034 originates here.");
    using Verifier = VerifyBoundNodeMatch<CompoundLiteralExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp((struct vector){ 1, 2 })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        compoundLiteralExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3047) {
    const StringRef Code = R"cpp(
	  namespace std {
	  template <typename T = void>
	  struct coroutine_handle {
	      static constexpr coroutine_handle from_address(void* addr) {
	        return {};
	      }
	  };
	
	  struct always_suspend {
	      bool await_ready() const noexcept;
	      bool await_resume() const noexcept;
	      template <typename T>
	      bool await_suspend(coroutine_handle<T>) const noexcept;
	  };
	
	  template <typename T>
	  struct coroutine_traits {
	      using promise_type = T::promise_type;
	  };
	  }  // namespace std
	
	  struct generator {
	      struct promise_type {
	          std::always_suspend yield_value(int&&);
	          std::always_suspend initial_suspend() const noexcept;
	          std::always_suspend final_suspend() const noexcept;
	          void return_void();
	          void unhandled_exception();
	          generator get_return_object();
	      };
	  };
	
	  std::always_suspend h();
	
	  generator g() { co_await h(); }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(20);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3047 originates here.");
    using Verifier = VerifyBoundNodeMatch<CoawaitExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(co_await h())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        coawaitExpr(has(callExpr(callee(functionDecl(hasName("h")))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3094) {
    const StringRef Code = R"cpp(
	  namespace std {
	  template <typename T = void>
	  struct coroutine_handle {
	      static constexpr coroutine_handle from_address(void* addr) {
	        return {};
	      }
	  };
	
	  struct always_suspend {
	      bool await_ready() const noexcept;
	      bool await_resume() const noexcept;
	      template <typename T>
	      bool await_suspend(coroutine_handle<T>) const noexcept;
	  };
	
	  template <typename T>
	  struct coroutine_traits {
	      using promise_type = T::promise_type;
	  };
	  }  // namespace std
	
	  template <typename T>
	  struct generator {
	      struct promise_type {
	          std::always_suspend yield_value(int&&);
	          std::always_suspend initial_suspend() const noexcept;
	          std::always_suspend final_suspend() const noexcept;
	          void return_void();
	          void unhandled_exception();
	          generator get_return_object();
	      };
	  };
	
	  template <typename T>
	  std::always_suspend h();
	
	  template <>
	  std::always_suspend h<void>();
	
	  template<typename T>
	  generator<T> g() { co_await h<T>(); }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(20);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3094 originates here.");
    using Verifier = VerifyBoundNodeMatch<DependentCoawaitExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(co_await h<T>())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        dependentCoawaitExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3146) {
    const StringRef Code = R"cpp(
	  namespace std {
	  template <typename T = void>
	  struct coroutine_handle {
	      static constexpr coroutine_handle from_address(void* addr) {
	        return {};
	      }
	  };
	
	  struct always_suspend {
	      bool await_ready() const noexcept;
	      bool await_resume() const noexcept;
	      template <typename T>
	      bool await_suspend(coroutine_handle<T>) const noexcept;
	  };
	
	  template <typename T>
	  struct coroutine_traits {
	      using promise_type = T::promise_type;
	  };
	  }  // namespace std
	
	  struct generator {
	      struct promise_type {
	          std::always_suspend yield_value(int&&);
	          std::always_suspend initial_suspend() const noexcept;
	          std::always_suspend final_suspend() const noexcept;
	          void return_void();
	          void unhandled_exception();
	          generator get_return_object();
	      };
	  };
	
	  generator f() {
	      while (true) {
	          co_yield 10;
	      }
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(20);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3146 originates here.");
    using Verifier = VerifyBoundNodeMatch<CoyieldExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(co_yield 10)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        coyieldExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3194) {
    const StringRef Code = R"cpp(
	  namespace std {
	  template <typename T = void>
	  struct coroutine_handle {
	      static constexpr coroutine_handle from_address(void* addr) {
	        return {};
	      }
	  };
	
	  struct suspend_always {
	      bool await_ready() const noexcept;
	      bool await_resume() const noexcept;
	      template <typename T>
	      bool await_suspend(coroutine_handle<T>) const noexcept;
	  };
	
	  template <typename...>
	  struct coroutine_traits {
	      struct promise_type {
	          std::suspend_always initial_suspend() const noexcept;
	          std::suspend_always final_suspend() const noexcept;
	          void return_void();
	          void unhandled_exception();
	          coroutine_traits get_return_object();
	      };
	  };
	  }  // namespace std
	
	  void f() { while (true) { co_return; } }
	
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(20);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3194 originates here.");
    using Verifier = VerifyBoundNodeMatch<CoroutineBodyStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp({ while (true) { co_return; } })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        coroutineBodyStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3235) {
    const StringRef Code = R"cpp(
	  int a = 0;
	  int* b = 0;
	  int *c = nullptr;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.Language == Lang_CXX11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3235 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXNullPtrLiteralExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(nullptr)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxNullPtrLiteralExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3264) {
    const StringRef Code = R"cpp(
	  void foo() { int *ptr; __atomic_load_n(ptr, 1); }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_3264 originates here.");
    using Verifier = VerifyBoundNodeMatch<AtomicExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(__atomic_load_n(ptr, 1))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        atomicExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3274) {
    const StringRef Code = R"cpp(
	  void f() {
	    int C = ({ int X = 4; X; });
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_3274 originates here.");
    using Verifier = VerifyBoundNodeMatch<StmtExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(({ int X = 4; X; }))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        stmtExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3286) {
    const StringRef Code = R"cpp(
	  void foo(bool a, bool b) {
	    !(a || b);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3286 originates here.");
    using Verifier = VerifyBoundNodeMatch<BinaryOperator>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(a || b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        binaryOperator().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3302) {
    const StringRef Code = R"cpp(
	  void foo(bool a, bool b) {
	    !a || b;
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3302 originates here.");
    using Verifier = VerifyBoundNodeMatch<UnaryOperator>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(!a)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        unaryOperator().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3316) {
    const StringRef Code = R"cpp(
	  int f(int a, int b, int c) {
	    return (a ? b : c) + 42;
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_3316 originates here.");
    using Verifier = VerifyBoundNodeMatch<ConditionalOperator>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(a ? b : c)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        conditionalOperator().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3329) {
    const StringRef Code = R"cpp(
	  int f(int a, int b) {
	    return (a ?: b) + 42;
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_3329 originates here.");
    using Verifier = VerifyBoundNodeMatch<BinaryConditionalOperator>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(a ?: b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        binaryConditionalOperator().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3345) {
    const StringRef Code = R"cpp(
	  int f(int a, int b) {
	    return (a ?: b) + 42;
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3345 originates here.");
    using Verifier = VerifyBoundNodeMatch<OpaqueValueExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(a)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        opaqueValueExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3360) {
    const StringRef Code = R"cpp(
	  struct S {
	    int x;
	  };
	  static_assert(sizeof(S) == sizeof(int));
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3360 originates here.");
    using Verifier = VerifyBoundNodeMatch<StaticAssertDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(static_assert(sizeof(S) == sizeof(int)))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        staticAssertDecl().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3380) {
    const StringRef Code = R"cpp(
	  void* p = reinterpret_cast<char*>(&p);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3380 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXReinterpretCastExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(reinterpret_cast<char*>(&p))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxReinterpretCastExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3396) {
    const StringRef Code = R"cpp(
	  long eight(static_cast<long>(8));
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3396 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXStaticCastExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(static_cast<long>(8))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxStaticCastExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3409) {
    const StringRef Code = R"cpp(
	  struct B { virtual ~B() {} }; struct D : B {};
	  B b;
	  D* p = dynamic_cast<D*>(&b);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3409 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXDynamicCastExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(dynamic_cast<D*>(&b))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxDynamicCastExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3424) {
    const StringRef Code = R"cpp(
	  int n = 42;
	  const int &r(n);
	  int* p = const_cast<int*>(&r);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3424 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXConstCastExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(const_cast<int*>(&r))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxConstCastExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3439) {
    const StringRef Code = R"cpp(
	  int i = (int) 2.2f;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_3439 originates here.");
    using Verifier = VerifyBoundNodeMatch<CStyleCastExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp((int) 2.2f)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cStyleCastExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3461) {
    const StringRef Code = R"cpp(
	  struct S {};
	  const S* s;
	  S* s2 = const_cast<S*>(s);
	
	  const int val = 0;
	  char val0 = val;
	  char val1 = (char)val;
	  char val2 = static_cast<char>(val);
	  int* val3 = reinterpret_cast<int*>(val);
	  char val4 = char(val);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3461 originates here.");
    using Verifier = VerifyBoundNodeMatch<ExplicitCastExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp((char)val)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(static_cast<char>(val))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(reinterpret_cast<int*>(val))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(const_cast<S*>(s))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(char(val))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        explicitCastExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3488) {
    const StringRef Code = R"cpp(
	void f(int);
	void g(int val1, int val2) {
	  unsigned int a = val1;
	  f(val2);
	}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_3488 originates here.");
    using Verifier = VerifyBoundNodeMatch<ImplicitCastExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(val1)cpp", 2);
	Matches.emplace_back(MatchKind::Code, R"cpp(f)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(val2)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        implicitCastExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3506) {
    const StringRef Code = R"cpp(
	  struct S {};
	  const S* s;
	  S* s2 = const_cast<S*>(s);
	
	  const int val = 0;
	  char val0 = 1;
	  char val1 = (char)2;
	  char val2 = static_cast<char>(3);
	  int* val3 = reinterpret_cast<int*>(4);
	  char val4 = char(5);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3506 originates here.");
    using Verifier = VerifyBoundNodeMatch<CastExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(const_cast<S*>(s))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(s)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(1)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp((char)2)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(2)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(static_cast<char>(3))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(3)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(reinterpret_cast<int*>(4))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(char(5))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(5)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        castExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3536) {
    const StringRef Code = R"cpp(
	  struct Foo {
	    Foo(int x);
	  };
	
	  void foo(int bar) {
	    Foo f = bar;
	    Foo g = (Foo) bar;
	    Foo h = Foo(bar);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3536 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXFunctionalCastExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(Foo(bar))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxFunctionalCastExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3557) {
    const StringRef Code = R"cpp(
	  struct Foo {
	    Foo(int x, int y);
	  };
	
	  void foo(int bar) {
	    Foo h = Foo(bar, bar);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3557 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXTemporaryObjectExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(Foo(bar, bar))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxTemporaryObjectExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3576) {
    const StringRef Code = R"cpp(
	  void f() {
	    const char* func_name = __func__;
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_3576 originates here.");
    using Verifier = VerifyBoundNodeMatch<PredefinedExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(__func__)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        predefinedExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3590) {
    const StringRef Code = R"cpp(
	  struct point2 { double x; double y; };
	  struct point2 ptarray[10] = { [0].x = 1.0 };
	  struct point2 pt = { .x = 2.0 };
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_3590 originates here.");
    using Verifier = VerifyBoundNodeMatch<DesignatedInitExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp([0].x = 1.0)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(.x = 2.0)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        designatedInitExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3605) {
    const StringRef Code = R"cpp(
	  struct point2 { double x; double y; };
	  struct point2 ptarray[10] = { [0].x = 1.0 };
	  struct point2 pt = { .x = 2.0 };
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_3605 originates here.");
    using Verifier = VerifyBoundNodeMatch<DesignatedInitExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp([0].x = 1.0)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        designatedInitExpr(designatorCountIs(2)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3620) {
    const StringRef Code = R"cpp(
	  int a = 0;
	  const int b = 1;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_3620 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(const int b = 1)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasType(qualType(isConstQualified()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3632) {
    const StringRef Code = R"cpp(
	  const int b = 1;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_3632 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(const int b = 1)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasType(type().bind("type"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3645) {
    const StringRef Code = R"cpp(
	  void foo(int val);
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_3645 originates here.");
    using Verifier = VerifyBoundNodeMatch<DeclaratorDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(void foo(int val))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(int val)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        declaratorDecl(hasTypeLoc(typeLoc().bind("type"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3661) {
    const StringRef Code = R"cpp(
	  void f(int a, int b);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3661 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(void f(int a, int b))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(hasAnyParameter(
eachOf(parmVarDecl(hasName("a")).bind("v"),
       parmVarDecl(hasName("b")).bind("v")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3684) {
    const StringRef Code = R"cpp(
	  char v0 = 'a';
	  int v1 = 1;
	  float v2 = 2.0;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_3684 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(char v0 = 'a')cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(int v1 = 1)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(anyOf(hasName("v0"), hasType(isInteger()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3700) {
    const StringRef Code = R"cpp(
	  int v0 = 0;
	  int v1 = 1;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_3700 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int v0 = 0)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(allOf(hasName("v0"), hasType(isInteger()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3718) {
    const StringRef Code = R"cpp(
	  int a = 0;
	  int b;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_3718 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int a = 0)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(int b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(optionally(hasInitializer(expr()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3732) {
    const StringRef Code = R"cpp(
	  int x = 42;
	  int y = sizeof(x) + alignof(x);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXXOrLater(11));
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3732 originates here.");
    using Verifier = VerifyBoundNodeMatch<UnaryExprOrTypeTraitExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(sizeof(x))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(alignof(x))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        unaryExprOrTypeTraitExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3746) {
    const StringRef Code = R"cpp(
	  void f() {
	    if (true);
	    for (; true; );
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3746 originates here.");
    using Verifier = VerifyBoundNodeMatch<Stmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(if (true);)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(for (; true; );)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        stmt(mapAnyOf(ifStmt, forStmt).with(
    hasCondition(cxxBoolLiteral(equals(true)))
    )).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_3746 originates here.");
    using Verifier = VerifyBoundNodeMatch<Stmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(if (true);)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(for (; true; );)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        stmt(anyOf(
    ifStmt(hasCondition(cxxBoolLiteral(equals(true)))).bind("trueCond"),
    forStmt(hasCondition(cxxBoolLiteral(equals(true)))).bind("trueCond")
    )).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3791) {
    const StringRef Code = R"cpp(
	  struct S{
	      bool operator!=(const S&) const;
	  };
	
	  void foo()
	  {
	     1 != 2;
	     S() != S();
	  }
	
	  template<typename T>
	  void templ()
	  {
	     3 != 4;
	     T() != S();
	  }
	  struct HasOpEq
	  {
	      friend bool
	      operator==(const HasOpEq &, const HasOpEq&) noexcept = default;
	  };
	
	  void inverse()
	  {
	      HasOpEq e1;
	      HasOpEq e2;
	      if (e1 != e2)
	          return;
	  }
	
	  struct HasSpaceship
	  {
	      friend bool
	      operator<=>(const HasSpaceship &,
	                  const HasSpaceship&) noexcept = default;
	  };
	
	  void use_spaceship()
	  {
	      HasSpaceship s1;
	      HasSpaceship s2;
	      if (s1 != s2)
	          return;
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(20);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3791 originates here.");
    using Verifier = VerifyBoundNodeMatch<Expr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(1 != 2)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(S() != S())cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(3 != 4)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(T() != S())cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(e1 != e2)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(s1 != s2)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        binaryOperation(
    hasOperatorName("!="),
    hasLHS(expr().bind("lhs")),
    hasRHS(expr().bind("rhs"))
  ).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_3858) {
    const StringRef Code = R"cpp(
	struct ConstructorTakesInt
	{
	  ConstructorTakesInt(int i) {}
	};
	
	void callTakesInt(int i)
	{
	}
	
	void doCall()
	{
	  callTakesInt(42);
	}
	
	void doConstruct()
	{
	  ConstructorTakesInt cti(42);
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3858 originates here.");
    using Verifier = VerifyBoundNodeMatch<Expr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(callTakesInt(42))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(cti(42))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        expr(invocation(hasArgument(0, integerLiteral(equals(42))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3889) {
    const StringRef Code = R"cpp(
	  int a, c; float b; int s = sizeof(a) + sizeof(b) + alignof(c);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXXOrLater(11));
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3889 originates here.");
    using Verifier = VerifyBoundNodeMatch<UnaryExprOrTypeTraitExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(sizeof(a))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(alignof(c))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        unaryExprOrTypeTraitExpr(hasArgumentOfType(asString("int"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3905) {
    const StringRef Code = R"cpp(
	  int x;
	  int s = sizeof(x) + alignof(x);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXXOrLater(11));
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3905 originates here.");
    using Verifier = VerifyBoundNodeMatch<UnaryExprOrTypeTraitExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(sizeof(x))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        unaryExprOrTypeTraitExpr(ofKind(UETT_SizeOf)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3923) {
    const StringRef Code = R"cpp(
	  int align = alignof(int);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXXOrLater(11));
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3923 originates here.");
    using Verifier = VerifyBoundNodeMatch<Expr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(alignof(int))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        alignOfExpr(expr()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3941) {
    const StringRef Code = R"cpp(
	  struct S { double x; double y; };
	  int size = sizeof(struct S);
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_3941 originates here.");
    using Verifier = VerifyBoundNodeMatch<Expr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(sizeof(struct S))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        sizeOfExpr(expr()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3961) {
    const StringRef Code = R"cpp(
	  class X;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3961 originates here.");
    using Verifier = VerifyBoundNodeMatch<NamedDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(class X)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        namedDecl(hasName("X")).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3969) {
    const StringRef Code = R"cpp(
	  namespace a { namespace b { class X; } }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_3969 originates here.");
    using Verifier = VerifyBoundNodeMatch<NamedDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(class X)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        namedDecl(hasName("::a::b::X")).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_3969 originates here.");
    using Verifier = VerifyBoundNodeMatch<NamedDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(class X)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        namedDecl(hasName("a::b::X")).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_3969 originates here.");
    using Verifier = VerifyBoundNodeMatch<NamedDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(class X)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        namedDecl(hasName("b::X")).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_3969 originates here.");
    using Verifier = VerifyBoundNodeMatch<NamedDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(class X)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        namedDecl(hasName("X")).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_3990) {
    const StringRef Code = R"cpp(
	  void f(int a, int b);
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_3990 originates here.");
    using Verifier = VerifyBoundNodeMatch<NamedDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int a)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(int b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        namedDecl(hasAnyName("a", "b")).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_3990 originates here.");
    using Verifier = VerifyBoundNodeMatch<NamedDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int a)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(int b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        namedDecl(hasAnyName("a", "b")).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4011) {
    const StringRef Code = R"cpp(
	  namespace foo { namespace bar { class X; } }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4011 originates here.");
    using Verifier = VerifyBoundNodeMatch<NamedDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(class X)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        namedDecl(matchesName("^::foo:.*X")).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4029) {
    const StringRef Code = R"cpp(
	  struct A { int operator*(); };
	  const A &operator<<(const A &a, const A &b);
	  void f(A a) {
	    a << a;   // <-- This matches
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4029 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXOperatorCallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(a << a)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxOperatorCallExpr(hasOverloadedOperatorName("<<")).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_4029 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(struct A { int operator*(); })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasMethod(hasOverloadedOperatorName("*"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4064) {
    const StringRef Code = R"cpp(
	  struct Point { double x; double y; };
	  Point operator+(const Point&, const Point&);
	  Point operator-(const Point&, const Point&);
	
	  Point sub(Point a, Point b) {
	    return b - a;
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4064 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(Point operator+(const Point&, const Point&))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(Point operator-(const Point&, const Point&))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(hasAnyOverloadedOperatorName("+", "-")).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_4064 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(Point operator+(const Point&, const Point&))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(Point operator-(const Point&, const Point&))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(anyOf(hasAnyOverloadedOperatorName("+"),
hasOverloadedOperatorName("-"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_4064 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXOperatorCallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(b - a)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxOperatorCallExpr(hasAnyOverloadedOperatorName("+", "-")).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_4064 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXOperatorCallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(b - a)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxOperatorCallExpr(anyOf(hasOverloadedOperatorName("+"),
hasOverloadedOperatorName("-"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4106) {
    const StringRef Code = R"cpp(
	  template <typename T>
	  struct S {
	      void mem();
	  };
	  template <typename T>
	  void x() {
	      S<T> s;
	      s.mem();
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4106 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXDependentScopeMemberExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(s.mem)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxDependentScopeMemberExpr(hasMemberName("mem")).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_4134) {
    const StringRef Code = R"cpp(
	  template <typename T>
	  struct S {
	      void mem();
	  };
	  template <typename T>
	  void x() {
	      S<T> s;
	      s.mem();
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4134 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXDependentScopeMemberExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(s.mem)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxDependentScopeMemberExpr(
  hasObjectExpression(declRefExpr(hasType(
    elaboratedType(namesType(templateSpecializationType(
      hasDeclaration(classTemplateDecl(has(cxxRecordDecl(has(
          cxxMethodDecl(hasName("mem")).bind("templMem")
          )))))
    )))
  ))),
  memberHasSameNameAsBoundNode("templMem")
).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_4182) {
    const StringRef Code = R"cpp(
	  class X {};
	  class Y : public X {};  // directly derived
	  class Z : public Y {};  // indirectly derived
	  typedef X A;
	  typedef A B;
	  class C : public B {};  // derived from a typedef of X
	
	  class Foo {};
	  typedef Foo Alias;
	  class Bar : public Alias {};
	  // derived from a type that Alias is a typedef of Foo
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4182 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(Y)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(Z)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(C)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(isDerivedFrom(hasName("X"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_4182 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(Bar)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(isDerivedFrom(hasName("Foo"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4233) {
    const StringRef Code = R"cpp(
	  class X {};
	  class Y : public X {};  // directly derived
	  class Z : public Y {};  // indirectly derived
	  typedef X A;
	  typedef A B;
	  class C : public B {};  // derived from a typedef of X
	
	  class Foo {};
	  typedef Foo Alias;
	  class Bar : public Alias {};  // derived from Alias, which is a
	                                // typedef of Foo
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4233 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(Y)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(Z)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(C)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(isDerivedFrom("X")).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_4233 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(Bar)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(isDerivedFrom("Foo")).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4281) {
    const StringRef Code = R"cpp(
	  class Foo {};
	  class Bar : Foo {};
	  class Baz : Bar {};
	  class SpecialBase {};
	  class Proxy : SpecialBase {};  // matches Proxy
	  class IndirectlyDerived : Proxy {};  //matches IndirectlyDerived
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4281 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(Proxy)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(IndirectlyDerived)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasAnyBase(hasType(cxxRecordDecl(hasName("SpecialBase"))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4303) {
    const StringRef Code = R"cpp(
	  class Foo {};
	  class Bar : Foo {};
	  class Baz : Bar {};
	  class SpecialBase {};
	  class Proxy : SpecialBase {};  // matches Proxy
	  class IndirectlyDerived : Proxy {};  // doesn't match
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4303 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(Proxy)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasDirectBase(hasType(cxxRecordDecl(hasName("SpecialBase"))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4362) {
    const StringRef Code = R"cpp(
	  class X {};
	  class Y : public X {};  // directly derived
	  class Z : public Y {};  // indirectly derived
	  typedef X A;
	  typedef A B;
	  class C : public B {};  // derived from a typedef of X
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4362 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(Y)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(C)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(isDirectlyDerivedFrom(namedDecl(hasName("X")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4375) {
    const StringRef Code = R"cpp(
	  class Foo {};
	  typedef Foo X;
	  class Bar : public Foo {};  // derived from a type that X is a typedef of
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4375 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(Bar)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(isDerivedFrom(hasName("X"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4417) {
    const StringRef Code = R"cpp(
	  class A { void func(); };
	  class B { void member(); };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4417 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(class A { void func(); })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasMethod(hasName("func"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4443) {
    const StringRef Code = R"cpp(
	  auto x = []{};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4443 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(auto x = []{})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasType(cxxRecordDecl(isLambda()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4456) {
    const StringRef Code = R"cpp(
	  class X {};  // Matches X, because X::X is a class of name X inside X.
	  class Y { class X {}; };
	  class Z { class Y { class X {}; }; };  // Does not match Z.
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4456 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(class X {})cpp", 3);
	Matches.emplace_back(MatchKind::Code, R"cpp(class Y { class X {}; })cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(has(cxxRecordDecl(hasName("X")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4473) {
    const StringRef Code = R"cpp(
	  int x =0;
	  double y = static_cast<double>(x);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4473 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXStaticCastExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(static_cast<double>(x))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxStaticCastExpr(has(ignoringParenImpCasts(declRefExpr()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4488) {
    const StringRef Code = R"cpp(
	  class X {};  // Matches X, because X::X is a class of name X inside X.
	  class Y { class X {}; };
	  class Z { class Y { class X {}; }; };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4488 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(class X {})cpp", 3);
	Matches.emplace_back(MatchKind::Code, R"cpp(class Y { class X {}; })cpp", 2);
	Matches.emplace_back(MatchKind::Code, R"cpp(class Z { class Y { class X {}; }; })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasDescendant(cxxRecordDecl(hasName("X")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4510) {
    const StringRef Code = R"cpp(
	  class X {};
	  class Y { class X {}; };  // Matches Y, because Y::X is a class of name X
	                            // inside Y.
	  class Z { class Y { class X {}; }; };  // Does not match Z.
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4510 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(class X)cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(class Y)cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(class Y::X)cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(class Z::Y::X)cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(class Z::Y)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(forEach(cxxRecordDecl(hasName("X")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4536) {
    const StringRef Code = R"cpp(
	  class X {};
	  class A { class X {}; };  // Matches A, because A::X is a class of name
	                            // X inside A.
	  class B { class C { class X {}; }; };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4536 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(X)cpp", 2);
	Matches.emplace_back(MatchKind::Name, R"cpp(A)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(B)cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(class B::C)cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(class B::C::X)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(forEachDescendant(cxxRecordDecl(hasName("X")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4554) {
    const StringRef Code = R"cpp(
	  struct A {
	    struct B {
	      struct C {};
	      struct D {};
	    };
	  };
	
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4554 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(A)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(A)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(A)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(A)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(A)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(A)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(A)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(B)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(B)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(forEachDescendant(cxxRecordDecl(
    forEachDescendant(cxxRecordDecl().bind("inner"))
  ).bind("middle"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4615) {
    const StringRef Code = R"cpp(
	  class A { class B {}; class C {}; };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4615 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(A)cpp", 3);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasName("::A"),
                findAll(cxxRecordDecl(isDefinition()).bind("m"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4637) {
    const StringRef Code = R"cpp(
	void f() { for (;;) { int x = 42; if (true) { int x = 43; } } }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4637 originates here.");
    using Verifier = VerifyBoundNodeMatch<CompoundStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp({ int x = 43; })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        compoundStmt(hasParent(ifStmt())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4655) {
    const StringRef Code = R"cpp(
	void f() { if (true) { int x = 42; } }
	void g() { for (;;) { int x = 43; } }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4655 originates here.");
    using Verifier = VerifyBoundNodeMatch<Expr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(42)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        expr(integerLiteral(hasAncestor(ifStmt()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4674) {
    const StringRef Code = R"cpp(
	  class X {};
	  class Y {};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4674 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(Y)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(unless(hasName("X"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4697) {
    const StringRef Code = R"cpp(
	  class X {};
	  typedef X Y;
	  Y y;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4697 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(Y y)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasType(qualType(hasDeclaration(decl().bind("d"))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_4697 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(Y y)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasType(hasUnqualifiedDesugaredType(
      recordType(hasDeclaration(decl().bind("d")))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4739) {
    const StringRef Code = R"cpp(
	  namespace N { template<class T> void f(T t); }
	  template <class T> void g() { using N::f; f(T()); }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4739 originates here.");
    using Verifier = VerifyBoundNodeMatch<UnresolvedLookupExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(f)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        unresolvedLookupExpr(hasAnyDeclaration(
    namedDecl(hasUnderlyingDecl(hasName("::N::f"))))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_4759) {
    const StringRef Code = R"cpp(
	  class Y { public: void m(); };
	  Y g();
	  class X : public Y {};
	  void z(Y y, X x) { y.m(); (g()).m(); x.m(); }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4759 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMemberCallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(y.m())cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp((g()).m())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMemberCallExpr(on(hasType(cxxRecordDecl(hasName("Y"))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_4759 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMemberCallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(x.m())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMemberCallExpr(on(hasType(cxxRecordDecl(hasName("X"))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_4759 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMemberCallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp((g()).m())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMemberCallExpr(on(callExpr())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4985) {
    const StringRef Code = R"cpp(
	  class Y { void x() { this->x(); x(); Y y; y.x(); } };
	  void f() { f(); }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4985 originates here.");
    using Verifier = VerifyBoundNodeMatch<CallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(this->x())cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(x())cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(y.x())cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(f())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        callExpr(callee(expr().bind("callee"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_4996) {
    const StringRef Code = R"cpp(
	  struct Dummy {};
	  // makes sure there is a callee, otherwise there would be no callee,
	  // just a builtin operator
	  Dummy operator+(Dummy, Dummy);
	  // not defining a '*' operator
	
	  template <typename... Args>
	  auto sum(Args... args) {
	      return (0 + ... + args);
	  }
	
	  template <typename... Args>
	  auto multiply(Args... args) {
	      return (args * ... * 1);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(17);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_4996 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXFoldExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp((0 + ... + args))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxFoldExpr(callee(expr().bind("op"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5041) {
    const StringRef Code = R"cpp(
	  class Y { public: void x(); };
	  void z() { Y y; y.x(); }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5041 originates here.");
    using Verifier = VerifyBoundNodeMatch<CallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(y.x())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        callExpr(callee(cxxMethodDecl(hasName("x")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5081) {
    const StringRef Code = R"cpp(
	 class X {};
	 void y(X &x) { x; X z; }
	 typedef int U;
	 class Y { friend class X; };
	 class Z : public virtual X {};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5081 originates here.");
    using Verifier = VerifyBoundNodeMatch<Expr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(x)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(z)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        expr(hasType(cxxRecordDecl(hasName("X")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_5081 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(z)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasType(cxxRecordDecl(hasName("X")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_5081 originates here.");
    using Verifier = VerifyBoundNodeMatch<TypedefDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(typedef int U)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        typedefDecl(hasType(asString("int"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_5081 originates here.");
    using Verifier = VerifyBoundNodeMatch<FriendDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(friend class X)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        friendDecl(hasType(asString("class X"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_5081 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(class Z : public virtual X {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasAnyBase(cxxBaseSpecifier(hasType(
asString("X"))).bind("b"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5121) {
    const StringRef Code = R"cpp(
	 class X {};
	 void y(X &x) { x; X z; }
	 class Y { friend class X; };
	 class Z : public virtual X {};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5121 originates here.");
    using Verifier = VerifyBoundNodeMatch<Expr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(x)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(z)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        expr(hasType(cxxRecordDecl(hasName("X")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_5121 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(z)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasType(cxxRecordDecl(hasName("X")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_5121 originates here.");
    using Verifier = VerifyBoundNodeMatch<FriendDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(friend class X)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        friendDecl(hasType(asString("class X"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_5121 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(class Z : public virtual X {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasAnyBase(cxxBaseSpecifier(hasType(
asString("X"))).bind("b"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5140) {
    const StringRef Code = R"cpp(
	class Base {};
	class Derived : Base {};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5140 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(class Derived : Base {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasAnyBase(hasType(cxxRecordDecl(hasName("Base"))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5166) {
    const StringRef Code = R"cpp(
	  int x;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_5166 originates here.");
    using Verifier = VerifyBoundNodeMatch<DeclaratorDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int x)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        declaratorDecl(hasTypeLoc(loc(asString("int")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5172) {
    const StringRef Code = R"cpp(
	struct point { point(double, double); };
	point p = point(1.0, -1.0);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5172 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXTemporaryObjectExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(point(1.0, -1.0))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxTemporaryObjectExpr(hasTypeLoc(loc(asString("point")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5181) {
    const StringRef Code = R"cpp(
	struct Foo { Foo(int, int); };
	Foo x = Foo(1, 2);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5181 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXTemporaryObjectExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(Foo(1, 2))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxTemporaryObjectExpr(hasTypeLoc(
                          loc(asString("Foo")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5219) {
    const StringRef Code = R"cpp(
	  class Y { public: void x(); };
	  void z() { Y* y; y->x(); }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5219 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMemberCallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(y->x())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMemberCallExpr(on(hasType(asString("Y *")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5234) {
    const StringRef Code = R"cpp(
	  class Y { public: void x(); };
	  void z() { Y *y; y->x(); }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5234 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMemberCallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(y->x())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMemberCallExpr(on(hasType(pointsTo(
     cxxRecordDecl(hasName("Y")))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5258) {
    const StringRef Code = R"cpp(
	  class A {};
	  using B = A;
	  B b;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5258 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(B b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasType(hasUnqualifiedDesugaredType(recordType()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5277) {
    const StringRef Code = R"cpp(
	  class X {
	    void a(X b) {
	      X &x = b;
	      const X &y = b;
	    }
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5277 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(X &x = b)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(const X &y = b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasType(references(cxxRecordDecl(hasName("X"))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5297) {
    const StringRef Code = R"cpp(
	  typedef int &int_ref;
	  int a;
	  int_ref b = a;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5297 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;


    EXPECT_TRUE(notMatches(
        Code,
        varDecl(hasType(qualType(referenceType()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_5297 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int_ref b = a)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasType(qualType(hasCanonicalType(referenceType())))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5326) {
    const StringRef Code = R"cpp(
	  class Y { public: void m(); };
	  Y g();
	  class X : public Y { public: void g(); };
	  void z(Y y, X x) { y.m(); x.m(); x.g(); (g()).m(); }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5326 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMemberCallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(y.m())cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(x.m())cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp((g()).m())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMemberCallExpr(onImplicitObjectArgument(hasType(
    cxxRecordDecl(hasName("Y"))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_5326 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMemberCallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp((g()).m())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMemberCallExpr(on(callExpr())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5354) {
    const StringRef Code = R"cpp(
	  class Y { public: void m(); };
	  class X : public Y { public: void g(); };
	  void z() { Y y; y.m(); Y *p; p->m(); X x; x.m(); x.g(); }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5354 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMemberCallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(y.m())cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(p->m())cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(x.m())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMemberCallExpr(thisPointerType(
    cxxRecordDecl(hasName("Y")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_5354 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMemberCallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(x.g())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMemberCallExpr(thisPointerType(
    cxxRecordDecl(hasName("X")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5385) {
    const StringRef Code = R"cpp(
	  void foo() {
	    bool x;
	    if (x) {}
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5385 originates here.");
    using Verifier = VerifyBoundNodeMatch<DeclRefExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(x)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        declRefExpr(to(varDecl(hasName("x")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5404) {
    const StringRef Code = R"cpp(
	  namespace a { int f(); }
	  using a::f;
	  int x = f();
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5404 originates here.");
    using Verifier = VerifyBoundNodeMatch<DeclRefExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(f)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        declRefExpr(throughUsingDecl(anything())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5412) {
    const StringRef Code = R"cpp(
	  namespace a { class X{}; }
	  using a::X;
	  X x;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5412 originates here.");
    using Verifier = VerifyBoundNodeMatch<TypeLoc>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(X)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        typeLoc(loc(usingType(throughUsingDecl(anything())))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5437) {
    const StringRef Code = R"cpp(
	  template <typename T> void foo(T);
	  template <typename T> void bar(T);
	  template <typename T> void baz(T t) {
	    foo(t);
	    bar(t);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5437 originates here.");
    using Verifier = VerifyBoundNodeMatch<UnresolvedLookupExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(foo)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        unresolvedLookupExpr(hasAnyDeclaration(
    functionTemplateDecl(hasName("foo")))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_5460) {
    const StringRef Code = R"cpp(
	  void foo() {
	    int a, b;
	    int c;
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_5460 originates here.");
    using Verifier = VerifyBoundNodeMatch<DeclStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int c;)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        declStmt(hasSingleDecl(anything())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5481) {
    const StringRef Code = R"cpp(
	  int y() { return 0; }
	  void foo() {
	    int x = y();
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_5481 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(x)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasInitializer(callExpr())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5499) {
    const StringRef Code = R"cpp(
	auto f = [x = 3]() { return x; };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5499 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(x = 3)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(isInitCapture()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5510) {
    const StringRef Code = R"cpp(
	  int main() {
	    int x;
	    int y;
	    float z;
	    auto f = [=]() { return x + y + z; };
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5510 originates here.");
    using Verifier = VerifyBoundNodeMatch<LambdaExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp([=]() { return x + y + z; })cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        lambdaExpr(forEachLambdaCapture(
    lambdaCapture(capturesVar(
    varDecl(hasType(isInteger())).bind("captured"))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5545) {
    const StringRef Code = R"cpp(
	void f() {
	  int x;
	  static int y;
	}
	static int z;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_5545 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(y)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(isStaticLocal()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5560) {
    const StringRef Code = R"cpp(
	void f() {
	  int x;
	  static int y;
	}
	int z;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_5560 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(x)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasLocalStorage()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5574) {
    const StringRef Code = R"cpp(
	void f() {
	  int x;
	  static int y;
	}
	int z;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_5574 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(y)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(z)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasGlobalStorage()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5588) {
    const StringRef Code = R"cpp(
	void f() {
	  int x;
	  static int y;
	  thread_local int z;
	}
	int a;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5588 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(x)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasAutomaticStorageDuration()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5609) {
    const StringRef Code = R"cpp(
	void f() {
	  int x;
	  static int y;
	  thread_local int z;
	}
	int a;
	static int b;
	extern int c;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5609 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(y)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(a)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(b)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(c)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasStaticStorageDuration()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5630) {
    const StringRef Code = R"cpp(
	void f() {
	  int x;
	  static int y;
	  thread_local int z;
	}
	int a;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5630 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(z)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasThreadStorageDuration()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5651) {
    const StringRef Code = R"cpp(
	void f(int y) {
	  try {
	  } catch (int x) {
	  }
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5651 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(x)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(isExceptionVariable()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5667) {
    const StringRef Code = R"cpp(
	  void f(int x, int y);
	  void foo() {
	    f(0, 0);
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_5667 originates here.");
    using Verifier = VerifyBoundNodeMatch<CallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(f(0, 0))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        callExpr(argumentCountIs(2)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5695) {
    const StringRef Code = R"cpp(
	  void f(int x, int y);
	  void g(int x, int y, int z);
	  void foo() {
	    f(0, 0);
	    g(0, 0, 0);
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_5695 originates here.");
    using Verifier = VerifyBoundNodeMatch<CallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(f(0, 0))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(g(0, 0, 0))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        callExpr(argumentCountAtLeast(2)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5725) {
    const StringRef Code = R"cpp(
	  void x(int) { int y; x(y); }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_5725 originates here.");
    using Verifier = VerifyBoundNodeMatch<CallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(x(y))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        callExpr(hasArgument(0, declRefExpr().bind("arg"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5747) {
    const StringRef Code = R"cpp(
	  template <typename... Args>
	  auto sum(Args... args) {
	      return (0 + ... + args);
	  }
	
	  template <typename... Args>
	  auto multiply(Args... args) {
	      return (args * ... * 1);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(17);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5747 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXFoldExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp((0 + ... + args))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp((args * ... * 1))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxFoldExpr(hasFoldInit(expr().bind("init"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5772) {
    const StringRef Code = R"cpp(
	  template <typename... Args>
	  auto sum(Args... args) {
	      return (0 + ... + args);
	  }
	
	  template <typename... Args>
	  auto multiply(Args... args) {
	      return (args * ... * 1);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(17);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5772 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXFoldExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp((0 + ... + args))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp((args * ... * 1))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxFoldExpr(hasPattern(expr().bind("pattern"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5797) {
    const StringRef Code = R"cpp(
	  template <typename... Args>
	  auto sum(Args... args) {
	      return (0 + ... + args);
	  }
	
	  template <typename... Args>
	  auto multiply(Args... args) {
	      return (args * ... * 1);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(17);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5797 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXFoldExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp((args * ... * 1))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxFoldExpr(isRightFold()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5817) {
    const StringRef Code = R"cpp(
	  template <typename... Args>
	  auto sum(Args... args) {
	      return (0 + ... + args);
	  }
	
	  template <typename... Args>
	  auto multiply(Args... args) {
	      return (args * ... * 1);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(17);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5817 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXFoldExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp((0 + ... + args))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxFoldExpr(isLeftFold()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5838) {
    const StringRef Code = R"cpp(
	  template <typename... Args>
	  auto sum(Args... args) {
	      return (0 + ... + args);
	  }
	
	  template <typename... Args>
	  auto multiply(Args... args) {
	      return (args * ...);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(17);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5838 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXFoldExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp((args * ...))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxFoldExpr(isUnaryFold()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5858) {
    const StringRef Code = R"cpp(
	  template <typename... Args>
	  auto sum(Args... args) {
	      return (0 + ... + args);
	  }
	
	  template <typename... Args>
	  auto multiply(Args... args) {
	      return (args * ...);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(17);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5858 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXFoldExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp((0 + ... + args))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxFoldExpr(isBinaryFold()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5878) {
    const StringRef Code = R"cpp(
	  int y = 42;
	  int x{y};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5878 originates here.");
    using Verifier = VerifyBoundNodeMatch<InitListExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp({y})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        initListExpr(hasInit(0, expr())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5895) {
    const StringRef Code = R"cpp(
	  void foo() {
	    int a, b;
	    int c;
	    int d = 2, e;
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_5895 originates here.");
    using Verifier = VerifyBoundNodeMatch<DeclStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int a, b;)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(int d = 2, e;)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        declStmt(declCountIs(2)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5916) {
    const StringRef Code = R"cpp(
	  void foo() {
	    int a, b = 0;
	    int c;
	    int d = 2, e;
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_5916 originates here.");
    using Verifier = VerifyBoundNodeMatch<DeclStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int d = 2, e;)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        declStmt(containsDeclaration(
      0, varDecl(hasInitializer(anything())))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_5916 originates here.");
    using Verifier = VerifyBoundNodeMatch<DeclStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int a, b = 0;)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(int d = 2, e;)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        declStmt(containsDeclaration(1, varDecl())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5942) {
    const StringRef Code = R"cpp(
	  void foo() {
	    try {}
	    catch (int) {}
	    catch (...) {}
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5942 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXCatchStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(catch (...) {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxCatchStmt(isCatchAll()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5960) {
    const StringRef Code = R"cpp(
	  struct Foo {
	    Foo() : foo_(1) { }
	    int foo_;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5960 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(Foo)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(has(cxxConstructorDecl(
  hasAnyConstructorInitializer(anything())
))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_5983) {
    const StringRef Code = R"cpp(
	  struct Foo {
	    Foo() : foo_(1) { }
	    int foo_;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_5983 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(Foo)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(has(cxxConstructorDecl(hasAnyConstructorInitializer(
    forField(hasName("foo_")))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6005) {
    const StringRef Code = R"cpp(
	  struct Foo {
	    Foo() : foo_(1) { }
	    int foo_;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6005 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(Foo)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(has(cxxConstructorDecl(hasAnyConstructorInitializer(
    withInitializer(integerLiteral(equals(1))))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6028) {
    const StringRef Code = R"cpp(
	  struct Bar { explicit Bar(const char*); };
	  struct Foo {
	    Foo() { }
	    Foo(int) : foo_("A") { }
	    Bar foo_{""};
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6028 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXConstructorDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(Foo(int) : foo_("A") { })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxConstructorDecl(hasAnyConstructorInitializer(isWritten())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6046) {
    const StringRef Code = R"cpp(
	  struct B {};
	  struct D : B {
	    int I;
	    D(int i) : I(i) {}
	  };
	  struct E : B {
	    E() : B() {}
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6046 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXConstructorDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(E() : B() {})cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(D(int i) : I(i) {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxConstructorDecl(hasAnyConstructorInitializer(isBaseInitializer())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6070) {
    const StringRef Code = R"cpp(
	  struct B {};
	  struct D : B {
	    int I;
	    D(int i) : I(i) {}
	  };
	  struct E : B {
	    E() : B() {}
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6070 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXConstructorDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(D(int i) : I(i) {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxConstructorDecl(hasAnyConstructorInitializer(isMemberInitializer())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6093) {
    const StringRef Code = R"cpp(
	  void x(int, int, int) { int y = 42; x(1, y, 42); }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_6093 originates here.");
    using Verifier = VerifyBoundNodeMatch<CallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(x(1, y, 42))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        callExpr(hasAnyArgument(ignoringImplicit(declRefExpr()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6131) {
    const StringRef Code = R"cpp(
	  int main() {
	    int x;
	    auto f = [x](){};
	    auto g = [x = 1](){};
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6131 originates here.");
    using Verifier = VerifyBoundNodeMatch<LambdaExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp([x](){})cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp([x = 1](){})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        lambdaExpr(hasAnyCapture(lambdaCapture().bind("capture"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6149) {
    const StringRef Code = R"cpp(
	  void foo() {
	    int t = 5;
	    auto f = [=](){ return t; };
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6149 originates here.");
    using Verifier = VerifyBoundNodeMatch<LambdaExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp([=](){ return t; })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        lambdaExpr(hasAnyCapture(lambdaCapture())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_6149 originates here.");
    using Verifier = VerifyBoundNodeMatch<LambdaExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp([=](){ return t; })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        lambdaExpr(hasAnyCapture(lambdaCapture(capturesVar(hasName("t"))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6176) {
    const StringRef Code = R"cpp(
	  void foo() {
	    int x;
	    auto f = [x](){};
	    auto g = [x = 1](){};
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6176 originates here.");
    using Verifier = VerifyBoundNodeMatch<LambdaExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp([x](){})cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp([x = 1](){})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        lambdaExpr(hasAnyCapture(
    lambdaCapture(capturesVar(hasName("x"))).bind("capture"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6201) {
    const StringRef Code = R"cpp(
	class C {
	  int cc;
	  int f() {
	    auto l = [this]() { return cc; };
	    return l();
	  }
	};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6201 originates here.");
    using Verifier = VerifyBoundNodeMatch<LambdaExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp([this]() { return cc; })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        lambdaExpr(hasAnyCapture(lambdaCapture(capturesThis()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6225) {
    const StringRef Code = R"cpp(
	void foo() {
	  struct Foo {
	    double x;
	  };
	  auto Val = Foo();
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6225 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXConstructExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(Foo())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxConstructExpr(requiresZeroInitialization()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6245) {
    const StringRef Code = R"cpp(
	  class X { void f(int x) {} };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6245 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMethodDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(f)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMethodDecl(hasParameter(0, hasType(asString("int")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6276) {
    const StringRef Code = R"cpp(
	struct A {
	 int operator-(this A, int);
	 void fun(this A &&self);
	 static int operator()(int);
	 int operator+(int);
	};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(23);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6276 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMethodDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int operator-(this A, int))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(void fun(this A &&self))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMethodDecl(isExplicitObjectMemberFunction()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6298) {
    const StringRef Code = R"cpp(
	  void f(int i);
	  int y;
	  void foo() {
	    f(y);
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_6298 originates here.");
    using Verifier = VerifyBoundNodeMatch<CallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(f(y))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        callExpr(
  forEachArgumentWithParam(
    declRefExpr(to(varDecl(hasName("y")))),
    parmVarDecl(hasType(isInteger()))
)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6361) {
    const StringRef Code = R"cpp(
	  void f(int i);
	  void foo(int y) {
	    f(y);
	    void (*f_ptr)(int) = f;
	    f_ptr(y);
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_6361 originates here.");
    using Verifier = VerifyBoundNodeMatch<CallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(f(y))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(f_ptr(y))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        callExpr(
  forEachArgumentWithParamType(
    declRefExpr(to(varDecl(hasName("y")))),
    qualType(isInteger()).bind("type")
)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6462) {
    const StringRef Code = R"cpp(
	void f(int a, int b, int c) {
	}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_6462 originates here.");
    using Verifier = VerifyBoundNodeMatch<ParmVarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(a)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        parmVarDecl(isAtPosition(0)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_6462 originates here.");
    using Verifier = VerifyBoundNodeMatch<ParmVarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        parmVarDecl(isAtPosition(1)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6489) {
    const StringRef Code = R"cpp(
	  class X { void f(int x, int y, int z) {} };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6489 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMethodDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(f)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMethodDecl(hasAnyParameter(hasName("y"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6531) {
    const StringRef Code = R"cpp(
	  void f(int i) {}
	  void g(int i, int j) {}
	  void h(int i, int j);
	  void j(int i);
	  void k(int x, int y, int z, ...);
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_6531 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(g)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(h)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(parameterCountIs(2)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_6531 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionProtoType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(void (int))cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        functionProtoType(parameterCountIs(1)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_6531 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionProtoType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(void (int, int, int, ...))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionProtoType(parameterCountIs(3)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6557) {
    const StringRef Code = R"cpp(
	  template <typename T, unsigned N, unsigned M>
	  struct Matrix {};
	
	  constexpr unsigned R = 2;
	  Matrix<int, R * 2, R * 4> M;
	
	  template <typename T, typename U>
	  void f(T&& t, U&& u) {}
	
	  void foo() {
	    bool B = false;
	    f(R, B);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6557 originates here.");
    using Verifier = VerifyBoundNodeMatch<TemplateSpecializationType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(Matrix<int, R * 2, R * 4>)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        templateSpecializationType(forEachTemplateArgument(isExpr(expr().bind("t_arg")))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}

    {
    SCOPED_TRACE("Test failure from docs_6557 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(f)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(forEachTemplateArgument(refersToType(qualType().bind("type")))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_6608) {
    const StringRef Code = R"cpp(
	  void nope();
	  [[noreturn]] void a();
	  __attribute__((noreturn)) void b();
	  struct c { [[noreturn]] c(); };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6608 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(a)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(b)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(c)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(isNoReturn()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6624) {
    const StringRef Code = R"cpp(
	  class X { int f() { return 1; } };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6624 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMethodDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(f)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMethodDecl(returns(asString("int"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6638) {
    const StringRef Code = R"cpp(
	  extern "C" void f() {}
	  extern "C" { void g() {} }
	  void h() {}
	  extern "C" int x = 1;
	  extern "C" int y = 2;
	  int z = 3;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6638 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(f)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(g)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(isExternC()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_6638 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(x)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(y)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(isExternC()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6663) {
    const StringRef Code = R"cpp(
	  static void f() {}
	  static int i = 0;
	  extern int j;
	  int k;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_6663 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(f)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(isStaticStorageClass()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_6663 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(i)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(isStaticStorageClass()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6682) {
    const StringRef Code = R"cpp(
	  void Func();
	  void DeletedFunc() = delete;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6682 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(DeletedFunc)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(isDeleted()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6695) {
    const StringRef Code = R"cpp(
	  class A { ~A(); };
	  class B { ~B() = default; };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6695 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(~B() = default)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(isDefaulted()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6708) {
    const StringRef Code = R"cpp(
	  static void f();
	  void g() __attribute__((weak));
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_6708 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(void g() __attribute__((weak)))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(isWeak()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6721) {
    const StringRef Code = R"cpp(
	  void f(int);
	  void g(int) noexcept;
	  void h(int) noexcept(true);
	  void i(int) noexcept(false);
	  void j(int) throw();
	  void k(int) throw(int);
	  void l(int) throw(...);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.Language == Lang_CXX11;
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6721 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(void j(int) throw())cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(void k(int) throw(int))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(void l(int) throw(...))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(hasDynamicExceptionSpec()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_6721 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionProtoType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(void (int) throw())cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(void (int) throw(int))cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(void (int) throw(...))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionProtoType(hasDynamicExceptionSpec()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6759) {
    const StringRef Code = R"cpp(
	  void f(int);
	  void g(int) noexcept;
	  void h(int) noexcept(false);
	  void i(int) throw();
	  void j(int) throw(int);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.Language == Lang_CXX11;
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6759 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(void g(int) noexcept)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(void i(int) throw())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(isNoThrow()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_6759 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionProtoType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(void (int) throw())cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(void (int) noexcept)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionProtoType(isNoThrow()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6803) {
    const StringRef Code = R"cpp(
	  consteval int a();
	  void b() { if consteval {} }
	  void c() { if ! consteval {} }
	  void d() { if ! consteval {} else {} }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(23);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6803 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(a)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(isConsteval()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_6803 originates here.");
    using Verifier = VerifyBoundNodeMatch<IfStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(if consteval {})cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(if ! consteval {})cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(if ! consteval {} else {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        ifStmt(isConsteval()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6825) {
    const StringRef Code = R"cpp(
	  constexpr int foo = 42;
	  constexpr int bar();
	  void baz() { if constexpr(1 > 0) {} }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6825 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(foo)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(isConstexpr()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_6825 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(bar)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(isConstexpr()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_6825 originates here.");
    using Verifier = VerifyBoundNodeMatch<IfStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(if constexpr(1 > 0) {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        ifStmt(isConstexpr()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6846) {
    const StringRef Code = R"cpp(
	  constinit int foo = 42;
	  constinit const char* bar = "bar";
	  int baz = 42;
	  [[clang::require_constant_initialization]] int xyz = 42;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(20);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6846 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(foo)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(bar)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(isConstinit()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6867) {
    const StringRef Code = R"cpp(
	 struct vec { int* begin(); int* end(); };
	 int foobar();
	 vec& get_range();
	 void foo() {
	   if (int i = foobar(); i > 0) {}
	   switch (int i = foobar(); i) {}
	   for (auto& a = get_range(); auto& x : a) {}
	 }
	 void bar() {
	   if (foobar() > 0) {}
	   switch (foobar()) {}
	   for (auto& x : get_range()) {}
	 }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(20);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6867 originates here.");
    using Verifier = VerifyBoundNodeMatch<IfStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(if (int i = foobar(); i > 0) {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        ifStmt(hasInitStatement(anything())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_6867 originates here.");
    using Verifier = VerifyBoundNodeMatch<SwitchStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(switch (int i = foobar(); i) {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        switchStmt(hasInitStatement(anything())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_6867 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXForRangeStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(for (auto& a = get_range(); auto& x : a) {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxForRangeStmt(hasInitStatement(anything())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6905) {
    const StringRef Code = R"cpp(
	void foo() {
	  if (true) {}
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6905 originates here.");
    using Verifier = VerifyBoundNodeMatch<IfStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(if (true) {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        ifStmt(hasCondition(cxxBoolLiteral(equals(true)))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6926) {
    const StringRef Code = R"cpp(
	void foo() {
	  if (false) true; else false;
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6926 originates here.");
    using Verifier = VerifyBoundNodeMatch<IfStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(if (false) true; else false)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        ifStmt(hasThen(cxxBoolLiteral(equals(true)))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6942) {
    const StringRef Code = R"cpp(
	void foo() {
	  if (false) false; else true;
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6942 originates here.");
    using Verifier = VerifyBoundNodeMatch<IfStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(if (false) false; else true)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        ifStmt(hasElse(cxxBoolLiteral(equals(true)))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6960) {
    const StringRef Code = R"cpp(
	  class X { int a; int b; };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6960 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(X)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(
    has(fieldDecl(hasName("a"), hasType(type().bind("t")))),
    has(fieldDecl(hasName("b"), hasType(type(equalsBoundNode("t")))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_6996) {
    const StringRef Code = R"cpp(
	struct A {};
	A* GetAPointer();
	void foo() {
	  if (A* a = GetAPointer()) {}
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_6996 originates here.");
    using Verifier = VerifyBoundNodeMatch<IfStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(if (A* a = GetAPointer()) {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        ifStmt(hasConditionVariableStatement(declStmt())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7017) {
    const StringRef Code = R"cpp(
	  int i[5];
	  void f() { i[1] = 42; }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_7017 originates here.");
    using Verifier = VerifyBoundNodeMatch<ArraySubscriptExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(i[1])cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        arraySubscriptExpr(hasIndex(integerLiteral())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7033) {
    const StringRef Code = R"cpp(
	  int i[5];
	  void f() { i[1] = 42; }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_7033 originates here.");
    using Verifier = VerifyBoundNodeMatch<ArraySubscriptExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(i[1])cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        arraySubscriptExpr(hasBase(implicitCastExpr(
    hasSourceExpression(declRefExpr())))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7053) {
    const StringRef Code = R"cpp(
	void foo() {
	  for (;;) {}
	}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_7053 originates here.");
    using Verifier = VerifyBoundNodeMatch<ForStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(for (;;) {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        forStmt(hasBody(compoundStmt().bind("body"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7063) {
    const StringRef Code = R"cpp(
	  void f();
	  void f() {}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_7063 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(f)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(hasBody(compoundStmt().bind("compound"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7090) {
    const StringRef Code = R"cpp(
	  void f();
	  void f() {}
	  void g();
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_7090 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(f)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(f)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(hasAnyBody(compoundStmt())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_7090 originates here.");
    using Verifier = VerifyBoundNodeMatch<CompoundStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp({})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        compoundStmt().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7111) {
    const StringRef Code = R"cpp(
	void foo() { { {}; 1+2; } }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_7111 originates here.");
    using Verifier = VerifyBoundNodeMatch<CompoundStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp({ {}; 1+2; })cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp({ { {}; 1+2; } })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        compoundStmt(hasAnySubstatement(compoundStmt().bind("compound"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7133) {
    const StringRef Code = R"cpp(
	void foo() {
	  { for (;;) {} }
	}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_7133 originates here.");
    using Verifier = VerifyBoundNodeMatch<CompoundStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp({})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        compoundStmt(statementCountIs(0)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7148) {
    const StringRef Code = R"cpp(
	void f(char, bool, double, int);
	void foo() {
	  f('\0', false, 3.14, 42);
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7148 originates here.");
    using Verifier = VerifyBoundNodeMatch<CharacterLiteral>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp('\0')cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        characterLiteral(equals(0U)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_7148 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXBoolLiteralExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(false)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxBoolLiteral(equals(false)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_7148 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXBoolLiteralExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(false)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxBoolLiteral(equals(0)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_7148 originates here.");
    using Verifier = VerifyBoundNodeMatch<FloatingLiteral>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(3.14)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        floatLiteral(equals(3.14)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_7148 originates here.");
    using Verifier = VerifyBoundNodeMatch<IntegerLiteral>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(42)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        integerLiteral(equals(42)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7166) {
    const StringRef Code = R"cpp(
	  int val = -1;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_7166 originates here.");
    using Verifier = VerifyBoundNodeMatch<UnaryOperator>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(-1)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        unaryOperator(hasOperatorName("-"),
              hasUnaryOperand(integerLiteral(equals(1)))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7217) {
    const StringRef Code = R"cpp(
	 void foo(bool a, bool b) {
	  !(a || b);
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7217 originates here.");
    using Verifier = VerifyBoundNodeMatch<BinaryOperator>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(a || b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        binaryOperator(hasOperatorName("||")).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7226) {
    const StringRef Code = R"cpp(
	  template <typename... Args>
	  auto sum(Args... args) {
	      return (0 + ... + args);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(17);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7226 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXFoldExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp((0 + ... + args))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxFoldExpr(hasOperatorName("+")).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7265) {
    const StringRef Code = R"cpp(
	void foo(int a, int b) {
	  if (a == b)
	    a += b;
	}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_7265 originates here.");
    using Verifier = VerifyBoundNodeMatch<BinaryOperator>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(a += b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        binaryOperator(isAssignmentOperator()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7274) {
    const StringRef Code = R"cpp(
	  struct S { S& operator=(const S&); };
	  void x() { S s1, s2; s1 = s2; }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7274 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXOperatorCallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(s1 = s2)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxOperatorCallExpr(isAssignmentOperator()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7292) {
    const StringRef Code = R"cpp(
	void foo(int a, int b) {
	  if (a == b)
	    a += b;
	}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_7292 originates here.");
    using Verifier = VerifyBoundNodeMatch<BinaryOperator>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(a == b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        binaryOperator(isComparisonOperator()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7301) {
    const StringRef Code = R"cpp(
	  struct S { bool operator<(const S& other); };
	  void x(S s1, S s2) { bool b1 = s1 < s2; }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7301 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXOperatorCallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(s1 < s2)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxOperatorCallExpr(isComparisonOperator()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7319) {
    const StringRef Code = R"cpp(
	void foo(bool a, bool b) {
	  a || b;
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7319 originates here.");
    using Verifier = VerifyBoundNodeMatch<BinaryOperator>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(a || b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        binaryOperator(hasLHS(expr().bind("lhs"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7343) {
    const StringRef Code = R"cpp(
	void foo(bool a, bool b) {
	  a || b;
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7343 originates here.");
    using Verifier = VerifyBoundNodeMatch<BinaryOperator>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(a || b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        binaryOperator(hasRHS(expr().bind("rhs"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7381) {
    const StringRef Code = R"cpp(
	void foo() {
	  1 + 2; // Match
	  2 + 1; // Match
	  1 + 1; // No match
	  2 + 2; // No match
	}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_7381 originates here.");
    using Verifier = VerifyBoundNodeMatch<BinaryOperator>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(1 + 2)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(2 + 1)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        binaryOperator(hasOperands(integerLiteral(equals(1)),
                                            integerLiteral(equals(2)))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7407) {
    const StringRef Code = R"cpp(
	void foo() {
	  !true;
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7407 originates here.");
    using Verifier = VerifyBoundNodeMatch<UnaryOperator>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(!true)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        unaryOperator(hasUnaryOperand(cxxBoolLiteral(equals(true)))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7429) {
    const StringRef Code = R"cpp(
	 struct URL { URL(const char*); };
	 URL url = "a string";
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7429 originates here.");
    using Verifier = VerifyBoundNodeMatch<CastExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp("a string")cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        castExpr(hasSourceExpression(cxxConstructExpr())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7437) {
    const StringRef Code = R"cpp(
	void foo(bool b) {
	  int a = b ?: 1;
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7437 originates here.");
    using Verifier = VerifyBoundNodeMatch<OpaqueValueExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(b)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        opaqueValueExpr(hasSourceExpression(
              implicitCastExpr(has(
                implicitCastExpr(has(declRefExpr())))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7462) {
    const StringRef Code = R"cpp(
	  int *p = 0;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_7462 originates here.");
    using Verifier = VerifyBoundNodeMatch<CastExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(0)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        castExpr(hasCastKind(CK_NullToPointer)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7494) {
    const StringRef Code = R"cpp(
	  struct S {};
	  class C {};
	  union U {};
	  enum E { Ok };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7494 originates here.");
    using Verifier = VerifyBoundNodeMatch<TagDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(struct S)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        tagDecl(isStruct()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7511) {
    const StringRef Code = R"cpp(
	  struct S {};
	  class C {};
	  union U {};
	  enum E { Ok };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7511 originates here.");
    using Verifier = VerifyBoundNodeMatch<TagDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(union U)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        tagDecl(isUnion()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7528) {
    const StringRef Code = R"cpp(
	  struct S {};
	  class C {};
	  union U {};
	  enum E { Ok };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7528 originates here.");
    using Verifier = VerifyBoundNodeMatch<TagDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(class C)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        tagDecl(isClass()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7545) {
    const StringRef Code = R"cpp(
	  struct S {};
	  class C {};
	  union U {};
	  enum E { Ok };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7545 originates here.");
    using Verifier = VerifyBoundNodeMatch<TagDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(enum E { Ok })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        tagDecl(isEnum()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7562) {
    const StringRef Code = R"cpp(
	  void foo(bool condition, int a, int b) {
	    condition ? a : b;
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7562 originates here.");
    using Verifier = VerifyBoundNodeMatch<ConditionalOperator>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(condition ? a : b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        conditionalOperator(hasTrueExpression(expr().bind("true"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7574) {
    const StringRef Code = R"cpp(
	  void foo(bool condition, int a, int b) {
	    condition ?: b;
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7574 originates here.");
    using Verifier = VerifyBoundNodeMatch<BinaryConditionalOperator>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(condition ?: b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        binaryConditionalOperator(hasTrueExpression(expr())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7595) {
    const StringRef Code = R"cpp(
	  void foo(bool condition, int a, int b) {
	    condition ? a : b;
	    condition ?: b;
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXX());
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7595 originates here.");
    using Verifier = VerifyBoundNodeMatch<ConditionalOperator>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(condition ? a : b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        conditionalOperator(hasFalseExpression(expr().bind("false"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_7595 originates here.");
    using Verifier = VerifyBoundNodeMatch<BinaryConditionalOperator>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(condition ?: b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        binaryConditionalOperator(hasFalseExpression(expr().bind("false"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7620) {
    const StringRef Code = R"cpp(
	  class A {};
	  class B;  // Doesn't match, as it has no body.
	  int va;
	  extern int vb;  // Doesn't match, as it doesn't define the variable.
	  void fa() {}
	  void fb();  // Doesn't match, as it has no body.
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7620 originates here.");
    using Verifier = VerifyBoundNodeMatch<TagDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(A)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        tagDecl(isDefinition()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_7620 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(va)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(isDefinition()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_7620 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(fa)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(isDefinition()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7661) {
    const StringRef Code = R"cpp(
	  void f(...);
	  void g(int);
	  template <typename... Ts> void h(Ts...);
	  void i();
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7661 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(void f(...))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(isVariadic()).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_7683) {
    const StringRef Code = R"cpp(
	  class A {
	   public:
	    A();
	    void foo();
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7683 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMethodDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(A())cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(void foo())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMethodDecl(ofClass(hasName("A"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7706) {
    const StringRef Code = R"cpp(
	  class A { virtual void f(); };
	  class B : public A { void f(); };
	  class C : public B { void f(); };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7706 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMethodDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(void f())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMethodDecl(ofClass(hasName("C")),
              forEachOverridden(cxxMethodDecl().bind("b"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7720) {
    const StringRef Code = R"cpp(
	  class A1 { virtual void f(); };
	  class A2 { virtual void f(); };
	  class C : public A1, public A2 { void f(); };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7720 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMethodDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(void f())cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(void f())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMethodDecl(ofClass(hasName("C")),
              forEachOverridden(cxxMethodDecl().bind("b"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7755) {
    const StringRef Code = R"cpp(
	  class A {
	   public:
	    virtual void x(); // matches x
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7755 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMethodDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(x)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMethodDecl(isVirtual()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7765) {
    const StringRef Code = R"cpp(
	  struct Base {};
	  struct DirectlyDerived : virtual Base {}; // matches Base
	  struct IndirectlyDerived : DirectlyDerived, Base {}; // matches Base
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7765 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(DirectlyDerived)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasDirectBase(cxxBaseSpecifier(isVirtual()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7786) {
    const StringRef Code = R"cpp(
	  class A {
	   public:
	    virtual void x();
	  };
	  class B : public A {
	   public:
	    void x();
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7786 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMethodDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(virtual void x())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMethodDecl(isVirtualAsWritten()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7811) {
    const StringRef Code = R"cpp(
	  class A final {};
	
	  struct B {
	    virtual void f();
	  };
	
	  struct C : B {
	    void f() final;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7811 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(A)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(isFinal()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_7811 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMethodDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(void f() final)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMethodDecl(isFinal()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7838) {
    const StringRef Code = R"cpp(
	  class A {
	   public:
	    virtual void x() = 0;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7838 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMethodDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(virtual void x() = 0)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMethodDecl(isPure()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7852) {
    const StringRef Code = R"cpp(
	struct A {
	  void foo() const;
	  void bar();
	};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7852 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMethodDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(foo)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMethodDecl(isConst()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7868) {
    const StringRef Code = R"cpp(
	struct A {
	  A &operator=(const A &);
	  A &operator=(A &&);
	};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7868 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMethodDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(A &operator=(const A &))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMethodDecl(isCopyAssignmentOperator()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7887) {
    const StringRef Code = R"cpp(
	struct A {
	  A &operator=(const A &);
	  A &operator=(A &&);
	};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7887 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMethodDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(A &operator=(A &&))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMethodDecl(isMoveAssignmentOperator()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7905) {
    const StringRef Code = R"cpp(
	  class A {
	   public:
	    virtual void x();
	  };
	  class B : public A {
	   public:
	    void x() override;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7905 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXMethodDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(void x() override)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxMethodDecl(isOverride()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7925) {
    const StringRef Code = R"cpp(
	  struct S {
	    S(); // #1
	    S(const S &) = default; // #2
	    S(S &&) = delete; // #3
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7925 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXConstructorDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(S())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxConstructorDecl(isUserProvided()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7944) {
    const StringRef Code = R"cpp(
	  class Y {
	    void x() { this->x(); x(); Y y; y.x(); a; this->b; Y::b; }
	    template <class T> void f() { this->f<T>(); f<T>(); }
	    int a;
	    static int b;
	  };
	  template <class T>
	  class Z {
	    void x() {
	      this->m;
	      this->t;
	      this->t->m;
	    }
	    int m;
	    T* t;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_7944 originates here.");
    using Verifier = VerifyBoundNodeMatch<MemberExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(this->x)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(x)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(a)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(this->b)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(this->m)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(this->t)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        memberExpr(isArrow()).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}

    {
    SCOPED_TRACE("Test failure from docs_7944 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXDependentScopeMemberExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(this->t->m)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxDependentScopeMemberExpr(isArrow()).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}

    {
    SCOPED_TRACE("Test failure from docs_7944 originates here.");
    using Verifier = VerifyBoundNodeMatch<UnresolvedMemberExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(this->f<T>)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(f<T>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        unresolvedMemberExpr(isArrow()).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_7982) {
    const StringRef Code = R"cpp(
	  void a(int);
	  void b(long);
	  void c(double);
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_7982 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(a)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(hasAnyParameter(hasType(isInteger()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_7994) {
    const StringRef Code = R"cpp(
	  void a(int);
	  void b(unsigned long);
	  void c(double);
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_7994 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(hasAnyParameter(hasType(isUnsignedInteger()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8010) {
    const StringRef Code = R"cpp(
	  void a(int);
	  void b(unsigned long);
	  void c(double);
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8010 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(a)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(hasAnyParameter(hasType(isSignedInteger()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8024) {
    const StringRef Code = R"cpp(
	  void a(char);
	  void b(wchar_t);
	  void c(double);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8024 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(a)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(hasAnyParameter(hasType(isAnyCharacter()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8059) {
    const StringRef Code = R"cpp(
	  void a(int);
	  void b(int const);
	  void c(const int);
	  void d(const int*);
	  void e(int const) {};
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8059 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(b)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(c)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(e)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(hasAnyParameter(hasType(isConstQualified()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8078) {
    const StringRef Code = R"cpp(
	  void a(int);
	  void b(int volatile);
	  void c(volatile int);
	  void d(volatile int*);
	  void e(int volatile) {};
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8078 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(b)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(c)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(e)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(hasAnyParameter(hasType(isVolatileQualified()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8099) {
    const StringRef Code = R"cpp(
	  typedef const int const_int;
	  const_int i = 0;
	  int *const j = nullptr;
	  int *volatile k;
	  int m;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8099 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(j)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(k)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasType(hasLocalQualifiers())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8117) {
    const StringRef Code = R"cpp(
	  struct { int first = 0, second = 1; } first, second;
	  int i = second.first;
	  int j = first.second;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8117 originates here.");
    using Verifier = VerifyBoundNodeMatch<MemberExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(second.first)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        memberExpr(member(hasName("first"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8136) {
    const StringRef Code = R"cpp(
	  struct X {
	    int m;
	    int f(X x) { x.m; return m; }
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8136 originates here.");
    using Verifier = VerifyBoundNodeMatch<MemberExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(x.m)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        memberExpr(hasObjectExpression(hasType(cxxRecordDecl(hasName("X"))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_8136 originates here.");
    using Verifier = VerifyBoundNodeMatch<MemberExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(m)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        memberExpr(hasObjectExpression(hasType(pointsTo(
cxxRecordDecl(hasName("X")))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8167) {
    const StringRef Code = R"cpp(
	  namespace X { void b(); }
	  using X::b;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8167 originates here.");
    using Verifier = VerifyBoundNodeMatch<UsingDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(using X::b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        usingDecl(hasAnyUsingShadowDecl(hasName("b"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8185) {
    const StringRef Code = R"cpp(
	  namespace X { int a; void b(); }
	  using X::a;
	  using X::b;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8185 originates here.");
    using Verifier = VerifyBoundNodeMatch<UsingDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(using X::b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        usingDecl(hasAnyUsingShadowDecl(hasTargetDecl(functionDecl()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8204) {
    const StringRef Code = R"cpp(
	  template <typename T> class X {};
	  class A {};
	  X<A> x;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8204 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(class X<class A>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasName("::X"),
isTemplateInstantiation()).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_8212) {
    const StringRef Code = R"cpp(
	  template <typename T> class X {};
	  class A {};
	  template class X<A>;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8212 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(template class X<A>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasName("::X"),
isTemplateInstantiation()).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_8220) {
    const StringRef Code = R"cpp(
	  template <typename T> class X {};
	  class A {};
	  extern template class X<A>;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8220 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(extern template class X<A>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasName("::X"),
isTemplateInstantiation()).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_8230) {
    const StringRef Code = R"cpp(
	  template <typename T>  class X {};
	  class A {};
	  template <> class X<A> {};
	  X<A> x;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8230 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;


    EXPECT_TRUE(notMatches(
        Code,
        cxxRecordDecl(hasName("::X"),
isTemplateInstantiation()).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_8259) {
    const StringRef Code = R"cpp(
	  template<typename T> void A(T t) { T i; }
	  void foo() {
	    A(0);
	    A(0U);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8259 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(void A(T t) { T i; })cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(isInstantiated()).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_8279) {
    const StringRef Code = R"cpp(
	  int j;
	  template<typename T> void A(T t) { T i; }
	  void foo() {
	    A(0);
	    A(0U);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8279 originates here.");
    using Verifier = VerifyBoundNodeMatch<DeclStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(T i;)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        declStmt(isInTemplateInstantiation()).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}

    {
    SCOPED_TRACE("Test failure from docs_8279 originates here.");
    using Verifier = VerifyBoundNodeMatch<DeclStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(T i;)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        declStmt(unless(isInTemplateInstantiation())).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_8304) {
    const StringRef Code = R"cpp(
	  template<typename T> void A(T t) { }
	  template<> void A(int N) { }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8304 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(template<> void A(int N) { })cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(isExplicitTemplateSpecialization()).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_8330) {
    const StringRef Code = R"cpp(
	  const int x = 0;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8330 originates here.");
    using Verifier = VerifyBoundNodeMatch<QualifiedTypeLoc>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        qualifiedTypeLoc().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8345) {
    const StringRef Code = R"cpp(
	  int* const x = nullptr;
	  const int y = 0;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXXOrLater(11));
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8345 originates here.");
    using Verifier = VerifyBoundNodeMatch<QualifiedTypeLoc>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int*)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        qualifiedTypeLoc(hasUnqualifiedLoc(pointerTypeLoc())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8362) {
    const StringRef Code = R"cpp(
	  int f() { return 5; }
	  void g() {}
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8362 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(f)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(hasReturnTypeLoc(loc(asString("int")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8378) {
    const StringRef Code = R"cpp(
	  int* x;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8378 originates here.");
    using Verifier = VerifyBoundNodeMatch<PointerTypeLoc>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int*)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        pointerTypeLoc().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8390) {
    const StringRef Code = R"cpp(
	  int* x;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8390 originates here.");
    using Verifier = VerifyBoundNodeMatch<PointerTypeLoc>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int*)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        pointerTypeLoc(hasPointeeLoc(loc(asString("int")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8403) {
    const StringRef Code = R"cpp(
	  int x = 3;
	  int& l = x;
	  int&& r = 3;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8403 originates here.");
    using Verifier = VerifyBoundNodeMatch<ReferenceTypeLoc>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int&)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(int&&)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        referenceTypeLoc().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8419) {
    const StringRef Code = R"cpp(
	  int x = 3;
	  int& xx = x;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8419 originates here.");
    using Verifier = VerifyBoundNodeMatch<ReferenceTypeLoc>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int&)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        referenceTypeLoc(hasReferentLoc(loc(asString("int")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8435) {
    const StringRef Code = R"cpp(
	  template <typename T> class C {};
	  C<char> var;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8435 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(C<char> var)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasTypeLoc(elaboratedTypeLoc(hasNamedTypeLoc(
templateSpecializationTypeLoc(typeLoc()))))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_8454) {
    const StringRef Code = R"cpp(
	  template<typename T> class A {};
	  A<int> a;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8454 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(A<int> a)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasTypeLoc(elaboratedTypeLoc(hasNamedTypeLoc(
templateSpecializationTypeLoc(hasAnyTemplateArgumentLoc(
hasTypeLoc(loc(asString("int"))))))))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_8480) {
    const StringRef Code = R"cpp(
	  template<typename T, typename U> class A {};
	  A<double, int> b;
	  A<int, double> c;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8480 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(A<double, int> b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasTypeLoc(elaboratedTypeLoc(hasNamedTypeLoc(
templateSpecializationTypeLoc(hasTemplateArgumentLoc(0,
hasTypeLoc(loc(asString("double"))))))))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_8505) {
    const StringRef Code = R"cpp(
	  struct s {};
	  struct s ss;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8505 originates here.");
    using Verifier = VerifyBoundNodeMatch<ElaboratedTypeLoc>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(struct s)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        elaboratedTypeLoc().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8518) {
    const StringRef Code = R"cpp(
	  template <typename T>
	  class C {};
	  class C<int> c;
	
	  class D {};
	  class D d;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8518 originates here.");
    using Verifier = VerifyBoundNodeMatch<ElaboratedTypeLoc>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(class C<int>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        elaboratedTypeLoc(hasNamedTypeLoc(templateSpecializationTypeLoc())).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_8538) {
    const StringRef Code = R"cpp(
	 struct S { bool func(); };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8538 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(func)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(returns(booleanType())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8549) {
    const StringRef Code = R"cpp(
	 struct S { void func(); };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8549 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(func)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(returns(voidType())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8564) {
    const StringRef Code = R"cpp(
	  enum E { Ok };
	  enum E e;
	  int b;
	  float c;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8564 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int b)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(float c)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasType(builtinType())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8577) {
    const StringRef Code = R"cpp(
	  int a[] = { 2, 3 };
	  int b[4];
	  void f() { int c[a[0]]; }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8577 originates here.");
    using Verifier = VerifyBoundNodeMatch<ArrayType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(int[4])cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(int[a[0]])cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(int[])cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        arrayType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8590) {
    const StringRef Code = R"cpp(
	  _Complex float f;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8590 originates here.");
    using Verifier = VerifyBoundNodeMatch<ComplexType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(_Complex float)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        complexType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8600) {
    const StringRef Code = R"cpp(
	  int i;
	  float f;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8600 originates here.");
    using Verifier = VerifyBoundNodeMatch<Type>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(float)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        type(realFloatingPointType()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8613) {
    const StringRef Code = R"cpp(
	  struct A {};
	  A a[7];
	  int b[7];
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8613 originates here.");
    using Verifier = VerifyBoundNodeMatch<ArrayType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(int[7])cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        arrayType(hasElementType(builtinType())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8631) {
    const StringRef Code = R"cpp(
	  void foo() {
	    int a[2];
	    int b[] = { 2, 3 };
	    int c[b[0]];
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8631 originates here.");
    using Verifier = VerifyBoundNodeMatch<ConstantArrayType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(int[2])cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        constantArrayType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8645) {
    const StringRef Code = R"cpp(
	  int a[42];
	  int b[2 * 21];
	  int c[41], d[43];
	  char *s = "abcd";
	  wchar_t *ws = L"abcd";
	  char *w = "a";
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8645 originates here.");
    using Verifier = VerifyBoundNodeMatch<ConstantArrayType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(int[42])cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        constantArrayType(hasSize(42)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_8645 originates here.");
    using Verifier = VerifyBoundNodeMatch<StringLiteral>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp("abcd")cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(L"abcd")cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        stringLiteral(hasSize(4)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8668) {
    const StringRef Code = R"cpp(
	  template<typename T, int Size>
	  class array {
	    T data[Size];
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8668 originates here.");
    using Verifier = VerifyBoundNodeMatch<DependentSizedArrayType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(T[Size])cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        dependentSizedArrayType().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_8683) {
    const StringRef Code = R"cpp(
	  template<typename T, int Size>
	  class vector {
	    typedef T __attribute__((ext_vector_type(Size))) type;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8683 originates here.");
    using Verifier = VerifyBoundNodeMatch<DependentSizedExtVectorType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(T __attribute__((ext_vector_type(Size))))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        dependentSizedExtVectorType().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_8698) {
    const StringRef Code = R"cpp(
	  int a[] = { 2, 3 };
	  int b[42];
	  void f(int c[]) { int d[a[0]]; };
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8698 originates here.");
    using Verifier = VerifyBoundNodeMatch<IncompleteArrayType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(int[])cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(int[])cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        incompleteArrayType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8711) {
    const StringRef Code = R"cpp(
	  void f() {
	    int a[] = { 2, 3 };
	    int b[42];
	    int c[a[0]];
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8711 originates here.");
    using Verifier = VerifyBoundNodeMatch<VariableArrayType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(int[a[0]])cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        variableArrayType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8726) {
    const StringRef Code = R"cpp(
	  void f(int b) {
	    int a[b];
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8726 originates here.");
    using Verifier = VerifyBoundNodeMatch<VariableArrayType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(int[b])cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        variableArrayType(hasSizeExpr(ignoringImpCasts(declRefExpr(to(
  varDecl(hasName("b"))))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8743) {
    const StringRef Code = R"cpp(
	  _Atomic(int) i;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8743 originates here.");
    using Verifier = VerifyBoundNodeMatch<AtomicType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(_Atomic(int))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        atomicType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8753) {
    const StringRef Code = R"cpp(
	  _Atomic(int) i;
	  _Atomic(float) f;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8753 originates here.");
    using Verifier = VerifyBoundNodeMatch<AtomicType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(_Atomic(int))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        atomicType(hasValueType(isInteger())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8767) {
    const StringRef Code = R"cpp(
	  void foo() {
	    auto n = 4;
	    int v[] = { 2, 3 };
	    for (auto i : v) { };
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8767 originates here.");
    using Verifier = VerifyBoundNodeMatch<AutoType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(auto)cpp", 5);

    EXPECT_TRUE(matches(
        Code,
        autoType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8784) {
    const StringRef Code = R"cpp(
	  short i = 1;
	  int j = 42;
	  decltype(i + j) result = i + j;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8784 originates here.");
    using Verifier = VerifyBoundNodeMatch<DecltypeType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(decltype(i + j))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        decltypeType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8800) {
    const StringRef Code = R"cpp(
	  auto a = 1;
	  auto b = 2.0;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8800 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(auto a = 1)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasType(autoType(hasDeducedType(isInteger())))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8816) {
    const StringRef Code = R"cpp(
	  decltype(1) a = 1;
	  decltype(2.0) b = 2.0;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8816 originates here.");
    using Verifier = VerifyBoundNodeMatch<DecltypeType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(decltype(1))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        decltypeType(hasUnderlyingType(isInteger())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8834) {
    const StringRef Code = R"cpp(
	  int (*f)(int);
	  void g();
	)cpp";

	const TestClangConfig& Conf = GetParam();

    {
    SCOPED_TRACE("Test failure from docs_8834 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(int (int))cpp", 1);
	if ((Conf.isCOrLater(23)) || (Conf.isCXX()))
		Matches.emplace_back(MatchKind::TypeStr, R"cpp(void (void))cpp", 1);
	if (Conf.isCOrEarlier(17))
		Matches.emplace_back(MatchKind::TypeStr, R"cpp(void ())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8849) {
    const StringRef Code = R"cpp(
	  int (*f)(int);
	  void g();
	)cpp";

	const TestClangConfig& Conf = GetParam();

    {
    SCOPED_TRACE("Test failure from docs_8849 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionProtoType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(int (int))cpp", 1);
	if ((Conf.isCOrLater(23)) || (Conf.isCXX()))
		Matches.emplace_back(MatchKind::TypeStr, R"cpp(void (void))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionProtoType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8863) {
    const StringRef Code = R"cpp(
	  int (*ptr_to_array)[4];
	  int *array_of_ptrs[4];
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8863 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(ptr_to_array)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasType(pointsTo(parenType()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8876) {
    const StringRef Code = R"cpp(
	  int (*ptr_to_array)[4];
	  int (*ptr_to_func)(int);
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_8876 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(ptr_to_func)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasType(pointsTo(parenType(innerType(functionType()))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8898) {
    const StringRef Code = R"cpp(
	  struct A { int i; };
	  int A::* ptr = &A::i;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8898 originates here.");
    using Verifier = VerifyBoundNodeMatch<MemberPointerType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(int struct A::*)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        memberPointerType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8911) {
    const StringRef Code = R"cpp(
	  typedef int* int_ptr;
	  void foo(char *str,
	           int val,
	           int *val_ptr,
	           int_ptr not_a_ptr,
	           int_ptr *ptr);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8911 originates here.");
    using Verifier = VerifyBoundNodeMatch<ParmVarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(char *str)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(int *val_ptr)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(int_ptr *ptr)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        parmVarDecl(hasType(pointerType())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8952) {
    const StringRef Code = R"cpp(
	  int *a;
	  int &b = *a;
	  int &&c = 1;
	  auto &d = b;
	  auto &&e = c;
	  auto &&f = 2;
	  int g = 5;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8952 originates here.");
    using Verifier = VerifyBoundNodeMatch<ReferenceType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(int &)cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(int &&)cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(auto &)cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(auto &&)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        referenceType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8973) {
    const StringRef Code = R"cpp(
	  int *a;
	  int &b = *a;
	  int &&c = 1;
	  auto &d = b;
	  auto &&e = c;
	  auto &&f = 2;
	  int g = 5;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8973 originates here.");
    using Verifier = VerifyBoundNodeMatch<LValueReferenceType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(int &)cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(auto &)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        lValueReferenceType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_8993) {
    const StringRef Code = R"cpp(
	  int *a;
	  int &b = *a;
	  int &&c = 1;
	  auto &d = b;
	  auto &&e = c;
	  auto &&f = 2;
	  int g = 5;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_8993 originates here.");
    using Verifier = VerifyBoundNodeMatch<RValueReferenceType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(int &&)cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(auto &&)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        rValueReferenceType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9014) {
    const StringRef Code = R"cpp(
	  int *a;
	  const int *b;
	  int * const c = nullptr;
	  const float *f;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = (Conf.isCOrLater(23)) || (Conf.isCXXOrLater(11));
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9014 originates here.");
    using Verifier = VerifyBoundNodeMatch<PointerType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(const int *)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        pointerType(pointee(isConstQualified(), isInteger())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9036) {
    const StringRef Code = R"cpp(
	  typedef int X;
	  X x = 0;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_9036 originates here.");
    using Verifier = VerifyBoundNodeMatch<TypedefType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(X)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        typedefType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9047) {
    const StringRef Code = R"cpp(
	  #define CDECL __attribute__((cdecl))
	  typedef void (CDECL *X)();
	  typedef void (__attribute__((cdecl)) *Y)();
	)cpp";

	const TestClangConfig& Conf = GetParam();

    {
    SCOPED_TRACE("Test failure from docs_9047 originates here.");
    using Verifier = VerifyBoundNodeMatch<MacroQualifiedType>;

    std::vector<Verifier::Match> Matches;
	if ((Conf.isCOrLater(23)) || (Conf.isCXX()))
		Matches.emplace_back(MatchKind::TypeStr, R"cpp(CDECL void (void))cpp", 1);
	if (Conf.isCOrEarlier(17))
		Matches.emplace_back(MatchKind::TypeStr, R"cpp(CDECL void ())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        macroQualifiedType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9063) {
    const StringRef Code = R"cpp(
	  enum C { Green };
	  enum class S { Red };
	
	  C c;
	  S s;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9063 originates here.");
    using Verifier = VerifyBoundNodeMatch<EnumType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(enum C)cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(enum S)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        enumType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9080) {
    const StringRef Code = R"cpp(
	  template <typename T>
	  class C { };
	
	  template class C<int>;
	  C<int> intvar;
	  C<char> charvar;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9080 originates here.");
    using Verifier = VerifyBoundNodeMatch<TemplateSpecializationType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(C<int>)cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(C<char>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        templateSpecializationType().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_9101) {
    const StringRef Code = R"cpp(
	  template <typename T>
	  class C { public: C(T); };
	
	  C c(123);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(17);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9101 originates here.");
    using Verifier = VerifyBoundNodeMatch<DeducedTemplateSpecializationType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(C)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        deducedTemplateSpecializationType().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_9116) {
    const StringRef Code = R"cpp(
	  template <typename T> struct A {
	    typedef __underlying_type(T) type;
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9116 originates here.");
    using Verifier = VerifyBoundNodeMatch<UnaryTransformType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(__underlying_type(T))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        unaryTransformType().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_9129) {
    const StringRef Code = R"cpp(
	  class C {};
	  struct S {};
	
	  C c;
	  S s;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9129 originates here.");
    using Verifier = VerifyBoundNodeMatch<RecordType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(class C)cpp", 3);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(struct S)cpp", 3);

    EXPECT_TRUE(matches(
        Code,
        recordType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9150) {
    const StringRef Code = R"cpp(
	  enum E { Ok };
	  class C {};
	
	  E e;
	  C c;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9150 originates here.");
    using Verifier = VerifyBoundNodeMatch<TagType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(enum E)cpp", 1);
	if (Conf.isCXX())
		Matches.emplace_back(MatchKind::TypeStr, R"cpp(class C)cpp", 3);

    EXPECT_TRUE(matches(
        Code,
        tagType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9170) {
    const StringRef Code = R"cpp(
	  namespace N {
	    namespace M {
	      class D {};
	    }
	  }
	  class C {};
	
	  C c;
	  N::M::D d;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9170 originates here.");
    using Verifier = VerifyBoundNodeMatch<ElaboratedType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(C)cpp", 3);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(N::M::D)cpp", 1);
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(D)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        elaboratedType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9196) {
    const StringRef Code = R"cpp(
	  namespace N {
	    namespace M {
	      class D {};
	    }
	  }
	  N::M::D d;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9196 originates here.");
    using Verifier = VerifyBoundNodeMatch<ElaboratedType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(N::M::D)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        elaboratedType(hasQualifier(hasPrefix(specifiesNamespace(hasName("N"))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9221) {
    const StringRef Code = R"cpp(
	  namespace N {
	    namespace M {
	      enum E { Ok };
	    }
	  }
	  N::M::E e = N::M::Ok;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9221 originates here.");
    using Verifier = VerifyBoundNodeMatch<ElaboratedType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(N::M::E)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        elaboratedType(namesType(enumType())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9241) {
    const StringRef Code = R"cpp(
	  namespace a { struct S {}; }
	  using a::S;
	  S s;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9241 originates here.");
    using Verifier = VerifyBoundNodeMatch<UsingType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(a::S)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        usingType().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9256) {
    const StringRef Code = R"cpp(
	  template <typename T>
	  void F(T t) {
	    T local;
	    int i = 1 + t;
	  }
	  void f() {
	    F(0);
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9256 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(T t)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(T local)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasType(substTemplateTypeParmType())).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_9278) {
    const StringRef Code = R"cpp(
	  template <typename T>
	  double F(T t);
	  int i;
	  double j = F(i);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9278 originates here.");
    using Verifier = VerifyBoundNodeMatch<SubstTemplateTypeParmType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(int)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        substTemplateTypeParmType(hasReplacementType(type())).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_9295) {
    const StringRef Code = R"cpp(
	  template <typename T> void f(int i);
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9295 originates here.");
    using Verifier = VerifyBoundNodeMatch<TemplateTypeParmType>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(T)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        templateTypeParmType().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_9306) {
    const StringRef Code = R"cpp(
	  template <typename T> struct S {
	    void f(S s);
	    void g(S<T> s);
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9306 originates here.");
    using Verifier = VerifyBoundNodeMatch<ParmVarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(S s)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        parmVarDecl(hasType(elaboratedType(namesType(injectedClassNameType())))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_9319) {
    const StringRef Code = R"cpp(
	  void f(int i[]) {
	    i[1] = 0;
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_9319 originates here.");
    using Verifier = VerifyBoundNodeMatch<ValueDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int i[])cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        valueDecl(hasType(decayedType(hasDecayedType(pointerType())))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_9319 originates here.");
    using Verifier = VerifyBoundNodeMatch<Expr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(i)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        expr(hasType(decayedType(hasDecayedType(pointerType())))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9343) {
    const StringRef Code = R"cpp(
	  namespace N {
	    namespace M {
	      class D {};
	    }
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9343 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(D)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasDeclContext(namedDecl(hasName("M")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9364) {
    const StringRef Code = R"cpp(
	  namespace ns {
	    struct A { static void f(); };
	    void A::f() {}
	    void g() { A::f(); }
	  }
	  ns::A a;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9364 originates here.");
    using Verifier = VerifyBoundNodeMatch<NestedNameSpecifier>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(ns)cpp", 1);
	Matches.emplace_back(MatchKind::Name, R"cpp(A)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        nestedNameSpecifier().bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9396) {
    const StringRef Code = R"cpp(
	  struct A { struct B { struct C {}; }; };
	  A::B::C c;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9396 originates here.");
    using Verifier = VerifyBoundNodeMatch<NestedNameSpecifier>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(A)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        nestedNameSpecifier(specifiesType(
  hasDeclaration(cxxRecordDecl(hasName("A")))
)).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9416) {
    const StringRef Code = R"cpp(
	  struct A { struct B { struct C {}; }; };
	  A::B::C c;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9416 originates here.");
    using Verifier = VerifyBoundNodeMatch<NestedNameSpecifierLoc>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(A::)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        nestedNameSpecifierLoc(specifiesTypeLoc(loc(qualType(
  hasDeclaration(cxxRecordDecl(hasName("A"))))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9433) {
    const StringRef Code = R"cpp(
	  struct A { struct B { struct C {}; }; };
	  A::B::C c;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9433 originates here.");
    using Verifier = VerifyBoundNodeMatch<NestedNameSpecifier>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeStr, R"cpp(struct A::B)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        nestedNameSpecifier(hasPrefix(specifiesType(asString(
"struct A")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9453) {
    const StringRef Code = R"cpp(
	  struct A { struct B { struct C {}; }; };
	  A::B::C c;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9453 originates here.");
    using Verifier = VerifyBoundNodeMatch<NestedNameSpecifierLoc>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(A::B::)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        nestedNameSpecifierLoc(hasPrefix(loc(specifiesType(asString(
"struct A"))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9474) {
    const StringRef Code = R"cpp(
	  namespace ns { struct A {}; }
	  ns::A a;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9474 originates here.");
    using Verifier = VerifyBoundNodeMatch<NestedNameSpecifier>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(ns)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        nestedNameSpecifier(specifiesNamespace(hasName("ns"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9495) {
    const StringRef Code = R"cpp(
	  struct [[nodiscard]] Foo{};
	  void bar(int * __attribute__((nonnull)) );
	  __declspec(noinline) void baz();
	
	  #pragma omp declare simd
	  int min();
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_9495 originates here.");
    using Verifier = VerifyBoundNodeMatch<Attr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(nodiscard)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(nonnull)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(noinline)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(declare simd)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        attr().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fdeclspec","-fopenmp" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_9538) {
    const StringRef Code = R"cpp(
	  void foo() {
	    switch (1) { case 1: case 2: default: switch (2) { case 3: case 4: ; } }
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_9538 originates here.");
    using Verifier = VerifyBoundNodeMatch<SwitchStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(switch (1) { case 1: case 2: default: switch (2) { case 3: case 4: ; } })cpp", 2);
	Matches.emplace_back(MatchKind::Code, R"cpp(switch (2) { case 3: case 4: ; })cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        switchStmt(forEachSwitchCase(caseStmt().bind("c"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9576) {
    const StringRef Code = R"cpp(
	  class A { A() : i(42), j(42) {} int i; int j; };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9576 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXConstructorDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(A)cpp", 2);

    EXPECT_TRUE(matches(
        Code,
        cxxConstructorDecl(forEachConstructorInitializer(
  forField(fieldDecl().bind("x")))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9605) {
    const StringRef Code = R"cpp(
	  struct S {
	    S(); // #1
	    S(const S &); // #2
	    S(S &&); // #3
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9605 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXConstructorDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(S(const S &))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxConstructorDecl(isCopyConstructor()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9623) {
    const StringRef Code = R"cpp(
	  struct S {
	    S(); // #1
	    S(const S &); // #2
	    S(S &&); // #3
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9623 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXConstructorDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(S(S &&))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxConstructorDecl(isMoveConstructor()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9641) {
    const StringRef Code = R"cpp(
	  struct S {
	    S(); // #1
	    S(const S &); // #2
	    S(S &&); // #3
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9641 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXConstructorDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(S())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxConstructorDecl(isDefaultConstructor()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9659) {
    const StringRef Code = R"cpp(
	  struct S {
	    S(); // #1
	    S(int) {} // #2
	    S(S &&) : S() {} // #3
	  };
	  S::S() : S(0) {} // #4
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9659 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXConstructorDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(S(S &&) : S() {})cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(S::S() : S(0) {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxConstructorDecl(isDelegatingConstructor()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9680) {
    const StringRef Code = R"cpp(
	  template<bool b>
	  struct S {
	    S(int); // #1
	    explicit S(double); // #2
	    operator int(); // #3
	    explicit operator bool(); // #4
	    explicit(false) S(bool); // # 7
	    explicit(true) S(char); // # 8
	    explicit(b) S(float); // # 9
	  };
	  S(int) -> S<true>; // #5
	  explicit S(double) -> S<false>; // #6
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(20);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9680 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXConstructorDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(explicit S(double))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(explicit(true) S(char))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxConstructorDecl(isExplicit()).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}

    {
    SCOPED_TRACE("Test failure from docs_9680 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXConversionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(explicit operator bool())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxConversionDecl(isExplicit()).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}

    {
    SCOPED_TRACE("Test failure from docs_9680 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXDeductionGuideDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(explicit S(double) -> S<false>)cpp", 1);
	Matches.emplace_back(MatchKind::TypeOfStr, R"cpp(auto (double) -> S<b>)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(explicit(true) S(char))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxDeductionGuideDecl(isExplicit()).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_9719) {
    const StringRef Code = R"cpp(
	  template<bool b>
	  struct S {
	    S(int); // #1
	    explicit S(double); // #2
	    operator int(); // #3
	    explicit operator bool(); // #4
	    explicit(false) S(bool); // # 7
	    explicit(true) S(char); // # 8
	    explicit(b) S(float); // # 9
	  };
	  S(int) -> S<true>; // #5
	  explicit S(double) -> S<false>; // #6
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(20);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9719 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXConstructorDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(explicit(false) S(bool))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(explicit(true) S(char))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxConstructorDecl(hasExplicitSpecifier(constantExpr())).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}

    {
    SCOPED_TRACE("Test failure from docs_9719 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXConversionDecl>;

    std::vector<Verifier::Match> Matches;


    EXPECT_TRUE(notMatches(
        Code,
        cxxConversionDecl(hasExplicitSpecifier(constantExpr())).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}

    {
    SCOPED_TRACE("Test failure from docs_9719 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXDeductionGuideDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::TypeOfStr, R"cpp(auto (float) -> S<b>)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxDeductionGuideDecl(hasExplicitSpecifier(declRefExpr())).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fno-delayed-template-parsing" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_9763) {
    const StringRef Code = R"cpp(
	  inline void f();
	  void g();
	  namespace n {
	  inline namespace m {}
	  }
	  inline int Foo = 5;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9763 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(f)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(isInline()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_9763 originates here.");
    using Verifier = VerifyBoundNodeMatch<NamespaceDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(m)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        namespaceDecl(isInline()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}

    {
    SCOPED_TRACE("Test failure from docs_9763 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(Foo)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(isInline()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9792) {
    const StringRef Code = R"cpp(
	  namespace n {
	  namespace {} // #1
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9792 originates here.");
    using Verifier = VerifyBoundNodeMatch<NamespaceDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(namespace {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        namespaceDecl(isAnonymous()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9805) {
    const StringRef Code = R"cpp(
	  class vector {};
	  namespace foo {
	    class vector {};
	    namespace std {
	      class vector {};
	    }
	  }
	  namespace std {
	    inline namespace __1 {
	      class vector {}; // #1
	      namespace experimental {
	        class vector {};
	      }
	    }
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9805 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(class vector {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasName("vector"), isInStdNamespace()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9830) {
    const StringRef Code = R"cpp(
	  class vector {};
	  namespace foo {
	    class vector {};
	    namespace {
	      class vector {}; // #1
	    }
	  }
	  namespace {
	    class vector {}; // #2
	    namespace foo {
	      class vector {}; // #3
	    }
	  }
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9830 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(vector)cpp", 6);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasName("vector"),
                        isInAnonymousNamespace()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9858) {
    const StringRef Code = R"cpp(
	  void foo() {
	    switch (1) { case 1: break; case 1+1: break; case 3 ... 4: break; }
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_9858 originates here.");
    using Verifier = VerifyBoundNodeMatch<CaseStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(case 1: break)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        caseStmt(hasCaseConstant(constantExpr(has(integerLiteral())))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

#if LLVM_HAS_NVPTX_TARGET
TEST_P(ASTMatchersDocTest, docs_9877) {
    const StringRef Code = R"cpp(
	  __attribute__((device)) void f() {}
	)cpp";

    const StringRef CudaHeader = R"cuda({cuda_header}
    )cuda";

    {
    SCOPED_TRACE("Test failure from docs_9877 originates here.");
    using Verifier = VerifyBoundNodeMatch<Decl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(f)cpp", 1);

    EXPECT_TRUE(matches(
        CudaHeader + Code,
        decl(hasAttr(clang::attr::CUDADevice)).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "--cuda-gpu-arch=sm_70" }));
	}
}
#endif


TEST_P(ASTMatchersDocTest, docs_9897) {
    const StringRef Code = R"cpp(
	  int foo(int a, int b) {
	    return a + b;
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_9897 originates here.");
    using Verifier = VerifyBoundNodeMatch<ReturnStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(return a + b)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        returnStmt(hasReturnValue(binaryOperator().bind("op"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

#if LLVM_HAS_NVPTX_TARGET
TEST_P(ASTMatchersDocTest, docs_9916) {
    const StringRef Code = R"cpp(
	  __global__ void kernel() {}
	  void f() {
	    kernel<<<32,32>>>();
	  }
	)cpp";

    const StringRef CudaHeader = R"cuda({cuda_header}
    )cuda";

    {
    SCOPED_TRACE("Test failure from docs_9916 originates here.");
    using Verifier = VerifyBoundNodeMatch<CUDAKernelCallExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(kernel<<<i, k>>>())cpp", 1);

    EXPECT_TRUE(matches(
        CudaHeader + Code,
        cudaKernelCallExpr().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "--cuda-gpu-arch=sm_70" }));
	}
}
#endif


TEST_P(ASTMatchersDocTest, docs_9932) {
    const StringRef Code = R"cpp(
	  #define NULL 0
	  void *v1 = NULL;
	  void *v2 = nullptr;
	  void *v3 = __null; // GNU extension
	  char *cp = (char *)0;
	  int *ip = 0;
	  int i = 0;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9932 originates here.");
    using Verifier = VerifyBoundNodeMatch<Expr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(NULL)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(nullptr)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(__null)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(0)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(0)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        expr(nullPointerConstant()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9958) {
    const StringRef Code = R"cpp(
	void foo()
	{
	    int arr[3];
	    auto &[f, s, t] = arr;
	
	    f = 42;
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9958 originates here.");
    using Verifier = VerifyBoundNodeMatch<BindingDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Name, R"cpp(f)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        bindingDecl(hasName("f"),
                forDecomposition(decompositionDecl())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_9981) {
    const StringRef Code = R"cpp(
	void foo()
	{
	    int arr[3];
	    auto &[f, s, t] = arr;
	
	    f = 42;
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_9981 originates here.");
    using Verifier = VerifyBoundNodeMatch<DecompositionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(auto &[f, s, t] = arr)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        decompositionDecl(hasBinding(0,
  bindingDecl(hasName("f")).bind("fBinding"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_10004) {
    const StringRef Code = R"cpp(
	void foo()
	{
	    int arr[3];
	    auto &[f, s, t] = arr;
	
	    f = 42;
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_10004 originates here.");
    using Verifier = VerifyBoundNodeMatch<DecompositionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(auto &[f, s, t] = arr)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        decompositionDecl(hasAnyBinding(bindingDecl(hasName("f")).bind("fBinding"))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_10032) {
    const StringRef Code = R"cpp(
	  struct F {
	    F& operator=(const F& other) {
	      []() { return 42 == 42; };
	      return *this;
	    }
	  };
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_10032 originates here.");
    using Verifier = VerifyBoundNodeMatch<ReturnStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(return *this)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        returnStmt(forFunction(hasName("operator="))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_10073) {
    const StringRef Code = R"cpp(
	struct F {
	  F& operator=(const F& other) {
	    []() { return 42 == 42; };
	    return *this;
	  }
	};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_10073 originates here.");
    using Verifier = VerifyBoundNodeMatch<ReturnStmt>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(return *this)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        returnStmt(forFunction(hasName("operator="))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_10134) {
    const StringRef Code = R"cpp(
	void f() {
	  int a;
	  static int b;
	}
	int c;
	static int d;
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_10134 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int c)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasExternalFormalLinkage()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_10146) {
    const StringRef Code = R"cpp(
	  namespace {
	    void f() {}
	  }
	  void g() {}
	  static void h() {}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_10146 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(void g() {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(hasExternalFormalLinkage()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_10166) {
    const StringRef Code = R"cpp(
	  void x(int val) {}
	  void y(int val = 0) {}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_10166 originates here.");
    using Verifier = VerifyBoundNodeMatch<ParmVarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int val = 0)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        parmVarDecl(hasDefaultArgument()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_10178) {
    const StringRef Code = R"cpp(
	  void x(int val = 7) {}
	  void y(int val = 42) {}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_10178 originates here.");
    using Verifier = VerifyBoundNodeMatch<ParmVarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(int val = 42)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        parmVarDecl(hasInitializer(integerLiteral(equals(42)))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_10193) {
    const StringRef Code = R"cpp(
	  struct MyClass { int x; };
	  MyClass *p1 = new MyClass[10];
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_10193 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXNewExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(new MyClass[10])cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxNewExpr(isArray()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_10205) {
    const StringRef Code = R"cpp(
	  void *operator new(decltype(sizeof(void*)), int, void*);
	  struct MyClass { int x; };
	  unsigned char Storage[sizeof(MyClass) * 10];
	  MyClass *p1 = new (16, Storage) MyClass();
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_10205 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXNewExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(new (16, Storage) MyClass())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxNewExpr(hasPlacementArg(0,
                      integerLiteral(equals(16)))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_10225) {
    const StringRef Code = R"cpp(
	  void* operator new(decltype(sizeof(void*)), void*);
	  struct MyClass { int x; };
	  unsigned char Storage[sizeof(MyClass) * 10];
	  MyClass *p1 = new (Storage) MyClass();
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_10225 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXNewExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(new (Storage) MyClass())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxNewExpr(hasAnyPlacementArg(anything())).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_10245) {
    const StringRef Code = R"cpp(
	  void* operator new(decltype(sizeof(void*)));
	  struct MyClass { int x; };
	  MyClass *p1 = new MyClass[10];
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_10245 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXNewExpr>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(new MyClass[10])cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxNewExpr(hasArraySize(
            ignoringImplicit(integerLiteral(equals(10))))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_10264) {
    const StringRef Code = R"cpp(
	class x {};
	class y;
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_10264 originates here.");
    using Verifier = VerifyBoundNodeMatch<CXXRecordDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(class x {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        cxxRecordDecl(hasDefinition()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_10276) {
    const StringRef Code = R"cpp(
	enum X {};
	enum class Y {};
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXX();
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_10276 originates here.");
    using Verifier = VerifyBoundNodeMatch<EnumDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(enum class Y {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        enumDecl(isScoped()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_10288) {
    const StringRef Code = R"cpp(
	int X() {}
	auto Y() -> int {}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_10288 originates here.");
    using Verifier = VerifyBoundNodeMatch<FunctionDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(auto Y() -> int {})cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        functionDecl(hasTrailingReturn()).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_10313) {
    const StringRef Code = R"cpp(
	struct H {};
	H G();
	void f() {
	  H D = G();
	}
	)cpp";

    const TestClangConfig& Conf = GetParam();
    const bool ConfigEnablesCheckingCode = Conf.isCXXOrLater(11);
    if (!ConfigEnablesCheckingCode) return;

    {
    SCOPED_TRACE("Test failure from docs_10313 originates here.");
    using Verifier = VerifyBoundNodeMatch<VarDecl>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(H D = G())cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        varDecl(hasInitializer(ignoringElidableConstructorCall(callExpr()))).bind("match"),
        std::make_unique<Verifier>("match", Matches)));
	}
}

TEST_P(ASTMatchersDocTest, docs_10355) {
    const StringRef Code = R"cpp(
	  void foo() {
	    #pragma omp parallel
	      {}
	    #pragma omp parallel default(none)
	      {
	        #pragma omp taskyield
	      }
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_10355 originates here.");
    using Verifier = VerifyBoundNodeMatch<OMPExecutableDirective>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(#pragma omp parallel)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(#pragma omp parallel default(none))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(#pragma omp taskyield)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        ompExecutableDirective().bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fopenmp" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_10377) {
    const StringRef Code = R"cpp(
	  void foo() {
	    #pragma omp parallel
	    {
	      #pragma omp taskyield
	    }
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_10377 originates here.");
    using Verifier = VerifyBoundNodeMatch<OMPExecutableDirective>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(#pragma omp taskyield)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        ompExecutableDirective(isStandaloneDirective()).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fopenmp" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_10399) {
    const StringRef Code = R"cpp(
	  void foo() {
	    #pragma omp parallel
	    ;
	    #pragma omp parallel
	    {}
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_10399 originates here.");
    using Verifier = VerifyBoundNodeMatch<OMPExecutableDirective>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(#pragma omp parallel)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        ompExecutableDirective(hasStructuredBlock(nullStmt().bind("stmt"))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fopenmp" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_10423) {
    const StringRef Code = R"cpp(
	  void foo() {
	  #pragma omp parallel
	    ;
	  #pragma omp parallel default(none)
	    ;
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_10423 originates here.");
    using Verifier = VerifyBoundNodeMatch<OMPExecutableDirective>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(#pragma omp parallel default(none))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        ompExecutableDirective(hasAnyClause(anything())).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fopenmp" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_10446) {
    const StringRef Code = R"cpp(
	  void foo() {
	    #pragma omp parallel default(none)
	      ;
	    #pragma omp parallel default(shared)
	      ;
	    #pragma omp parallel default(private)
	      ;
	    #pragma omp parallel default(firstprivate)
	      ;
	    #pragma omp parallel
	      ;
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_10446 originates here.");
    using Verifier = VerifyBoundNodeMatch<OMPExecutableDirective>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(#pragma omp parallel default(none))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(#pragma omp parallel default(shared))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(#pragma omp parallel default(private))cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(#pragma omp parallel default(firstprivate))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        ompExecutableDirective(hasAnyClause(ompDefaultClause())).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fopenmp" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_10474) {
    const StringRef Code = R"cpp(
	  void foo() {
	    #pragma omp parallel
	      ;
	    #pragma omp parallel default(none)
	      ;
	    #pragma omp parallel default(shared)
	      ;
	    #pragma omp parallel default(private)
	      ;
	    #pragma omp parallel default(firstprivate)
	      ;
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_10474 originates here.");
    using Verifier = VerifyBoundNodeMatch<OMPExecutableDirective>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(#pragma omp parallel default(none))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        ompExecutableDirective(hasAnyClause(ompDefaultClause(isNoneKind()))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fopenmp" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_10500) {
    const StringRef Code = R"cpp(
	  void foo() {
	    #pragma omp parallel
	      ;
	    #pragma omp parallel default(none)
	      ;
	  #pragma omp parallel default(shared)
	      ;
	  #pragma omp parallel default(private)
	      ;
	  #pragma omp parallel default(firstprivate)
	      ;
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_10500 originates here.");
    using Verifier = VerifyBoundNodeMatch<OMPExecutableDirective>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(#pragma omp parallel default(shared))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        ompExecutableDirective(hasAnyClause(ompDefaultClause(isSharedKind()))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fopenmp" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_10527) {
    const StringRef Code = R"cpp(
	  void foo() {
	    #pragma omp parallel
	      ;
	  #pragma omp parallel default(none)
	      ;
	  #pragma omp parallel default(shared)
	      ;
	  #pragma omp parallel default(private)
	      ;
	  #pragma omp parallel default(firstprivate)
	      ;
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_10527 originates here.");
    using Verifier = VerifyBoundNodeMatch<OMPExecutableDirective>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(#pragma omp parallel default(private))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        ompExecutableDirective(hasAnyClause(ompDefaultClause(isPrivateKind()))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fopenmp" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_10554) {
    const StringRef Code = R"cpp(
	  void foo() {
	    #pragma omp parallel
	      ;
	    #pragma omp parallel default(none)
	      ;
	    #pragma omp parallel default(shared)
	      ;
	    #pragma omp parallel default(private)
	      ;
	    #pragma omp parallel default(firstprivate)
	      ;
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_10554 originates here.");
    using Verifier = VerifyBoundNodeMatch<OMPExecutableDirective>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(#pragma omp parallel default(firstprivate))cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        ompExecutableDirective(hasAnyClause(ompDefaultClause(isFirstPrivateKind()))).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fopenmp" }));
	}
}

TEST_P(ASTMatchersDocTest, docs_10581) {
    const StringRef Code = R"cpp(
	  void foo() {
	    #pragma omp parallel
	      ;
	    #pragma omp parallel for
	      for (int i = 0; i < 10; ++i) {}
	    #pragma omp          for
	      for (int i = 0; i < 10; ++i) {}
	  }
	)cpp";

    {
    SCOPED_TRACE("Test failure from docs_10581 originates here.");
    using Verifier = VerifyBoundNodeMatch<OMPExecutableDirective>;

    std::vector<Verifier::Match> Matches;
	Matches.emplace_back(MatchKind::Code, R"cpp(#pragma omp parallel)cpp", 1);
	Matches.emplace_back(MatchKind::Code, R"cpp(#pragma omp parallel for)cpp", 1);

    EXPECT_TRUE(matches(
        Code,
        ompExecutableDirective(isAllowedToContainClauseKind(
OpenMPClauseKind::OMPC_default)).bind("match"),
        std::make_unique<Verifier>("match", Matches),
		{ "-fopenmp" }));
	}
}
// doc_test_end

static std::vector<TestClangConfig> allTestClangConfigs() {
  std::vector<TestClangConfig> all_configs;
  for (TestLanguage lang : {
#define TESTLANGUAGE(lang, version, std_flag, index) Lang_##lang##version,
#include "clang/Testing/TestLanguage.def"
       }) {
    TestClangConfig config;
    config.Language = lang;

    // Use an unknown-unknown triple so we don't instantiate the full system
    // toolchain.  On Linux, instantiating the toolchain involves stat'ing
    // large portions of /usr/lib, and this slows down not only this test, but
    // all other tests, via contention in the kernel.
    //
    // FIXME: This is a hack to work around the fact that there's no way to do
    // the equivalent of runToolOnCodeWithArgs without instantiating a full
    // Driver.  We should consider having a function, at least for tests, that
    // invokes cc1.
    config.Target = "i386-unknown-unknown";
    all_configs.push_back(config);

    // Windows target is interesting to test because it enables
    // `-fdelayed-template-parsing`.
    config.Target = "x86_64-pc-win32-msvc";
    all_configs.push_back(config);
  }
  return all_configs;
}

INSTANTIATE_TEST_SUITE_P(
    ASTMatchersTests, ASTMatchersDocTest,
    testing::ValuesIn(allTestClangConfigs()),
    [](const testing::TestParamInfo<TestClangConfig> &Info) {
      return Info.param.toShortString();
    });
} // namespace ast_matchers
} // namespace clang
