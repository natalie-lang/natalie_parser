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

void test() {
    TM::SharedPtr<TM::String> code = read_file("test/boardslam.rb");
    TM::SharedPtr<TM::String> file = new String { "(string)" };
    auto parser = Parser { code, file };
    auto tree = parser.tree();
    auto creator = DebugCreator { tree };
    tree->transform(&creator);
    printf("%s\n", creator.to_string().c_str());
    delete tree;
}

int main() {
    test();
    return 0;
}
