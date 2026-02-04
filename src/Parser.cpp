// src/Parser.cpp
#include "oeselect/Parser.h"
#include "oeselect/Error.h"
#include "oeselect/predicates/NamePredicate.h"
#include "oeselect/predicates/LogicalPredicates.h"

#include <tao/pegtl.hpp>
#include <stack>

namespace pegtl = tao::pegtl;

namespace OESel {
namespace Grammar {

// Whitespace
struct ws : pegtl::star<pegtl::space> {};
struct ws_required : pegtl::plus<pegtl::space> {};

// Identifiers and patterns
struct glob_char : pegtl::sor<pegtl::alnum, pegtl::one<'*', '?', '_', '-'>> {};
struct glob_pattern : pegtl::plus<glob_char> {};
struct quoted_string : pegtl::seq<
    pegtl::one<'"'>,
    pegtl::star<pegtl::not_one<'"'>>,
    pegtl::one<'"'>
> {};
struct value : pegtl::sor<quoted_string, glob_pattern> {};

// Keywords - case insensitive
struct kw_name : TAO_PEGTL_ISTRING("name") {};
struct kw_and : TAO_PEGTL_ISTRING("and") {};
struct kw_or : TAO_PEGTL_ISTRING("or") {};
struct kw_not : TAO_PEGTL_ISTRING("not") {};
struct kw_xor : TAO_PEGTL_ISTRING("xor") {};

// Forward declarations for recursive grammar
struct expression;

// Specifiers (leaf nodes)
struct name_spec : pegtl::seq<kw_name, ws_required, value> {};

// For now, just name - use sor for future expansion
struct specifier : pegtl::sor<name_spec> {};

// Markers for parentheses (for action tracking)
struct open_paren : pegtl::one<'('> {};
struct close_paren : pegtl::one<')'> {};

// Parenthesized expression
struct paren_expr : pegtl::seq<
    open_paren,
    ws,
    expression,
    ws,
    close_paren
> {};

// Primary: specifier or parenthesized expression
struct primary : pegtl::sor<paren_expr, specifier> {};

// NOT expression: "not" prefix (unary, highest precedence)
struct not_op : kw_not {};
struct not_expr : pegtl::sor<
    pegtl::seq<not_op, ws_required, not_expr>,  // Allow chained "not not x"
    primary
> {};

// Individual operator markers (for action tracking)
struct and_op : kw_and {};
struct or_op : kw_or {};
struct xor_op : kw_xor {};

// AND expression (higher precedence than OR)
struct and_expr : pegtl::seq<
    not_expr,
    pegtl::star<pegtl::seq<ws_required, and_op, ws_required, not_expr>>
> {};

// OR expression (higher precedence than XOR)
struct or_expr : pegtl::seq<
    and_expr,
    pegtl::star<pegtl::seq<ws_required, or_op, ws_required, and_expr>>
> {};

// XOR expression (lowest precedence)
struct xor_expr : pegtl::seq<
    or_expr,
    pegtl::star<pegtl::seq<ws_required, xor_op, ws_required, or_expr>>
> {};

// Top-level expression
struct expression : xor_expr {};

// Selection rule
struct selection : pegtl::seq<ws, expression, ws, pegtl::eof> {};

}  // namespace Grammar

// Context for tracking operators at a given nesting level
struct OpContext {
    int and_count = 0;
    int or_count = 0;
    int xor_count = 0;
    int not_count = 0;
};

// Parser state with operator counting stacks
struct ParserState {
    std::stack<Predicate::Ptr> operands;
    std::string current_value;

    // Stack of operator contexts - one per nesting level (parentheses)
    std::stack<OpContext> contexts;

    ParserState() {
        contexts.push(OpContext{});
    }

    OpContext& currentContext() {
        return contexts.top();
    }

    void pushContext() {
        contexts.push(OpContext{});
    }

    void popContext() {
        if (contexts.size() > 1) {
            contexts.pop();
        }
    }

    void pushOperand(Predicate::Ptr pred) {
        operands.push(std::move(pred));
    }

    Predicate::Ptr popOperand() {
        if (operands.empty()) {
            throw SelectionError("Internal parser error: operand stack underflow");
        }
        auto pred = std::move(operands.top());
        operands.pop();
        return pred;
    }

    Predicate::Ptr getResult() {
        if (operands.empty()) {
            throw SelectionError("Internal parser error: no result");
        }
        return operands.top();
    }
};

// Actions
template<typename Rule>
struct Action : pegtl::nothing<Rule> {};

template<>
struct Action<Grammar::value> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, ParserState& state) {
        std::string val = in.string();
        // Remove quotes if present
        if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
            val = val.substr(1, val.size() - 2);
        }
        state.current_value = val;
    }
};

template<>
struct Action<Grammar::name_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<NamePredicate>(state.current_value));
    }
};

// Opening parenthesis - push new context
template<>
struct Action<Grammar::open_paren> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushContext();
    }
};

// Closing parenthesis - pop context
template<>
struct Action<Grammar::close_paren> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.popContext();
    }
};

// Track NOT operator
template<>
struct Action<Grammar::not_op> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.currentContext().not_count++;
    }
};

// Track AND operator
template<>
struct Action<Grammar::and_op> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.currentContext().and_count++;
    }
};

// Track OR operator
template<>
struct Action<Grammar::or_op> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.currentContext().or_count++;
    }
};

// Track XOR operator
template<>
struct Action<Grammar::xor_op> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.currentContext().xor_count++;
    }
};

// Apply NOT - wrap current operand
template<>
struct Action<Grammar::not_expr> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        auto& ctx = state.currentContext();
        if (ctx.not_count > 0) {
            int not_count = ctx.not_count;
            ctx.not_count = 0;  // Reset for next usage

            if (!state.operands.empty()) {
                for (int i = 0; i < not_count; ++i) {
                    auto operand = state.popOperand();
                    state.pushOperand(std::make_shared<NotPredicate>(std::move(operand)));
                }
            }
        }
    }
};

// Apply AND - combine operands
template<>
struct Action<Grammar::and_expr> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        auto& ctx = state.currentContext();
        if (ctx.and_count > 0) {
            int and_count = ctx.and_count;
            ctx.and_count = 0;  // Reset

            if (state.operands.size() >= static_cast<size_t>(and_count + 1)) {
                std::vector<Predicate::Ptr> children;
                for (int i = 0; i <= and_count; ++i) {
                    children.push_back(state.popOperand());
                }
                std::reverse(children.begin(), children.end());
                state.pushOperand(std::make_shared<AndPredicate>(std::move(children)));
            }
        }
    }
};

// Apply OR - combine operands
template<>
struct Action<Grammar::or_expr> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        auto& ctx = state.currentContext();
        if (ctx.or_count > 0) {
            int or_count = ctx.or_count;
            ctx.or_count = 0;  // Reset

            if (state.operands.size() >= static_cast<size_t>(or_count + 1)) {
                std::vector<Predicate::Ptr> children;
                for (int i = 0; i <= or_count; ++i) {
                    children.push_back(state.popOperand());
                }
                std::reverse(children.begin(), children.end());
                state.pushOperand(std::make_shared<OrPredicate>(std::move(children)));
            }
        }
    }
};

// Apply XOR - combine operands
template<>
struct Action<Grammar::xor_expr> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        auto& ctx = state.currentContext();
        if (ctx.xor_count > 0) {
            int xor_count = ctx.xor_count;
            ctx.xor_count = 0;  // Reset

            if (state.operands.size() >= static_cast<size_t>(xor_count + 1)) {
                std::vector<Predicate::Ptr> children;
                for (int i = 0; i <= xor_count; ++i) {
                    children.push_back(state.popOperand());
                }
                std::reverse(children.begin(), children.end());
                state.pushOperand(std::make_shared<XOrPredicate>(std::move(children)));
            }
        }
    }
};

// TruePredicate for empty selections
class TruePredicateImpl : public Predicate {
public:
    bool Evaluate(Context&, const OEChem::OEAtomBase&) const override { return true; }
    std::string ToCanonical() const override { return "all"; }
    PredicateType Type() const override { return PredicateType::True; }
};

Predicate::Ptr ParseSelection(const std::string& sele) {
    if (sele.empty()) {
        return std::make_shared<TruePredicateImpl>();
    }

    ParserState state;
    pegtl::memory_input input(sele, "selection");

    try {
        if (!pegtl::parse<Grammar::selection, Action>(input, state)) {
            throw SelectionError("Failed to parse selection: " + sele);
        }
    } catch (const pegtl::parse_error& e) {
        throw SelectionError(e.what());
    }

    if (state.operands.empty()) {
        throw SelectionError("No predicate created for: " + sele);
    }

    return state.getResult();
}

}  // namespace OESel
