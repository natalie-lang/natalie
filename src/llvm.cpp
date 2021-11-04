#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/IRBuilder.h>
#include <stdio.h>
#include <vector>

#include "natalie/parser.hpp"
#include "natalie/string.hpp"

using namespace llvm;

Value *walk_tree(Natalie::Node *node, IRBuilder<> &builder, Function *printf_fn, Function *nat_string_fn, Function *nat_string_to_c_str_fn) {
    switch (node->type()) {
    case Natalie::Node::Type::Block: {
        Value *last_result = nullptr; // FIXME: not nullptr
        for (auto n : static_cast<Natalie::BlockNode *>(node)->nodes())
            last_result = walk_tree(n, builder, printf_fn, nat_string_fn, nat_string_to_c_str_fn);
        return last_result;
    }
    case Natalie::Node::Type::Call: {
        auto call_node = static_cast<Natalie::CallNode *>(node);
        if (call_node->receiver()->type() == Natalie::Node::Type::Nil && strcmp(call_node->message(), "puts") == 0) {
            assert(call_node->args().size() == 1);
            auto value = walk_tree(call_node->args()[0], builder, printf_fn, nat_string_fn, nat_string_to_c_str_fn);

            // assuming Natalie::String* (for now)
            if (value->getType() == builder.getInt8PtrTy()) {
                auto cstr = builder.CreateCall(nat_string_to_c_str_fn, { value });
                auto format_str = builder.CreateGlobalStringPtr("%s\n");
                return builder.CreateCall(printf_fn, { format_str, cstr });
            } else {
                auto format_str = builder.CreateGlobalStringPtr("%lli\n");
                return builder.CreateCall(printf_fn, { format_str, value });
            }
        } else {
            printf("I don't know how to compile function calls yet.\n");
            abort();
        }
    }
    case Natalie::Node::Type::Integer: {
        auto i64 = builder.getInt64Ty();
        return ConstantInt::get(i64, static_cast<Natalie::IntegerNode *>(node)->number());
    }
    case Natalie::Node::Type::String: {
        auto cstr = static_cast<Natalie::StringNode *>(node)->string()->c_str();
        auto str_ptr = builder.CreateGlobalStringPtr(cstr);
        return builder.CreateCall(nat_string_fn, { str_ptr });
    }
    default:
        printf("I don't know how to handle node type %d\n", (int)node->type());
        abort();
    }
}

int main(int argc, char **argv) {
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmParser();
    LLVMInitializeNativeAsmPrinter();

    assert(argc > 1);

    auto parser = Natalie::Parser { new Natalie::String(argv[1]), new Natalie::String("(string)") };
    auto tree = parser.tree();

    auto context = std::make_unique<LLVMContext>();
    IRBuilder<> builder(*context);

    auto module = std::make_unique<Module>("hello", *context);

    // build a 'main' function
    auto i32 = builder.getInt32Ty();
    auto prototype = FunctionType::get(i32, false);
    Function *main_fn = Function::Create(prototype, Function::ExternalLinkage, "main", module.get());
    BasicBlock *body = BasicBlock::Create(*context, "body", main_fn);
    builder.SetInsertPoint(body);

    // use libc's printf function
    auto i8p = builder.getInt8PtrTy();
    auto printf_prototype = FunctionType::get(i8p, true);
    auto printf_fn = Function::Create(printf_prototype, Function::ExternalLinkage, "printf", module.get());

    // Natalie::String *nat_string(const char *str)
    auto nat_string_prototype = FunctionType::get(i8p, std::vector<Type *> { i8p }, false);
    auto nat_string_fn = Function::Create(nat_string_prototype, Function::ExternalLinkage, "nat_string", module.get());

    // const char *nat_string_to_c_str(const Natalie::String *string)
    auto nat_string_to_c_str_prototype = FunctionType::get(i8p, std::vector<Type *> { i8p }, false);
    auto nat_string_to_c_str_fn = Function::Create(nat_string_to_c_str_prototype, Function::ExternalLinkage, "nat_string_to_c_str", module.get());

    walk_tree(tree, builder, printf_fn, nat_string_fn, nat_string_to_c_str_fn);

    // return 0 from main
    auto ret = ConstantInt::get(i32, 0);
    builder.CreateRet(ret);

    // if you want to print the LLVM IR:
    module->print(llvm::outs(), nullptr);
    printf("\n\n------------------\n");

    // execute it!
    ExecutionEngine *executionEngine = EngineBuilder(std::move(module)).setEngineKind(llvm::EngineKind::JIT).create();
    Function *main = executionEngine->FindFunctionNamed(StringRef("main"));
    auto result = executionEngine->runFunction(main, {});

    // return the result
    return result.IntVal.getLimitedValue();
}
