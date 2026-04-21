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
#include <codegen/block_label.hpp>
#include <codegen/function_handle.hpp>
#include <codegen/handle.hpp>

#include <cassert>

#include "codegen/function_declaration.hpp"
#include "codegen/parameter.hpp"
#include "codegen/types.hpp"
#include "operators.hpp"

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Context;

struct IRBuilder {
  struct InsertionGuard {
    InsertionGuard(IRBuilder &, BlockLabel);
    ~InsertionGuard();

    InsertionGuard(const InsertionGuard &) = delete;
    InsertionGuard &operator=(const InsertionGuard &) = delete;
    InsertionGuard(InsertionGuard &&) = delete;
    InsertionGuard &operator=(InsertionGuard &&) = delete;

  private:
    IRBuilder &m_parent;
    BlockLabel m_captured;
  };

  struct FunctionGuard {
    FunctionGuard(IRBuilder &, FunctionHandle);
    ~FunctionGuard();

    FunctionGuard(const FunctionGuard &) = delete;
    FunctionGuard &operator=(const FunctionGuard &) = delete;
    FunctionGuard(FunctionGuard &&) = delete;
    FunctionGuard &operator=(FunctionGuard &&) = delete;

  private:
    IRBuilder &m_parent;
    BlockLabel m_captured_block;
    FunctionHandle m_captured_function;
  };

  explicit IRBuilder(Context &ctx)
      : m_ctx(ctx), m_current_function_handle(nullptr),
        m_current_block_label(nullptr) {}

  [[nodiscard]] LocalHandle add_alloca(const std::string &name, TypeId) const;
  void add_branch(Handle condition, BlockLabel if_true,
                  BlockLabel if_false) const;
  void add_jump(BlockLabel) const;
  [[nodiscard]] Handle create_gep(TypeId struct_type_id, Handle struct_handle,
                                  Argument initial_index,
                                  std::vector<Argument> indices) const;
  [[nodiscard]] Handle create_gep_field(TypeId struct_type_id,
                                        Handle struct_handle,
                                        size_t field_index) const;
  [[nodiscard]] Handle create_ptr_to_int(Handle source,
                                         TypeId destination_type) const;
  [[nodiscard]] Handle create_call(Handle callee,
                                   std::vector<Argument> arguments,
                                   TypeId return_type_id) const;
  void create_call_void(Handle callee, std::vector<Argument> arguments) const;
  void create_store(Handle destination, Argument value) const;
  [[nodiscard]] Handle create_load(Handle source, TypeId type) const;
  [[nodiscard]] Handle create_icmp(Handle lhs, Handle rhs,
                                   LLVMIntegerCompareCondition cond,
                                   TypeId type) const;
  [[nodiscard]] Handle create_binary(Handle lhs, Handle rhs,
                                     LLVMBinaryOperator op, TypeId type) const;
  [[nodiscard]] Handle create_unary(Handle input, UnaryOperator op,
                                    TypeId type) const;
  [[nodiscard]] Handle create_cast(Handle input, LLVMCastOperator op,
                                   TypeId from, TypeId to) const;
  void create_return(Handle value, TypeId type) const;
  void create_return_void() const;

  [[nodiscard]] Handle malloc_struct(TypeId struct_type) const;
  void store_to_struct(TypeId struct_type, Handle base, size_t index,
                       Argument value) const;
  [[nodiscard]] Handle load_from_struct(TypeId struct_type, Handle base,
                                        size_t index, TypeId value_type) const;

  // Outside of IRBuilder, no one needs to know about functions/blocks directly
  // Just pass around these labels and handles
  BlockLabel make_block(const std::string &name);
  void enter_block(BlockLabel);
  [[nodiscard]] InsertionGuard push_block(BlockLabel);
  [[nodiscard]] InsertionGuard push_new_block(const std::string &name);

  FunctionHandle make_function(FunctionDeclaration signature);
  void enter_function(FunctionHandle);
  [[nodiscard]] FunctionGuard push_function(FunctionHandle);
  [[nodiscard]] FunctionGuard push_new_function(FunctionDeclaration signature);

  [[nodiscard]] FunctionHandle current_function_handle() const;
  [[nodiscard]] BlockLabel current_block_label() const;

private:
  [[nodiscard]] BasicBlock &block() const;
  [[nodiscard]] static BlockLabel entry_block_of(FunctionHandle);

  Context &m_ctx;
  FunctionHandle m_current_function_handle;
  BlockLabel m_current_block_label;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
