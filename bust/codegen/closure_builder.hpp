//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Short-lived helper that emits the closure ABI for a single
//*            lambda with captures. Owns the interned env-struct type and
//*            the resolved list of captured arguments; drives IRBuilder to
//*            allocate the env, emit the body prologue, and package the
//*            fat pointer.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
