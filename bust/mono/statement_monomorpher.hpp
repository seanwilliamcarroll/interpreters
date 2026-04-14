//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Statement-level monomorpher. Dispatches on the statement
//*            variant: expressions are cloned 1:1 via the expression
//*            substituter, while let bindings are handed off to the
//*            let binding monomorpher which may fan out into multiple
//*            specialized bindings.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

//****************************************************************************
namespace bust::mono {
//****************************************************************************

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
