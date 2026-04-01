//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Blip source file
//*
//*
//****************************************************************************

#include "ast.hpp"
#include "ast_dumper.hpp"
#include "environment.hpp"
#include "evaluator.hpp"
#include "type_checker.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>

#include <ast_printer.hpp>
#include <blip.hpp>
#include <blip_tokens.hpp>
#include <lexer.hpp>
#include <parser.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

Blip::Blip(std::istream &in_stream, const char *hint)
    : m_parser(std::make_unique<Parser>(make_lexer(in_stream, hint))),
      m_top_value_env(default_value_environment()),
      m_top_type_env(default_type_environment()) {}

void Blip::rep() {
  m_parser->reset();

  std::unique_ptr<ProgramNode> ast{};

  try {
    ast = m_parser->parse();
  } catch (const std::runtime_error &exception) {
    std::cerr << "Error during parsing\n\n";
    std::cerr << exception.what() << "\n";
    return;
  }

  try {
    TypeChecker type_checker(m_top_type_env);
    type_checker.check(*ast);

    Evaluator evaluator(m_top_value_env, std::cout);
    evaluator.evaluate(*ast);
  } catch (const std::runtime_error &exception) {
    std::cerr
        << "Dumping AST on successful parse, but type or evaluator error!\n\n";
    AstDumper dumper;
    std::cerr << dumper.dump(*ast);
    std::cerr << exception.what() << "\n";
  }

  // Keep it around
  m_past_programs.push_back(std::move(ast));
}

//****************************************************************************
} // namespace blip
//****************************************************************************
