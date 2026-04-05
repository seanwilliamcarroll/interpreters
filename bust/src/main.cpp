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

#include <cstring>
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
  bust::Mode mode = bust::Mode::RUN;
  const char *filename = nullptr;

  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--dump-ast") == 0) {
      mode = bust::Mode::DUMP_AST;
    } else if (std::strcmp(argv[i], "--dump-hir") == 0) {
      mode = bust::Mode::DUMP_HIR;
    } else if (std::strcmp(argv[i], "--eval") == 0) {
      mode = bust::Mode::EVAL;
    } else if (std::strcmp(argv[i], "--llvm-ir") == 0) {
      mode = bust::Mode::LLVM_IR;
    } else if (argv[i][0] == '-') {
      std::cerr << "Unknown option: " << argv[i] << "\n";
      return 1;
    } else if (filename == nullptr) {
      filename = argv[i];
    } else {
      std::cerr << "Usage: bust [--dump-ast|--dump-hir|--eval|--llvm-ir] "
                   "[script.bu]\n";
      return 1;
    }
  }

  try {
    if (filename == nullptr) {
      repl();
    } else {
      std::fstream input_file;
      input_file.open(filename, std::fstream::in);
      bust::Bust bust_lang(input_file, filename, mode);
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
