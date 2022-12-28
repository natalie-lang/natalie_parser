#include <time.h>

#include "fragments.hpp"
#include "natalie_parser/creator/debug_creator.hpp"
#include "natalie_parser/parser.hpp"

using namespace NatalieParser;

int passed = 0;
int failed = 0;

TM::String read_file(TM::String &path) {
    auto buf = TM::String();
    FILE *fp = fopen(path.c_str(), "r");
    assert(fp);
    char cbuf[1024];
    while (fgets(cbuf, 1024, fp))
        buf.append(cbuf);
    fclose(fp);
    return buf;
}

TM::String test_code(TM::String code, TM::String path = "(string)") {
    TM::SharedPtr<TM::String> code_ptr = new String { code };
    TM::SharedPtr<TM::String> file = new String { path };
    auto parser = Parser { code_ptr, file };
    auto tree = parser.tree();
    auto creator = DebugCreator {};
    tree->transform(&creator);
    auto result = creator.to_string();
    return result;
}

void test_code_with_syntax_error(TM::String code) {
    try {
        test_code(code);
    } catch (NatalieParser::Parser::SyntaxError &) {
        printf(".");
        return;
    }
    printf("\nExpected `%s' to raise a SyntaxError\n", code.c_str());
    abort();
}

void test_code_ignoring_syntax_errors(TM::String code) {
    try {
        test_code(code);
    } catch (NatalieParser::Parser::SyntaxError &) {
        // noop
    }
    printf(".");
}

void test_file(TM::String path, size_t expected_output_size) {
    printf("testing %s for memory errors\n", path.c_str());
    auto code = read_file(path);
    auto output = test_code(code, path);
    if (output.size() == expected_output_size) {
        printf(".");
    } else {
        printf("\nExpected output to be %zu bytes, but it was %zu bytes.\n", expected_output_size, output.size());
        printf("%s\n", output.c_str());
        abort();
    }
    printf("\n");
}

void test_fragments() {
    printf("testing fragments for memory errors\n");
    auto fragments = build_fragments();
    for (auto fragment : *fragments) {
        test_code(fragment);
        printf(".");
    }
    printf("\n");
    delete fragments;
}

void test_fragments_with_syntax_errors() {
    printf("testing with intentional syntax errors for memory errors\n");
    auto fragments = build_fragments();
    for (auto fragment : *fragments) {
        test_code_with_syntax_error(fragment + "\n^");
        printf(".");
    }
    printf("\n");
    delete fragments;
}

void test_fragments_with_fuzzing(int seed) {
    printf("fuzzing with seed %d\n", seed);
    char bad_chars[] = { '`', '~', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '-', '_', '+', '=', '[', ']', '{', '}', '|', '\\', '7', 'a', '<', '>', ',', '.', '/', '?', ' ', '\n', '\t', '\v' };
    auto fragments = build_fragments();
    srand(seed);
    for (auto fragment : *fragments) {
        if (fragment.size() == 0) continue;
        auto index = rand() % fragment.size();
        auto bad_char = bad_chars[rand() % sizeof(bad_chars)];
        fragment.insert(index, bad_char);
        if (getenv("DEBUG_FUZZ"))
            printf("frag = '%s'\n", fragment.c_str());
        test_code_ignoring_syntax_errors(fragment);
        printf(".");
    }
    printf("\n");
    delete fragments;
}

int main() {
    try {
        test_file("test/support/boardslam.rb", 4371);
        test_fragments();
    } catch (NatalieParser::Parser::SyntaxError &e) {
        printf("\nSyntaxError: %s\n", e.message());
        abort();
    }
    test_fragments_with_syntax_errors();
    srand(time(NULL));
    int seed = rand();
    // seed = 1106784158;
    test_fragments_with_fuzzing(seed);
    printf("\n");
    return 0;
}
