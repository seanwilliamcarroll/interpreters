//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Module representation for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/function.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "codegen/handle.hpp"
#include "codegen/parameter.hpp"
#include "codegen/types.hpp"
#include "exceptions.hpp"
#include "zir/nodes.hpp"

//****************************************************************************
namespace bust::codegen {
//****************************************************************************
static constexpr const char *FAT_PTR_STRING_LITERAL = "closure";
static constexpr const char *FAT_PTR_FN_PTR_STRING_LITERAL = "fn_ptr";
static constexpr const char *FAT_PTR_ENV_PTR_STRING_LITERAL = "env_ptr";
static constexpr const char *CAPTURE_ENV_STRING_LITERAL = "env";
static constexpr Parameter CAPTURE_ENV_PARAMETER =
    Parameter{.m_name = ParameterHandle{CAPTURE_ENV_STRING_LITERAL},
              .m_type = LLVMType::PTR};

struct Global {
  // TODO
};

struct CaptureEnv {
  TypeHandle m_type_name;
  std::vector<Parameter> m_captures;
};

struct Module {

  explicit Module() {
    m_handle_to_capture_env["__closure"] = CaptureEnv{
        .m_type_name = TypeHandle{.m_handle = FAT_PTR_STRING_LITERAL},
        .m_captures = {
            Parameter{.m_name = ParameterHandle{FAT_PTR_FN_PTR_STRING_LITERAL},
                      .m_type = LLVMType::PTR},
            Parameter{.m_name = ParameterHandle{FAT_PTR_ENV_PTR_STRING_LITERAL},
                      .m_type = LLVMType::PTR}}};
  }

  Function &new_function(FunctionDeclaration signature) {
    m_functions.emplace_back(std::make_unique<Function>(std::move(signature)));
    return *m_functions.back();
  }

  [[nodiscard]] const std::vector<Global> &globals() const { return m_globals; }

  [[nodiscard]] const std::vector<std::unique_ptr<Function>> &
  functions() const {
    return m_functions;
  }

  Function &current_function() { return *m_current_function; }

  void set_current_function(Function &function) {
    m_current_function = &function;
  }

  void add_extern_function_declaration(
      std::unique_ptr<FunctionDeclaration> func_declaration) {
    m_extern_functions.emplace_back(std::move(func_declaration));
  }

  [[nodiscard]] const std::vector<std::unique_ptr<FunctionDeclaration>> &
  extern_functions() const {
    return m_extern_functions;
  }

  Handle add_capture_env(const std::string &handle,
                         const std::vector<Parameter> &captures) {
    auto type_handle =
        TypeHandle{.m_handle = CAPTURE_ENV_STRING_LITERAL + std::string(".") +
                               std::to_string(m_next_capture_env_index++)};
    auto capture_env =
        CaptureEnv{.m_type_name = type_handle, .m_captures = captures};
    m_handle_to_capture_env[handle] = capture_env;
    return type_handle;
  }

  [[nodiscard]] const std::unordered_map<std::string, CaptureEnv> &
  handles_to_capture_envs() const {
    return m_handle_to_capture_env;
  }

  const CaptureEnv &get_capture_env(const std::string &handle) {
    auto iter = m_handle_to_capture_env.find(handle);
    if (iter == m_handle_to_capture_env.end()) {
      throw core::InternalCompilerError(
          "Could not find capture env stored under: " + handle);
    }
    return iter->second;
  }

private:
  std::vector<Global> m_globals;
  std::vector<std::unique_ptr<Function>> m_functions;
  std::vector<std::unique_ptr<FunctionDeclaration>> m_extern_functions;
  Function *m_current_function = nullptr;
  std::unordered_map<std::string, CaptureEnv> m_handle_to_capture_env;
  size_t m_next_capture_env_index = 0;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
