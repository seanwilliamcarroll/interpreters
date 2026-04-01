//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Environment implementation for blip evaluation
//*
//*
//****************************************************************************

#include "value.hpp"
#include <environment.hpp>
#include <exceptions.hpp>
#include <functional>
#include <memory>
#include <stdexcept>
#include <variant>

//****************************************************************************
namespace blip {
namespace {
//****************************************************************************

template <typename InnerType>
bool both_are_type(const Value &a, const Value &b) {
  return std::holds_alternative<InnerType>(a) &&
         std::holds_alternative<InnerType>(b);
}

template <typename InnerType> bool is_type(const Value &value) {
  return std::holds_alternative<InnerType>(value);
}

//****************************************************************************
} // namespace
//****************************************************************************

template <typename InnerType>
BuiltInFunction make_builtin(const std::string &name, auto int_operator) {
  return BuiltInFunction{
      .m_name = name,
      .m_expected_arguments = 2,
      .m_native_function = [=](std::vector<Value> args) -> Value {
        if (both_are_type<InnerType>(args[0], args[1])) {
          return int_operator(std::get<InnerType>(args[0]),
                              std::get<InnerType>(args[1]));
        }
        throw std::runtime_error("Cannot use " + std::string(name) +
                                 " with non int parameters!");
      }};
}

std::shared_ptr<ValueEnvironment> default_value_environment() {
  auto env = std::make_shared<ValueEnvironment>();

  // Strict int only
  auto do_int_define = [env](const std::string &name, auto op) {
    env->define(name, make_builtin<int>(name, op));
  };
  auto do_double_define = [env](const std::string &name, auto op) {
    env->define(name, make_builtin<double>(name, op));
  };

  do_int_define("+", std::plus<int>());
  do_double_define("fadd", std::plus<double>());

  do_int_define("-", std::minus<int>());
  do_double_define("fsub", std::minus<double>());

  do_int_define("*", std::multiplies<int>());
  do_double_define("fmul", std::multiplies<double>());

  // Want to catch special case of division
  env->define("/",
              BuiltInFunction{
                  .m_name = "/",
                  .m_expected_arguments = 2,
                  .m_native_function = [](std::vector<Value> args) -> Value {
                    if (both_are_type<int>(args[0], args[1])) {
                      auto divisor = std::get<int>(args[1]);
                      if (divisor == 0) {
                        throw std::runtime_error("Cannot divide by 0!");
                      }
                      return std::get<int>(args[0]) / divisor;
                    }
                    throw std::runtime_error(
                        "Cannot use / with non int parameters!");
                  }});

  env->define("fdiv",
              BuiltInFunction{
                  .m_name = "fdiv",
                  .m_expected_arguments = 2,
                  .m_native_function = [](std::vector<Value> args) -> Value {
                    if (both_are_type<double>(args[0], args[1])) {
                      auto divisor = std::get<double>(args[1]);
                      if (divisor == 0) {
                        throw std::runtime_error("Cannot divide by 0!");
                      }
                      return std::get<double>(args[0]) / divisor;
                    }
                    throw std::runtime_error(
                        "Cannot use fdiv with non double parameters!");
                  }});

  do_int_define(">", std::greater<int>());
  do_double_define(">.", std::greater<double>());

  do_int_define("<", std::less<int>());
  do_double_define("<.", std::less<double>());

  do_int_define("=", std::equal_to<int>());
  do_double_define("=.", std::equal_to<double>());

  return env;
}

std::shared_ptr<TypeEnvironment> default_type_environment() {
  auto env = std::make_shared<TypeEnvironment>();

  // (double, double) -> double
  auto double_binop = std::make_shared<FunctionType>(
      FunctionType{{BaseType::Double, BaseType::Double}, BaseType::Double});

  // (int, int) -> int
  auto int_binop = std::make_shared<FunctionType>(
      FunctionType{{BaseType::Int, BaseType::Int}, BaseType::Int});

  // (int, int) -> bool
  auto int_cmp = std::make_shared<FunctionType>(
      FunctionType{{BaseType::Int, BaseType::Int}, BaseType::Bool});

  // (double, double) -> bool
  auto double_cmp = std::make_shared<FunctionType>(
      FunctionType{{BaseType::Double, BaseType::Double}, BaseType::Bool});

  env->define("+", int_binop);
  env->define("fadd", double_binop);

  env->define("-", int_binop);
  env->define("fsub", double_binop);

  env->define("*", int_binop);
  env->define("fmul", double_binop);

  env->define("/", int_binop);
  env->define("fdiv", double_binop);

  env->define(">", int_cmp);
  env->define(">.", double_cmp);
  env->define("<", int_cmp);
  env->define("<.", double_cmp);
  env->define("=", int_cmp);
  env->define("=.", double_cmp);

  return env;
}

//****************************************************************************
} // namespace blip
//****************************************************************************
