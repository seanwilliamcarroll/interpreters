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

#include "environment.hpp"
#include "evaluator.hpp"
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
      m_top_env(default_value_environment()) {}

void Blip::rep() {
  m_parser->reset();
  try {
    auto ast = m_parser->parse();

    Evaluator evaluator(m_top_env, std::cout);
    evaluator.evaluate(*ast);
  } catch (const std::runtime_error &exception) {
    std::cerr << exception.what() << "\n";
  }
}

//****************************************************************************
} // namespace blip
//****************************************************************************
