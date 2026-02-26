#pragma once

#include "../optimizer.h"

#include <optional>

namespace kai {

using Register = Bytecode::Register;
std::optional<Register> get_dst_reg(const Bytecode::Instruction &instr);

}  // namespace kai
