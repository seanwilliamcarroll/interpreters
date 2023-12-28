//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : LexerInterface abstract interface
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <memory>

#include <sc/sc.hpp>
#include <sc/token.hpp>

//****************************************************************************
namespace sc {
//****************************************************************************

class LexerInterface {
public:
  virtual std::unique_ptr<Token> get_next_token() = 0;
  virtual ~LexerInterface();

protected:
  LexerInterface(std::istream &in_stream, const KeywordsMap &keywords,
                 const std::string &file_name = "<input>");

  virtual KeywordsMap construct_keywords_map() const = 0;

  std::istream &m_in_stream;
  const KeywordsMap m_keywords;
  SourceLocation m_current_loc;
};

//****************************************************************************
} // namespace sc
//****************************************************************************
