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

#pragma once

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>

#include "natalie/array_value.hpp"
#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/nil_value.hpp"
#include "natalie/symbol_value.hpp"
#include "natalie/value.hpp"

extern "C" {
typedef struct {
    void **stack;
} fiber_stack_struct;

extern int fiber_asm_switch(fiber_stack_struct *, fiber_stack_struct *, int, Natalie::Env *, Natalie::FiberValue *);
extern void fiber_wrapper_func(Natalie::Env *, Natalie::FiberValue *);
extern void fiber_exit();
}

namespace Natalie {

class FiberValue : public Value {
public:
    enum class Status {
        Created,
        Active,
        Suspended,
        Terminated,
    };

    FiberValue()
        : Value { Value::Type::Fiber, GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("Fiber"))->as_class() } { }

    FiberValue(ClassValue *klass)
        : Value { Value::Type::Fiber, klass } { }

    ~FiberValue() {
        if (!m_stack_memory)
            return;
        int err = munmap(m_stack_memory, STACK_SIZE);
        if (err != 0) {
            fprintf(stderr, "unmapping failed (errno=%d)\n", errno);
            abort();
        }
    }

    static void build_main_fiber(void *start_of_stack) {
        assert(!s_main); // can only be built once
        auto fiber = new FiberValue;
        assert(start_of_stack);
        fiber->m_start_of_stack = start_of_stack;
        s_main = fiber;
        s_current = fiber;
    }

    const int STACK_SIZE = 1024 * 1024;

    FiberValue *initialize(Env *env, Block *block);

    static ValuePtr yield(Env *env, size_t argc, ValuePtr *args);

    void create_stack(Env *env, int stack_size) {
#ifdef __x86_64
        // x86-64: rbx, rbp, r12, r13, r14, r15
        static const int NUM_REGISTERS = 6;
#else
        // x86: ebx, ebp, edi, esi
        static const int NUM_REGISTERS = 4;
#endif
        assert(stack_size % 16 == 0);

#ifdef __OpenBSD__
        auto mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK;
#else
        auto mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS;
#endif
        m_stack_memory = mmap(nullptr, stack_size, PROT_READ | PROT_WRITE, mmap_flags, -1, 0);
        if (m_stack_memory == MAP_FAILED) {
            fprintf(stderr, "mapping failed (errno=%d)\n", errno);
            abort();
        }

        // FIXME: Fibers only support stacks that grow toward lower addresses
        void *comparison_ptr = &stack_size;
        assert(comparison_ptr < Heap::the().start_of_stack());

        m_start_of_stack = reinterpret_cast<char *>(m_stack_memory) + stack_size;
        m_fiber.stack = reinterpret_cast<void **>(m_start_of_stack);
#ifdef __APPLE__
        assert((uintptr_t)m_fiber.stack % 16 == 0);
#endif

        // 4 bytes below 16-byte alignment: mac os x wants return address here so this points to a call instruction.
        *(--m_fiber.stack) = (void *)((uintptr_t)&fiber_exit);
        // 8 bytes below 16-byte alignment: will "return" to start this function
        *(--m_fiber.stack) = (void *)((uintptr_t)&fiber_wrapper_func); // Cast to avoid ISO C warnings.
        // push NULL words to initialize the registers loaded by fiber_asm_switch
        for (int i = 0; i < NUM_REGISTERS; ++i) {
            *(--m_fiber.stack) = 0;
        }
    }

    ValuePtr resume(Env *env, size_t argc, ValuePtr *args);
    void yield_back(Env *env, size_t argc, ValuePtr *args);

    void *start_of_stack() { return m_start_of_stack; }

    fiber_stack_struct *fiber() { return &m_fiber; }
    Block *block() { return m_block; }
    void set_status(Status status) { m_status = status; }
    void set_end_of_stack(void *ptr) { m_end_of_stack = ptr; }

    SymbolValue *status(Env *env) {
        switch (m_status) {
        case Status::Created:
            return SymbolValue::intern("created");
        case Status::Active:
            return SymbolValue::intern("active");
        case Status::Suspended:
            return SymbolValue::intern("suspended");
        case Status::Terminated:
            return SymbolValue::intern("terminated");
        }
        NAT_UNREACHABLE();
    }

    virtual void visit_children(Visitor &) override final;

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(
            buf,
            len,
            "<FiberValue %p stack=%p..%p>",
            this,
            m_start_of_stack,
            reinterpret_cast<char *>(m_start_of_stack) + STACK_SIZE);
    }

    static FiberValue *current() { return s_current; }

    static FiberValue *main() { return s_main; }

    Vector<ValuePtr> &args() { return m_args; }
    void set_args(size_t argc, ValuePtr *args);

    ExceptionValue *error() { return m_error; }
    void set_error(ExceptionValue *error) { m_error = error; }

private:
    Block *m_block { nullptr };
    ::fiber_stack_struct m_fiber {};
    void *m_stack_memory { nullptr };
    void *m_start_of_stack { nullptr };
    void *m_end_of_stack { nullptr };
    Status m_status { Status::Created };
    Vector<ValuePtr> m_args {};
    FiberValue *m_previous_fiber { nullptr };
    ExceptionValue *m_error { nullptr };

    inline static FiberValue *s_current = nullptr;
    inline static FiberValue *s_main = nullptr;
};

}
