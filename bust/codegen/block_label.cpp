//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Out-of-line definitions for BlockLabel.
//*
//*
//****************************************************************************

#include <codegen/basic_block.hpp>
#include <codegen/block_label.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

const std::string &BlockLabel::name() const { return m_block->label(); }

BlockLabel BlockLabel::null() { return BlockLabel{nullptr}; }

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
