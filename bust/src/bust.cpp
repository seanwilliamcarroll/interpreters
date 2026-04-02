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

#include <bust.hpp>

//****************************************************************************

namespace bust {

Bust::Bust(std::istream &input, const char *filename)
    : m_input(input), m_filename(filename) {}

void Bust::rep() {
  // TODO: lex, parse, evaluate
  (void)m_input;
  (void)m_filename;
}

} // namespace bust

//****************************************************************************
