//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Unit tests for bust::mono::Mangler
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include <hir/type_arena.hpp>
#include <hir/types.hpp>
#include <mono/name_mangler.hpp>

#include <regex>
#include <string>

#include <doctest/doctest.h>

//****************************************************************************
namespace bust {
//****************************************************************************
TEST_SUITE("bust.name_mangler") {

  // Valid C identifier: starts with letter/underscore, followed by
  // alphanumerics/underscores. LLVM globals accept this.
  static const std::regex k_identifier_regex("^[A-Za-z_][A-Za-z0-9_]*$");

  TEST_CASE("mangle is deterministic") {
    hir::TypeArena arena;
    mono::Mangler mangler_a{arena};
    mono::Mangler mangler_b{arena};
    CHECK(mangler_a.mangle("id", hir::BindingId{0}, arena.m_i64) ==
          mangler_b.mangle("id", hir::BindingId{0}, arena.m_i64));
  }

  TEST_CASE("same Mangler reused across calls does not accumulate") {
    // Guards against the stringstream-clear gotcha: calling mangle twice
    // on the same instance should produce fresh output each time.
    hir::TypeArena arena;
    mono::Mangler mangler{arena};
    auto first = mangler.mangle("id", hir::BindingId{0}, arena.m_i64);
    auto second = mangler.mangle("id", hir::BindingId{0}, arena.m_i64);
    CHECK(first == second);
  }

  TEST_CASE("different types yield different names") {
    hir::TypeArena arena;
    mono::Mangler mangler_i64{arena};
    mono::Mangler mangler_bool{arena};
    CHECK(mangler_i64.mangle("id", hir::BindingId{0}, arena.m_i64) !=
          mangler_bool.mangle("id", hir::BindingId{0}, arena.m_bool));
  }

  TEST_CASE("different names yield different mangled names") {
    hir::TypeArena arena;
    mono::Mangler mangler_a{arena};
    mono::Mangler mangler_b{arena};
    CHECK(mangler_a.mangle("id", hir::BindingId{0}, arena.m_i64) !=
          mangler_b.mangle("other", hir::BindingId{0}, arena.m_i64));
  }

  TEST_CASE("different binding ids yield different mangled names") {
    // Same name + same type but different binding sites (e.g. shadowing at
    // top level) must not collide.
    hir::TypeArena arena;
    mono::Mangler mangler_a{arena};
    mono::Mangler mangler_b{arena};
    CHECK(mangler_a.mangle("id", hir::BindingId{0}, arena.m_i64) !=
          mangler_b.mangle("id", hir::BindingId{1}, arena.m_i64));
  }

  TEST_CASE("mangled primitive type is a valid identifier") {
    hir::TypeArena arena;
    mono::Mangler mangler{arena};
    auto name = mangler.mangle("id", hir::BindingId{0}, arena.m_i64);
    CHECK(std::regex_match(name, k_identifier_regex));
  }

  TEST_CASE("mangled function type is a valid identifier") {
    // fn(i64) -> i64
    hir::TypeArena arena;
    auto fn_type_id = arena.intern(hir::FunctionType{
        .m_parameters = {arena.m_i64}, .m_return_type = arena.m_i64});
    mono::Mangler mangler{arena};
    auto name = mangler.mangle("apply", hir::BindingId{0}, fn_type_id);
    CHECK(std::regex_match(name, k_identifier_regex));
  }

  TEST_CASE("nested function types mangle without collision") {
    // fn(i64) -> i64  vs  fn(bool) -> i64 — must differ
    hir::TypeArena arena;
    auto fn_i64_to_i64 = arena.intern(hir::FunctionType{
        .m_parameters = {arena.m_i64}, .m_return_type = arena.m_i64});
    auto fn_bool_to_i64 = arena.intern(hir::FunctionType{
        .m_parameters = {arena.m_bool}, .m_return_type = arena.m_i64});
    mono::Mangler mangler_a{arena};
    mono::Mangler mangler_b{arena};
    CHECK(mangler_a.mangle("apply", hir::BindingId{0}, fn_i64_to_i64) !=
          mangler_b.mangle("apply", hir::BindingId{0}, fn_bool_to_i64));
  }

  TEST_CASE("parameter order matters") {
    // fn(i64, bool) -> i64  vs  fn(bool, i64) -> i64 — must differ
    hir::TypeArena arena;
    auto fn_i64_bool = arena.intern(
        hir::FunctionType{.m_parameters = {arena.m_i64, arena.m_bool},
                          .m_return_type = arena.m_i64});
    auto fn_bool_i64 = arena.intern(
        hir::FunctionType{.m_parameters = {arena.m_bool, arena.m_i64},
                          .m_return_type = arena.m_i64});
    mono::Mangler mangler_a{arena};
    mono::Mangler mangler_b{arena};
    CHECK(mangler_a.mangle("f", hir::BindingId{0}, fn_i64_bool) !=
          mangler_b.mangle("f", hir::BindingId{0}, fn_bool_i64));
  }

} // TEST_SUITE
//****************************************************************************
} // namespace bust
//****************************************************************************
