//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Short-lived helper that emits the closure ABI for a single
//*            lambda with captures. Owns the interned env-struct type and
//*            the resolved list of captured arguments; drives IRBuilder to
//*            allocate the env, emit the body prologue, and package the
//*            fat pointer.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/context.hpp>
#include <codegen/symbol_table.hpp>
#include <codegen/types.hpp>
#include <codegen/value.hpp>
#include <zir/nodes.hpp>

#include <cassert>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct CapturedBinding {
  std::string m_source_name;
  Value m_outer_value;
  TypeId m_internal_type_id;
};

struct ClosureBuilder {
  ClosureBuilder(Context &, const std::vector<zir::IdentifierExpr> &captures);

  Value allocate_and_populate_env();

  void emit_capture_load_prologue();

  Value package_fat_pointer(Value function, Value env_handle);

private:
  Context &m_ctx;
  std::vector<CapturedBinding> m_captured_bindings;
  TypeId m_type_id;
  Value m_env;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
