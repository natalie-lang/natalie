#include "natalie.hpp"
#include "natalie/array_object.hpp"
#include "natalie/env.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {

Value ParserObject::parse(Env *env, Value code, Value source_path) {
    code->assert_type(env, Object::Type::String, "String");
    if (source_path)
        source_path->assert_type(env, Object::Type::String, "String");
    else
        source_path = new StringObject { "(string)" };
    code->assert_type(env, Object::Type::String, "String");
    auto parser = Parser {
        new String(code->as_string()->to_low_level_string()),
        new String(source_path->as_string()->to_low_level_string())
    };
    Node *tree;
    try {
        tree = parser.tree();
    } catch (Parser::SyntaxError &e) {
        env->raise("SyntaxError", e.message());
    }
    auto ast = node_to_ruby(env, tree);
    delete tree;
    return ast;
}

Value ParserObject::tokens(Env *env, Value code, Value with_line_and_column_numbers) {
    code->assert_type(env, Object::Type::String, "String");
    auto lexer = Lexer {
        new String(code->as_string()->to_low_level_string()),
        new String("(string)")
    };
    auto array = new ArrayObject {};
    auto the_tokens = lexer.tokens();
    auto include_line_and_column_numbers = with_line_and_column_numbers && with_line_and_column_numbers->is_truthy();
    for (auto token : *the_tokens) {
        auto token_value = token_to_ruby(env, token, include_line_and_column_numbers);
        if (token_value->is_truthy())
            array->push(token_value);
    }
    return array;
}

Value ParserObject::token_to_ruby(Env *env, Token &token, bool with_line_and_column_numbers) {
    if (token.is_eof())
        return NilObject::the();
    validate_token(env, token);
    const char *type = token.type_value();
    auto hash = new HashObject {};
    hash->put(env, "type"_s, SymbolObject::intern(type));
    switch (token.type()) {
    case Token::Type::PercentLowerI:
    case Token::Type::PercentUpperI:
    case Token::Type::PercentLowerW:
    case Token::Type::PercentUpperW:
    case Token::Type::Regexp:
    case Token::Type::String:
        hash->put(env, "literal"_s, new StringObject { token.literal_or_blank() });
        break;
    case Token::Type::BareName:
    case Token::Type::ClassVariable:
    case Token::Type::Constant:
    case Token::Type::GlobalVariable:
    case Token::Type::InstanceVariable:
    case Token::Type::Symbol:
    case Token::Type::SymbolKey:
        hash->put(env, "literal"_s, SymbolObject::intern(token.literal_or_blank()));
        break;
    case Token::Type::Float:
        hash->put(env, "literal"_s, new FloatObject { token.get_double() });
        break;
    case Token::Type::Integer:
        hash->put(env, "literal"_s, Value::integer(token.get_integer()));
        break;
    case Token::Type::InterpolatedRegexpEnd:
        if (token.options())
            hash->put(env, "options"_s, new StringObject { token.options().value().ref() });
        break;
    default:
        void();
    }
    if (with_line_and_column_numbers) {
        hash->put(env, "line"_s, Value::integer(token.line()));
        hash->put(env, "column"_s, Value::integer(token.column()));
    }
    return hash;
}

void ParserObject::validate_token(Env *env, Token &token) {
    try {
        token.validate();
    } catch (Parser::SyntaxError &e) {
        env->raise("SyntaxError", e.message());
    }
}

Value ParserObject::class_or_module_name_to_ruby(Env *env, Node *name) {
    if (name->type() == Node::Type::Identifier) {
        auto identifier = static_cast<IdentifierNode *>(name);
        return SymbolObject::intern(identifier->name().ref());
    } else {
        return node_to_ruby(env, name);
    }
}

Value ParserObject::node_to_ruby(Env *env, Node *node) {
    switch (node->type()) {
    case Node::Type::Alias: {
        auto alias_node = static_cast<AliasNode *>(node);
        return new SexpObject {
            env,
            node,
            {
                "alias"_s,
                node_to_ruby(env, alias_node->new_name()),
                node_to_ruby(env, alias_node->existing_name()),
            }
        };
    }
    case Node::Type::Arg: {
        auto arg_node = static_cast<ArgNode *>(node);
        if (arg_node->value()) {
            return new SexpObject {
                env,
                node,
                {
                    "lasgn"_s,
                    SymbolObject::intern(arg_node->name().ref()),
                    node_to_ruby(env, arg_node->value()),
                }
            };
        } else {
            String name;
            if (arg_node->name())
                name = String(arg_node->name().ref());
            else
                name = String();
            if (arg_node->splat()) {
                name.prepend_char('*');
            } else if (arg_node->kwsplat()) {
                name.prepend_char('*');
                name.prepend_char('*');
            } else if (arg_node->block_arg()) {
                name.prepend_char('&');
            }
            return SymbolObject::intern(name);
        }
    }
    case Node::Type::Array: {
        auto array_node = static_cast<ArrayNode *>(node);
        auto sexp = new SexpObject { env, node, { "array"_s } };
        for (auto item_node : array_node->nodes()) {
            sexp->push(node_to_ruby(env, item_node));
        }
        return sexp;
    }
    case Node::Type::ArrayPattern: {
        auto array_node = static_cast<ArrayNode *>(node);
        auto sexp = new SexpObject { env, node, { "array_pat"_s } };
        if (!array_node->nodes().is_empty())
            sexp->push(NilObject::the()); // NOTE: I don't know what this nil is for
        for (auto item_node : array_node->nodes()) {
            sexp->push(node_to_ruby(env, item_node));
        }
        return sexp;
    }
    case Node::Type::Assignment: {
        auto assignment_node = static_cast<AssignmentNode *>(node);
        Value left;
        switch (assignment_node->identifier()->type()) {
        case Node::Type::MultipleAssignment: {
            auto masgn = static_cast<MultipleAssignmentNode *>(assignment_node->identifier());
            auto sexp = multiple_assignment_to_ruby_with_array(env, masgn);
            auto value = node_to_ruby(env, assignment_node->value());
            sexp->push(value);
            return sexp;
        }
        case Node::Type::Colon2:
        case Node::Type::Colon3:
        case Node::Type::Identifier: {
            auto sexp = build_assignment_sexp(env, assignment_node, assignment_node->identifier());
            sexp->push(node_to_ruby(env, assignment_node->value()));
            return sexp;
        }
        default:
            NAT_UNREACHABLE();
        }
    }
    case Node::Type::AttrAssign: {
        auto attr_assign_node = static_cast<AttrAssignNode *>(node);
        auto sexp = new SexpObject {
            env,
            node,
            {
                "attrasgn"_s,
                node_to_ruby(env, attr_assign_node->receiver()),
                SymbolObject::intern(attr_assign_node->message().ref()),
            }
        };
        for (auto arg : attr_assign_node->args()) {
            sexp->push(node_to_ruby(env, arg));
        }
        return sexp;
    }
    case Node::Type::Begin: {
        auto begin_node = static_cast<BeginNode *>(node);
        assert(begin_node->body());
        auto *sexp = new SexpObject { env, node, { "rescue"_s } };
        if (!begin_node->body()->is_empty())
            sexp->push(node_to_ruby(env, begin_node->body()->without_unnecessary_nesting()));
        for (auto rescue_node : begin_node->rescue_nodes()) {
            sexp->push(node_to_ruby(env, rescue_node));
        }
        if (begin_node->else_body())
            sexp->push(node_to_ruby(env, begin_node->else_body()->without_unnecessary_nesting()));
        if (begin_node->ensure_body()) {
            if (begin_node->rescue_nodes().is_empty())
                (*sexp)[0] = "ensure"_s;
            else
                sexp = new SexpObject { env, node, { "ensure"_s, sexp } };
            sexp->push(node_to_ruby(env, begin_node->ensure_body()->without_unnecessary_nesting()));
        }
        return sexp;
    }
    case Node::Type::BeginRescue: {
        auto begin_rescue_node = static_cast<BeginRescueNode *>(node);
        auto array = new ArrayNode { begin_rescue_node->token() };
        for (auto exception_node : begin_rescue_node->exceptions()) {
            array->add_node(exception_node);
        }
        if (begin_rescue_node->name())
            array->add_node(begin_rescue_node->name_to_node());
        auto *rescue_node = new SexpObject {
            env,
            node,
            {
                "resbody"_s,
                node_to_ruby(env, array),
            }
        };
        for (auto node : begin_rescue_node->body()->nodes()) {
            rescue_node->push(node_to_ruby(env, node));
        }
        return rescue_node;
    }
    case Node::Type::Block: {
        auto block_node = static_cast<BlockNode *>(node);
        auto *array = new SexpObject { env, node, { "block"_s } };
        for (auto item_node : block_node->nodes()) {
            array->push(node_to_ruby(env, item_node));
        }
        return array;
    }
    case Node::Type::BlockPass: {
        auto block_pass_node = static_cast<BlockPassNode *>(node);
        auto *sexp = new SexpObject { env, node, { "block_pass"_s } };
        sexp->push(node_to_ruby(env, block_pass_node->node()));
        return sexp;
    }
    case Node::Type::Break: {
        auto break_node = static_cast<BreakNode *>(node);
        auto sexp = new SexpObject { env, node, { "break"_s } };
        if (break_node->arg())
            sexp->push(node_to_ruby(env, break_node->arg()));
        return sexp;
    }
    case Node::Type::Call: {
        auto call_node = static_cast<CallNode *>(node);
        auto sexp = new SexpObject {
            env,
            node,
            {
                "call"_s,
                node_to_ruby(env, call_node->receiver()),
                SymbolObject::intern(call_node->message().ref()),
            }
        };
        for (auto arg : call_node->args()) {
            sexp->push(node_to_ruby(env, arg));
        }
        return sexp;
    }
    case Node::Type::Case: {
        auto case_node = static_cast<CaseNode *>(node);
        auto sexp = new SexpObject {
            env,
            case_node,
            {
                "case"_s,
                node_to_ruby(env, case_node->subject()),
            }
        };
        for (auto when_node : case_node->nodes()) {
            sexp->push(node_to_ruby(env, when_node));
        }
        if (case_node->else_node()) {
            sexp->push(node_to_ruby(env, case_node->else_node()->without_unnecessary_nesting()));
        } else {
            sexp->push(NilObject::the());
        }
        return sexp;
    }
    case Node::Type::CaseIn: {
        auto case_in_node = static_cast<CaseInNode *>(node);
        auto sexp = new SexpObject {
            env,
            case_in_node,
            {
                "in"_s,
                node_to_ruby(env, case_in_node->pattern()),
            }
        };
        for (auto node : case_in_node->body()->nodes()) {
            sexp->push(node_to_ruby(env, node));
        }
        return sexp;
    }
    case Node::Type::CaseWhen: {
        auto case_when_node = static_cast<CaseWhenNode *>(node);
        auto sexp = new SexpObject {
            env,
            case_when_node,
            {
                "when"_s,
                node_to_ruby(env, case_when_node->condition()),
            }
        };
        for (auto node : case_when_node->body()->nodes()) {
            sexp->push(node_to_ruby(env, node));
        }
        return sexp;
    }
    case Node::Type::Class: {
        auto class_node = static_cast<ClassNode *>(node);
        auto sexp = new SexpObject { env, class_node, { "class"_s, class_or_module_name_to_ruby(env, class_node->name()), node_to_ruby(env, class_node->superclass()) } };
        if (!class_node->body()->is_empty()) {
            for (auto node : class_node->body()->nodes()) {
                sexp->push(node_to_ruby(env, node));
            }
        }
        return sexp;
    }
    case Node::Type::Colon2: {
        auto colon2_node = static_cast<Colon2Node *>(node);
        return new SexpObject {
            env,
            colon2_node,
            {
                "colon2"_s,
                node_to_ruby(env, colon2_node->left()),
                SymbolObject::intern(colon2_node->name().ref()),
            }
        };
    }
    case Node::Type::Colon3: {
        auto colon3_node = static_cast<Colon3Node *>(node);
        return new SexpObject {
            env,
            colon3_node,
            {
                "colon3"_s,
                SymbolObject::intern(colon3_node->name().ref()),
            }
        };
    }
    case Node::Type::Constant: {
        auto constant_node = static_cast<ConstantNode *>(node);
        return new SexpObject {
            env,
            constant_node,
            {
                "const"_s,
                SymbolObject::intern(*constant_node->token().literal_string()),
            }
        };
    }
    case Node::Type::Defined: {
        auto defined_node = static_cast<DefinedNode *>(node);
        return new SexpObject {
            env,
            defined_node,
            {
                "defined"_s,
                node_to_ruby(env, defined_node->arg()),
            }
        };
    }
    case Node::Type::Def: {
        auto def_node = static_cast<DefNode *>(node);
        SexpObject *sexp;
        if (def_node->self_node()) {
            sexp = new SexpObject {
                env,
                def_node,
                {
                    "defs"_s,
                    node_to_ruby(env, def_node->self_node()),
                    SymbolObject::intern(*def_node->name()),
                    build_args_sexp(env, def_node, def_node->args()),
                }
            };
        } else {
            sexp = new SexpObject {
                env,
                def_node,
                {
                    "defn"_s,
                    SymbolObject::intern(*def_node->name()),
                    build_args_sexp(env, def_node, def_node->args()),
                }
            };
        }
        if (def_node->body()->is_empty()) {
            sexp->push(new SexpObject { env, def_node, { "nil"_s } });
        } else {
            for (auto node : def_node->body()->nodes()) {
                sexp->push(node_to_ruby(env, node));
            }
        }
        return sexp;
    }
    case Node::Type::EvaluateToString: {
        auto evaluate_to_string_node = static_cast<EvaluateToStringNode *>(node);
        return new SexpObject { env, evaluate_to_string_node, { "evstr"_s, node_to_ruby(env, evaluate_to_string_node->node()) } };
    }
    case Node::Type::False: {
        auto false_node = static_cast<FalseNode *>(node);
        return new SexpObject { env, false_node, { "false"_s } };
    }
    case Node::Type::Hash: {
        auto hash_node = static_cast<HashNode *>(node);
        auto sexp = new SexpObject { env, hash_node, { "hash"_s } };
        for (auto node : hash_node->nodes()) {
            sexp->push(node_to_ruby(env, node));
        }
        return sexp;
    }
    case Node::Type::HashPattern: {
        auto hash_node = static_cast<HashNode *>(node);
        auto sexp = new SexpObject { env, hash_node, { "hash_pat"_s } };
        sexp->push(NilObject::the()); // NOTE: I don't know what this nil is for
        for (auto node : hash_node->nodes()) {
            sexp->push(node_to_ruby(env, node));
        }
        return sexp;
    }
    case Node::Type::Identifier: {
        auto identifier_node = static_cast<IdentifierNode *>(node);
        switch (identifier_node->token_type()) {
        case Token::Type::BareName:
            if (identifier_node->is_lvar()) {
                return new SexpObject { env, identifier_node, { "lvar"_s, SymbolObject::intern(identifier_node->name().ref()) } };
            } else {
                return new SexpObject { env, identifier_node, { "call"_s, NilObject::the(), SymbolObject::intern(identifier_node->name().ref()) } };
            }
        case Token::Type::ClassVariable:
            return new SexpObject { env, identifier_node, { "cvar"_s, SymbolObject::intern(identifier_node->name().ref()) } };
        case Token::Type::Constant:
            return new SexpObject { env, identifier_node, { "const"_s, SymbolObject::intern(identifier_node->name().ref()) } };
        case Token::Type::GlobalVariable: {
            auto ref = identifier_node->nth_ref();
            if (ref > 0)
                return new SexpObject { env, identifier_node, { "nth_ref"_s, Value::integer(ref) } };
            else
                return new SexpObject { env, identifier_node, { "gvar"_s, SymbolObject::intern(identifier_node->name().ref()) } };
        }
        case Token::Type::InstanceVariable:
            return new SexpObject { env, identifier_node, { "ivar"_s, SymbolObject::intern(identifier_node->name().ref()) } };
        default:
            NAT_NOT_YET_IMPLEMENTED();
        }
    }
    case Node::Type::If: {
        auto if_node = static_cast<IfNode *>(node);
        return new SexpObject {
            env,
            if_node,
            {
                "if"_s,
                node_to_ruby(env, if_node->condition()),
                node_to_ruby(env, if_node->true_expr()),
                node_to_ruby(env, if_node->false_expr()),
            }
        };
    }
    case Node::Type::Iter: {
        auto iter_node = static_cast<IterNode *>(node);
        auto sexp = new SexpObject {
            env,
            iter_node,
            {
                "iter"_s,
                node_to_ruby(env, iter_node->call()),
            }
        };
        if (iter_node->args().is_empty())
            sexp->push(Value::integer(0));
        else
            sexp->push(build_args_sexp(env, iter_node, iter_node->args()));
        if (!iter_node->body()->is_empty()) {
            if (iter_node->body()->has_one_node())
                sexp->push(node_to_ruby(env, iter_node->body()->nodes()[0]));
            else
                sexp->push(node_to_ruby(env, iter_node->body()));
        }
        return sexp;
    }
    case Node::Type::InterpolatedRegexp: {
        auto interpolated_regexp_node = static_cast<InterpolatedRegexpNode *>(node);
        auto sexp = new SexpObject { env, interpolated_regexp_node, { "dregx"_s } };
        for (size_t i = 0; i < interpolated_regexp_node->nodes().size(); i++) {
            auto node = interpolated_regexp_node->nodes()[i];
            if (i == 0 && node->type() == Node::Type::String)
                sexp->push(new StringObject { static_cast<StringNode *>(node)->string().ref() });
            else
                sexp->push(node_to_ruby(env, node));
        }
        if (interpolated_regexp_node->options() != 0)
            sexp->push(Value::integer(interpolated_regexp_node->options()));
        return sexp;
    }
    case Node::Type::InterpolatedShell: {
        auto interpolated_shell_node = static_cast<InterpolatedShellNode *>(node);
        auto sexp = new SexpObject { env, interpolated_shell_node, { "dxstr"_s } };
        for (size_t i = 0; i < interpolated_shell_node->nodes().size(); i++) {
            auto node = interpolated_shell_node->nodes()[i];
            if (i == 0 && node->type() == Node::Type::String)
                sexp->push(new StringObject { static_cast<StringNode *>(node)->string().ref() });
            else
                sexp->push(node_to_ruby(env, node));
        }
        return sexp;
    }
    case Node::Type::InterpolatedString: {
        auto interpolated_string_node = static_cast<InterpolatedStringNode *>(node);
        auto sexp = new SexpObject { env, interpolated_string_node, { "dstr"_s } };
        for (size_t i = 0; i < interpolated_string_node->nodes().size(); i++) {
            auto node = interpolated_string_node->nodes()[i];
            if (i == 0 && node->type() == Node::Type::String)
                sexp->push(new StringObject { static_cast<StringNode *>(node)->string().ref() });
            else
                sexp->push(node_to_ruby(env, node));
        }
        return sexp;
    }
    case Node::Type::KeywordArg: {
        auto keyword_arg_node = static_cast<KeywordArgNode *>(node);
        auto sexp = new SexpObject {
            env,
            keyword_arg_node,
            {
                "kwarg"_s,
                SymbolObject::intern(*keyword_arg_node->name()),
            }
        };
        if (keyword_arg_node->value())
            sexp->push(node_to_ruby(env, keyword_arg_node->value()));
        return sexp;
    }
    case Node::Type::KeywordSplat: {
        auto keyword_splat_node = static_cast<KeywordSplatNode *>(node);
        auto sexp = new SexpObject { env, keyword_splat_node, { "hash"_s } };
        auto kwsplat_sexp = new SexpObject { env, keyword_splat_node, { "kwsplat"_s } };
        if (keyword_splat_node->node())
            kwsplat_sexp->push(node_to_ruby(env, keyword_splat_node->node()));
        sexp->push(kwsplat_sexp);
        return sexp;
    }
    case Node::Type::Integer: {
        auto integer_node = static_cast<IntegerNode *>(node);
        return new SexpObject { env, integer_node, { "lit"_s, Value::integer(integer_node->number()) } };
    }
    case Node::Type::Float: {
        auto float_node = static_cast<FloatNode *>(node);
        return new SexpObject { env, float_node, { "lit"_s, new FloatObject { float_node->number() } } };
    }
    case Node::Type::LogicalAnd: {
        auto logical_and_node = static_cast<LogicalAndNode *>(node);
        auto sexp = new SexpObject {
            env,
            logical_and_node,
            {
                "and"_s,
                node_to_ruby(env, logical_and_node->left()),
                node_to_ruby(env, logical_and_node->right()),
            }
        };

        return sexp;
    }
    case Node::Type::LogicalOr: {
        auto logical_or_node = static_cast<LogicalOrNode *>(node);
        auto sexp = new SexpObject {
            env,
            logical_or_node,
            {
                "or"_s,
                node_to_ruby(env, logical_or_node->left()),
                node_to_ruby(env, logical_or_node->right()),
            }
        };

        return sexp;
    }
    case Node::Type::Match: {
        auto match_node = static_cast<MatchNode *>(node);
        return new SexpObject {
            env,
            match_node,
            {
                match_node->regexp_on_left() ? "match2"_s : "match3"_s,
                node_to_ruby(env, match_node->regexp()),
                node_to_ruby(env, match_node->arg()),
            }
        };
    }
    case Node::Type::Module: {
        auto module_node = static_cast<ModuleNode *>(node);
        auto sexp = new SexpObject { env, module_node, { "module"_s, class_or_module_name_to_ruby(env, module_node->name()) } };
        if (!module_node->body()->is_empty()) {
            for (auto node : module_node->body()->nodes()) {
                sexp->push(node_to_ruby(env, node));
            }
        }
        return sexp;
    }
    case Node::Type::MultipleAssignment: {
        auto multiple_assignment_node = static_cast<MultipleAssignmentNode *>(node);
        auto sexp = new SexpObject { env, multiple_assignment_node, { "masgn"_s } };
        for (auto node : multiple_assignment_node->nodes()) {
            switch (node->type()) {
            case Node::Type::Arg:
            case Node::Type::MultipleAssignment:
                sexp->push(node_to_ruby(env, node));
                break;
            default:
                NAT_NOT_YET_IMPLEMENTED(); // maybe not needed?
            }
        }
        return sexp;
    }
    case Node::Type::Next: {
        auto next_node = static_cast<NextNode *>(node);
        auto sexp = new SexpObject { env, next_node, { "next"_s } };
        if (next_node->arg())
            sexp->push(node_to_ruby(env, next_node->arg()));
        return sexp;
    }
    case Node::Type::Nil: {
        auto nil_node = static_cast<NilNode *>(node);
        return NilObject::the();
    }
    case Node::Type::NilSexp: {
        auto nil_sexp_node = static_cast<NilSexpNode *>(node);
        return new SexpObject { env, nil_sexp_node, { "nil"_s } };
    }
    case Node::Type::Not: {
        auto not_node = static_cast<NotNode *>(node);
        return new SexpObject { env, not_node, { "not"_s, node_to_ruby(env, not_node->expression()) } };
    }
    case Node::Type::OpAssign: {
        auto op_assign_node = static_cast<OpAssignNode *>(node);
        auto sexp = build_assignment_sexp(env, op_assign_node, op_assign_node->name());
        assert(op_assign_node->op());
        auto call = new CallNode { op_assign_node->token(), op_assign_node->name(), op_assign_node->op() };
        call->add_arg(op_assign_node->value());
        sexp->push(node_to_ruby(env, call));
        return sexp;
    }
    case Node::Type::OpAssignAccessor: {
        auto op_assign_accessor_node = static_cast<OpAssignAccessorNode *>(node);
        if (*op_assign_accessor_node->message() == "[]=") {
            auto arg_list = new SexpObject {
                env,
                op_assign_accessor_node,
                {
                    "arglist"_s,
                }
            };
            for (auto arg : op_assign_accessor_node->args()) {
                arg_list->push(node_to_ruby(env, arg));
            }
            return new SexpObject {
                env,
                op_assign_accessor_node,
                {
                    "op_asgn1"_s,
                    node_to_ruby(env, op_assign_accessor_node->receiver()),
                    arg_list,
                    SymbolObject::intern(*op_assign_accessor_node->op()),
                    node_to_ruby(env, op_assign_accessor_node->value()),
                }
            };
        } else {
            assert(op_assign_accessor_node->args().is_empty());
            return new SexpObject {
                env,
                op_assign_accessor_node,
                {
                    "op_asgn2"_s,
                    node_to_ruby(env, op_assign_accessor_node->receiver()),
                    SymbolObject::intern(*op_assign_accessor_node->message()),
                    SymbolObject::intern(*op_assign_accessor_node->op()),
                    node_to_ruby(env, op_assign_accessor_node->value()),
                }
            };
        }
    }
    case Node::Type::OpAssignAnd: {
        auto op_assign_and_node = static_cast<OpAssignAndNode *>(node);
        auto assignment_node = new AssignmentNode {
            op_assign_and_node->token(),
            op_assign_and_node->name(),
            op_assign_and_node->value()
        };
        return new SexpObject {
            env,
            op_assign_and_node,
            {
                "op_asgn_and"_s,
                node_to_ruby(env, op_assign_and_node->name()),
                node_to_ruby(env, assignment_node),
            },
        };
    }
    case Node::Type::OpAssignOr: {
        auto op_assign_or_node = static_cast<OpAssignOrNode *>(node);
        auto assignment_node = new AssignmentNode {
            op_assign_or_node->token(),
            op_assign_or_node->name(),
            op_assign_or_node->value()
        };
        return new SexpObject {
            env,
            op_assign_or_node,
            {
                "op_asgn_or"_s,
                node_to_ruby(env, op_assign_or_node->name()),
                node_to_ruby(env, assignment_node),
            },
        };
    }
    case Node::Type::Pin: {
        auto pin_node = static_cast<PinNode *>(node);
        auto sexp = new SexpObject { env, node, { "pin"_s } };
        sexp->push(node_to_ruby(env, pin_node->identifier()));
        return sexp;
    }
    case Node::Type::Range: {
        auto range_node = static_cast<RangeNode *>(node);
        if (range_node->first()->type() == Node::Type::Integer && range_node->last()->type() == Node::Type::Integer) {
            auto first = static_cast<IntegerNode *>(range_node->first())->number();
            auto last = static_cast<IntegerNode *>(range_node->last())->number();
            return new SexpObject {
                env,
                range_node,
                { "lit"_s,
                    new RangeObject {
                        Value::integer(first),
                        Value::integer(last),
                        range_node->exclude_end() } }
            };
        }
        return new SexpObject {
            env,
            range_node,
            { range_node->exclude_end() ? "dot3"_s : "dot2"_s, node_to_ruby(env, range_node->first()), node_to_ruby(env, range_node->last()) }
        };
    }
    case Node::Type::Regexp: {
        auto regexp_node = static_cast<RegexpNode *>(node);
        auto regexp = new RegexpObject { env, regexp_node->pattern()->c_str() };
        if (regexp_node->options())
            regexp->set_options(*regexp_node->options());
        return new SexpObject { env, regexp_node, { "lit"_s, regexp } };
    }
    case Node::Type::Return: {
        auto return_node = static_cast<ReturnNode *>(node);
        auto sexp = new SexpObject { env, return_node, { "return"_s } };
        auto value = return_node->value();
        if (value->type() != Node::Type::Nil)
            sexp->push(node_to_ruby(env, value));
        return sexp;
    }
    case Node::Type::SafeCall: {
        auto safe_call_node = static_cast<SafeCallNode *>(node);
        auto sexp = new SexpObject {
            env,
            safe_call_node,
            {
                "safe_call"_s,
                node_to_ruby(env, safe_call_node->receiver()),
                SymbolObject::intern(*safe_call_node->message()),
            }
        };

        for (auto arg : safe_call_node->args()) {
            sexp->push(node_to_ruby(env, arg));
        }
        return sexp;
    }
    case Node::Type::Self: {
        auto self_node = static_cast<SelfNode *>(node);
        return new SexpObject { env, self_node, { "self"_s } };
    }
    case Node::Type::Sclass: {
        auto sclass_node = static_cast<SclassNode *>(node);
        auto sexp = new SexpObject {
            env,
            sclass_node,
            {
                "sclass"_s,
                node_to_ruby(env, sclass_node->klass()),
            }
        };
        for (auto node : sclass_node->body()->nodes()) {
            sexp->push(node_to_ruby(env, node));
        }
        return sexp;
    }
    case Node::Type::Shell: {
        auto shell_node = static_cast<ShellNode *>(node);
        return new SexpObject { env, shell_node, { "xstr"_s, new StringObject(shell_node->string().ref()) } };
    }
    case Node::Type::SplatAssignment: {
        auto splat_assignment_node = static_cast<SplatAssignmentNode *>(node);
        auto sexp = new SexpObject { env, splat_assignment_node, { "splat"_s } };
        if (splat_assignment_node->node())
            sexp->push(build_assignment_sexp(env, splat_assignment_node, splat_assignment_node->node()));
        return sexp;
    }
    case Node::Type::Splat: {
        auto splat_node = static_cast<SplatNode *>(node);
        auto sexp = new SexpObject { env, splat_node, { "splat"_s } };
        if (splat_node->node())
            sexp->push(node_to_ruby(env, splat_node->node()));
        return sexp;
    }
    case Node::Type::SplatValue: {
        auto splat_value_node = static_cast<SplatValueNode *>(node);
        return new SexpObject { env, splat_value_node, { "svalue"_s, node_to_ruby(env, splat_value_node->value()) } };
    }
    case Node::Type::StabbyProc: {
        auto stabby_proc_node = static_cast<StabbyProcNode *>(node);
        return new SexpObject { env, stabby_proc_node, { "lambda"_s } };
    }
    case Node::Type::String: {
        auto string_node = static_cast<StringNode *>(node);
        return new SexpObject { env, string_node, { "str"_s, new StringObject(*string_node->string()) } };
    }
    case Node::Type::Symbol: {
        auto symbol_node = static_cast<SymbolNode *>(node);
        return new SexpObject { env, symbol_node, { "lit"_s, SymbolObject::intern(symbol_node->name().ref()) } };
    }
    case Node::Type::ToArray: {
        auto to_array_node = static_cast<ToArrayNode *>(node);
        return new SexpObject { env, to_array_node, { "to_ary"_s, node_to_ruby(env, to_array_node->value()) } };
    }
    case Node::Type::True: {
        auto true_node = static_cast<TrueNode *>(node);
        return new SexpObject { env, true_node, { "true"_s } };
    }
    case Node::Type::Super: {
        auto super_node = static_cast<SuperNode *>(node);
        if (super_node->empty_parens()) {
            return new SexpObject { env, super_node, { "super"_s } };
        } else if (super_node->args().is_empty()) {
            return new SexpObject { env, super_node, { "zsuper"_s } };
        }
        auto sexp = new SexpObject { env, super_node, { "super"_s } };
        for (auto arg : super_node->args()) {
            sexp->push(node_to_ruby(env, arg));
        }
        return sexp;
    }
    case Node::Type::Until:
    case Node::Type::While: {
        auto while_node = static_cast<WhileNode *>(node);
        Value is_pre, body;
        if (while_node->pre())
            is_pre = TrueObject::the();
        else
            is_pre = FalseObject::the();
        if (while_node->body()->is_empty())
            body = NilObject::the();
        else
            body = node_to_ruby(env, while_node->body()->without_unnecessary_nesting());
        return new SexpObject {
            env,
            while_node,
            {
                node->type() == Node::Type::Until ? "until"_s : "while"_s,
                node_to_ruby(env, while_node->condition()),
                body,
                is_pre,
            }
        };
    }
    case Node::Type::Yield: {
        auto yield_node = static_cast<YieldNode *>(node);
        auto sexp = new SexpObject { env, yield_node, { "yield"_s } };
        if (yield_node->args().is_empty())
            return sexp;
        for (auto arg : yield_node->args()) {
            sexp->push(node_to_ruby(env, arg));
        }
        return sexp;
    }
    default:
        NAT_UNREACHABLE();
    }
}

SexpObject *ParserObject::build_args_sexp(Env *env, Node *parent, Vector<Node *> &args) {
    auto sexp = new SexpObject { env, parent, { "args"_s } };
    for (auto arg : args) {
        switch (arg->type()) {
        case Node::Type::Arg:
        case Node::Type::KeywordArg:
        case Node::Type::MultipleAssignment:
            sexp->push(node_to_ruby(env, arg));
            break;
        default:
            NAT_UNREACHABLE();
        }
    }
    return sexp;
}

SexpObject *ParserObject::build_assignment_sexp(Env *env, Node *parent, Node *identifier) {
    SymbolObject *type;
    switch (identifier->token().type()) {
    case Token::Type::BareName:
        type = "lasgn"_s;
        break;
    case Token::Type::ClassVariable:
        type = "cvdecl"_s;
        break;
    case Token::Type::Constant:
    case Token::Type::ConstantResolution:
        type = "cdecl"_s;
        break;
    case Token::Type::GlobalVariable:
        type = "gasgn"_s;
        break;
    case Token::Type::InstanceVariable:
        type = "iasgn"_s;
        break;
    default:
        printf("got token type %d\n", (int)identifier->token().type());
        NAT_UNREACHABLE();
    }
    Value name;
    switch (identifier->type()) {
    case Node::Type::Colon2:
    case Node::Type::Colon3:
        name = node_to_ruby(env, identifier);
        break;
    case Node::Type::Identifier:
        name = SymbolObject::intern(static_cast<IdentifierNode *>(identifier)->name().ref());
        break;
    default:
        NAT_NOT_YET_IMPLEMENTED("node type %d", (int)identifier->type());
    }
    return new SexpObject {
        env,
        parent,
        { type, name }
    };
}

SexpObject *ParserObject::multiple_assignment_to_ruby_with_array(Env *env, MultipleAssignmentNode *node) {
    auto sexp = new SexpObject { env, node, { "masgn"_s } };
    auto array = new SexpObject { env, node, { "array"_s } };
    for (auto identifier : node->nodes()) {
        switch (identifier->type()) {
        case Node::Type::Colon2:
        case Node::Type::Colon3:
        case Node::Type::Identifier:
            array->push(build_assignment_sexp(env, node, identifier));
            break;
        case Node::Type::Splat:
        case Node::Type::SplatAssignment:
            array->push(node_to_ruby(env, identifier));
            break;
        case Node::Type::MultipleAssignment:
            array->push(multiple_assignment_to_ruby_with_array(env, static_cast<MultipleAssignmentNode *>(identifier)));
            break;
        default:
            NAT_UNREACHABLE();
        }
    }
    sexp->push(array);
    return sexp;
}

}
