#include "source.h"

#include <fstream>
#include <sstream>

namespace rsharp {

SourceFile::SourceFile(std::string path, std::string text)
    : path_(std::move(path)), text_(std::move(text)) {
    line_starts_.push_back(0);
    for (uint32_t i = 0; i < text_.size(); ++i) {
        if (text_[i] == '\n') line_starts_.push_back(i + 1);
    }
}

std::string_view SourceFile::line_text(uint32_t line) const {
    if (line == 0 || line > line_starts_.size()) return {};
    uint32_t start = line_starts_[line - 1];
    uint32_t end = (line < line_starts_.size()) ? line_starts_[line] : static_cast<uint32_t>(text_.size());
    std::string_view v(text_);
    v = v.substr(start, end - start);
    while (!v.empty() && (v.back() == '\n' || v.back() == '\r')) v.remove_suffix(1);
    return v;
}

bool SourceFile::load(const std::string& path, SourceFile& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    out = SourceFile(path, ss.str());
    return true;
}

}  // namespace rsharp
