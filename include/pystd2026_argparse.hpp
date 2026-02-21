// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2026.hpp>

namespace pystd2026 {

typedef Variant<bool, int64_t, CString, Vector<CString>> ArgumentValue;

enum class ArgumentType : uint8_t {
    String,
    StringArray,
    Boolean,
    Integer,
};

enum class ArgumentAction : uint8_t {
    Store,
    StoreTrue,
    StoreFalse,
    AppendArray,
};

struct Argument {
    char short_arg = 0;
    uint32_t nargs = 1;
    CString long_arg;
    U8String help;
    CString name;
    ArgumentType type = ArgumentType::String;
    Optional<ArgumentValue> default_value;
    ArgumentAction aaction = ArgumentAction::Store;
    Optional<int64_t> minval;
    Optional<int64_t> maxval;

    bool needs_parameter() const {
        return !(aaction == ArgumentAction::StoreTrue || aaction == ArgumentAction::StoreFalse);
    }
};

struct ArgValue {
    CString name;
    ArgumentValue v;
};

struct ParseResult {
    Vector<ArgValue> values;
    Vector<CString> posargs;
    Vector<CString> extra_args;

    ArgValue *find(CStringView csv) {
        for(auto &v : values) {
            if(v.name == csv) {
                return &v;
            }
        }
        return nullptr;
    }
};

class ArgParse;

class ParseOutput {
public:
    ParseOutput() noexcept = default;
    ParseOutput(ParseOutput &&o) noexcept = default;
    ParseOutput(ParseResult pr_) : pr{pystd2026::move(pr_)} {}

    ParseOutput &operator=(ParseOutput &&o) noexcept = default;

    const ArgumentValue *value_of(CStringView arg_name) const;
    ArgumentValue *value_of(CStringView arg_name);

private:
    ParseResult pr;
};

class ArgParse {
public:
    explicit ArgParse(U8String description_) : description{move(description_)} {}

    void add_argument(Argument a);

    Optional<ParseOutput> parse_args(int argc, const char **argv);

    void permit_posargs() { store_posargs = true; }

private:
    friend class ParseOutput;
    Optional<size_t> find_long_argument(CStringView optname);
    Optional<size_t> find_short_argument(char c);

    ParseResult create_value_obj() const;

    int process_long_argument(
        int argc, const char **argv, ParseResult &result, int i, CStringView current);
    int process_short_argument(
        int argc, const char **argv, ParseResult &result, int i, CStringView current);
    [[noreturn]] void print_help_and_exit();

    void argument_matched(ParseResult &pr, size_t match_index, CStringView value);

    const char *progname = nullptr;
    U8String description;
    bool store_posargs = false;

    Vector<Argument> arguments;
    Vector<CString> posargs;
    Vector<CString> extra_args; // Those after a "--".
};

} // namespace pystd2026
