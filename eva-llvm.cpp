
#include <string>
#include "/Users/alisedighi/Desktop/llvm/src/EvaLLVM.h"


int main(int argc, char const *argv[])
{

    std::string program = R"(
       (var VERSION 42)

       (begin
            (var VERSION "Hello")
            (printf "Version: %s\n\n" VERSION))

       (printf "version: %d\n" VERSION)


    )";


    EvaLLVM vm{};


    vm.exec(program);
    return 0;
}
