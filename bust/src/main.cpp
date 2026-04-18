//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Main driver of bust interpreter.
//*
//*
//****************************************************************************

#include <bust.hpp>

#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <stdexcept>

//****************************************************************************

int main(int argc, const char *argv[]) {
  bust::Options options;
  const char *filename = nullptr;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--dump-source") == 0) {
      options.dump_source = true;
    } else if (std::strcmp(argv[i], "--dump-ast") == 0) {
      options.dump_ast = true;
    } else if (std::strcmp(argv[i], "--dump-hir") == 0) {
      options.dump_hir = true;
    } else if (std::strcmp(argv[i], "--dump-mono") == 0) {
      options.dump_mono = true;
    } else if (std::strcmp(argv[i], "--dump-zir") == 0) {
      options.dump_zir = true;
    } else if (std::strcmp(argv[i], "--dump-llvm-ir") == 0) {
      options.dump_llvm_ir = true;
    } else if (argv[i][0] == '-') {
      std::cerr << "Unknown option: " << argv[i] << "\n";
      return 1;
    } else if (filename == nullptr) {
      filename = argv[i];
    } else {
      std::cerr << "Usage: bust [--dump-source] [--dump-ast] [--dump-hir] "
                   "[--dump-mono] [--dump-zir] [--dump-llvm-ir] <script.bu>\n";
      return 1;
    }
  }

  if (filename == nullptr) {
    std::cerr << "Usage: bust [--dump-ast] [--dump-hir] [--dump-mono] "
                 "[--dump-zir] [--dump-llvm-ir] <script.bu>\n";
    return 1;
  }

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
