#include "shape.h"

namespace kai {

std::string_view describe(Shape::Kind kind) {
  switch (kind) {
    case Shape::Kind::Unknown:        return "Unknown";
    case Shape::Kind::Non_Struct:     return "Non_Struct";
    case Shape::Kind::Struct_Literal: return "Struct_Literal";
    case Shape::Kind::Array:          return "Array";
    case Shape::Kind::Function:       return "Function";
  }
}

}  // namespace kai
