/* Copyright (c) 2020 Evan Jones
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ----
 *
 * Large portions of this file were copied from Even Jones' libfiber library
 * made available at: https://github.com/evanj/libfiber
 */

#include "natalie.hpp"

namespace Natalie {

FiberValue *FiberValue::initialize(Env *env, Block *block) {
    assert(this != FiberValue::main()); // can never be main fiber
    env->ensure_block_given(block);
    create_stack(env, STACK_SIZE);
    m_block = block;
    return this;
}

ValuePtr FiberValue::resume(Env *env, size_t argc, ValuePtr *args) {
    if (m_status == Status::Terminated)
        env->raise("FiberError", "dead fiber called");
    if (m_previous_fiber)
        env->raise("FiberError", "double resume");
    auto previous_fiber = FiberValue::current();
    m_previous_fiber = s_current;
    s_current = this;
    set_args(argc, args);

    Heap::the().set_start_of_stack(m_start_of_stack);
    previous_fiber->m_end_of_stack = &args;
    fiber_asm_switch(fiber(), previous_fiber->fiber(), 0, env, this);

    if (m_error)
        env->raise_exception(m_error);

    auto fiber_args = FiberValue::current()->args();
    if (fiber_args.size() == 0) {
        return NilValue::the();
    } else if (fiber_args.size() == 1) {
        return fiber_args.at(0);
    } else {
        return new ArrayValue { fiber_args.size(), fiber_args.data() };
    }
}

ValuePtr FiberValue::yield(Env *env, size_t argc, ValuePtr *args) {
    auto current_fiber = FiberValue::current();
    if (!current_fiber->m_previous_fiber)
        env->raise("FiberError", "can't yield from root fiber");
    current_fiber->set_status(Status::Suspended);
    current_fiber->m_end_of_stack = &args;
    current_fiber->yield_back(env, argc, args);

    auto fiber_args = FiberValue::current()->args();
    if (fiber_args.size() == 0) {
        return NilValue::the();
    } else if (fiber_args.size() == 1) {
        return fiber_args.at(0);
    } else {
        return new ArrayValue { fiber_args.size(), fiber_args.data() };
    }
}

void FiberValue::yield_back(Env *env, size_t argc, ValuePtr *args) {
    assert(m_previous_fiber);
    s_current = m_previous_fiber;
    s_current->set_args(argc, args);
    m_previous_fiber = nullptr;
    Heap::the().set_start_of_stack(s_current->start_of_stack());
    fiber_asm_switch(s_current->fiber(), this->fiber(), 0, env, s_current);
}

void FiberValue::visit_children(Visitor &visitor) {
    Value::visit_children(visitor);
    visitor.visit(m_block);
    if (m_start_of_stack == Heap::the().start_of_stack())
        return; // this is the currently active fiber, so don't walk its stack a second time
    if (!m_end_of_stack) {
        assert(m_status == Status::Created);
        return; // this fiber hasn't been started yet, so the stack shouldn't have anything on it
    }
    for (char *ptr = reinterpret_cast<char *>(m_end_of_stack); ptr < m_start_of_stack; ptr += sizeof(intptr_t)) {
        Cell *potential_cell = *reinterpret_cast<Cell **>(ptr);
        if (Heap::the().is_a_heap_cell_in_use(potential_cell)) {
            visitor.visit(potential_cell);
        }
    }
    for (auto arg : m_args) {
        visitor.visit(arg);
    }
    visitor.visit(m_previous_fiber);
    visitor.visit(m_error);
}

void FiberValue::set_args(size_t argc, ValuePtr *args) {
    m_args.clear();
    for (size_t i = 0; i < argc; ++i) {
        m_args.push(args[i]);
    }
}

}

extern "C" {

void fiber_exit() {
    // fiber_asm_switch should never return for an exiting fiber.
    NAT_UNREACHABLE();
}

void fiber_wrapper_func(Natalie::Env *env, Natalie::FiberValue *fiber) {
    Natalie::Heap::the().set_start_of_stack(fiber->start_of_stack());
    fiber->set_status(Natalie::FiberValue::Status::Active);
    assert(fiber->block());
    Natalie::ValuePtr return_args[1];
    bool reraise = false;

    try {
        // NOTE: we cannot pass the env from this fiber (stack) across to another fiber (stack),
        // because doing so can cause unexpected results when env->caller() is used. (Since that
        // calling Env might have been overwritten in the other stack.)
        // That means a backtrace built from inside the fiber will end abruptly.
        // But that seems to be what Ruby does too.
        Natalie::Env e {};
        return_args[0] = NAT_RUN_BLOCK_WITHOUT_BREAK((&e), fiber->block(), fiber->args().size(), fiber->args().data(), nullptr);
    } catch (Natalie::ExceptionValue *exception) {
        fiber->set_error(exception);
        reraise = true;
    }

    fiber->set_status(Natalie::FiberValue::Status::Terminated);
    fiber->set_end_of_stack(&fiber);

    if (reraise)
        fiber->yield_back(env, 0, nullptr);
    else
        fiber->yield_back(env, 1, return_args);
}

#ifdef __x86_64
/* arguments:
 * rdi: next fiber
 * rsi: current fiber
 * rdx: return value
 * rcx: Natalie::Env *
 * r8: Natalie::Fiber *
 * */
asm(".globl " NAT_ASM_PREFIX "fiber_asm_switch\n" NAT_ASM_PREFIX "fiber_asm_switch:\n"
#ifndef __APPLE__
    "\t.type fiber_asm_switch, @function\n"
#endif
    // Move return value into rax
    "\tmovq %rdx, %rax\n"

    // save registers to current stack
    "\tpushq %rbx\n"
    "\tpushq %rbp\n"
    "\tpushq %r12\n"
    "\tpushq %r13\n"
    "\tpushq %r14\n"
    "\tpushq %r15\n"

    // save current stack pointer into fiber struct
    "\tmovq %rsp, (%rsi)\n"

    // move args for function
    "\tmovq %rdi, %r15\n"
    "\tmovq %rcx, %rdi\n" // Env *
    "\tmovq %r8, %rsi\n" // Fiber *

    // swap stack
    "\tmovq (%r15), %rsp\n"

    // restore registers from new stack
    "\tpopq %r15\n"
    "\tpopq %r14\n"
    "\tpopq %r13\n"
    "\tpopq %r12\n"
    "\tpopq %rbp\n"
    "\tpopq %rbx\n"

    // return to the "next" fiber with rax set to return_value
    "\tret\n");
#else
asm(".globl " NAT_ASM_PREFIX "fiber_asm_switch\n" NAT_ASM_PREFIX "fiber_asm_switch:\n"
#ifndef __APPLE__
    "\t.type fiber_asm_switch, @function\n"
#endif
    // Move return value into eax, current pointer into ecx, next pointer into edx
    "\tmov 12(%esp), %eax\n"
    "\tmov 8(%esp), %ecx\n"
    "\tmov 4(%esp), %edx\n"

    // save registers: ebx ebp esi edi (esp into structure)
    "\tpush %ebx\n"
    "\tpush %ebp\n"
    "\tpush %esi\n"
    "\tpush %edi\n"
    "\tmov %esp, (%ecx)\n"

    // restore registers
    "\tmov (%edx), %esp\n"
    "\tpop %edi\n"
    "\tpop %esi\n"
    "\tpop %ebp\n"
    "\tpop %ebx\n"

    // return to the "next" fiber with eax set to return_value
    "\tret\n");
#endif
}
