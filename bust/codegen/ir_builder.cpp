//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the IR builder.
//*
//*
//****************************************************************************

#include <codegen/context.hpp>
#include <codegen/function_declaration.hpp>
#include <codegen/handle.hpp>
#include <codegen/instructions.hpp>
#include <codegen/ir_builder.hpp>
#include <codegen/parameter.hpp>
#include <codegen/types.hpp>

#include <cassert>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

BasicBlock &IRBuilder::block() const { return *current_block_label().m_block; }

BlockLabel IRBuilder::entry_block_of(FunctionHandle function) {
  return BlockLabel{&function.m_function->entry_basic_block()};
}

NamedHandle IRBuilder::add_alloca(const std::string &name,
                                  TypeId type_id) const {
  auto output_handle = m_ctx.symbols().define_named(name);

  current_function_handle().m_function->add_alloca_instruction(
      {.m_handle = output_handle, .m_type = type_id});

  return output_handle;
}

void IRBuilder::create_store(Handle destination, Argument value) const {
  auto [source, type] = std::move(value);
  block().add_instruction(
      StoreInstruction{.m_destination = std::move(destination),
                       .m_source = source,
                       .m_type = type});
}

Handle IRBuilder::create_load(Handle source, TypeId type) const {
  auto temp_ssa = m_ctx.symbols().next_ssa_temporary();
  block().add_instruction(LoadInstruction{
      .m_destination = temp_ssa,
      .m_source = std::move(source),
      .m_type = type,
  });
  return temp_ssa;
}

Handle IRBuilder::create_gep(TypeId struct_type_id, Handle struct_handle,
                             Argument initial_index,
                             std::vector<Argument> indices) const {
  auto ptr_to_struct_field = m_ctx.symbols().next_ssa_temporary();
  block().add_instruction(
      GetElementPtrInstruction{.m_destination = ptr_to_struct_field,
                               .m_struct_type = struct_type_id,
                               .m_struct_handle = std::move(struct_handle),
                               .m_initial_index = std::move(initial_index),
                               .m_additional_indices = std::move(indices)});
  return ptr_to_struct_field;
}

Handle IRBuilder::create_gep_field(TypeId struct_type_id, Handle struct_handle,
                                   size_t field_index) const {
  return create_gep(
      struct_type_id, std::move(struct_handle),
      Argument{.m_name = LiteralHandle::zero(), .m_type = m_ctx.m_i32},
      {Argument{.m_name = LiteralHandle{std::to_string(field_index)},
                .m_type = m_ctx.m_i32}});
}

Handle IRBuilder::create_ptr_to_int(Handle source,
                                    TypeId destination_type) const {
  auto destination = m_ctx.symbols().next_ssa_temporary();
  block().add_instruction(
      PtrToIntInstruction{.m_destination = destination,
                          .m_source = std::move(source),
                          .m_destination_type = destination_type});
  return destination;
}

Handle IRBuilder::create_call(Handle callee, std::vector<Argument> arguments,
                              TypeId return_type_id) const {
  auto target_handle = m_ctx.symbols().next_ssa_temporary();
  // Allocate the env
  block().add_instruction(CallInstruction{
      .m_target = target_handle,
      .m_callee = std::move(callee),
      .m_arguments = std::move(arguments),
      .m_return_type = return_type_id,
  });
  return target_handle;
}

void IRBuilder::create_call_void(Handle callee,
                                 std::vector<Argument> arguments) const {
  block().add_instruction(CallVoidInstruction{
      .m_callee = std::move(callee),
      .m_arguments = std::move(arguments),
  });
}

void IRBuilder::add_branch(Handle condition, BlockLabel if_true,
                           BlockLabel if_false) const {
  block().add_terminal(BranchInstruction{
      .m_condition = std::move(condition),
      .m_iftrue = if_true,
      .m_iffalse = if_false,
  });
}

void IRBuilder::add_jump(BlockLabel block_label) const {
  block().add_terminal(JumpInstruction{.m_target = block_label});
}

Handle IRBuilder::create_icmp(Handle lhs, Handle rhs,
                              LLVMIntegerCompareCondition cond,
                              TypeId type) const {
  auto temp = m_ctx.symbols().next_ssa_temporary();
  block().add_instruction(IntegerCompareInstruction{.m_result = temp,
                                                    .m_lhs = std::move(lhs),
                                                    .m_rhs = std::move(rhs),
                                                    .m_condition = cond,
                                                    .m_type = type});
  return temp;
}

Handle IRBuilder::create_binary(Handle lhs, Handle rhs, LLVMBinaryOperator op,
                                TypeId type) const {
  auto temp = m_ctx.symbols().next_ssa_temporary();
  block().add_instruction(BinaryInstruction{.m_result = temp,
                                            .m_lhs = std::move(lhs),
                                            .m_rhs = std::move(rhs),
                                            .m_operator = op,
                                            .m_type = type});
  return temp;
}

Handle IRBuilder::create_unary(Handle input, UnaryOperator op,
                               TypeId type) const {
  auto temp = m_ctx.symbols().next_ssa_temporary();
  block().add_instruction(UnaryInstruction{
      .m_result = temp,
      .m_input = std::move(input),
      .m_operator = op,
      .m_type = type,
  });
  return temp;
}

Handle IRBuilder::create_cast(Handle input, LLVMCastOperator op, TypeId from,
                              TypeId to) const {
  auto temp = m_ctx.symbols().next_ssa_temporary();
  block().add_instruction(CastInstruction{
      .m_destination = temp,
      .m_source = std::move(input),
      .m_operator = op,
      .m_from = from,
      .m_to = to,
  });
  return temp;
}

void IRBuilder::create_return(Handle value, TypeId type) const {
  block().add_terminal(
      ReturnInstruction{.m_value = std::move(value), .m_type = type});
}

void IRBuilder::create_return_void() const {
  block().add_terminal(ReturnVoidInstruction{});
}

void IRBuilder::emit_parameter_prologue(
    const std::vector<Parameter> &parameters) {
  // Make allocas for all parameters
  for (const auto &parameter : parameters) {
    auto alloca_handle =
        m_ctx.builder().add_alloca(parameter.m_name, parameter.m_type);

    m_ctx.builder().create_store(
        alloca_handle,
        {.m_name = NamedHandle{parameter.m_name}, .m_type = parameter.m_type});
  }
}

Handle IRBuilder::malloc_struct(TypeId struct_type) const {
  auto size_ptr = create_gep(
      struct_type, LiteralHandle::null(),
      Argument{.m_name = LiteralHandle::one(), .m_type = m_ctx.m_i32}, {});
  auto size_i64 = create_ptr_to_int(size_ptr, m_ctx.m_i64);
  // TODO: Move this out and generalize allocator
  auto malloc_handle =
      GlobalHandle{std::string{conventions::allocator_function}};
  return create_call(malloc_handle,
                     {Argument{.m_name = size_i64, .m_type = m_ctx.m_i64}},
                     m_ctx.m_ptr);
}

void IRBuilder::store_to_struct(TypeId struct_type, Handle base, size_t index,
                                Argument value) const {
  auto field_ptr = create_gep_field(struct_type, std::move(base), index);
  create_store(field_ptr, std::move(value));
}

Handle IRBuilder::load_from_struct(TypeId struct_type, Handle base,
                                   size_t index, TypeId value_type) const {
  auto field_ptr = create_gep_field(struct_type, std::move(base), index);
  return create_load(field_ptr, value_type);
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
  return BlockLabel{&new_block};
}

void IRBuilder::enter_block(BlockLabel block_label) {
  m_current_block_label = block_label;
}

IRBuilder::InsertionGuard IRBuilder::push_block(BlockLabel block_label) {
  return {*this, block_label};
}

IRBuilder::InsertionGuard IRBuilder::push_new_block(const std::string &name) {
  auto new_block_label = make_block(name);
  return {*this, new_block_label};
}

FunctionHandle IRBuilder::make_function(FunctionDeclaration signature) {
  auto &new_function = m_ctx.module().new_function(std::move(signature));
  return FunctionHandle{&new_function};
}

void IRBuilder::enter_function(FunctionHandle function_handle) {
  m_current_function_handle = function_handle;
  enter_block(entry_block_of(function_handle));
}

IRBuilder::FunctionGuard
IRBuilder::push_function(FunctionHandle function_handle) {
  return {*this, function_handle};
}

IRBuilder::FunctionGuard
IRBuilder::push_new_function(FunctionDeclaration signature) {
  auto new_function_handle = make_function(std::move(signature));
  return {*this, new_function_handle};
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
