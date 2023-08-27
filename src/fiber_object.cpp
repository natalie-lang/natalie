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

FiberObject *FiberObject::initialize(Env *env, Value blocking, Value storage, Block *block) {
    assert(this != FiberObject::main()); // can never be main fiber
    env->ensure_block_given(block);
    create_stack(env, STACK_SIZE);
    m_block = block;
    if (blocking != nullptr)
        m_blocking = blocking->is_truthy();
    m_file = env->file();
    m_line = env->line();
    if (storage != nullptr && !storage->is_nil()) {
        if (!storage->is_hash())
            env->raise("TypeError", "storage must be a hash");
        if (storage->is_frozen())
            env->raise("FrozenError", "storage must not be frozen");
        auto *hash = storage->as_hash();
        for (auto it = hash->begin(); it != hash->end(); it++) {
            if (!it->key->is_symbol())
                env->raise("TypeError", "wrong argument type Object (expected Symbol)");
        }
        m_storage = hash;
    }
    return this;
}

Value FiberObject::inspect(Env *env) {
    const TM::String file_and_line { m_file && m_line ? TM::String::format(" {}:{}", *m_file, *m_line) : "" };
    return StringObject::format("#<{}:{}{} ({})>", m_klass->inspect_str(), String::hex(object_id(), String::HexFormat::LowercaseAndPrefixed), file_and_line, status(env));
}

bool FiberObject::is_alive() const {
    return m_status != Status::Terminated;
}

bool FiberObject::is_blocking() const {
    return m_blocking;
}

Value FiberObject::is_blocking_current() {
    return s_current->is_blocking() ? IntegerObject::create(1) : FalseObject::the();
}

Value FiberObject::resume(Env *env, Args args) {
    if (m_status == Status::Terminated)
        env->raise("FiberError", "dead fiber called");
    if (m_previous_fiber)
        env->raise("FiberError", "attempt to resume the current fiber");
    auto previous_fiber = FiberObject::current();
    m_previous_fiber = s_current;
    s_current = this;
    set_args(args);

    Heap::the().set_start_of_stack(m_start_of_stack);
    previous_fiber->m_end_of_stack = &args;
    fiber_asm_switch(fiber(), previous_fiber->fiber(), 0, env, this);

    if (m_error)
        env->raise_exception(m_error);

    auto fiber_args = FiberObject::current()->args();
    if (fiber_args.size() == 0) {
        return NilObject::the();
    } else if (fiber_args.size() == 1) {
        return fiber_args.at(0);
    } else {
        return new ArrayObject { fiber_args.size(), fiber_args.data() };
    }
}

Value FiberObject::storage(Env *env) const {
    auto fiber = FiberObject::current();
    if (this != fiber)
        env->raise("ArgumentError", "Fiber storage can only be accessed from the Fiber it belongs to");
    while (fiber->m_storage == nullptr && fiber->m_previous_fiber != nullptr)
        fiber = fiber->m_previous_fiber;
    if (fiber->m_storage == nullptr)
        return NilObject::the();
    return fiber->m_storage;
}

Value FiberObject::yield(Env *env, Args args) {
    auto current_fiber = FiberObject::current();
    if (!current_fiber->m_previous_fiber)
        env->raise("FiberError", "can't yield from root fiber");
    current_fiber->set_status(Status::Suspended);
    current_fiber->m_end_of_stack = &args;
    current_fiber->yield_back(env, args);

    auto fiber_args = FiberObject::current()->args();
    if (fiber_args.size() == 0) {
        return NilObject::the();
    } else if (fiber_args.size() == 1) {
        return fiber_args.at(0);
    } else {
        return new ArrayObject { fiber_args.size(), fiber_args.data() };
    }
}

void FiberObject::yield_back(Env *env, Args args) {
    assert(m_previous_fiber);
    s_current = m_previous_fiber;
    s_current->set_args(args);
    m_previous_fiber = nullptr;
    Heap::the().set_start_of_stack(s_current->start_of_stack());
    fiber_asm_switch(s_current->fiber(), this->fiber(), 0, env, s_current);
}

void FiberObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    for (auto arg : m_args)
        visitor.visit(arg);
    visitor.visit(m_previous_fiber);
    visitor.visit(m_error);
    visitor.visit(m_block);
    visitor.visit(m_storage);
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
}

void FiberObject::set_args(Args args) {
    m_args.clear();
    for (size_t i = 0; i < args.size(); ++i) {
        m_args.push(args[i]);
    }
}

}

extern "C" {

void fiber_exit() {
    // fiber_asm_switch should never return for an exiting fiber.
    NAT_UNREACHABLE();
}

void fiber_wrapper_func(Natalie::Env *env, Natalie::FiberObject *fiber) {
    Natalie::Heap::the().set_start_of_stack(fiber->start_of_stack());
    fiber->set_status(Natalie::FiberObject::Status::Resumed);
    assert(fiber->block());
    Natalie::Value return_arg = nullptr;
    bool reraise = false;

    try {
        // NOTE: we cannot pass the env from this fiber (stack) across to another fiber (stack),
        // because doing so can cause unexpected results when env->caller() is used. (Since that
        // calling Env might have been overwritten in the other stack.)
        // That means a backtrace built from inside the fiber will end abruptly.
        // But that seems to be what Ruby does too.
        Natalie::Env e {};

        return_arg = NAT_RUN_BLOCK_WITHOUT_BREAK((&e), fiber->block(), Natalie::Args(fiber->args()), nullptr);
    } catch (Natalie::ExceptionObject *exception) {
        fiber->set_error(exception);
        reraise = true;
    }

    fiber->set_status(Natalie::FiberObject::Status::Terminated);
    fiber->set_end_of_stack(&fiber);

    if (reraise)
        fiber->yield_back(env, {});
    else
        fiber->yield_back(env, { return_arg });
}

#if defined(__x86_64)
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

/* aarch64 arguments:
 * x0: next fiber
 * x1: current fiber
 * x2: return value
 * x3: Natalie::Env *
 * x4: Natalie::Fiber *
 * */
#elif defined(__aarch64__)
asm(".globl " NAT_ASM_PREFIX "fiber_asm_switch\n" NAT_ASM_PREFIX "fiber_asm_switch:\n"
#ifndef __APPLE__
    "\t.type fiber_asm_switch, @function\n"
#endif
    // Move return value into x8
    "\tmov x8, x2\n"

    // save return address onto the stack
    "\tstp x30, x29, [sp, #-16]!\n"

    // save registers to current stack
    "\tstp x10, x9, [sp, #-16]!\n"
    "\tstp x12, x11, [sp, #-16]!\n"
    "\tstp x14, x13, [sp, #-16]!\n"
    "\tstp x16, x15, [sp, #-16]!\n"
    "\tstp x18, x17, [sp, #-16]!\n"
    "\tstp x20, x19, [sp, #-16]!\n"
    "\tstp x22, x21, [sp, #-16]!\n"
    "\tstp x24, x23, [sp, #-16]!\n"
    "\tstp x26, x25, [sp, #-16]!\n"
    "\tstp x28, x27, [sp, #-16]!\n"

    // save current stack pointer into fiber struct
    "\tmov x15, sp\n"
    "\tstr x15, [x1]\n"

    // move args for function
    "\tmov x15, x0\n"
    "\tmov x0, x3\n" // Env *
    "\tmov x1, x4\n" // Fiber *

    // swap stack
    "\tldr x14, [x15]\n"
    "\tmov sp, x14\n"

    // restore registers from new stack
    "\tldp x28, x27, [sp], #16\n"
    "\tldp x26, x25, [sp], #16\n"
    "\tldp x24, x23, [sp], #16\n"
    "\tldp x22, x21, [sp], #16\n"
    "\tldp x20, x19, [sp], #16\n"
    "\tldp x18, x17, [sp], #16\n"
    "\tldp x16, x15, [sp], #16\n"
    "\tldp x14, x13, [sp], #16\n"
    "\tldp x12, x11, [sp], #16\n"
    "\tldp x10, x9, [sp], #16\n"

    // restore return address register for `ret` instruction
    "\tldp x30, x29, [sp], #16\n"

    // return to the "next" fiber with x8 set to return_value
    "\tret\n");
#else
// TODO x86
#endif
}
