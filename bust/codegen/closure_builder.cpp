//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the closure builder.
//*
//*
//****************************************************************************

#include <codegen/closure_builder.hpp>

#include "codegen/handle.hpp"

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
    auto binding = m_ctx.arena().get(capture.m_id);

    auto type_id = m_ctx.to_type(binding.m_type);
    m_captured_bindings.emplace_back(CapturedBinding{
        .m_source_name = binding.m_name,
        .m_outer_handle = m_ctx.symbols().lookup(binding.m_name),
        .m_type_id = type_id,
    });
    struct_types.emplace_back(type_id);
  }
  m_type_id = m_ctx.type().intern_struct(
      std::string{conventions::env_struct_name},
      StructType{.m_fields = std::move(struct_types)});
}

Handle ClosureBuilder::allocate_and_populate_env() {
  auto env_handle = m_ctx.builder().malloc_struct(m_type_id);
  // Emit the code to store captures into env
  for (const auto &[index, capture] :
       std::views::zip(std::views::iota(0ULL), m_captured_bindings)) {
    // If it is alloca, we need to load it before using
    auto stored_location =
        m_ctx.builder().create_load(capture.m_outer_handle, capture.m_type_id);

    m_ctx.builder().store_to_struct(m_type_id, env_handle, index,
                                    Argument{
                                        .m_name = stored_location,
                                        .m_type = capture.m_type_id,
                                    });
  }

  return env_handle;
}

void ClosureBuilder::emit_capture_load_prologue() {
  for (const auto &[index, capture] :
       std::views::zip(std::views::iota(0ULL), m_captured_bindings)) {
    auto capture_temp_handle =
        m_ctx.builder().add_alloca(capture.m_source_name, capture.m_type_id);
    auto value = m_ctx.builder().load_from_struct(
        m_type_id, NamedHandle{m_ctx.env_parameter().m_name}, index,
        capture.m_type_id);
    m_ctx.builder().create_store(
        capture_temp_handle, {.m_name = value, .m_type = capture.m_type_id});
  }
}

Handle ClosureBuilder::package_fat_pointer(GlobalHandle function,
                                           Handle env_handle) {
  const auto &closure_type_id = m_ctx.m_fat_ptr;

  auto closure_handle = m_ctx.builder().malloc_struct(closure_type_id);

  m_ctx.builder().store_to_struct(closure_type_id, closure_handle, 0,
                                  Argument{
                                      .m_name = std::move(function),
                                      .m_type = m_ctx.m_ptr,
                                  });

  m_ctx.builder().store_to_struct(closure_type_id, closure_handle, 1,
                                  Argument{
                                      .m_name = std::move(env_handle),
                                      .m_type = m_ctx.m_ptr,
                                  });
  return closure_handle;
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
