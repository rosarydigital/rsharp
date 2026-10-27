// rsharp: the R# compiler driver. Also installed as `rsh`.
//
// Commands (most are stubs until their milestone lands):
//   rsharp version
//   rsharp build <file.rsh> [-o out] [--release]    (M6)
//   rsharp run   <file.rsh>                         (M6)
//   rsharp lex   <file.rsh>                         (M1, debugging aid)

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "diagnostics.h"
#include "lexer.h"
#include "source.h"
#include "token.h"

namespace {

constexpr const char* kVersion = "0.2.0-dev";

void print_usage() {
    std::cout << "R# (Rosary Sharp) compiler " << kVersion << "\n\n"
              << "Usage:\n"
              << "  rsharp version\n"
              << "  rsharp lex   <file.rsh>     print the token stream (debugging aid)\n"
              << "  rsharp build <file.rsh>     compile to a native executable (coming in M6)\n"
              << "  rsharp run   <file.rsh>     build and run (coming in M6)\n";
}

int not_implemented(std::string_view cmd, std::string_view milestone) {
    std::cerr << "rsharp " << cmd << ": not implemented yet (" << milestone << ")\n";
    return 2;
}

// `rsharp lex file.rsh`: print one token per line as  line:col  Kind  "text"
int run_lex(const rsharp::SourceFile& file) {
    rsharp::DiagnosticSink diags(file);
    rsharp::Lexer lexer(file, diags);
    std::vector<rsharp::Token> tokens = lexer.tokenize();

    for (const rsharp::Token& t : tokens) {
        std::cout << t.span.line << ':' << t.span.column << "\t" << rsharp::token_kind_name(t.kind);
        bool structural = t.kind == rsharp::TokenKind::Newline || t.kind == rsharp::TokenKind::Indent ||
                          t.kind == rsharp::TokenKind::Dedent || t.kind == rsharp::TokenKind::Eof;
        if (!structural) std::cout << "\t" << t.text;
        std::cout << '\n';
    }
    if (diags.has_errors()) {
        std::cerr << '\n';
        diags.print(std::cerr);
        std::cerr << diags.error_count() << " error(s)\n";
        return 1;
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    std::string_view cmd = argv[1];

    if (cmd == "version" || cmd == "--version" || cmd == "-v") {
        std::cout << "rsharp " << kVersion << "\n";
        return 0;
    }
    if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        print_usage();
        return 0;
    }

    if (cmd == "lex" || cmd == "build" || cmd == "run") {
        if (argc < 3) {
            std::cerr << "rsharp " << cmd << ": missing input file\n";
            return 1;
        }
        rsharp::SourceFile file("", "");
        if (!rsharp::SourceFile::load(argv[2], file)) {
            std::cerr << "rsharp: cannot read '" << argv[2] << "'\n";
            return 1;
        }
        if (cmd == "lex") return run_lex(file);
        return not_implemented(cmd, "milestone M6: CLI + C backend");
    }

    std::cerr << "rsharp: unknown command '" << cmd << "'\n\n";
    print_usage();
    return 1;
}
