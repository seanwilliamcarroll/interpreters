//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : LanguageInterface class source file
//*
//*
//****************************************************************************

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include <sc/language_interface.hpp> // For LanguageInterface classes
#include <sc/sc.hpp>                 // For forward decls

//****************************************************************************
namespace sc {
//****************************************************************************

LanguageInterface::LanguageInterface(std::unique_ptr<LexerInterface> lexer)
    : m_lexer(std::move(lexer)) {}

//****************************************************************************
} // namespace sc
//****************************************************************************
