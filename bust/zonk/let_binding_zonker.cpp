//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Let binding zonker implementation.
//*
//*
//****************************************************************************

#include "zonk/expression_zonker.hpp"
#include <zonk/let_binding_zonker.hpp>

//****************************************************************************
namespace bust::zonk {
//****************************************************************************

hir::LetBinding LetBindingZonker::zonk(hir::LetBinding let_binding) {
  auto zonker = ExpressionZonker{m_ctx};

  auto zonked_expression = zonker.zonk(std::move(let_binding.m_expression));

  auto zonked_identifier_expression = zonker(std::move(let_binding.m_variable));

  auto &zonked_identifier =
      std::get<hir::Identifier>(zonked_identifier_expression);

  return hir::LetBinding{{let_binding.m_location},
                         std::move(zonked_identifier),
                         std::move(zonked_expression)};
}

//****************************************************************************
} // namespace bust::zonk
//****************************************************************************
