//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the IR builder.
//*
//*
//****************************************************************************

#include <codegen/arena.hpp>
#include <codegen/basic_block.hpp>
#include <codegen/block_label.hpp>
#include <codegen/context.hpp>
#include <codegen/function.hpp>
#include <codegen/function_declaration.hpp>
#include <codegen/function_handle.hpp>
#include <codegen/instructions.hpp>
#include <codegen/ir_builder.hpp>
#include <codegen/module.hpp>
#include <codegen/parameter.hpp>
#include <codegen/types.hpp>
#include <codegen/value.hpp>
#include <operators.hpp>
#include <types.hpp>

#include <cassert>
#include <string_view>
#include <utility>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

BasicBlock &IRBuilder::block() const { return *current_block_label().m_block; }

BlockLabel IRBuilder::entry_block_of(FunctionHandle function) {
  return BlockLabel{
      &function.m_function->entry_basic_block(),
  };
}

Value IRBuilder::emit_alloca(TypeId type_id) {
  auto output = next_ssa_temporary(m_ctx.m_ptr);
  current_function_handle().m_function->add_alloca_instruction({
      .m_value_ptr = output,
      .m_type_id = type_id,
  });
  return output;
}

Value IRBuilder::emit_alloca(TypeId type_id, std::string_view hint) {
  auto output = Value{
      .m_handle =
          NamedHandle{
              .m_handle = std::string{hint},
          },
      .m_type_id = m_ctx.m_ptr,
  };
  current_function_handle().m_function->add_alloca_instruction({
      .m_value_ptr = output,
      .m_type_id = type_id,
  });
  return output;
}

void IRBuilder::emit_store(Value destination, Value source) {
  block().add_instruction(StoreInstruction{
      .m_destination = std::move(destination),
      .m_source = std::move(source),
  });
}

Value IRBuilder::emit_load(Value source, TypeId loaded_type_id) {
  auto temp_ssa = next_ssa_temporary(loaded_type_id);
  block().add_instruction(LoadInstruction{
      .m_destination = temp_ssa,
      .m_source = std::move(source),
  });
  return temp_ssa;
}

Value IRBuilder::emit_gep(Value ptr, TypeId aggregate_type_id,
                          Index initial_index, std::vector<Index> indices) {
  // This instruction is giving us a pointer to a specific field in a struct
  // We need to tell it where the struct is (with the ptr) and what its type is
  // Then we index into it based on GEP's syntax
  // initial being an array index, and indices being nested indices from there
  // Common case is array index of 0 with one element in indices with index to
  // struct field position
  auto ptr_to_struct_field = next_ssa_temporary(m_ctx.m_ptr);
  block().add_instruction(GetElementPtrInstruction{
      .m_destination = ptr_to_struct_field,
      .m_aggregate_type_id = aggregate_type_id,
      .m_ptr = std::move(ptr),
      .m_initial_index = initial_index,
      .m_additional_indices = std::move(indices),
  });
  return ptr_to_struct_field;
}

Value IRBuilder::emit_gep_field(Value ptr, TypeId aggregate_type_id,
                                size_t field_index) {
  // Shortcut for common case usage of GEP
  return emit_gep(std::move(ptr), aggregate_type_id,
                  Index{
                      .m_index = 0,
                      .m_type = m_ctx.m_i32,
                  },
                  {
                      Index{
                          .m_index = field_index,
                          .m_type = m_ctx.m_i32,
                      },
                  });
}
Value IRBuilder::emit_extractvalue(Value source, TypeId aggregate_type_id,
                                   size_t index) {
  const auto &struct_type = m_ctx.type().as_struct(aggregate_type_id);
  auto destination = next_ssa_temporary(struct_type.m_fields[index]);
  block().add_instruction(ExtractValueInstruction{
      .m_destination = destination,
      .m_source = std::move(source),
      .m_aggregate_type_id = aggregate_type_id,
      .m_index = index,
  });
  return destination;
}

Value IRBuilder::emit_ptr_to_int(Value source, TypeId destination_type) {
  auto destination = next_ssa_temporary(destination_type);
  block().add_instruction(PtrToIntInstruction{
      .m_destination = destination,
      .m_source = std::move(source),
  });
  return destination;
}

Value IRBuilder::emit_call(Value callee, std::vector<Value> arguments,
                           TypeId return_type_id) {
  auto destination = next_ssa_temporary(return_type_id);
  block().add_instruction(CallInstruction{
      .m_destination = destination,
      .m_callee = std::move(callee),
      .m_arguments = std::move(arguments),
  });
  return destination;
}

void IRBuilder::emit_call_void(Value callee, std::vector<Value> arguments) {
  block().add_instruction(CallVoidInstruction{
      .m_callee = std::move(callee),
      .m_arguments = std::move(arguments),
  });
}

void IRBuilder::emit_branch(Value condition, BlockLabel if_true,
                            BlockLabel if_false) {
  block().add_terminal(BranchInstruction{
      .m_condition = std::move(condition),
      .m_iftrue = if_true,
      .m_iffalse = if_false,
  });
}

void IRBuilder::emit_jump(BlockLabel block_label) {
  block().add_terminal(JumpInstruction{
      .m_target = block_label,
  });
}

Value IRBuilder::emit_icmp(Value lhs, Value rhs,
                           LLVMIntegerCompareCondition cond) {
  auto destination = next_ssa_temporary(m_ctx.m_i1);
  block().add_instruction(IntegerCompareInstruction{
      .m_destination = destination,
      .m_lhs = std::move(lhs),
      .m_rhs = std::move(rhs),
      .m_condition = cond,
  });
  return destination;
}

Value IRBuilder::emit_binary(Value lhs, Value rhs, LLVMBinaryOperator op) {
  auto destination = next_ssa_temporary(lhs.m_type_id);
  block().add_instruction(BinaryInstruction{
      .m_destination = destination,
      .m_lhs = std::move(lhs),
      .m_rhs = std::move(rhs),
      .m_operator = op,
  });
  return destination;
}

Value IRBuilder::emit_unary(Value source, UnaryOperator op) {
  auto destination = next_ssa_temporary(source.m_type_id);
  block().add_instruction(UnaryInstruction{
      .m_destination = destination,
      .m_source = std::move(source),
      .m_operator = op,
  });
  return destination;
}

Value IRBuilder::emit_cast(Value input, LLVMCastOperator op, TypeId to) {
  auto destination = next_ssa_temporary(to);
  block().add_instruction(CastInstruction{
      .m_destination = destination,
      .m_source = std::move(input),
      .m_operator = op,
  });
  return destination;
}

void IRBuilder::emit_return(Value value) {
  block().add_terminal(ReturnInstruction{
      .m_value = std::move(value),
  });
}

void IRBuilder::emit_return_void() {
  block().add_terminal(ReturnVoidInstruction{});
}

Value IRBuilder::malloc_struct(TypeId struct_type) {
  auto size_ptr = emit_gep(
      Value{
          .m_handle = LiteralHandle::null(),
          .m_type_id = m_ctx.m_ptr,
      },
      struct_type,
      Index{
          .m_index = 1,
          .m_type = m_ctx.m_i32,
      },
      {});
  auto size_i64 = emit_ptr_to_int(size_ptr, m_ctx.m_i64);
  // TODO: Move this out and generalize allocator
  return emit_call(m_ctx.allocator_symbol(), {size_i64}, m_ctx.m_ptr);
}
Value IRBuilder::alloca_struct(TypeId struct_type) {
  return emit_alloca(struct_type);
}

void IRBuilder::store_to_struct(Value ptr, TypeId struct_type, size_t index,
                                Value value) {
  auto field_ptr = emit_gep_field(std::move(ptr), struct_type, index);
  emit_store(field_ptr, std::move(value));
}

Value IRBuilder::load_from_struct(Value ptr, TypeId struct_type, size_t index) {
  auto field_ptr = emit_gep_field(std::move(ptr), struct_type, index);
  auto value_type = m_ctx.type().as_struct(struct_type).m_fields[index];
  return emit_load(field_ptr, value_type);
}

IRBuilder::InsertionGuard::InsertionGuard(IRBuilder &parent,
                                          BlockLabel block_label)
    : m_parent(parent), m_captured(m_parent.current_block_label()) {
  m_parent.enter_block(block_label);
}

IRBuilder::InsertionGuard::~InsertionGuard() {
  m_parent.enter_block(m_captured);
}

IRBuilder::FunctionGuard::FunctionGuard(IRBuilder &parent,
                                        FunctionHandle function)
    : m_parent(parent), m_captured_block(m_parent.current_block_label()),
      m_captured_function(m_parent.current_function_handle()) {
  m_parent.enter_function(function);
}

IRBuilder::FunctionGuard::~FunctionGuard() {
  m_parent.enter_function(m_captured_function);
  m_parent.enter_block(m_captured_block);
}

BlockLabel IRBuilder::make_block(const std::string &name) {
  assert(m_current_function_handle.m_function != nullptr &&
         "Must enter a function before making a block!");
  auto &new_block = m_current_function_handle.m_function->new_basic_block(name);
  return BlockLabel{
      &new_block,
  };
}

void IRBuilder::enter_block(BlockLabel block_label) {
  m_current_block_label = block_label;
}

IRBuilder::InsertionGuard IRBuilder::push_block(BlockLabel block_label) {
  return {
      *this,
      block_label,
  };
}

IRBuilder::InsertionGuard IRBuilder::push_new_block(const std::string &name) {
  auto new_block_label = make_block(name);
  return {
      *this,
      new_block_label,
  };
}

FunctionHandle IRBuilder::make_function(FunctionDeclaration signature) {
  auto &new_function = m_ctx.module().new_function(std::move(signature));
  return FunctionHandle{
      &new_function,
  };
}

void IRBuilder::enter_function(FunctionHandle function_handle) {
  m_current_function_handle = function_handle;
  enter_block(entry_block_of(function_handle));
}

IRBuilder::FunctionGuard
IRBuilder::push_function(FunctionHandle function_handle) {
  return {
      *this,
      function_handle,
  };
}

IRBuilder::FunctionGuard
IRBuilder::push_new_function(FunctionDeclaration signature) {
  auto new_function_handle = make_function(std::move(signature));
  return {
      *this,
      new_function_handle,
  };
}

FunctionHandle IRBuilder::current_function_handle() const {
  return m_current_function_handle;
}

BlockLabel IRBuilder::current_block_label() const {
  return m_current_block_label;
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
