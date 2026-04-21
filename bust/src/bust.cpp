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
#include <frontend.hpp>
#include <hir/dump.hpp>
#include <mono/dump.hpp>
#include <monomorpher.hpp>
#include <pipeline.hpp>
#include <type_checker.hpp>
#include <validate_main.hpp>
#include <zir/dump.hpp>
#include <zir_lowerer.hpp>

#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#include <sys/wait.h>

//****************************************************************************

namespace bust {

Bust::Bust(std::istream &input, const char *filename, Options options)
    : m_input(input), m_filename(filename), m_options(options) {}

void Bust::run() {
  if (m_options.dump_source) {
    std::cout << "=== Source ===\n"
              << std::string(std::istreambuf_iterator<char>(m_input),
                             std::istreambuf_iterator<char>())
              << "\n";
    m_input.clear();
    m_input.seekg(0);
  }

  auto program = parse_program(m_input, m_filename);

  if (m_options.dump_ast) {
    std::cout << "=== AST ===\n" << ast::Dumper::dump(program) << "\n";
  }

  auto typed = run_pipeline(std::move(program), ValidateMain{}, TypeChecker{});

  if (m_options.dump_hir) {
    std::cout << "=== HIR ===\n" << hir::Dumper::dump(typed) << "\n";
  }

  auto monomorphed = run_pipeline(std::move(typed), Monomorpher{});

  if (m_options.dump_mono) {
    std::cout << "=== Monomorphed HIR ===\n"
              << mono::Dumper::dump(monomorphed) << "\n";
  }

  auto zir = run_pipeline(std::move(monomorphed), ZirLowerer{});

  if (m_options.dump_zir) {
    std::cout << "=== ZIR HIR ===\n" << zir::Dumper::dump(zir) << "\n";
  }

  auto ir = CodeGen{}(zir);

  if (m_options.dump_llvm_ir) {
    std::cout << "=== LLVM IR ===\n" << ir << "\n";
  }

  // Write IR to a temp file and run via lli, then surface its exit code.
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
}

} // namespace bust

//****************************************************************************
