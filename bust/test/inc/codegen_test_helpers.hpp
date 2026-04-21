//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared helpers for the bust.codegen.* test suites: runs the full
//*            front-end + codegen pipeline, renders both ZIR and LLVM IR as
//*            strings, and provides CHECK_RUN / CHECK_RUN_OUTPUT macros that
//*            attach both dumps to doctest's INFO context for failure output.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen.hpp>
#include <frontend.hpp>
#include <monomorpher.hpp>
#include <type_checker.hpp>
#include <zir/dump.hpp>
#include <zir/program.hpp>
#include <zir_lowerer.hpp>

#include <unistd.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

#include <doctest/doctest.h>
#include <sys/wait.h>

//****************************************************************************
namespace bust::test {
//****************************************************************************

inline zir::Program type_check(const std::string &source) {
  std::istringstream input(source);
  auto program = parse_program(input, "test");
  TypeChecker checker;
  Monomorpher monomorpher;
  ZirLowerer zir_lowerer;
  return zir_lowerer(monomorpher(checker(program)));
}

struct GenResult {
  std::string zir;
  std::string ir;
};

inline GenResult generate(const std::string &source) {
  auto program = type_check(source);
  auto zir_text = zir::Dumper::dump(program);
  CodeGen gen;
  auto ir = gen(program);
  return {.zir = std::move(zir_text), .ir = std::move(ir)};
}

// Convenience: produces just the IR string. For tests that only check
// IR text shape, not execution.
inline std::string codegen(const std::string &source) {
  return generate(source).ir;
}

#ifdef BUST_LLI_PATH

inline int run_via_lli(const std::string &ir) {
  auto path = std::filesystem::temp_directory_path() /
              ("bust_codegen_" + std::to_string(::getpid()) + ".ll");
  {
    std::ofstream out(path);
    out << ir;
  }
  std::string cmd = std::string(BUST_LLI_PATH) + " " + path.string();
  int status = std::system(cmd.c_str());
  std::filesystem::remove(path);
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  return -1;
}

struct RunResult {
  int exit_code;
  std::string stdout_output;
};

inline RunResult run_via_lli_capture(const std::string &ir) {
  auto path = std::filesystem::temp_directory_path() /
              ("bust_codegen_" + std::to_string(::getpid()) + ".ll");
  {
    std::ofstream out(path);
    out << ir;
  }
  std::string cmd =
      std::string(BUST_LLI_PATH) + " " + path.string() + " 2>/dev/null";

  constexpr std::size_t stdout_buffer_size = 256;
  std::string output;
  std::array<char, stdout_buffer_size> buffer{};
  FILE *pipe = ::popen(cmd.c_str(), "r");
  if (pipe != nullptr) {
    while (std::fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
      output += buffer.data();
    }
    int status = ::pclose(pipe);
    std::filesystem::remove(path);
    int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return {.exit_code = exit_code, .stdout_output = output};
  }
  std::filesystem::remove(path);
  return {.exit_code = -1, .stdout_output = ""};
}

// Runs `source` and checks it exits with `expected`. On failure, both the
// ZIR dump and the generated LLVM IR are attached to the test output.
#define CHECK_RUN(source, expected)                                            \
  do {                                                                         \
    const std::string _src = (source);                                         \
    const auto _gen = ::bust::test::generate(_src);                            \
    INFO("source: " << _src);                                                  \
    INFO("ZIR:\n" << _gen.zir);                                                \
    INFO("LLVM IR:\n" << _gen.ir);                                             \
    CHECK(::bust::test::run_via_lli(_gen.ir) == (expected));                   \
  } while (0)

// Runs `source`, checks exit code AND stdout output.
#define CHECK_RUN_OUTPUT(source, expected_exit, expected_output)               \
  do {                                                                         \
    const std::string _src = (source);                                         \
    const auto _gen = ::bust::test::generate(_src);                            \
    INFO("source: " << _src);                                                  \
    INFO("ZIR:\n" << _gen.zir);                                                \
    INFO("LLVM IR:\n" << _gen.ir);                                             \
    auto _result = ::bust::test::run_via_lli_capture(_gen.ir);                 \
    CHECK(_result.exit_code == (expected_exit));                               \
    CHECK(_result.stdout_output == (expected_output));                         \
  } while (0)

#endif // BUST_LLI_PATH

//****************************************************************************
} // namespace bust::test
//****************************************************************************
