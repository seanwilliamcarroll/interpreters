//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Main entry point for blip interpreter
//*
//*
//****************************************************************************

#include <iostream>                                      // For cout
#include <string>                                        // For string
#include <blip.hpp>                                      // For Blip class
#include <sc/token.hpp>                                  // For Token

//****************************************************************************

int main(int argc, const char *argv[])
{
  if (argc == 1) {
    // TODO: Lauch blip REPL
    std::cout << "Launches blip REPL" << std::endl;
  } else if (argc == 2) {
    // TODO: Read in file, lex, parse, evaluate
    std::cout << "blip interprets file: " << std::string(argv[1]) << std::endl;
  } else {
    std::cerr << "Usage: blip [script]" << std::endl;
    return 1;
  }
  return 0;
}

//****************************************************************************
