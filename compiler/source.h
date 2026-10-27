#pragma once
// Source files and locations. Every token and AST node carries a Span so
// diagnostics can point at the exact place in the .rsh file.

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace rsharp {

struct Span {
    uint32_t offset = 0;  // byte offset into the file
    uint32_t length = 0;  // length in bytes
    uint32_t line = 1;    // 1-based
    uint32_t column = 1;  // 1-based, in bytes (UTF-8 aware columns come later)
};

class SourceFile {
public:
    SourceFile(std::string path, std::string text);

    const std::string& path() const { return path_; }
    std::string_view text() const { return text_; }

    // Returns the full text of the 1-based line (no trailing newline).
    std::string_view line_text(uint32_t line) const;

    // Loads a file from disk. Returns false if it cannot be read.
    static bool load(const std::string& path, SourceFile& out);

private:
    std::string path_;
    std::string text_;
    std::vector<uint32_t> line_starts_;  // byte offset where each line begins
};

}  // namespace rsharp
