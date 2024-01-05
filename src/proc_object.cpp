#include "natalie.hpp"

namespace Natalie {

Value ProcObject::initialize(Env *env, Block *block) {
    m_block = block;
    return this;
}

Value ProcObject::call(Env *env, Args args, Block *block) {
    assert(m_block);
    if (is_lambda() && m_break_point != 0) {
        try {
            return NAT_RUN_BLOCK_WITHOUT_BREAK(env, m_block, args, block);
        } catch (ExceptionObject *exception) {
            if (exception->is_local_jump_error_with_break_point(m_break_point))
                return exception->send(env, "exit_value"_s);
            throw exception;
        }
    }
    return NAT_RUN_BLOCK_WITHOUT_BREAK(env, m_block, args, block);
}

bool ProcObject::equal_value(Value other) const {
    return other->is_proc() && other->as_proc()->m_block == m_block;
}

Value ProcObject::source_location() {
    assert(m_block);
    auto file = m_block->env()->file();
    if (file == nullptr) return NilObject::the();
    return new ArrayObject { new StringObject { file }, Value::integer(static_cast<nat_int_t>(m_block->env()->line())) };
}

StringObject *ProcObject::to_s(Env *env) {
    assert(m_block);
    String suffix {};
    if (m_block->env()->file())
        suffix.append(String::format(" {}:{}", m_block->env()->file(), m_block->env()->line()));
    if (is_lambda())
        suffix.append(" (lambda)");
    auto str = String::format("#<{}:{}{}>", m_klass->inspect_str(), String::hex(object_id(), String::HexFormat::LowercaseAndPrefixed), suffix);
    return new StringObject { std::move(str), Encoding::ASCII_8BIT };
}

}
