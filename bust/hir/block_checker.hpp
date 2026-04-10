//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Block checker — scope management and block type checking.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <vector>

#include <ast/nodes.hpp>
#include <hir/context.hpp>
#include <hir/nodes.hpp>
#include <hir/types.hpp>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct BlockChecker {
  Type get_statement_type(const Statement &);
  Block check_block(const ast::Block &);
  Block check_block_with_parameters(const std::vector<Identifier> &parameters,
                                    const ast::Block &ast_block);
  Block check_callable_body(const std::vector<Identifier> &parameters,
                            const Type &return_type,
                            const ast::Block &ast_body);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
