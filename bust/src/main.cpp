//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Main driver of bust interpreter.
//*
//*
//****************************************************************************

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <bust.hpp>

//****************************************************************************

void repl() {
  for (;;) {
    std::cout << "bust> ";

    std::string next_line;
    std::getline(std::cin, next_line);

    if (next_line == "quit") {
      return;
    }

    std::istringstream line_stream(next_line);
    bust::Bust line_bust(line_stream);
    line_bust.rep();
  }
}

int main(int argc, const char *argv[]) {
  if (argc > 2) {
    std::cerr << "Usage: bust [script.bu]\n";
    return 1;
  }

  try {
    if (argc == 1) {
      repl();
    } else if (argc == 2) {
      std::fstream input_file;
      input_file.open(argv[1], std::fstream::in);
      bust::Bust bust_lang(input_file, argv[1]);
      bust_lang.rep();
      input_file.close();
    }
  } catch (std::runtime_error &run_err) {
    std::cerr << "Runtime error: " << run_err.what() << "\n";
    return 1;
  } catch (std::exception &exc) {
    std::cerr << "Exception: " << exc.what() << "\n";
    return 1;
  }

  return 0;
}

//****************************************************************************
