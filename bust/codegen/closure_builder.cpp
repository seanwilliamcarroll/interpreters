//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the closure builder.
//*
//*
//****************************************************************************

#include <codegen/arena.hpp>
#include <codegen/closure_builder.hpp>
#include <codegen/context.hpp>
#include <codegen/ir_builder.hpp>
#include <codegen/naming_conventions.hpp>
#include <codegen/symbol_table.hpp>
#include <codegen/types.hpp>
#include <codegen/value.hpp>
#include <types.hpp>
#include <zir/arena.hpp>
#include <zir/nodes.hpp>

#include <assert.h>

#include <optional>
#include <ranges>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

ClosureBuilder::ClosureBuilder(Context &ctx,
                               const std::vector<zir::IdentifierExpr> &captures)
    : m_ctx(ctx) {
  assert(!captures.empty() && "ClosureBuilder expects captures!");
  m_captured_bindings.reserve(captures.size());
  std::vector<TypeId> struct_types;
  struct_types.reserve(captures.size());
  for (const auto &capture : captures) {
    auto zir_binding = m_ctx.arena().get(capture.m_id);

    auto binding = m_ctx.symbols().lookup(zir_binding.m_name);

    if (!std::holds_alternative<AllocaBinding>(binding)) {
      // Don't capture FunctionBindings
      continue;
    }

    // Assumes AllocaBinding
    const auto &alloca_binding = std::get<AllocaBinding>(binding);
    m_captured_bindings.emplace_back(CapturedBinding{
        .m_source_name = zir_binding.m_name,
        .m_outer_value = alloca_binding.m_ptr,
        .m_internal_type_id = alloca_binding.m_internal_type_id,
    });
    struct_types.emplace_back(alloca_binding.m_internal_type_id);
  }
  m_type_id = m_ctx.type().intern(StructType{
      .m_fields = std::move(struct_types),
      .m_name = std::make_optional(std::string{conventions::env_struct_name}),
  });
}

Value ClosureBuilder::allocate_and_populate_env() {
  auto env = m_ctx.builder().malloc_struct(m_type_id);
  // Emit the code to store captures into env
  for (const auto &[index, capture] :
       std::views::zip(std::views::iota(0ULL), m_captured_bindings)) {
    // If it is alloca, we need to load it before using
    auto stored_value = m_ctx.builder().emit_load(capture.m_outer_value,
                                                  capture.m_internal_type_id);

    m_ctx.builder().store_to_struct(env, m_type_id, index, stored_value);
  }

  return env;
}

void ClosureBuilder::emit_capture_load_prologue() {
  for (const auto &[index, capture] :
       std::views::zip(std::views::iota(0ULL), m_captured_bindings)) {
    auto value =
        m_ctx.builder().load_from_struct(m_ctx.env(), m_type_id, index);

    m_ctx.define_local(capture.m_source_name, capture.m_internal_type_id,
                       value);
  }
}

Value ClosureBuilder::package_fat_pointer(Value function, Value env_handle) {
  const auto &closure_type_id = m_ctx.m_fat_ptr;

  auto closure = m_ctx.builder().malloc_struct(closure_type_id);

  m_ctx.builder().store_to_struct(closure, closure_type_id, 0,
                                  std::move(function));
  m_ctx.builder().store_to_struct(closure, closure_type_id, 1,
                                  std::move(env_handle));

  return closure;
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
