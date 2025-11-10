#ifndef EvaLLVM_h
#define EvaLLVM_h

#include <string>
#include <regex>
#include <map>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"

#include "./Environment.h"
#include "./parser/EvaGrammar.h"


using syntax::EvaGrammar;
using Env = std::shared_ptr<Environment>;

class EvaLLVM
{
public:

    EvaLLVM() : parser(std::make_unique<EvaGrammar>())
    {
        moduleInit();
        setupExternalFunctions();
        setupGlobalEnvironment();
    }

    void exec(const std::string& program)
    {

        //1. pars the program
        auto ast = parser->parse("(begin" + program + ")");


        //2. Compile to LLVM IR
        //for the starting point we do not have the
        //ast so lets just return 42 like before
        compile(ast);

        //print generated code
        module -> print(llvm::outs(), nullptr);

        //3.save module IR to file
        saveModuleToFile("./out.ll");

    }

private:

    void compile (const Exp& ast)
    {
        //1.create main function
        fn = createFunction("main",
                llvm::FunctionType::get(/*return type*/builder->getInt32Ty(),
                                        /*vararg*/ false ), GlobalEnv);

        //we always start our code generation in the global
        //environment so we pass the GlobalEnv first to
        //creatfunction because function are also symbols
        //so we have to install those symbols into the
        //enviroment


        //createGlobalVar("VERSION", builder->getInt32(42));

        //llvm:FunctionType::get has multiple ctor one of them just get
        //the retunr type.

        //2.compile main function
        //the gen function accept any AST and will be
        //the recursive compiler (*******)
        gen(ast, GlobalEnv);


        //3. when the result is ready we should
        //return it. since we wanna return 42,
        //we should cast what ever the result is
        //to int.
        //auto i32Result = // we commented that because it will cast str to int
            //builder -> CreateIntCast(result, builder->getInt32Ty(), true);
        // the boolian if for withe rthe i32Result is signed.

        //create ret instruction
        builder -> CreateRet(builder->getInt32(0));
    }


    //now lets introduce our gen method(*******)
    //the result of the gen function should be LLVM value.
    //this is a very basic type which can accept pretty much
    //everything might be numbers, string , etc
    //here the result is a ineger number
    llvm::Value *gen(const Exp& exp, Env env)
    {
        switch(exp.type)
        {
            //Numbers
            case ExpType::NUMBER:
                return builder->getInt32(exp.number);
            case ExpType::STRING:{
                auto re = std::regex("\\\\n");
                auto str = std::regex_replace(exp.string, re, "\n");
                return builder->CreateGlobalString(str);
            }
            case ExpType::SYMBOL:
                //variables, operators
                if(exp.string == "true" || exp.string == "false")
                {
                    return builder -> getInt1(exp.string == "true" ? true : false);
                }else //for variables
                {
                    //since the value now can be update
                    //we cannot use the initializer in the
                    //global environment. we should load
                    //the vraible into local variable.

                    auto varName = exp.string;
                    auto value = env -> lookup(varName);

                    //However the vriable could be a funcion or
                    //global variable so we should use the dynamic castig
                    //from llvm. So if it will be global variable we will
                    //loaded onto the stack using createLoad


                    //local variables are handeled with llvm::AllocaInst
                    //which is allocatio on stack.
                    //in contrast to the global variables which are handeled
                    //with llvm::GlobalVariable.
                    //so from out environment if we are able to cast the variable
                    //to a local variable, we should load the variable from the stack.
                    if(auto localVar = llvm::dyn_cast<llvm::AllocaInst>(value))
                    {
                        return builder->CreateLoad(localVar->getAllocatedType(), localVar,
                                                   varName.c_str());
                    }
                    else if(auto globalVar = llvm::dyn_cast<llvm::GlobalVariable>(value))
                    {
                        return builder -> CreateLoad(globalVar->getInitializer()->getType(),
                                                     globalVar, varName.c_str());
                    }
                }
             case ExpType::LIST:
                auto tag = exp.list[0];
                if(tag.type == ExpType::SYMBOL)
                {
                    auto op = tag.string;

                    //lets annotating our variables with types
                    //we will use i32 for all numbers
                    //variable decleration (var x (+ y 10))
                    //or we can annotate a vraible name or
                    //a parameter with a type (var (x number) 42)
                    //so the first argument is the name and the
                    //second is the type.
                    if(op == "var")
                    {
                        //now we should take the name
                        //and extract the name and the type
                        //with extractVarName
                        auto varNameDecl = exp.list[1];
                        auto varName = extractVarName(varNameDecl);

                        auto init = gen(exp.list[2], env);

                        auto varType = extractVarType(varNameDecl);

                        //once we extract the name and the type we
                        //should allocate the variable.
                        auto varBinding = allocVar(varName, varType, env);

                        //Then after allocation we should store the it's
                        //initializer to the binding
                        return builder->CreateStore(init, varBinding);


                    }
                    else if(op == "set")
                    {
                        auto value = gen(exp.list[2], env);
                        auto varName = exp.list[1].string;
                        auto varBinding = env->lookup(varName);

                        return builder->CreateStore(value, varBinding);
                    }
                    else if(op == "begin")
                    {

                        auto blockEnv = std::make_shared<Environment>(
                                        std::map<std::string, llvm::Value*>{}, env);

                        //compile each experssion within the block.
                        //result is the last evaluated expression.

                        llvm::Value* blockRes;

                        for(auto i = 1 ; i < exp.list.size(); ++i)
                        {
                            blockRes = gen(exp.list[i], blockEnv);
                        }
                        return blockRes;
                    }
                    else if(op == "printf")
                    {
                        auto printn = module -> getFunction("printf");
                        std::vector<llvm::Value*> args{};

                        for(auto i = 1; i < exp.list.size(); ++i)
                        {
                            args.push_back(gen(exp.list[i], env));
                        }
                        return builder -> CreateCall(printn, args);
                    }
                }
        }

        return builder-> getInt32(0);

    }


    //exract var or parameter name considering type.
    // x -> x
    // (x number) -> x
    std::string extractVarName(const Exp& exp)
    {
        return exp.type == ExpType::LIST ? exp.list[0].string : exp.string;
    }


    //extract var or parameter type with i32 as the default
    // x -> i32
    // (x number) -> number
    llvm::Type* extractVarType(const Exp& exp, llvm::Value* initVal = nullptr)
    {
        return exp.type == ExpType::LIST ? getTypeFromString(exp.list[1].string)
                                        : builder -> getInt32Ty();
    }

    llvm::Type* getTypeFromString(const std::string& type_)
    {
        //number  -> i32
        if(type_ == "number")
        {
            return builder -> getInt32Ty();
        }

        //string -> i8* (aka char*)
        if(type_ == "string")
        {
            //getcontext in llvm20+ gives i8*
            return llvm::PointerType::get(builder->getContext(),0);
        }

        return builder -> getInt32Ty();
    }

    llvm::Value* allocVar(const std::string& name, llvm::Type* type_, Env env)
    {
        //All the local variables shoudl be allocated at the begening of the
        //stack frame.
        //Since the builder is emiting code, it might be already somewhere deeper
        //in the bytecode. So to emit at the begining of function we introduce our
        //second builder ->varsBuilder

        //explicitly say the varBuilder to insert point at
        // at the begining of the function
        varsBuilder -> SetInsertPoint(&fn->getEntryBlock());

        auto varAlloc = varsBuilder -> CreateAlloca(type_, nullptr, name.c_str());

        //add to the environment
        env->define(name, varAlloc);

        return varAlloc;
    }


    //create global variables
    llvm::GlobalVariable* createGlobalVar(const std::string& name,
                                          llvm::Constant* init)
    {
        module -> getOrInsertGlobal(name, init->getType());
        auto variable = module -> getNamedGlobal(name);
        variable -> setAlignment(llvm::MaybeAlign(4));
        variable -> setConstant(false);
        variable -> setInitializer(init);
        return variable;
    }

    //Now we said that we can distinguish the function from the
    //function declation and the definition. so if a external
    //funcion comes from another module and so on. We should
    //be possible to define an external function.

    void setupExternalFunctions()
    {
        //from stl, printf accepts the format string,
        //which is the null terminated charcter pointer string
        //and the char class from C++ us just an alias for
        //the byte.
        //The way we do that in llvm is exactly the same.
        //we create an extra type like
        //auto bytePtrTry = builder -> getInt8Ty() -> getPointerTo();
        auto bytePtrTry = llvm::PointerType::get(builder->getContext(),0);


        //to declare a function like printf we use getOrInsertFunction.
        module -> getOrInsertFunction("printf",
                //int printf(const char* format, ...)
                llvm::FunctionType::get(
                    /*return type*/ builder->getInt32Ty(),
                    /*format arg*/ bytePtrTry,
                    /*vararg*/true));
    }





    //cerateFunction:
    //Remembber the gloal is to see how the llvm compiles a .ll file.
    //we said we need three things for that.
    //1. module , 2.context , 3.IR builder
    //we also said that inside of the module we have
    //Function Definitions
    //now we should define the CreateFunction function


    //WHAT WE NEED TO CREATE A FUNCTION?
    //
    //1. function type.
    //   what is function type? It is the type with parameters and
    //   return type.
    //
    //2. Entry Basics Block.
    //   which create the function body.
    //   We can think about a Block as a list of instructions:
    //   A function has always the entry block. We will use an
    //   explicit (entry:)label for the block.
    //   A function can have one or multi blocks. Some of the blocks
    //   may contain the so-called TERMINATOR INSTRUCTIONS,
    //   such as branch(if-else, switch, .) which can be
    //   transit to other blocks. So in general A functiin
    //   is a n abstration which has arguments, entry blocks,
    //   and multiple optional blocks.
    //
    //   Here for our createFunction we need just one entry block.
    //   For that we need the name and the type of the function.

    llvm::Function *createFunction(const std::string& fnName,
                                   llvm::FunctionType* fnType,
                                   Env env)
    {
        //needless to say we have function decleration and
        //functoion definition.
        //function decleration is only the function signiture that
        //is the FUNCTION PROTOTYPE, which has information about
        //it's type, that is the return and parameter type and
        //the function name.

        //The function declation may already be exist, to
        //check this we can call the method on the module
        //which is getFunction.
        auto fn = module -> getFunction(fnName);

        //if it does not exit we create it.
        if(fn == nullptr)
        {
            fn = createFunctionProto(fnName, fnType, env);
        }

        //Now the function is define, we can create its
        //basic blocks.
        createFunctionBlock(fn);
        return fn;

    }

    llvm::Function* createFunctionProto(const std::string& fnName,
                                       llvm::FunctionType* fnType,
                                       Env env)
    {
        auto fn = llvm::Function::Create(fnType, llvm::Function::ExternalLinkage,
                                         fnName, *module);

        //once the function is defined we can call varify that.
        verifyFunction(*fn);

        //we will use the deifne method to install functions
        env -> define(fnName, fn);
        return fn;
    }


    void createFunctionBlock(llvm::Function* fn)
    {
        auto entry = createBB("entry", fn); // create Basic Block

        //The builder that is the code emitter works as a state machine.
        //once we allocated the new block, we have to explicitly tell
        //the builder to emit the code exactly to this block, so
        builder -> SetInsertPoint(entry);
    }


    // to create the basic block, which is called the appropriate
    // method in the basic block class, create, passing the name
    // and the function.
    llvm::BasicBlock* createBB(std::string name, llvm::Function* fn = nullptr)
    {
        return llvm::BasicBlock::Create(*ctx, name, fn);
    }


    void saveModuleToFile(const std::string& fileName)
    {
        std::error_code errorCode;
        llvm::raw_fd_ostream outLL(fileName, errorCode);
        module->print(outLL, nullptr);
    }

    void moduleInit()
    {
        ctx = std::make_unique<llvm::LLVMContext>();
        module = std::make_unique<llvm::Module>("EvaLLVM", *ctx);
        varsBuilder = std::make_unique<llvm::IRBuilder<>>(*ctx);
        builder = std::make_unique<llvm::IRBuilder<>>(*ctx);
    }

    void setupGlobalEnvironment()
    {
        //for simplicity we have the global onbject,
        //where we can set all our global variables
        std::map<std::string, llvm::Value*> globalObject
        {
            {"VERSION", builder->getInt32(42)},
        };

        //then we need the global record
        std::map<std::string, llvm::Value*> globalRec{};


        //When we setet our global variables inside the globalObject
        //so we go throug global object and fill the global record
        for(auto& entry : globalObject)
        {
            globalRec[entry.first] =
                createGlobalVar(entry.first, (llvm::Constant*)entry.second);
        }

        GlobalEnv = std::make_shared<Environment>(globalRec, nullptr);


        //Now our gen function should now accept the environment. because
        //inorder to find the variables we should loopup in that
        //environment.
    }


    //define the parser instance
    std::unique_ptr<EvaGrammar> parser;

    //store the global environment
    std::shared_ptr<Environment> GlobalEnv{};


    //currently compiling function
    llvm::Function *fn;

    std::unique_ptr<llvm::LLVMContext> ctx{};
    std::unique_ptr<llvm::Module> module{};

    //extra builder for the vriables decleration.
    //prepend just at the begining of the function
    //entry block
    std::unique_ptr<llvm::IRBuilder<>> varsBuilder{};


    std::unique_ptr<llvm::IRBuilder<>> builder{};

};



#endif
