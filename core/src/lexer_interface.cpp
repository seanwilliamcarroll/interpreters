//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : LexerInterface class source file
//*
//*
//****************************************************************************

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include <sc/lexer_interface.hpp> // For Token classes
#include <sc/sc.hpp>              // For forward decls

//****************************************************************************
namespace sc {
//****************************************************************************
LexerInterface::LexerInterface(std::istream &in_stream,
                               const KeywordsMap &keywords,
                               const std::string &file_name)
    : m_in_stream(in_stream), m_keywords(keywords),
      m_current_loc(SourceLocation(file_name)) {}

LexerInterface::~LexerInterface() {}

//****************************************************************************
} // namespace sc
//****************************************************************************
