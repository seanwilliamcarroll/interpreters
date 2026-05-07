//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared helpers for the bust.integration test suite. Loads
//*            programs from BUST_PROGRAMS_DIR, parses header-comment
//*            manifests (EXPECT_EXIT / EXPECT_STDOUT / EXPECT_FAIL), and
//*            runs the full compiler pipeline with per-stage dump
//*            capture so a failing test always reports source plus all
//*            dumps that completed before the failure.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <ast/dump.hpp>
#include <codegen.hpp>
#include <frontend.hpp>
#include <hir/dump.hpp>
#include <mono/dump.hpp>
#include <monomorpher.hpp>
#include <test/inc/codegen_test_helpers.hpp>
#include <type_checker.hpp>
#include <zir/dump.hpp>
#include <zir_lowerer.hpp>

#include <exception>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#include <doctest/doctest.h>

//****************************************************************************
namespace bust::test {
//****************************************************************************

// --- File loading ----------------------------------------------------------

inline std::string load_program(const std::string &name) {
#ifndef BUST_PROGRAMS_DIR
#error "BUST_PROGRAMS_DIR must be defined by CMake"
#endif
  std::filesystem::path path = std::filesystem::path(BUST_PROGRAMS_DIR) / name;
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("Could not load program: " + path.string());
  }
  std::stringstream buf;
  buf << in.rdbuf();
  return buf.str();
}

// --- Manifest --------------------------------------------------------------

struct Manifest {
  std::optional<int> expect_exit;
  std::string expect_stdout;
  std::optional<std::string> expect_fail;
};

// Decodes a quoted string literal with standard escapes (\n \t \r \\ \" \0).
// Walks forward respecting escapes so an embedded \" doesn't terminate early.
inline std::string parse_string_literal(const std::string &val) {
  if (val.size() < 2 || val.front() != '"') {
    throw std::runtime_error("Expected quoted string, got: " + val);
  }
  std::string result;
  std::size_t i = 1;
  for (; i < val.size(); ++i) {
    if (val[i] == '"') {
      return result;
    }
    if (val[i] == '\\' && i + 1 < val.size()) {
      ++i;
      switch (val[i]) {
      case 'n':
        result += '\n';
        break;
      case 't':
        result += '\t';
        break;
      case 'r':
        result += '\r';
        break;
      case '\\':
        result += '\\';
        break;
      case '"':
        result += '"';
        break;
      case '0':
        result += '\0';
        break;
      default:
        throw std::runtime_error(std::string{"Unknown escape \\"} + val[i] +
                                 " in string literal");
      }
    } else {
      result += val[i];
    }
  }
  throw std::runtime_error("Unterminated string literal: " + val);
}

inline Manifest parse_manifest(const std::string &source) {
  auto strip = [](std::string &s) {
    auto first = s.find_first_not_of(" \t");
    if (first == std::string::npos) {
      s.clear();
      return;
    }
    auto last = s.find_last_not_of(" \t\r");
    s = s.substr(first, last - first + 1);
  };

  Manifest m;
  std::istringstream in(source);
  std::string line;
  while (std::getline(in, line)) {
    strip(line);
    if (line.size() < 2 || line.substr(0, 2) != "//") {
      continue;
    }
    line = line.substr(2);
    strip(line);

    auto take = [&](const std::string &prefix) -> std::optional<std::string> {
      if (line.size() >= prefix.size() &&
          line.substr(0, prefix.size()) == prefix) {
        std::string val = line.substr(prefix.size());
        strip(val);
        return val;
      }
      return std::nullopt;
    };

    if (auto exit_val = take("EXPECT_EXIT:")) {
      m.expect_exit = std::stoi(*exit_val);
    } else if (auto stdout_val = take("EXPECT_STDOUT:")) {
      m.expect_stdout = parse_string_literal(*stdout_val);
    } else if (auto fail_val = take("EXPECT_FAIL:")) {
      m.expect_fail = *fail_val;
    }
  }
  return m;
}

// --- Pipeline with per-stage dumps -----------------------------------------

enum class PipelineStage : std::uint8_t {
  Parse,
  TypeCheck,
  Monomorphize,
  Lower,
  Codegen,
};

inline std::string to_string(PipelineStage stage) {
  switch (stage) {
  case PipelineStage::Parse:
    return "parse";
  case PipelineStage::TypeCheck:
    return "typecheck";
  case PipelineStage::Monomorphize:
    return "monomorph";
  case PipelineStage::Lower:
    return "lower";
  case PipelineStage::Codegen:
    return "codegen";
  }
  return "unknown";
}

inline std::optional<PipelineStage> parse_stage(const std::string &name) {
  if (name == "parse")
    return PipelineStage::Parse;
  if (name == "typecheck")
    return PipelineStage::TypeCheck;
  if (name == "monomorph")
    return PipelineStage::Monomorphize;
  if (name == "lower")
    return PipelineStage::Lower;
  if (name == "codegen")
    return PipelineStage::Codegen;
  return std::nullopt;
}

struct PipelineDumps {
  std::string source;
  std::string ast;
  std::string hir;
  std::string mono;
  std::string zir;
  std::string ir;
  std::optional<PipelineStage> failed_at;
  std::exception_ptr error;

  [[nodiscard]] std::string error_message() const {
    if (!error) {
      return "";
    }
    try {
      std::rethrow_exception(error);
    } catch (const std::exception &e) {
      return e.what();
    } catch (...) {
      return "unknown exception";
    }
  }
};

// Runs each stage in a try/catch. Captures the dump of each successful
// stage. If a stage throws, records the stage and the exception_ptr and
// returns; later stages are not attempted. Never throws.
//
// std::optional is used for the per-stage Programs because the
// hir/mono/zir Program types are move-only (their member arenas have
// deleted copy/assign), so we cannot default-construct then re-assign.
// emplace() forwards to the move constructor.
inline PipelineDumps run_pipeline(const std::string &source,
                                  const std::string &filename = "test") {
  PipelineDumps p;
  p.source = source;

  std::optional<ast::Program> ast_prog;
  try {
    std::istringstream input(source);
    ast_prog.emplace(parse_program(input, filename.c_str()));
    p.ast = ast::Dumper::dump(*ast_prog);
  } catch (...) {
    p.failed_at = PipelineStage::Parse;
    p.error = std::current_exception();
    return p;
  }

  std::optional<hir::Program> hir_prog;
  try {
    TypeChecker checker;
    hir_prog.emplace(checker(*ast_prog));
    p.hir = hir::Dumper::dump(*hir_prog);
  } catch (...) {
    p.failed_at = PipelineStage::TypeCheck;
    p.error = std::current_exception();
    return p;
  }

  std::optional<mono::Program> mono_prog;
  try {
    Monomorpher monomorpher;
    mono_prog.emplace(monomorpher(std::move(*hir_prog)));
    p.mono = mono::Dumper::dump(*mono_prog);
  } catch (...) {
    p.failed_at = PipelineStage::Monomorphize;
    p.error = std::current_exception();
    return p;
  }

  std::optional<zir::Program> zir_prog;
  try {
    ZirLowerer lowerer;
    zir_prog.emplace(lowerer(std::move(*mono_prog)));
    p.zir = zir::Dumper::dump(*zir_prog);
  } catch (...) {
    p.failed_at = PipelineStage::Lower;
    p.error = std::current_exception();
    return p;
  }

  try {
    CodeGen gen;
    p.ir = gen(*zir_prog);
  } catch (...) {
    p.failed_at = PipelineStage::Codegen;
    p.error = std::current_exception();
    return p;
  }

  return p;
}

// --- Asserting against the manifest ----------------------------------------

inline void assert_pipeline_result(const PipelineDumps &p, const Manifest &m) {
  if (m.expect_fail) {
    auto expected = parse_stage(*m.expect_fail);
    REQUIRE_MESSAGE(expected.has_value(),
                    "Unknown EXPECT_FAIL stage: " << *m.expect_fail);
    REQUIRE_MESSAGE(p.failed_at.has_value(), "Expected pipeline to fail at "
                                                 << *m.expect_fail
                                                 << ", but it succeeded");
    CHECK_MESSAGE(*p.failed_at == *expected,
                  "Expected failure at " << *m.expect_fail << ", but failed at "
                                         << to_string(*p.failed_at) << ": "
                                         << p.error_message());
    return;
  }

  REQUIRE_MESSAGE(!p.failed_at.has_value(), "Pipeline failed at "
                                                << to_string(*p.failed_at)
                                                << ": " << p.error_message());
  REQUIRE_MESSAGE(m.expect_exit.has_value(),
                  "Program manifest must include EXPECT_EXIT");

#ifdef BUST_LLI_PATH
  auto result = run_via_lli_capture(p.ir);
  CHECK(result.exit_code == *m.expect_exit);
  CHECK(result.stdout_output == m.expect_stdout);
#endif
}

// --- Test macro ------------------------------------------------------------
//
// Loads the program by name (relative to bust/programs/), runs the full
// pipeline, attaches every populated dump via INFO() (lazy — only printed
// on test failure), then asserts against the manifest. On any failure,
// the test report includes source plus every dump that completed before
// the failure.
#define RUN_INTEGRATION_PROGRAM(name)                                          \
  do {                                                                         \
    const auto _src = ::bust::test::load_program((name));                      \
    const auto _manifest = ::bust::test::parse_manifest(_src);                 \
    const auto _pipe = ::bust::test::run_pipeline(_src, (name));               \
    INFO("=== program: " << (name) << " ===");                                 \
    INFO("=== source ===\n" << _pipe.source);                                  \
    if (!_pipe.ast.empty()) {                                                  \
      INFO("=== AST ===\n" << _pipe.ast);                                      \
    }                                                                          \
    if (!_pipe.hir.empty()) {                                                  \
      INFO("=== HIR ===\n" << _pipe.hir);                                      \
    }                                                                          \
    if (!_pipe.mono.empty()) {                                                 \
      INFO("=== MONO ===\n" << _pipe.mono);                                    \
    }                                                                          \
    if (!_pipe.zir.empty()) {                                                  \
      INFO("=== ZIR ===\n" << _pipe.zir);                                      \
    }                                                                          \
    if (!_pipe.ir.empty()) {                                                   \
      INFO("=== LLVM IR ===\n" << _pipe.ir);                                   \
    }                                                                          \
    ::bust::test::assert_pipeline_result(_pipe, _manifest);                    \
  } while (0)

//****************************************************************************
} // namespace bust::test
//****************************************************************************
