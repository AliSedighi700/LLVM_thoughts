
#include <string>
#include "/Users/alisedighi/Desktop/llvm/src/EvaLLVM.h"


int main(int argc, char const *argv[])
{

    std::string program = R"(
      (var X 42)

       (begin
            (var (X string) "Hello")
            (printf "X: %s\n\n" X))

       (printf "X: %d\n" X)

       (set X 100)
       (printf "X: %d\n" X)


    )";


    EvaLLVM vm{};


    vm.exec(program);
    return 0;
}
