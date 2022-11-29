#include "natalie.hpp"
#include "natalie_parser/creator.hpp"
#include "natalie_parser/node.hpp"

namespace Natalie {

class NatalieCreator : public Cell, public NatalieParser::Creator {
public:
    NatalieCreator(Env *env)
        : m_env { env } {
        reset();
    }

    void reset() {
        // FIXME: cache Sexp lookup
        auto Sexp = GlobalEnv::the()->Object()->const_find(m_env, "Sexp"_s);
        m_sexp = Sexp.send(m_env, "new"_s)->as_array();
    }

    virtual void set_comments(const TM::String &comments) override {
        m_sexp->ivar_set(m_env, "@comments"_s, new StringObject(comments));
    }

    virtual void set_type(const char *type) override {
        m_sexp->set(0, SymbolObject::intern(type));
    }

    virtual void append(const NatalieParser::Node &node) override {
        if (node.type() == NatalieParser::Node::Type::Nil) {
            m_sexp->push(NilObject::the());
            return;
        }
        NatalieCreator creator { m_env };
        creator.set_assignment(assignment());
        node.transform(&creator);
        m_sexp->push(creator.sexp());
    }

    virtual void append_array(const NatalieParser::ArrayNode &array) override {
        NatalieCreator creator { m_env };
        creator.set_assignment(assignment());
        array.ArrayNode::transform(&creator);
        m_sexp->push(creator.sexp());
    }

    virtual void append_false() override {
        m_sexp->push(FalseObject::the());
    }

    virtual void append_bignum(TM::String &number) override {
        m_sexp->push(new IntegerObject(Integer(number)));
    }

    virtual void append_fixnum(int64_t number) override {
        m_sexp->push(Value::integer(number));
    }

    virtual void append_float(double number) override {
        m_sexp->push(new FloatObject(number));
    }

    virtual void append_nil() override {
        m_sexp->push(NilObject::the());
    }

    virtual void append_range(int64_t first, int64_t last, bool exclude_end) override {
        m_sexp->push(RangeObject::create(m_env, Value::integer(first), Value::integer(last), exclude_end));
    }

    virtual void append_regexp(TM::String &pattern, int options) override {
        auto regexp = new RegexpObject(m_env, pattern, options);
        m_sexp->push(regexp);
    }

    virtual void append_sexp(std::function<void(NatalieParser::Creator *)> fn) override {
        NatalieCreator creator { m_env };
        fn(&creator);
        m_sexp->push(creator.sexp());
    }

    virtual void append_string(TM::String &string) override {
        m_sexp->push(new StringObject(string));
    }

    virtual void append_symbol(TM::String &name) override {
        m_sexp->push(SymbolObject::intern(name));
    }

    virtual void append_true() override {
        m_sexp->push(TrueObject::the());
    }

    virtual void make_complex_number() override {
        // TODO
    }

    virtual void make_rational_number() override {
        // TODO
    }

    virtual void wrap(const char *type) override {
        auto inner = m_sexp;
        reset();
        set_type(type);
        m_sexp->push(inner);
    }

    Value sexp() const { return m_sexp; }

    virtual void visit_children(Visitor &visitor) override {
        visitor.visit(m_env);
        visitor.visit(m_sexp);
    }

private:
    Env *m_env { nullptr };
    ArrayObject *m_sexp { nullptr };
};
}
