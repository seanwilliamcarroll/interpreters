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
    : m_parser(
          std::make_unique<Parser>(make_lexer(in_stream,
                                              {{"if", TokenType::IF},
                                               {"while", TokenType::WHILE},
                                               {"set", TokenType::SET},
                                               {"begin", TokenType::BEGIN},
                                               {"print", TokenType::PRINT},
                                               {"define", TokenType::DEFINE}},
                                              hint))) {}

void Blip::rep() {
  m_parser->reset();
  try {
    auto ast = m_parser->parse();
    std::cout << "Parsed program with " << ast->get_program().size()
              << " top level expressions\n";
    AstPrinter printer;
    std::cout << printer.print(*ast);
  } catch (const std::runtime_error &exception) {
    std::cerr << exception.what() << "\n";
  }
}

//****************************************************************************
} // namespace blip
//****************************************************************************
