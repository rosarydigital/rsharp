#include "diagnostics.h"

#include <algorithm>
#include <ostream>

namespace rsharp {

void DiagnosticSink::error(Span span, std::string message, std::string help) {
    diags_.push_back({Severity::Error, std::move(message), span, std::move(help)});
    ++error_count_;
}

void DiagnosticSink::warning(Span span, std::string message, std::string help) {
    diags_.push_back({Severity::Warning, std::move(message), span, std::move(help)});
}

void DiagnosticSink::print(std::ostream& out) const {
    for (const Diagnostic& d : diags_) {
        const char* label = d.severity == Severity::Error     ? "error"
                            : d.severity == Severity::Warning ? "warning"
                                                              : "note";
        out << label << ": " << d.message << '\n';
        out << " --> " << file_.path() << ':' << d.span.line << ':' << d.span.column << '\n';

        std::string gutter = std::to_string(d.span.line);
        std::string pad(gutter.size(), ' ');
        out << pad << " |\n";
        out << gutter << " | " << file_.line_text(d.span.line) << '\n';
        out << pad << " | " << std::string(d.span.column > 0 ? d.span.column - 1 : 0, ' ')
            << std::string(std::max<uint32_t>(d.span.length, 1), '^') << '\n';
        if (!d.help.empty()) out << pad << " = help: " << d.help << '\n';
        out << '\n';
    }
}

}  // namespace rsharp
