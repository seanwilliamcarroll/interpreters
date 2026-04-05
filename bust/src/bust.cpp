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
#include <hir/dump.hpp>
#include <iostream>
#include <parser.hpp>
#include <pipeline.hpp>
#include <type_checker.hpp>
#include <validate_main.hpp>

//****************************************************************************

namespace bust {

Bust::Bust(std::istream &input, const char *filename, Mode mode)
    : m_input(input), m_filename(filename), m_mode(mode) {}

void Bust::rep() {
  auto lexer = make_lexer(m_input, m_filename);
  Parser parser(std::move(lexer));
  auto program = parser.parse();

  if (m_mode == Mode::DUMP_AST) {
    std::cout << ast::Dumper::dump(program);
    return;
  }

  auto typed = run_pipeline(std::move(program), ValidateMain{}, TypeChecker{});

  switch (m_mode) {
  case Mode::RUN:
  case Mode::DUMP_HIR:
    std::cout << hir::Dumper::dump(typed);
    return;
  case Mode::EVAL:
    throw std::runtime_error("--eval not yet implemented");
  case Mode::LLVM_IR:
    throw std::runtime_error("--llvm-ir not yet implemented");
  case Mode::DUMP_AST:
    std::unreachable();
  }
}

} // namespace bust

//****************************************************************************
