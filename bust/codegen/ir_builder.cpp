//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the IR builder.
//*
//*
//****************************************************************************

#include <codegen/context.hpp>
#include <codegen/ir_builder.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

LocalHandle IRBuilder::add_alloca(const std::string &name, TypeId type_id) {
  auto output_handle = m_ctx.symbols().define_local(name);

  m_ctx.function().add_alloca_instruction(
      {.m_handle = output_handle, .m_type = type_id});

  return output_handle;
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
