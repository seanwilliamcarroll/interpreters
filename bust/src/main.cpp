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

#include <bust.hpp>

//****************************************************************************

int main(int argc, const char *argv[]) {
  bust::Options options;
  const char *filename = nullptr;
  bool has_llvm_ir = false;

  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--dump-ast") == 0) {
      options.dump_ast = true;
    } else if (std::strcmp(argv[i], "--dump-hir") == 0) {
      options.dump_hir = true;
    } else if (std::strcmp(argv[i], "--llvm-ir") == 0) {
      has_llvm_ir = true;
    } else if (argv[i][0] == '-') {
      std::cerr << "Unknown option: " << argv[i] << "\n";
      return 1;
    } else if (filename == nullptr) {
      filename = argv[i];
    } else {
      std::cerr << "Usage: bust [--dump-ast] [--dump-hir] [--llvm-ir] "
                   "<script.bu>\n";
      return 1;
    }
  }

  if (filename == nullptr) {
    std::cerr << "Usage: bust [--dump-ast] [--dump-hir] [--llvm-ir] "
                 "<script.bu>\n";
    return 1;
  }

  options.llvm_ir = has_llvm_ir;

  try {
    std::ifstream input_file(filename);
    bust::Bust bust_lang(input_file, filename, options);
    bust_lang.run();
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
