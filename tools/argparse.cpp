// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <string.h>
#include <pystd2025.hpp>

namespace pystd2025 {

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
    ArgumentType atype;
    ArgumentAction aaction;
    Optional<int64_t> minval;
    Optional<int64_t> maxval;
};

struct ArgValue {
    size_t index;
    CString value;
};

struct ParseResult {
    pystd2025::Vector<ArgValue> values;
};

class ArgParse;

class ParseOutput {
public:
    ParseOutput() noexcept = default;
    ParseOutput(ParseOutput &&o) noexcept = default;
    ParseOutput(ParseResult pr_) : pr{pystd2025::move(pr_)} {}

    ParseOutput &operator=(ParseOutput &&o) noexcept = default;

    const CString *value_of(const ArgParse &parser, CStringView arg_name);

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

    int process_long_argument(
        int argc, const char **argv, ParseResult &result, int i, CStringView current);
    int process_short_argument(
        int argc, const char **argv, ParseResult &result, int i, CStringView current);
    [[noreturn]] void print_help_and_exit();
    const char *progname = nullptr;
    U8String description;
    bool store_posargs = false;

    Vector<Argument> arguments;
    Vector<CString> posargs;
};

void ArgParse::add_argument(Argument a) {
    // FIXME, check for dupes.
    arguments.push_back(move(a));
}

Optional<ParseOutput> ArgParse::parse_args(int argc, const char **argv) {
    progname = argv[0];
    ParseResult result;
    for(int i = 1; i < argc; ++i) {
        CStringView current(argv[i]);
        if(current.is_empty()) {
            printf("Empty argument in argumentlist.\n");
            exit(1);
        }
        if(current == "--help" || current == "-h") {
            print_help_and_exit();
        }
        if(current == "--") {
            // FIXME, end and store posargs.
            if(store_posargs) {
                for(int k = i + 1; k < argc; ++k) {
                    posargs.push_back(CString(argv[k]));
                }
            } else {
                if(i + 1 < argc) {
                    fprintf(stderr,
                            "Arguments after \"--\", but storing posargs is not permitted.\n");
                    exit(1);
                }
            }
        } else if(current.starts_with("--")) {
            const auto consumed_extra_args = process_long_argument(argc, argv, result, i, current);
            i += consumed_extra_args;
        } else if(current.starts_with("-")) {
            const auto consumed_extra_args = process_short_argument(argc, argv, result, i, current);
            i += consumed_extra_args;
        } else {
            if(store_posargs) {
                posargs.push_back(CString(current));
            } else {
                fprintf(stderr, "Unknown argument at location %d: %s\n", i, argv[i]);
                exit(1);
            }
        }
    }
    return Optional<ParseOutput>(ParseOutput(pystd2025::move(result)));
}

Optional<size_t> ArgParse::find_long_argument(CStringView optname) {
    for(size_t i = 0; i < arguments.size(); ++i) {
        if(arguments[i].long_arg == optname) {
            return i;
        }
    }
    return Optional<size_t>{};
}

int ArgParse::process_long_argument(
    int argc, const char **argv, ParseResult &result, int i, CStringView current) {
    CStringView keypart;
    CStringView valuepart;
    auto splitpoint = current.find('=');
    int extra_consumed_args = 0;
    if(splitpoint != (size_t)-1) {
        keypart = current.substr(0, splitpoint);
        valuepart = current.substr(splitpoint + 1);
    } else {
        keypart = current;
        if(i + 1 >= argc) {
            fprintf(stderr, "Last entry on the command line would need a further argument.\n");
            exit(1);
        }
        valuepart = argv[i + 1];
        extra_consumed_args = 1;
    }

    const auto opt_result = find_long_argument(keypart);
    if(!opt_result) {
        fprintf(stderr, "Unknown argument: %s\n", argv[i]);
        exit(1);
    }
    const auto match_index = opt_result.value();
    result.values.push_back(ArgValue{match_index, valuepart});

    return extra_consumed_args;
}

int ArgParse::process_short_argument(
    int argc, const char **argv, ParseResult &result, int i, CStringView current) {
    CStringView keypart;
    CStringView valuepart;
    auto splitpoint = current.find('=');
    if(splitpoint != (size_t)-1) {
        keypart = current.substr(0, splitpoint);
        valuepart = current.substr(splitpoint + 1);
    } else {
        keypart = current;
    }

    return 0;
}

void ArgParse::print_help_and_exit() {
    printf("usage: %s [-h]\n", progname);
    printf("\n");
    if(!description.is_empty()) {
        printf("%s\n", description.c_str());
    }
    if(!arguments.is_empty()) {
        printf("\nArguments\n\n");
    }
    for(const auto &a : arguments) {
        if(!a.long_arg.is_empty()) {
            printf("%s ", a.long_arg.c_str());
        }
        if(a.short_arg != 0) {
            printf("-%c ", a.short_arg);
        }
        if(!a.help.is_empty()) {
            printf("          %s\n", a.help.c_str());
        }
    }
    exit(0);
}

const CString *ParseOutput::value_of(const ArgParse &parser, CStringView arg_name) {
    for(const auto &s : pr.values) {
        if(parser.arguments[s.index].name == arg_name) {
            return &s.value;
        }
    }
    return nullptr;
}

} // namespace pystd2025

using namespace pystd2025;

int main(int argc, const char **argv) {
    ArgParse parser(U8String("Test application for command line parser."));
    Argument foo;
    foo.long_arg = "--foo";
    foo.name = "foo";
    foo.help = pystd2025::U8String("The foo value to use.");
    parser.add_argument(foo);
    Argument bar;
    bar.long_arg = "--bar";
    bar.name = "bar";
    bar.help = pystd2025::U8String("The bar to barnicate.");
    parser.add_argument(bar);
    auto result = pystd2025::move(parser.parse_args(argc, argv).value());

    const auto *r1 = result.value_of(parser, "foo");
    printf("Foo: %s\n", r1 ? r1->c_str() : "undef");
    const auto *r2 = result.value_of(parser, "bar");
    printf("Bar: %s\n", r2 ? r2->c_str() : "undef");
    return 0;
}
