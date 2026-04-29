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
#include <codegen/function_declaration.hpp>
#include <codegen/function_handle.hpp>
#include <codegen/parameter.hpp>
#include <codegen/types.hpp>
#include <codegen/value.hpp>
#include <operators.hpp>

#include <stddef.h>

#include <string>
#include <string_view>
#include <vector>

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

  [[nodiscard]] Value emit_alloca(TypeId);
  [[nodiscard]] Value emit_alloca(TypeId, std::string_view hint);
  void emit_store(Value destination, Value source);
  [[nodiscard]] Value emit_load(Value source, TypeId loaded_type_id);
  [[nodiscard]] Value emit_gep(Value ptr, TypeId aggregate_type_id,
                               Index initial_index, std::vector<Index> indices);
  [[nodiscard]] Value emit_gep_field(Value ptr, TypeId aggregate_type_id,
                                     size_t field_index);
  [[nodiscard]] Value emit_extractvalue(Value source, TypeId aggregate_type_id,
                                        size_t index);
  [[nodiscard]] Value emit_ptr_to_int(Value source, TypeId destination_type);
  [[nodiscard]] Value emit_call(Value callee, std::vector<Value> arguments,
                                TypeId return_type_id);
  void emit_call_void(Value callee, std::vector<Value> arguments);
  void emit_branch(Value condition, BlockLabel if_true, BlockLabel if_false);
  void emit_jump(BlockLabel);
  [[nodiscard]] Value emit_icmp(Value lhs, Value rhs,
                                LLVMIntegerCompareCondition cond);
  [[nodiscard]] Value emit_binary(Value lhs, Value rhs, LLVMBinaryOperator op);
  [[nodiscard]] Value emit_unary(Value source, UnaryOperator op);
  [[nodiscard]] Value emit_cast(Value input, LLVMCastOperator op, TypeId to);
  void emit_return(Value value);
  void emit_return_void();

  [[nodiscard]] Value malloc_struct(TypeId struct_type);
  [[nodiscard]] Value alloca_struct(TypeId struct_type);
  void store_to_struct(Value ptr, TypeId struct_type, size_t index,
                       Value value);
  [[nodiscard]] Value load_from_struct(Value ptr, TypeId struct_type,
                                       size_t index);

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

  Value next_ssa_temporary(TypeId type_id) {
    return {
        .m_handle = TemporaryHandle{m_ssa_count++},
        .m_type_id = type_id,
    };
  }

private:
  [[nodiscard]] BasicBlock &block() const;
  [[nodiscard]] static BlockLabel entry_block_of(FunctionHandle);

  Context &m_ctx;
  FunctionHandle m_current_function_handle;
  BlockLabel m_current_block_label;
  size_t m_ssa_count = 1;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
