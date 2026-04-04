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
#include <iostream>
#include <parser.hpp>
#include <pipeline.hpp>
#include <type_checker.hpp>
#include <validate_main.hpp>

//****************************************************************************

namespace bust {

Bust::Bust(std::istream &input, const char *filename)
    : m_input(input), m_filename(filename) {}

void Bust::rep() {
  auto lexer = make_lexer(m_input, m_filename);
  Parser parser(std::move(lexer));
  auto program = parser.parse();

  auto validated =
      run_pipeline(std::move(program), ValidateMain{}, TypeChecker{});

  // TODO: evaluate
  // std::cout << ast::Dumper::dump(validated);
}

} // namespace bust

//****************************************************************************
