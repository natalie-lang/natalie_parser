#include "natalie_parser/creator/debug_creator.hpp"
#include "natalie_parser/parser.hpp"

using namespace NatalieParser;

TM::String *read_file(const char *path) {
    auto buf = new TM::String;
    FILE *fp = fopen(path, "r");
    assert(fp);
    char cbuf[1024];
    while (fgets(cbuf, 1024, fp))
        buf->append(cbuf);
    fclose(fp);
    return buf;
}

void test_boardslam() {
    TM::SharedPtr<TM::String> code = read_file("test/boardslam.rb");
    TM::SharedPtr<TM::String> file = new String { "test/boardslam.rb" };
    auto parser = Parser { code, file };
    auto tree = parser.tree();
    auto creator = DebugCreator {};
    tree->transform(&creator);
    auto output = creator.to_string();
    size_t expected_output_size = 4363;
    if (output.size() != expected_output_size) {
        printf("Expected output to be %zu bytes, but it was %zu bytes.\n", expected_output_size, output.size());
        printf("%s\n", output.c_str());
        abort();
    }
    delete tree;
}

int main() {
    test_boardslam();
    return 0;
}
