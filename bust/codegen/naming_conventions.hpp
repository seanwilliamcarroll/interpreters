//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Centralized naming conventions for codegen IR emission.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <cstddef>
#include <string>
#include <string_view>

//****************************************************************************
namespace bust::codegen::conventions {
//****************************************************************************

// Closure ABI -----------------------------------------------------------------

constexpr std::string_view closure_type = "closure";

inline std::string make_closure_name(const std::string &input) {
  return input + "." + std::string{closure_type};
}

constexpr std::string_view thunk_suffix = "thunk";

inline std::string make_thunk(const std::string &input) {
  return input + "." + std::string{thunk_suffix};
}

// The env pointer parameter that every user function (except main) takes as
// its first argument, and the struct type used to materialize it at lambda
// creation sites.
constexpr std::string_view env_parameter_name = "env";
constexpr std::string_view env_struct_name = "env";

// Runtime ABI -----------------------------------------------------------------

// Allocator used by malloc_struct. A future refactor may make this
// configurable on Context; for now it is hard-wired to the C ABI.
constexpr std::string_view allocator_function = "malloc";

// Program entry point --------------------------------------------------------

constexpr std::string_view main_function_name = "main";

inline bool is_main(const std::string &name) {
  return name == main_function_name;
}

// Block labels ---------------------------------------------------------------

constexpr std::string_view entry_block_label = "entry";
constexpr std::string_view then_block_label = "then";
constexpr std::string_view else_block_label = "else";
constexpr std::string_view merge_block_label = "merge";
constexpr std::string_view rhs_block_label = "rhs";

// Synthetic local / global name roots ----------------------------------------

constexpr std::string_view if_result_local = "if_result";
constexpr std::string_view short_circuit_result_local =
    "short_circuit_logic_result";
constexpr std::string_view lambda_global = "lambda";

constexpr std::string_view param_prefix = "param_";

constexpr std::string_view parameter_alloca_suffix = "addr";

inline std::string make_param_name(std::size_t index) {
  return std::string{param_prefix} + std::to_string(index);
}

inline std::string make_alloca_name(const std::string &input) {
  return input + "." + std::string{parameter_alloca_suffix};
}

//****************************************************************************
} // namespace bust::codegen::conventions
//****************************************************************************
