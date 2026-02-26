#include "ast.h"
#include "bytecode.h"
#include "cxxopts.hpp"
#include "optimizer.h"
#include "parser.h"
#include "typechecker.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

enum class Backend {
  Ast,
  Bytecode,
};

std::string trim(std::string_view input) {
  size_t begin = 0;
  while (begin < input.size() &&
         std::isspace(static_cast<unsigned char>(input[begin])) != 0) {
    ++begin;
  }
  size_t end = input.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(input[end - 1])) != 0) {
    --end;
  }
  return std::string(input.substr(begin, end - begin));
}

std::string read_file(const std::string &path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open file: " + path);
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

void ensure_bytecode_program_returns_value(kai::Ast::Block &program) {
  if (program.children.empty()) {
    return;
  }

  if (program.children.back()->type == kai::Ast::Type::Return) {
    return;
  }

  auto last_statement = std::move(program.children.back());
  program.children.back() =
      std::make_unique<kai::Ast::Return>(std::move(last_statement));
}

void print_errors(const std::string &source, const kai::ErrorReporter &reporter) {
  for (const auto &error : reporter.errors()) {
    if (error->location.begin != nullptr) {
      const auto lc = kai::line_column(source, error->location.begin);
      std::cerr << lc.line << ":" << lc.column << ": ";
    }
    std::cerr << "error: " << error->format_error() << "\n";
  }
}

std::optional<kai::Value> run_source(const std::string &source, Backend backend) {
  kai::ErrorReporter reporter;
  kai::Parser parser(source, reporter);
  auto program = parser.parse_program();

  if (reporter.has_errors()) {
    print_errors(source, reporter);
    return std::nullopt;
  }

  kai::TypeChecker checker(reporter);
  checker.visit_program(*program);

  if (reporter.has_errors()) {
    print_errors(source, reporter);
    return std::nullopt;
  }

  if (backend == Backend::Ast) {
    kai::AstInterpreter interpreter;
    return interpreter.interpret(*program);
  }

  ensure_bytecode_program_returns_value(*program);
  kai::BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  kai::BytecodeOptimizer optimizer;
  optimizer.optimize(generator.blocks());

  kai::BytecodeInterpreter interpreter;
  return interpreter.interpret(generator.blocks());
}

bool dump_source(const std::string &source, Backend backend) {
  kai::ErrorReporter reporter;
  kai::Parser parser(source, reporter);
  auto program = parser.parse_program();

  if (reporter.has_errors()) {
    print_errors(source, reporter);
    return false;
  }

  kai::TypeChecker checker(reporter);
  checker.visit_program(*program);

  if (reporter.has_errors()) {
    print_errors(source, reporter);
    return false;
  }

  if (backend == Backend::Ast) {
    program->dump(std::cout);
    std::cout << "\n";
    return true;
  }

  kai::BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  kai::BytecodeOptimizer optimizer;
  optimizer.optimize(generator.blocks());
  generator.dump();
  return true;
}

std::string normalize_repl_input(std::string_view line) {
  std::string normalized = trim(line);
  if (normalized.empty()) {
    return {};
  }

  const char last = normalized.back();
  if (last != ';' && last != '{' && last != '}') {
    normalized.push_back(';');
  }
  return normalized;
}

void repl(Backend backend) {
  std::string source;
  std::string line;
  int brace_depth = 0;

  while (true) {
    std::cout << (brace_depth > 0 ? "... " : ">>> ");
    if (!std::getline(std::cin, line)) {
      std::cout << "\n";
      break;
    }

    const std::string normalized = normalize_repl_input(line);
    if (normalized.empty()) {
      continue;
    }

    const std::string previous_source = source;
    const int previous_brace_depth = brace_depth;
    if (!source.empty()) {
      source.push_back('\n');
    }
    source += normalized;

    for (char ch : normalized) {
      if (ch == '{') {
        ++brace_depth;
      } else if (ch == '}') {
        --brace_depth;
      }
    }

    if (brace_depth < 0) {
      std::cerr << "error: unmatched closing brace\n";
      brace_depth = 0;
      continue;
    }

    if (brace_depth > 0) {
      continue;
    }

    const auto value = run_source(source, backend);
    if (!value.has_value()) {
      source = previous_source;
      brace_depth = previous_brace_depth;
      continue;
    }
    std::cout << *value << "\n";
  }
}

}  // namespace

int main(int argc, char **argv) {
  try {
    cxxopts::Options options("kai", "kai language CLI");
    options.positional_help("[file]");
    options.parse_positional({"file"});
    options.add_options()
        ("ast", "Use the AST interpreter backend")
        ("bytecode", "Use the bytecode interpreter backend (default)")
        ("dump", "Dump the representation for the active backend and exit")
        ("h,help", "Show help")
        ("file", "Input source file", cxxopts::value<std::vector<std::string>>());

    const auto result = options.parse(argc, argv);

    if (result.count("help") != 0) {
      std::cout << options.help() << "\n";
      return 0;
    }

    const bool use_ast = result.count("ast") != 0;
    const bool use_bytecode = result.count("bytecode") != 0;

    if (use_ast && use_bytecode) {
      std::cerr << "error: --ast and --bytecode are mutually exclusive\n";
      return 1;
    }

    const Backend backend = use_ast ? Backend::Ast : Backend::Bytecode;
    const bool do_dump = result.count("dump") != 0;

    std::vector<std::string> files;
    if (result.count("file") != 0) {
      files = result["file"].as<std::vector<std::string>>();
    }

    if (files.size() > 1) {
      std::cerr << "error: expected at most one input file\n";
      return 1;
    }

    if (files.size() == 1) {
      const std::string source = read_file(files[0]);
      if (do_dump) {
        return dump_source(source, backend) ? 0 : 1;
      }

      const auto value = run_source(source, backend);
      if (!value.has_value()) {
        return 1;
      }
      std::cout << *value << "\n";
      return 0;
    }

    if (do_dump) {
      std::cerr << "error: --dump requires an input file\n";
      return 1;
    }

    repl(backend);
    return 0;
  } catch (const cxxopts::exceptions::exception &ex) {
    std::cerr << "error: " << ex.what() << "\n";
    return 1;
  } catch (const std::exception &ex) {
    std::cerr << "error: " << ex.what() << "\n";
    return 1;
  }
}
