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
    [[noreturn]] void print_help_and_exit();
    const char *progname = nullptr;
    U8String description;
    bool store_posargs;

    Vector<Argument> arguments;
    Vector<CString> posargs;
};

Optional<ParseResult> ArgParse::parse_args(int argc, const char **argv) {
    progname = argv[0];
    for(int i = 1; i < argc; ++i) {
        CStringView current(argv[1], 0, strlen(argv[1]));
        if(current.is_empty()) {
            printf("Empty argument in argumentlist.\n");
            exit(1);
        }
        if(current == "--help" || current == "-h") {
            print_help_and_exit();
        }
        if(current.starts_with("--")) {
            fprintf(stderr, "Long options not supported yet.\n");
            abort();
        } else if(current.starts_with("-")) {
            fprintf(stderr, "Short options not supported yet.\n");
            abort();
        } else {
            fprintf(stderr, "Positional arguments not supported yet.\n");
            abort();
        }
    }
    return Optional<ParseResult>();
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
