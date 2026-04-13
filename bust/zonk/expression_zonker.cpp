//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Expression zonker implementation.
//*
//*
//****************************************************************************

#include <variant>
#include <zonk/expression_zonker.hpp>

//****************************************************************************
namespace bust::zonk {
//****************************************************************************

hir::Expression ExpressionZonker::zonk(hir::Expression expression) {
  // TODO: Resolve expression.m_type, then visit the inner ExprKind
  auto zonked_kind = std::visit(*this, std::move(expression.m_expression));
  return {{expression.m_location}, expression.m_type, std::move(zonked_kind)};
}

hir::ExprKind ExpressionZonker::operator()(hir::Identifier identifier) {
  return identifier;
}

hir::ExprKind ExpressionZonker::operator()(hir::LiteralUnit literal) {
  return literal;
}

hir::ExprKind ExpressionZonker::operator()(hir::LiteralI8 literal) {
  return literal;
}

hir::ExprKind ExpressionZonker::operator()(hir::LiteralI32 literal) {
  return literal;
}

hir::ExprKind ExpressionZonker::operator()(hir::LiteralI64 literal) {
  return literal;
}

hir::ExprKind ExpressionZonker::operator()(hir::LiteralBool literal) {
  return literal;
}

hir::ExprKind ExpressionZonker::operator()(hir::LiteralChar literal) {
  return literal;
}

hir::ExprKind ExpressionZonker::operator()(std::unique_ptr<hir::Block> block) {
  return std::move(block);
}

hir::ExprKind
ExpressionZonker::operator()(std::unique_ptr<hir::IfExpr> if_expr) {
  return std::move(if_expr);
}

hir::ExprKind
ExpressionZonker::operator()(std::unique_ptr<hir::CallExpr> call_expr) {
  return std::move(call_expr);
}

hir::ExprKind
ExpressionZonker::operator()(std::unique_ptr<hir::BinaryExpr> binary_expr) {
  return std::move(binary_expr);
}

hir::ExprKind
ExpressionZonker::operator()(std::unique_ptr<hir::UnaryExpr> unary_expr) {
  return std::move(unary_expr);
}

hir::ExprKind
ExpressionZonker::operator()(std::unique_ptr<hir::ReturnExpr> return_expr) {
  return std::move(return_expr);
}

hir::ExprKind
ExpressionZonker::operator()(std::unique_ptr<hir::CastExpr> cast_expr) {
  return std::move(cast_expr);
}

hir::ExprKind
ExpressionZonker::operator()(std::unique_ptr<hir::LambdaExpr> lambda_expr) {
  return std::move(lambda_expr);
}

//****************************************************************************
} // namespace bust::zonk
//****************************************************************************
