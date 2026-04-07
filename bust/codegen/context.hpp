//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Shared context for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <string>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

/// Threaded through the codegen sub-visitors. Holds the output buffer being
/// built up and any cross-node state (fresh-name counters, current function,
/// etc.) that later features will need.
struct Context {
  std::string m_output{};
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
