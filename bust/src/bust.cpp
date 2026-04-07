//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Implementation of the bust driver class.
//*
//*
//****************************************************************************

#include "lexer.hpp"
#include <ast/dump.hpp>
#include <bust.hpp>
#include <evaluator.hpp>
#include <hir/dump.hpp>
#include <iostream>
#include <parser.hpp>
#include <pipeline.hpp>
#include <type_checker.hpp>
#include <validate_main.hpp>

//****************************************************************************

namespace bust {

Bust::Bust(std::istream &input, const char *filename, Options options)
    : m_input(input), m_filename(filename), m_options(options) {}

void Bust::run() {
  auto lexer = make_lexer(m_input, m_filename);
  Parser parser(std::move(lexer));
  auto program = parser.parse();

  if (m_options.dump_ast) {
    std::cout << ast::Dumper::dump(program);
  }

  auto typed = run_pipeline(std::move(program), ValidateMain{}, TypeChecker{});

  if (m_options.dump_hir) {
    std::cout << hir::Dumper::dump(typed);
  }

  if (m_options.llvm_ir) {
    throw std::runtime_error("--llvm-ir not yet implemented");
  }

  auto result = Evaluator{}(typed);
  std::cout << "Program returned: " << result << "\n";
}

} // namespace bust

//****************************************************************************
