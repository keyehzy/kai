#include "ast.h"
#include "bytecode.h"
#include "cxxopts.hpp"
#include "parser.h"

#include <algorithm>
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

enum class DumpTarget {
  Ast,
  Bytecode,
};

std::string to_lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return value;
}

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

std::optional<Backend> parse_backend(const std::string &raw) {
  const std::string value = to_lower(raw);
  if (value == "ast") {
    return Backend::Ast;
  }
  if (value == "bytecode") {
    return Backend::Bytecode;
  }
  return std::nullopt;
}

std::optional<DumpTarget> parse_dump_target(const std::string &raw) {
  const std::string value = to_lower(raw);
  if (value == "ast") {
    return DumpTarget::Ast;
  }
  if (value == "bytecode") {
    return DumpTarget::Bytecode;
  }
  return std::nullopt;
}

std::vector<std::string> normalize_cli_arguments(int argc, char **argv) {
  std::vector<std::string> args;
  args.reserve(static_cast<size_t>(argc));
  args.emplace_back(argv[0]);

  for (int i = 1; i < argc; ++i) {
    std::string current = argv[i];
    if (current == "--dump" && i + 1 < argc) {
      const std::string next = to_lower(argv[i + 1]);
      if (next == "ast" || next == "bytecode") {
        args.emplace_back("--dump=" + std::string(argv[i + 1]));
        ++i;
        continue;
      }
    }
    args.emplace_back(std::move(current));
  }

  return args;
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

void ensure_bytecode_program_returns_value(kai::ast::Ast::Block &program) {
  if (program.children.empty()) {
    return;
  }

  if (program.children.back()->type == kai::ast::Ast::Type::Return) {
    return;
  }

  auto last_statement = std::move(program.children.back());
  program.children.back() =
      std::make_unique<kai::ast::Ast::Return>(std::move(last_statement));
}

std::unique_ptr<kai::ast::Ast::Block> parse_program(const std::string &source) {
  kai::Parser parser(source);
  return parser.parse_program();
}

kai::ast::Value run_source(const std::string &source, Backend backend) {
  auto program = parse_program(source);

  if (backend == Backend::Ast) {
    kai::ast::AstInterpreter interpreter;
    return interpreter.interpret(*program);
  }

  ensure_bytecode_program_returns_value(*program);
  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter interpreter;
  return interpreter.interpret(generator.blocks());
}

void dump_source(const std::string &source, DumpTarget dump_target) {
  auto program = parse_program(source);
  if (dump_target == DumpTarget::Ast) {
    program->dump(std::cout);
    std::cout << "\n";
    return;
  }

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();
  generator.dump();
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

    std::cout << run_source(source, backend) << "\n";
  }
}

}  // namespace

int main(int argc, char **argv) {
  try {
    cxxopts::Options options("kai", "kai language CLI");
    options.positional_help("[file]");
    options.parse_positional({"file"});
    options.add_options()("dump",
                          "Dump AST or bytecode and exit (default: bytecode)",
                          cxxopts::value<std::string>()->implicit_value("bytecode"))(
        "interp", "Interpreter backend: ast or bytecode (default: bytecode)",
        cxxopts::value<std::string>()->default_value("bytecode"))(
        "h,help", "Show help")("file", "Input source file",
                               cxxopts::value<std::vector<std::string>>());

    const std::vector<std::string> normalized_args =
        normalize_cli_arguments(argc, argv);
    std::vector<const char *> argv_pointers;
    argv_pointers.reserve(normalized_args.size());
    for (const auto &arg : normalized_args) {
      argv_pointers.push_back(arg.c_str());
    }

    const auto result =
        options.parse(static_cast<int>(argv_pointers.size()), argv_pointers.data());

    if (result.count("help") != 0) {
      std::cout << options.help() << "\n";
      return 0;
    }

    const std::string interp_name = result["interp"].as<std::string>();
    const auto backend = parse_backend(interp_name);
    if (!backend.has_value()) {
      std::cerr << "error: invalid value for --interp: " << interp_name
                << " (expected ast or bytecode)\n";
      return 1;
    }

    std::optional<DumpTarget> dump_target = std::nullopt;
    if (result.count("dump") != 0) {
      const std::string dump_name = result["dump"].as<std::string>();
      dump_target = parse_dump_target(dump_name);
      if (!dump_target.has_value()) {
        std::cerr << "error: invalid value for --dump: " << dump_name
                  << " (expected ast or bytecode)\n";
        return 1;
      }
    }

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
      if (dump_target.has_value()) {
        dump_source(source, *dump_target);
        return 0;
      }

      std::cout << run_source(source, *backend) << "\n";
      return 0;
    }

    if (dump_target.has_value()) {
      std::cerr << "error: --dump requires an input file\n";
      return 1;
    }

    repl(*backend);
    return 0;
  } catch (const cxxopts::exceptions::exception &ex) {
    std::cerr << "error: " << ex.what() << "\n";
    return 1;
  } catch (const std::exception &ex) {
    std::cerr << "error: " << ex.what() << "\n";
    return 1;
  }
}
