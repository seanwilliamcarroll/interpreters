//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : IR builder that owns the insertion point and emits instructions.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/basic_block.hpp>
#include <codegen/handle.hpp>

#include "codegen/types.hpp"

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Context;

struct InsertionGuard;

struct IRBuilder {

  explicit IRBuilder(Context &ctx) : m_ctx(ctx) {}

  [[nodiscard]] InsertionGuard at(BasicBlock &);

  LocalHandle add_alloca(const std::string &name, TypeId);

  Context &ctx() { return m_ctx; }
  BasicBlock &insertion_point() { return *m_insertion_point; }

private:
  Context &m_ctx;
  BasicBlock *m_insertion_point = nullptr;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
