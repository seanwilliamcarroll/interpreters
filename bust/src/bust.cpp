//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the bust driver class.
//*
//*
//****************************************************************************

#include <ast/dump.hpp>
#include <bust.hpp>
#include <codegen.hpp>
#include <cstdlib>
#include <evaluator.hpp>
#include <fstream>
#include <hir/dump.hpp>
#include <iostream>
#include <memory>
#include <parser.hpp>
#include <pipeline.hpp>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <type_checker.hpp>
#include <unistd.h>
#include <utility>
#include <validate_main.hpp>
#include <zonker.hpp>

#include <lexer.hpp>

//****************************************************************************

namespace bust {

Bust::Bust(std::istream &input, const char *filename, Options options)
    : m_input(input), m_filename(filename), m_options(options) {}

void Bust::run() {
  auto lexer = make_lexer(m_input, m_filename);
  Parser parser(std::move(lexer));
  auto program = parser.parse();

  if (m_options.dump_ast) {
    std::cout << "=== AST ===\n" << ast::Dumper::dump(program) << "\n";
  }

  auto typed = run_pipeline(std::move(program), ValidateMain{}, TypeChecker{});

  if (m_options.dump_hir) {
    std::cout << "=== HIR ===\n" << hir::Dumper::dump(typed) << "\n";
  }

  auto zonked = run_pipeline(std::move(typed), Zonker{});

  if (m_options.llvm_ir) {
    auto ir = CodeGen{}(zonked);
    std::cout << "=== LLVM IR ===\n" << ir << "\n";

    // Write IR to a temp file and run via lli, then surface its exit code
    // as our "Program returned" value (mirrors the Evaluator path).
    char tmp_path[] = "/tmp/bust-XXXXXX.ll";
    int fd = mkstemps(tmp_path, 3);
    if (fd < 0) {
      throw std::runtime_error("failed to create temp file for lli");
    }
    {
      std::ofstream out(tmp_path);
      out << ir;
    }
    ::close(fd);

    std::string cmd = std::string("lli ") + tmp_path;
    int raw = std::system(cmd.c_str());
    ::unlink(tmp_path);

    if (raw < 0 || !WIFEXITED(raw)) {
      throw std::runtime_error("lli did not exit normally");
    }
    std::cout << "Program returned: " << WEXITSTATUS(raw) << "\n";
    return;
  }

  auto result = Evaluator{}(zonked);
  std::cout << "Program returned: " << result << "\n";
}

} // namespace bust

//****************************************************************************
