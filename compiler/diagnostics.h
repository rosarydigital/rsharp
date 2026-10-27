#pragma once
// Compiler diagnostics (errors / warnings) with Rust/Elm-style output:
//
//   error: tabs are not allowed for indentation
//    --> hello.rsh:3:1
//     |
//   3 | 	print("hi")
//     | ^
//     = help: use 4 spaces

#include <iosfwd>
#include <string>
#include <vector>

#include "source.h"

namespace rsharp {

enum class Severity { Error, Warning, Note };

struct Diagnostic {
    Severity severity = Severity::Error;
    std::string message;
    Span span;
    std::string help;  // optional hint, empty if none
};

class DiagnosticSink {
public:
    explicit DiagnosticSink(const SourceFile& file) : file_(file) {}

    void error(Span span, std::string message, std::string help = {});
    void warning(Span span, std::string message, std::string help = {});

    bool has_errors() const { return error_count_ > 0; }
    size_t error_count() const { return error_count_; }
    const std::vector<Diagnostic>& all() const { return diags_; }

    // Pretty-print every diagnostic collected so far.
    void print(std::ostream& out) const;

private:
    const SourceFile& file_;
    std::vector<Diagnostic> diags_;
    size_t error_count_ = 0;
};

}  // namespace rsharp
