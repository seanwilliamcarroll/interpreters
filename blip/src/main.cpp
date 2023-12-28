//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : An example executable.
//*
//*
//****************************************************************************

#include <iostream>                                      // For cout
#include <sstream>                                       // For stringstream
#include <fstream>                                       // For fstream
#include <blip.hpp>                                      // For example

//****************************************************************************

void repl(std::iostream& input_stream, blip::Blip& blip_lang) {
  for (;;) {
    std::cout << "-> ";

    std::string next_line;
    std::getline(std::cin, next_line);

    input_stream << next_line;
    
    blip_lang.rep();
  }
}

int main(int argc, const char* argv[]) {

  if (argc == 1) {
    std::stringstream s;
    blip::Blip blip_lang(s);
    repl(s, blip_lang);
  } else if (argc == 2){
    // Load and run file
    std::fstream input_file;
    // Unsafe, but good for testing
    input_file.open(argv[1], std::fstream::in);
    
    blip::Blip blip_lang(input_file, argv[1]);

    blip_lang.rep();

    input_file.close();
  } else {
    std::cerr << "Usage: blip [script.bl]" << std::endl;
    return 1;
  }
  return 0;
}

//****************************************************************************
