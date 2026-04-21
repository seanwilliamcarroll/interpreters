//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Mono dump utility for debugging.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/dump.hpp>
#include <mono/nodes.hpp>

//****************************************************************************
namespace bust::mono {
//****************************************************************************

class Dumper {
public:
  static std::string dump(const Program &program) {
    return hir::Dumper::dump(program.m_type_arena, program.m_top_items);
  }
};

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
