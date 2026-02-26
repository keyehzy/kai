#include "optimizer_internal.h"

namespace kai {

using Type = Bytecode::Instruction::Type;

std::optional<Register> get_dst_reg(const Bytecode::Instruction &instr) {
  switch (instr.type()) {
    case Type::Move:
      return derived_cast<const Bytecode::Instruction::Move &>(instr).dst;
    case Type::Load:
      return derived_cast<const Bytecode::Instruction::Load &>(instr).dst;
    case Type::LessThan:
      return derived_cast<const Bytecode::Instruction::LessThan &>(instr).dst;
    case Type::LessThanImmediate:
      return derived_cast<const Bytecode::Instruction::LessThanImmediate &>(instr).dst;
    case Type::GreaterThan:
      return derived_cast<const Bytecode::Instruction::GreaterThan &>(instr).dst;
    case Type::GreaterThanImmediate:
      return derived_cast<const Bytecode::Instruction::GreaterThanImmediate &>(instr).dst;
    case Type::LessThanOrEqual:
      return derived_cast<const Bytecode::Instruction::LessThanOrEqual &>(instr).dst;
    case Type::LessThanOrEqualImmediate:
      return derived_cast<const Bytecode::Instruction::LessThanOrEqualImmediate &>(instr)
          .dst;
    case Type::GreaterThanOrEqual:
      return derived_cast<const Bytecode::Instruction::GreaterThanOrEqual &>(instr).dst;
    case Type::GreaterThanOrEqualImmediate:
      return derived_cast<const Bytecode::Instruction::GreaterThanOrEqualImmediate &>(instr)
          .dst;
    case Type::Equal:
      return derived_cast<const Bytecode::Instruction::Equal &>(instr).dst;
    case Type::EqualImmediate:
      return derived_cast<const Bytecode::Instruction::EqualImmediate &>(instr).dst;
    case Type::NotEqual:
      return derived_cast<const Bytecode::Instruction::NotEqual &>(instr).dst;
    case Type::NotEqualImmediate:
      return derived_cast<const Bytecode::Instruction::NotEqualImmediate &>(instr).dst;
    case Type::Add:
      return derived_cast<const Bytecode::Instruction::Add &>(instr).dst;
    case Type::AddImmediate:
      return derived_cast<const Bytecode::Instruction::AddImmediate &>(instr).dst;
    case Type::Subtract:
      return derived_cast<const Bytecode::Instruction::Subtract &>(instr).dst;
    case Type::SubtractImmediate:
      return derived_cast<const Bytecode::Instruction::SubtractImmediate &>(instr).dst;
    case Type::Multiply:
      return derived_cast<const Bytecode::Instruction::Multiply &>(instr).dst;
    case Type::MultiplyImmediate:
      return derived_cast<const Bytecode::Instruction::MultiplyImmediate &>(instr).dst;
    case Type::Divide:
      return derived_cast<const Bytecode::Instruction::Divide &>(instr).dst;
    case Type::DivideImmediate:
      return derived_cast<const Bytecode::Instruction::DivideImmediate &>(instr).dst;
    case Type::Modulo:
      return derived_cast<const Bytecode::Instruction::Modulo &>(instr).dst;
    case Type::ModuloImmediate:
      return derived_cast<const Bytecode::Instruction::ModuloImmediate &>(instr).dst;
    case Type::Call:
      return derived_cast<const Bytecode::Instruction::Call &>(instr).dst;
    case Type::ArrayCreate:
      return derived_cast<const Bytecode::Instruction::ArrayCreate &>(instr).dst;
    case Type::ArrayLiteralCreate:
      return derived_cast<const Bytecode::Instruction::ArrayLiteralCreate &>(instr).dst;
    case Type::ArrayLoad:
      return derived_cast<const Bytecode::Instruction::ArrayLoad &>(instr).dst;
    case Type::ArrayLoadImmediate:
      return derived_cast<const Bytecode::Instruction::ArrayLoadImmediate &>(instr).dst;
    case Type::StructCreate:
      return derived_cast<const Bytecode::Instruction::StructCreate &>(instr).dst;
    case Type::StructLiteralCreate:
      return derived_cast<const Bytecode::Instruction::StructLiteralCreate &>(instr).dst;
    case Type::StructLoad:
      return derived_cast<const Bytecode::Instruction::StructLoad &>(instr).dst;
    case Type::Negate:
      return derived_cast<const Bytecode::Instruction::Negate &>(instr).dst;
    case Type::LogicalNot:
      return derived_cast<const Bytecode::Instruction::LogicalNot &>(instr).dst;
    case Type::Jump:
    case Type::JumpConditional:
    case Type::JumpEqualImmediate:
    case Type::JumpGreaterThanImmediate:
    case Type::JumpLessThanOrEqual:
    case Type::TailCall:
    case Type::Return:
    case Type::ArrayStore:
      return std::nullopt;
  }
  return std::nullopt;
}

}  // namespace kai
