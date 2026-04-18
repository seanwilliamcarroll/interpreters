//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of expression generator.
//*
//*
//****************************************************************************

#include <codegen/basic_block.hpp>
#include <codegen/context.hpp>
#include <codegen/expression_generator.hpp>
#include <codegen/function.hpp>
#include <codegen/function_declaration.hpp>
#include <codegen/handle.hpp>
#include <codegen/instructions.hpp>
#include <codegen/module.hpp>
#include <codegen/parameter.hpp>
#include <codegen/statement_generator.hpp>
#include <codegen/symbol_table.hpp>
#include <codegen/types.hpp>
#include <exceptions.hpp>
#include <operators.hpp>
#include <zir/arena.hpp>
#include <zir/nodes.hpp>
#include <zir/types.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

Handle ExpressionGenerator::generate(const zir::ExprId &expr_id) {
  return generate(m_ctx.arena().get(expr_id));
}

Handle ExpressionGenerator::generate(const zir::ExprKind &expr_kind) {
  return std::visit(*this, expr_kind);
}

Handle ExpressionGenerator::generate(const zir::Expression &expression) {
  return generate(expression.m_expr_kind);
}

Handle ExpressionGenerator::operator()(const zir::IdentifierExpr &identifier) {
  auto binding = m_ctx.arena().get(identifier.m_id);
  auto identifier_handle = m_ctx.symbols().lookup(binding.m_name);

  // GlobalHandle (functions) and ParameterHandle (SSA args) are used directly.
  // Only LocalHandle (alloca slots) need a load.
  if (!std::holds_alternative<LocalHandle>(identifier_handle)) {
    return identifier_handle;
  }

  auto ssa_temp = m_ctx.symbols().next_ssa_temporary();

  m_ctx.block().add_instruction(
      LoadInstruction{.m_destination = ssa_temp,
                      .m_source = identifier_handle,
                      .m_type = m_ctx.to_type(binding.m_type)});

  return ssa_temp;
}

Handle ExpressionGenerator::operator()(const zir::Unit & /*unused*/) {
  return {};
}

Handle ExpressionGenerator::operator()(const zir::I8 &literal) {
  return LiteralHandle{std::to_string(literal.m_value)};
}

Handle ExpressionGenerator::operator()(const zir::I32 &literal) {
  return LiteralHandle{std::to_string(literal.m_value)};
}

Handle ExpressionGenerator::operator()(const zir::I64 &literal) {
  return LiteralHandle{std::to_string(literal.m_value)};
}

Handle ExpressionGenerator::operator()(const zir::Bool &literal) {
  return LiteralHandle{literal.m_value ? "true" : "false"};
}

Handle ExpressionGenerator::operator()(const zir::Char &literal) {
  return LiteralHandle{std::to_string(static_cast<int8_t>(literal.m_value))};
}

Handle ExpressionGenerator::operator()(const zir::Block &block) {
  for (const auto &statement : block.m_statements) {
    StatementGenerator{m_ctx}.generate(statement);
  }

  if (block.m_final_expression.has_value()) {
    return generate(block.m_final_expression.value());
  }

  return {};
}

zir::TypeId ExpressionGenerator::get_block_type(const zir::Block &block) const {
  if (block.m_final_expression.has_value()) {
    const auto &expression =
        m_ctx.arena().get(block.m_final_expression.value());
    return expression.m_type_id;
  }
  return m_ctx.arena().type().m_unit;
}

Handle ExpressionGenerator::operator()(const zir::IfExpr &if_expression) {
  auto &function = m_ctx.function();

  // Get handle to current after condition target generated, so even with nested
  // blocks, we're still starting right after the condition
  auto condition_target = generate(if_expression.m_condition);
  auto &final_condition_block = function.current_basic_block();

  // Create starting blocks that will always exist, then and merge
  auto &starting_then_block = function.new_basic_block("then");
  auto &starting_merge_block = function.new_basic_block("merge");

  // Do then branch, capture the final block for later if needed
  function.set_insertion_point(starting_then_block);
  auto then_target = generate(if_expression.m_then_block);
  auto &final_then_block = function.current_basic_block();
  final_then_block.add_terminal(
      JumpInstruction{.m_target = starting_merge_block.label()});

  if (!if_expression.m_else_block.has_value()) {
    // Just bare if, else is merge
    final_condition_block.add_terminal(
        BranchInstruction{.m_condition = condition_target,
                          .m_iftrue = starting_then_block.label(),
                          .m_iffalse = starting_merge_block.label()});
    // Subsequent instructions should go into merge block
    function.set_insertion_point(starting_merge_block);
    // Void, no handle to return
    return {};
  }

  // Now we handle the else, capturing first block and last block related to
  // this branch
  auto &starting_else_block = function.new_basic_block("else");
  function.set_insertion_point(starting_else_block);
  auto else_target = generate(if_expression.m_else_block.value());
  auto &final_else_block = function.current_basic_block();
  final_else_block.add_terminal(
      JumpInstruction{.m_target = starting_merge_block.label()});

  // Now we can merge on then vs else
  final_condition_block.add_terminal(
      BranchInstruction{.m_condition = condition_target,
                        .m_iftrue = starting_then_block.label(),
                        .m_iffalse = starting_else_block.label()});

  // Subsequent instructions should go into merge block
  function.set_insertion_point(starting_merge_block);

  // We only return a value when there is an else block
  // AND the types of the then and else expressions are NOT Unit
  const auto &if_return_type_id = get_block_type(if_expression.m_then_block);

  const auto if_type_is_unit = if_return_type_id == m_ctx.arena().type().m_unit;
  auto if_statement_returns_value =
      if_expression.m_else_block.has_value() && !if_type_is_unit;
  if (!if_statement_returns_value) {
    // Void, no handle to return
    return {};
  }

  auto result_handle = m_ctx.symbols().define_local("if_result");

  function.add_alloca_instruction(AllocaInstruction{
      .m_handle = result_handle, .m_type = m_ctx.to_type(if_return_type_id)});

  // Need to store the value generated by each branch for use in the merge block
  final_then_block.add_instruction(
      StoreInstruction{.m_destination = result_handle,
                       .m_source = then_target,
                       .m_type = m_ctx.to_type(if_return_type_id)});

  final_else_block.add_instruction(
      StoreInstruction{.m_destination = result_handle,
                       .m_source = else_target,
                       .m_type = m_ctx.to_type(if_return_type_id)});

  auto ssa_temp = m_ctx.symbols().next_ssa_temporary();
  starting_merge_block.add_instruction(
      LoadInstruction{.m_destination = ssa_temp,
                      .m_source = {result_handle},
                      .m_type = m_ctx.to_type(if_return_type_id)});
  return ssa_temp;
}

Handle ExpressionGenerator::operator()(const zir::CallExpr &call_expression) {

  const auto &closure_struct = m_ctx.module().get_capture_env("__closure");

  auto closure_handle = generate(call_expression.m_callee);
  auto callee_handle = m_ctx.symbols().next_ssa_temporary();
  auto env_handle = m_ctx.symbols().next_ssa_temporary();

  // Decide if env is null or not
  {
    auto temp_dest = m_ctx.symbols().next_ssa_temporary();
    // Load the env value ptr
    m_ctx.function().current_basic_block().add_instruction(
        GetElementPtrInstruction{
            .m_destination = temp_dest,
            .m_struct_type = closure_struct.m_type_name,
            .m_struct_handle = closure_handle,
            .m_initial_index =
                Argument{.m_name = LiteralHandle{"0"}, .m_type = LLVMType::I32},
            .m_additional_indices = {Argument{.m_name = LiteralHandle{"0"},
                                              .m_type = LLVMType::I32}}});

    m_ctx.function().current_basic_block().add_instruction(
        LoadInstruction{.m_destination = callee_handle,
                        .m_source = temp_dest,
                        .m_type = LLVMType::PTR});
  }

  {
    auto temp_dest = m_ctx.symbols().next_ssa_temporary();
    // Load the env value ptr
    m_ctx.function().current_basic_block().add_instruction(
        GetElementPtrInstruction{
            .m_destination = temp_dest,
            .m_struct_type = closure_struct.m_type_name,
            .m_struct_handle = closure_handle,
            .m_initial_index =
                Argument{.m_name = LiteralHandle{"0"}, .m_type = LLVMType::I32},
            .m_additional_indices = {Argument{.m_name = LiteralHandle{"1"},
                                              .m_type = LLVMType::I32}}});

    m_ctx.function().current_basic_block().add_instruction(
        LoadInstruction{.m_destination = env_handle,
                        .m_source = temp_dest,
                        .m_type = LLVMType::PTR});
  }

  std::vector<Argument> arguments;
  arguments.emplace_back(
      Argument{.m_name = env_handle, .m_type = LLVMType::PTR});
  std::ranges::transform(
      call_expression.m_arguments,

      std::back_inserter(arguments),
      [this](const auto &argument_expr_id) -> Argument {
        auto expression = m_ctx.arena().get(argument_expr_id);
        auto handle = generate(expression);
        return {.m_name = handle,
                .m_type = m_ctx.to_type(expression.m_type_id)};
      });

  auto callee_expr = m_ctx.arena().get(call_expression.m_callee);

  auto function_return_type_id =
      m_ctx.arena().as_function(callee_expr.m_type_id).m_return_type;

  if (function_return_type_id == m_ctx.arena().type().m_unit) {
    m_ctx.block().add_instruction(
        CallVoidInstruction{.m_callee = std::move(callee_handle),
                            .m_arguments = std::move(arguments)});
    return {};
  }

  auto ssa_temp = m_ctx.symbols().next_ssa_temporary();
  m_ctx.block().add_instruction(
      CallInstruction{.m_target = ssa_temp,
                      .m_callee = std::move(callee_handle),
                      .m_arguments = std::move(arguments),
                      .m_return_type = m_ctx.to_type(function_return_type_id)});

  return ssa_temp;
}

LLVMBinaryOperator to_llvm_op(BinaryOperator op) {
  switch (op) {
  case BinaryOperator::PLUS:
    return LLVMBinaryOperator::ADD;
  case BinaryOperator::MINUS:
    return LLVMBinaryOperator::SUB;
  case BinaryOperator::MULTIPLIES:
    return LLVMBinaryOperator::MUL;
  case BinaryOperator::DIVIDES:
    return LLVMBinaryOperator::SDIV;
  case BinaryOperator::MODULUS:
    return LLVMBinaryOperator::SREM;

  default:
    throw core::InternalCompilerError(
        "unsupported binary operator in LLVM lowering");
  }
}

bool is_signed_type(const zir::Type &type) {
  return std::visit(
      [](const auto &t) -> bool {
        using T = std::decay_t<decltype(t)>;
        if constexpr (std::is_same_v<T, zir::I8Type> ||
                      std::is_same_v<T, zir::I32Type> ||
                      std::is_same_v<T, zir::I64Type>) {
          return true;
        } else if constexpr (std::is_same_v<T, zir::UnitType> ||
                             std::is_same_v<T, zir::BoolType> ||
                             std::is_same_v<T, zir::CharType> ||
                             std::is_same_v<T, zir::NeverType> ||
                             std::is_same_v<T, zir::FunctionType>) {
          throw core::InternalCompilerError(
              "Should never had checked signedness on non numeric type");
        }
      },
      type);
}

LLVMIntegerCompareCondition to_llvm_compare_condition(BinaryOperator op,
                                                      const zir::Type &type) {
  switch (op) {
  case BinaryOperator::EQ:
    return LLVMIntegerCompareCondition::EQ;
  case BinaryOperator::NOT_EQ:
    return LLVMIntegerCompareCondition::NE;

  case BinaryOperator::LT:
    if (is_signed_type(type)) {
      return LLVMIntegerCompareCondition::SLT;
    } else {
      return LLVMIntegerCompareCondition::ULT;
    }
  case BinaryOperator::LT_EQ:
    if (is_signed_type(type)) {
      return LLVMIntegerCompareCondition::SLE;
    } else {
      return LLVMIntegerCompareCondition::ULE;
    }
  case BinaryOperator::GT:
    if (is_signed_type(type)) {
      return LLVMIntegerCompareCondition::SGT;
    } else {
      return LLVMIntegerCompareCondition::UGT;
    }
  case BinaryOperator::GT_EQ:
    if (is_signed_type(type)) {
      return LLVMIntegerCompareCondition::SGE;
    } else {
      return LLVMIntegerCompareCondition::UGE;
    }

  default:
    throw core::InternalCompilerError(
        "unsupported compare operator in LLVM lowering");
  }
}

bool is_binary_compare(BinaryOperator op) {
  switch (op) {
  case BinaryOperator::EQ:
  case BinaryOperator::LT:
  case BinaryOperator::LT_EQ:
  case BinaryOperator::GT:
  case BinaryOperator::GT_EQ:
  case BinaryOperator::NOT_EQ:
    return true;
  default:
    return false;
  }
}

bool is_logical_op(BinaryOperator op) {
  switch (op) {
  case BinaryOperator::LOGICAL_AND:
  case BinaryOperator::LOGICAL_OR:
    return true;
  default:
    return false;
  }
}

Handle ExpressionGenerator::generate_integer_compare_instruction(
    const zir::BinaryExpr &binary_expression) {

  auto lhs = m_ctx.arena().get(binary_expression.m_lhs);

  auto compare_condition = to_llvm_compare_condition(
      binary_expression.m_operator, m_ctx.arena().get(lhs.m_type_id));

  auto lhs_handle = generate(binary_expression.m_lhs);

  auto rhs_handle = generate(binary_expression.m_rhs);

  auto result_handle = m_ctx.symbols().next_ssa_temporary();

  m_ctx.block().add_instruction(
      IntegerCompareInstruction{.m_result = result_handle,
                                .m_lhs = lhs_handle,
                                .m_rhs = rhs_handle,
                                .m_condition = compare_condition,
                                .m_type = m_ctx.to_type(lhs.m_type_id)});

  return result_handle;
}

Handle ExpressionGenerator::generate_arithmetic_binary_instruction(
    const zir::BinaryExpr &binary_expression) {

  auto lhs = m_ctx.arena().get(binary_expression.m_lhs);

  auto lhs_handle = generate(binary_expression.m_lhs);

  auto rhs_handle = generate(binary_expression.m_rhs);

  auto result_handle = m_ctx.symbols().next_ssa_temporary();

  m_ctx.block().add_instruction(
      BinaryInstruction{.m_result = result_handle,
                        .m_lhs = lhs_handle,
                        .m_rhs = rhs_handle,
                        .m_operator = to_llvm_op(binary_expression.m_operator),
                        .m_type = m_ctx.to_type(lhs.m_type_id)});

  return result_handle;
}

Handle ExpressionGenerator::generate_logical_binary_instruction(
    const zir::BinaryExpr &binary_expression) {
  // Supporting short circuiting is similar to if expressions
  // lhs executes no matter what
  // For AND, branch to merge on negative, else to rhs
  // For OR, branch to merge on positive, else to rhs
  // rhs jumps to merge
  // at merge, we still need to do a compare on the result

  auto result_handle =
      m_ctx.symbols().define_local("short_circuit_logic_result");

  m_ctx.function().add_alloca_instruction(
      AllocaInstruction{.m_handle = result_handle, .m_type = LLVMType::I1});

  auto lhs_handle = generate(binary_expression.m_lhs);
  auto &final_lhs_block = m_ctx.block();
  // Store lhs in result handle
  final_lhs_block.add_instruction(StoreInstruction{
      .m_destination = result_handle,
      .m_source = lhs_handle,
      .m_type = LLVMType::I1,
  });

  auto &starting_rhs_block = m_ctx.function().new_basic_block("rhs");
  m_ctx.function().set_insertion_point(starting_rhs_block);
  auto rhs_handle = generate(binary_expression.m_rhs);
  auto &final_rhs_block = m_ctx.block();

  final_rhs_block.add_instruction(StoreInstruction{
      .m_destination = result_handle,
      .m_source = rhs_handle,
      .m_type = LLVMType::I1,
  });

  auto &starting_merge_block = m_ctx.function().new_basic_block("merge");
  m_ctx.function().set_insertion_point(starting_merge_block);

  // Figure out terminal instructions now

  // LHS always branches
  const auto is_and_op =
      binary_expression.m_operator == BinaryOperator::LOGICAL_AND;
  final_lhs_block.add_terminal(BranchInstruction{
      .m_condition = lhs_handle,
      .m_iftrue =
          is_and_op ? starting_rhs_block.label() : starting_merge_block.label(),
      .m_iffalse =
          is_and_op ? starting_merge_block.label() : starting_rhs_block.label(),
  });

  final_rhs_block.add_terminal(JumpInstruction{
      .m_target = starting_merge_block.label(),
  });

  auto final_result = m_ctx.symbols().next_ssa_temporary();

  m_ctx.block().add_instruction(LoadInstruction{
      .m_destination = final_result,
      .m_source = result_handle,
      .m_type = LLVMType::I1,
  });

  return final_result;
}

Handle
ExpressionGenerator::operator()(const zir::BinaryExpr &binary_expression) {
  if (is_binary_compare(binary_expression.m_operator)) {
    return generate_integer_compare_instruction(binary_expression);
  }

  if (is_logical_op(binary_expression.m_operator)) {
    return generate_logical_binary_instruction(binary_expression);
  }

  return generate_arithmetic_binary_instruction(binary_expression);
}

Handle ExpressionGenerator::operator()(const zir::UnaryExpr &unary_expression) {

  auto expression = m_ctx.arena().get(unary_expression.m_expression);

  auto input_handle = generate(unary_expression.m_expression);

  auto result_handle = m_ctx.symbols().next_ssa_temporary();

  m_ctx.block().add_instruction(UnaryInstruction{
      .m_result = result_handle,
      .m_input = input_handle,
      .m_operator = unary_expression.m_operator,
      .m_type = m_ctx.to_type(expression.m_type_id),
  });

  return result_handle;
}

Handle ExpressionGenerator::operator()(const zir::ReturnExpr & /*unused*/) {
  return {};
}

Handle ExpressionGenerator::operator()(const zir::CastExpr &cast_expression) {

  auto expression = m_ctx.arena().get(cast_expression.m_expression);

  // We've type checked, so we know it's a valid cast
  auto input_handle = generate(cast_expression.m_expression);

  const auto &from = m_ctx.to_type(expression.m_type_id);
  const auto &to = m_ctx.to_type(cast_expression.m_new_type);

  if (from == to) {
    // Nothing casted, noop
    return input_handle;
  }

  LLVMCastOperator cast_op;

  if (width_bits(from) > width_bits(to)) {
    // Truncation
    cast_op = LLVMCastOperator::TRUNC;
  } else if (from == LLVMType::I1) {
    // Unsigned extend
    cast_op = LLVMCastOperator::ZEXT;
  } else {
    // Signed extend
    cast_op = LLVMCastOperator::SEXT;
  }

  auto ssa_temporary = m_ctx.symbols().next_ssa_temporary();

  m_ctx.block().add_instruction(CastInstruction{
      .m_destination = ssa_temporary,
      .m_source = input_handle,
      .m_operator = cast_op,
      .m_from = from,
      .m_to = to,
  });

  return ssa_temporary;
}

FunctionDeclaration ExpressionGenerator::generate_lambda_signature(
    const zir::LambdaExpr &lambda_expr) {
  std::vector<Parameter> parameters;
  parameters.emplace_back(CAPTURE_ENV_PARAMETER);
  std::ranges::transform(
      lambda_expr.m_parameters, std::back_inserter(parameters),
      [this](const zir::IdentifierExpr &parameter) -> Parameter {
        auto binding = m_ctx.arena().get(parameter.m_id);

        auto handle = m_ctx.symbols().define_parameter(binding.m_name);
        return Parameter{.m_name = std::move(handle),
                         .m_type = m_ctx.to_type(binding.m_type)};
      });

  auto lambda_handle = m_ctx.symbols().define_uniqued_global("lambda");

  return FunctionDeclaration{.m_function_id = lambda_handle,
                             .m_return_type =
                                 m_ctx.to_type(lambda_expr.m_return_type),
                             .m_parameters = std::move(parameters)};
}

struct FunctionScopeGuard {
  FunctionScopeGuard(Context &ctx)
      : m_ctx(ctx), m_original_function(ctx.module().current_function()) {}
  ~FunctionScopeGuard() {
    m_ctx.module().set_current_function(m_original_function);
  }

  Context &m_ctx;
  Function &m_original_function;
};

Handle ExpressionGenerator::operator()(const zir::LambdaExpr &lambda_expr) {
  ScopeGuard scope_guard(m_ctx.symbols());
  FunctionScopeGuard function_guard(m_ctx);
  // Need to capture any variables into an environment struct for this
  // particular lambda
  // Need to generate a top level function, uniquified name
  // Need to generate a func/env ptr pair to return, returning a handle to them

  // Start with pure lambda, no captures

  auto signature = generate_lambda_signature(lambda_expr);

  auto lambda_handle = signature.m_function_id;

  // Before we create the lambda, add the capture storage and malloc

  auto &lambda_creation_basic_block = m_ctx.function().entry_basic_block();

  auto &lambda_function = m_ctx.module().new_function(std::move(signature));
  m_ctx.module().set_current_function(lambda_function);

  Handle env_handle = LiteralHandle{"null"};

  if (!lambda_expr.m_captures.empty()) {
    // We need to load all the captured bindings before we execute the body
    std::vector<Parameter> captured_bindings;
    captured_bindings.reserve(lambda_expr.m_captures.size());
    for (const auto &capture : lambda_expr.m_captures) {
      auto binding = m_ctx.arena().get(capture.m_id);

      captured_bindings.emplace_back(
          Parameter{.m_name = ParameterHandle{binding.m_name},
                    .m_type = m_ctx.to_type(binding.m_type)});
    }
    auto capture_env_type_handle = m_ctx.module().add_capture_env(
        get_raw_handle(lambda_handle), captured_bindings);

    for (const auto &[index, capture] :
         std::views::zip(std::views::iota(0ULL), captured_bindings)) {
      auto temp_dest = m_ctx.symbols().next_ssa_temporary();
      // Load the env value ptr
      m_ctx.function().current_basic_block().add_instruction(
          GetElementPtrInstruction{
              .m_destination = temp_dest,
              .m_struct_type = capture_env_type_handle,
              .m_struct_handle = ParameterHandle{CAPTURE_ENV_STRING_LITERAL},
              .m_initial_index = Argument{.m_name = LiteralHandle{"0"},
                                          .m_type = LLVMType::I32},
              .m_additional_indices = {
                  Argument{.m_name = LiteralHandle{std::to_string(index)},
                           .m_type = LLVMType::I32}}});

      // Store the value itself
      m_ctx.function().current_basic_block().add_instruction(
          LoadInstruction{.m_destination = capture.m_name,
                          .m_source = temp_dest,
                          .m_type = capture.m_type});
    }

    // At creation site, we need to store these parameters
    auto malloc_handle = m_ctx.symbols().lookup("malloc");
    auto temp_size_bytes_ptr = m_ctx.symbols().next_ssa_temporary();
    lambda_creation_basic_block.add_instruction(GetElementPtrInstruction{
        .m_destination = temp_size_bytes_ptr,
        .m_struct_type = capture_env_type_handle,
        .m_struct_handle = LiteralHandle{"null"},
        .m_initial_index =
            Argument{.m_name = LiteralHandle{"1"}, .m_type = LLVMType::I32},
        .m_additional_indices = {}});

    auto temp_size_bytes_i64 = m_ctx.symbols().next_ssa_temporary();
    lambda_creation_basic_block.add_instruction(PtrToIntInstruction{
        .m_destination = temp_size_bytes_i64,
        .m_source = temp_size_bytes_ptr,
        .m_destination_type = LLVMType::I64,
    });

    // How do we store this env handle? In fat ptr?
    env_handle = m_ctx.symbols().define_env_handle(CAPTURE_ENV_STRING_LITERAL);

    // Allocate the env
    lambda_creation_basic_block.add_instruction(CallInstruction{
        .m_target = env_handle,
        .m_callee = malloc_handle,
        .m_arguments = {Argument{.m_name = temp_size_bytes_i64,
                                 .m_type = LLVMType::I64}},
        .m_return_type = LLVMType::PTR,
    });

    for (const auto &[index, capture] :
         std::views::zip(std::views::iota(0ULL), captured_bindings)) {
      // For each capture, we need to get a pointer
      auto temp_dest = m_ctx.symbols().next_ssa_temporary();
      // Load the env value ptr
      lambda_creation_basic_block.add_instruction(GetElementPtrInstruction{
          .m_destination = temp_dest,
          .m_struct_type = capture_env_type_handle,
          .m_struct_handle = env_handle,
          .m_initial_index =
              Argument{.m_name = LiteralHandle{"0"}, .m_type = LLVMType::I32},
          .m_additional_indices = {
              Argument{.m_name = LiteralHandle{std::to_string(index)},
                       .m_type = LLVMType::I32}}});

      // Store the value itself
      lambda_creation_basic_block.add_instruction(StoreInstruction{
          .m_destination = temp_dest,
          .m_source = m_ctx.symbols().lookup(capture.m_name.m_handle),
          .m_type = capture.m_type});
    }
  }

  auto return_value = generate(lambda_expr.m_body);

  if (lambda_expr.m_return_type == m_ctx.arena().type().m_unit) {
    lambda_function.current_basic_block().add_terminal(ReturnVoidInstruction{});
  } else {
    lambda_function.current_basic_block().add_terminal(
        ReturnInstruction{.m_value = return_value,
                          .m_type = m_ctx.to_type(lambda_expr.m_return_type)});
  }

  // Need to store lambda (function) handle and env handle in a closure struct,
  // then return ptr to that

  // At return site, need to return fat pointer, first alloc struct
  auto malloc_handle = m_ctx.symbols().lookup("malloc");
  auto temp_size_bytes_ptr = m_ctx.symbols().next_ssa_temporary();
  const auto &closure_struct = m_ctx.module().get_capture_env("__closure");

  lambda_creation_basic_block.add_instruction(GetElementPtrInstruction{
      .m_destination = temp_size_bytes_ptr,
      .m_struct_type = closure_struct.m_type_name,
      .m_struct_handle = LiteralHandle{"null"},
      .m_initial_index =
          Argument{.m_name = LiteralHandle{"1"}, .m_type = LLVMType::I32},
      .m_additional_indices = {}});

  auto temp_size_bytes_i64 = m_ctx.symbols().next_ssa_temporary();
  lambda_creation_basic_block.add_instruction(PtrToIntInstruction{
      .m_destination = temp_size_bytes_i64,
      .m_source = temp_size_bytes_ptr,
      .m_destination_type = LLVMType::I64,
  });

  // How do we store this env handle? In fat ptr?
  auto closure_handle = m_ctx.symbols().next_ssa_temporary();
  // Allocate the env
  lambda_creation_basic_block.add_instruction(CallInstruction{
      .m_target = closure_handle,
      .m_callee = malloc_handle,
      .m_arguments = {Argument{.m_name = temp_size_bytes_i64,
                               .m_type = LLVMType::I64}},
      .m_return_type = LLVMType::PTR,
  });

  {
    // Store env and lambda handles
    // For each capture, we need to get a pointer
    auto temp_dest = m_ctx.symbols().next_ssa_temporary();
    // Load the env value ptr
    lambda_creation_basic_block.add_instruction(GetElementPtrInstruction{
        .m_destination = temp_dest,
        .m_struct_type = closure_struct.m_type_name,
        .m_struct_handle = closure_handle,
        .m_initial_index =
            Argument{.m_name = LiteralHandle{"0"}, .m_type = LLVMType::I32},
        .m_additional_indices = {
            Argument{.m_name = LiteralHandle{"0"}, .m_type = LLVMType::I32}}});

    // Store the value itself
    lambda_creation_basic_block.add_instruction(
        StoreInstruction{.m_destination = temp_dest,
                         .m_source = lambda_handle,
                         .m_type = LLVMType::PTR});
  }

  {
    // Store env and lambda handles
    // For each capture, we need to get a pointer
    auto temp_dest = m_ctx.symbols().next_ssa_temporary();
    // Load the env value ptr
    lambda_creation_basic_block.add_instruction(GetElementPtrInstruction{
        .m_destination = temp_dest,
        .m_struct_type = closure_struct.m_type_name,
        .m_struct_handle = closure_handle,
        .m_initial_index =
            Argument{.m_name = LiteralHandle{"0"}, .m_type = LLVMType::I32},
        .m_additional_indices = {
            Argument{.m_name = LiteralHandle{"1"}, .m_type = LLVMType::I32}}});

    // Store the value itself
    lambda_creation_basic_block.add_instruction(
        StoreInstruction{.m_destination = temp_dest,
                         .m_source = env_handle,
                         .m_type = LLVMType::PTR});
  }

  return closure_handle;
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
