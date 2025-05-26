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
    CString long_arg;
    U8String help;
    CString name;
    ArgumentType atype;
    ArgumentAction aaction;
    int64_t minval, maxval;
};

class ParseResult {};

class ArgParse {
public:
    explicit ArgParse(U8String description_) : description{move(description_)} {}

    void add_argument(Argument a);

    Optional<ParseResult> parse_args(int argc, const char **argv);

private:
    int process_long_argument(
        int argc, const char **argv, ParseResult &result, int i, CStringView current);
    [[noreturn]] void print_help_and_exit();
    const char *progname = nullptr;
    U8String description;
    bool store_posargs;

    Vector<Argument> arguments;
    Vector<CString> posargs;
};

void ArgParse::add_argument(Argument a) {
    // FIXME, check for dupes.
    arguments.emplace_back(move(a));
}

Optional<ParseResult> ArgParse::parse_args(int argc, const char **argv) {
    progname = argv[0];
    ParseResult result;
    for(int i = 1; i < argc; ++i) {
        CStringView current(argv[1]);
        if(current.is_empty()) {
            printf("Empty argument in argumentlist.\n");
            exit(1);
        }
        if(current == "--help" || current == "-h") {
            print_help_and_exit();
        }
        if(current.starts_with("--")) {
            const auto consumed_extra_args = process_long_argument(argc, argv, result, i, current);
            i += consumed_extra_args;
        } else if(current.starts_with("-")) {
            fprintf(stderr, "Short options not supported yet.\n");
            abort();
        } else {
            fprintf(stderr, "Positional arguments not supported yet.\n");
            abort();
        }
    }
    return Optional<ParseResult>(move(result));
}

int ArgParse::process_long_argument(
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
    exit(0);
}

} // namespace pystd2025

using namespace pystd2025;

int main(int argc, const char **argv) {
    ArgParse parser(U8String("Test application for command line parser."));
    auto result = parser.parse_args(argc, argv);
    return 0;
}
