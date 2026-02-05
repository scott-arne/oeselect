// src/Parser.cpp
#include "oeselect/Parser.h"
#include "oeselect/Error.h"
#include "oeselect/predicates/NamePredicate.h"
#include "oeselect/predicates/LogicalPredicates.h"
#include "oeselect/predicates/AtomPropertyPredicates.h"
#include "oeselect/predicates/ComponentPredicates.h"
#include "oeselect/predicates/AtomTypePredicates.h"
#include "oeselect/predicates/DistancePredicates.h"
#include "oeselect/predicates/ExpansionPredicates.h"

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
struct kw_resn : TAO_PEGTL_ISTRING("resn") {};
struct kw_resi : TAO_PEGTL_ISTRING("resi") {};
struct kw_chain : TAO_PEGTL_ISTRING("chain") {};
struct kw_elem : TAO_PEGTL_ISTRING("elem") {};
struct kw_index : TAO_PEGTL_ISTRING("index") {};
struct kw_and : TAO_PEGTL_ISTRING("and") {};
struct kw_or : TAO_PEGTL_ISTRING("or") {};
struct kw_not : TAO_PEGTL_ISTRING("not") {};
struct kw_xor : TAO_PEGTL_ISTRING("xor") {};

// Component keywords (no value required)
struct kw_protein : TAO_PEGTL_ISTRING("protein") {};
struct kw_ligand : TAO_PEGTL_ISTRING("ligand") {};
struct kw_water : TAO_PEGTL_ISTRING("water") {};
struct kw_solvent : TAO_PEGTL_ISTRING("solvent") {};
struct kw_organic : TAO_PEGTL_ISTRING("organic") {};
struct kw_backbone : pegtl::sor<TAO_PEGTL_ISTRING("backbone"), TAO_PEGTL_ISTRING("bb")> {};
struct kw_sidechain : pegtl::sor<TAO_PEGTL_ISTRING("sidechain"), TAO_PEGTL_ISTRING("sc")> {};
struct kw_metal : pegtl::sor<TAO_PEGTL_ISTRING("metals"), TAO_PEGTL_ISTRING("metal")> {};

// Atom type keywords
// Note: Order matters - longer matches must come first
// polar_hydrogen before hydrogen, nonpolar_hydrogen before hydrogen
struct kw_heavy : TAO_PEGTL_ISTRING("heavy") {};
struct kw_polar_hydrogen : pegtl::sor<TAO_PEGTL_ISTRING("polar_hydrogen"), TAO_PEGTL_ISTRING("polarh")> {};
struct kw_nonpolar_hydrogen : pegtl::sor<TAO_PEGTL_ISTRING("nonpolar_hydrogen"), TAO_PEGTL_ISTRING("apolarh")> {};
struct kw_hydrogen : pegtl::sor<TAO_PEGTL_ISTRING("hydrogen"), TAO_PEGTL_ISTRING("h")> {};

// Numbers and ranges
struct number : pegtl::plus<pegtl::digit> {};
struct range : pegtl::seq<number, pegtl::one<'-'>, number> {};

// Floating point number for distance predicates
struct float_num : pegtl::seq<
    pegtl::opt<pegtl::one<'-'>>,
    pegtl::plus<pegtl::digit>,
    pegtl::opt<pegtl::seq<pegtl::one<'.'>, pegtl::star<pegtl::digit>>>
> {};

// Distance keywords
struct kw_around : TAO_PEGTL_ISTRING("around") {};
struct kw_xaround : TAO_PEGTL_ISTRING("xaround") {};
struct kw_beyond : TAO_PEGTL_ISTRING("beyond") {};

// Expansion keywords
struct kw_byres : TAO_PEGTL_ISTRING("byres") {};
struct kw_bychain : TAO_PEGTL_ISTRING("bychain") {};

// Comparison operators
struct comp_ge : pegtl::string<'>', '='> {};
struct comp_le : pegtl::string<'<', '='> {};
struct comp_gt : pegtl::one<'>'> {};
struct comp_lt : pegtl::one<'<'> {};
struct comp_op : pegtl::sor<comp_ge, comp_le, comp_gt, comp_lt> {};

// Element symbol (1-2 letters, case insensitive)
struct element_symbol : pegtl::seq<pegtl::alpha, pegtl::opt<pegtl::alpha>> {};

// Chain ID (single character)
struct chain_id : pegtl::alpha {};

// Specifiers (leaf nodes)
struct name_spec : pegtl::seq<kw_name, ws_required, value> {};
struct resn_spec : pegtl::seq<kw_resn, ws_required, value> {};
struct resi_comp_spec : pegtl::seq<kw_resi, ws_required, comp_op, ws, number> {};
struct resi_range_spec : pegtl::seq<kw_resi, ws_required, range> {};
struct resi_exact_spec : pegtl::seq<kw_resi, ws_required, number> {};
struct resi_spec : pegtl::sor<resi_comp_spec, resi_range_spec, resi_exact_spec> {};
struct chain_spec : pegtl::seq<kw_chain, ws_required, chain_id> {};
struct elem_spec : pegtl::seq<kw_elem, ws_required, element_symbol> {};
struct index_range_spec : pegtl::seq<kw_index, ws_required, range> {};
struct index_exact_spec : pegtl::seq<kw_index, ws_required, number> {};
struct index_spec : pegtl::sor<index_range_spec, index_exact_spec> {};

// Component specifiers (keyword-only, no value)
struct protein_spec : kw_protein {};
struct ligand_spec : kw_ligand {};
struct water_spec : kw_water {};
struct solvent_spec : kw_solvent {};
struct organic_spec : kw_organic {};
struct backbone_spec : kw_backbone {};
struct sidechain_spec : kw_sidechain {};
struct metal_spec : kw_metal {};

// Atom type specifiers
struct heavy_spec : kw_heavy {};
struct polar_hydrogen_spec : kw_polar_hydrogen {};
struct nonpolar_hydrogen_spec : kw_nonpolar_hydrogen {};
struct hydrogen_spec : kw_hydrogen {};

// Forward declarations for recursive grammar
struct expression;
struct primary;

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

// Distance specifiers - take a radius and a nested primary expression
// The nested selection can be a simple specifier or a parenthesized expression
// around 5.0 protein
// around 5.0 (name CA or name CB)
// xaround 3.0 name REF
// beyond 10.0 water
struct around_spec : pegtl::seq<kw_around, ws_required, float_num, ws_required, primary> {};
struct xaround_spec : pegtl::seq<kw_xaround, ws_required, float_num, ws_required, primary> {};
struct beyond_spec : pegtl::seq<kw_beyond, ws_required, float_num, ws_required, primary> {};

// Expansion specifiers - take a nested selection
struct byres_spec : pegtl::seq<kw_byres, ws_required, primary> {};
struct bychain_spec : pegtl::seq<kw_bychain, ws_required, primary> {};

// All specifiers
// IMPORTANT: Order matters - longer/more specific matches must come first
// polar_hydrogen_spec and nonpolar_hydrogen_spec must come before hydrogen_spec
// because hydrogen_spec's "h" alias could match the start of other keywords
// xaround must come before around since "around" is a prefix of "xaround"
// bychain must come before byres since "byres" could be a prefix match issue
struct specifier : pegtl::sor<
    name_spec, resn_spec, resi_spec, chain_spec, elem_spec, index_spec,
    protein_spec, ligand_spec, water_spec, solvent_spec, organic_spec,
    backbone_spec, sidechain_spec, metal_spec,
    heavy_spec, polar_hydrogen_spec, nonpolar_hydrogen_spec, hydrogen_spec,
    xaround_spec, around_spec, beyond_spec,
    bychain_spec, byres_spec
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

    // For resi/index parsing
    int first_number = 0;
    int second_number = 0;
    ResiPredicate::Op comp_op = ResiPredicate::Op::Eq;

    // For distance predicate parsing
    float current_radius = 0.0f;

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

// Resn specifier - uses same value as name
template<>
struct Action<Grammar::resn_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<ResnPredicate>(state.current_value));
    }
};

// Number action - capture for resi/index
template<>
struct Action<Grammar::number> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, ParserState& state) {
        // Shift first_number to second_number if we already have a first
        state.second_number = state.first_number;
        state.first_number = std::stoi(in.string());
    }
};

// Comparison operator actions
template<>
struct Action<Grammar::comp_ge> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.comp_op = ResiPredicate::Op::Ge;
    }
};

template<>
struct Action<Grammar::comp_le> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.comp_op = ResiPredicate::Op::Le;
    }
};

template<>
struct Action<Grammar::comp_gt> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.comp_op = ResiPredicate::Op::Gt;
    }
};

template<>
struct Action<Grammar::comp_lt> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.comp_op = ResiPredicate::Op::Lt;
    }
};

// Resi specifiers
template<>
struct Action<Grammar::resi_comp_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<ResiPredicate>(state.first_number, state.comp_op));
        state.comp_op = ResiPredicate::Op::Eq;  // Reset
    }
};

template<>
struct Action<Grammar::resi_range_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        // After parsing range, second_number has start, first_number has end
        state.pushOperand(std::make_shared<ResiPredicate>(state.second_number, state.first_number));
    }
};

template<>
struct Action<Grammar::resi_exact_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<ResiPredicate>(state.first_number, ResiPredicate::Op::Eq));
    }
};

// Chain specifier
template<>
struct Action<Grammar::chain_id> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, ParserState& state) {
        state.current_value = in.string();
    }
};

template<>
struct Action<Grammar::chain_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<ChainPredicate>(state.current_value));
    }
};

// Element specifier
template<>
struct Action<Grammar::element_symbol> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, ParserState& state) {
        state.current_value = in.string();
    }
};

template<>
struct Action<Grammar::elem_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<ElemPredicate>(state.current_value));
    }
};

// Index specifiers
template<>
struct Action<Grammar::index_range_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        // After parsing range, second_number has start, first_number has end
        state.pushOperand(std::make_shared<IndexPredicate>(
            static_cast<unsigned int>(state.second_number),
            static_cast<unsigned int>(state.first_number)));
    }
};

template<>
struct Action<Grammar::index_exact_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<IndexPredicate>(static_cast<unsigned int>(state.first_number)));
    }
};

// Component specifiers
template<>
struct Action<Grammar::protein_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<ProteinPredicate>());
    }
};

template<>
struct Action<Grammar::ligand_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<LigandPredicate>());
    }
};

template<>
struct Action<Grammar::water_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<WaterPredicate>());
    }
};

template<>
struct Action<Grammar::solvent_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<SolventPredicate>());
    }
};

template<>
struct Action<Grammar::organic_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<OrganicPredicate>());
    }
};

template<>
struct Action<Grammar::backbone_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<BackbonePredicate>());
    }
};

template<>
struct Action<Grammar::sidechain_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<SidechainPredicate>());
    }
};

template<>
struct Action<Grammar::metal_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<MetalPredicate>());
    }
};

// Atom type specifiers
template<>
struct Action<Grammar::heavy_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<HeavyPredicate>());
    }
};

template<>
struct Action<Grammar::hydrogen_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<HydrogenPredicate>());
    }
};

template<>
struct Action<Grammar::polar_hydrogen_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<PolarHydrogenPredicate>());
    }
};

template<>
struct Action<Grammar::nonpolar_hydrogen_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<NonpolarHydrogenPredicate>());
    }
};

// Float number action - capture for distance predicates
template<>
struct Action<Grammar::float_num> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, ParserState& state) {
        state.current_radius = std::stof(in.string());
    }
};

// Distance specifier actions
template<>
struct Action<Grammar::around_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        // The reference selection is already on the stack from parsing primary
        auto reference = state.popOperand();
        state.pushOperand(std::make_shared<AroundPredicate>(state.current_radius, std::move(reference)));
    }
};

template<>
struct Action<Grammar::xaround_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        // The reference selection is already on the stack from parsing primary
        auto reference = state.popOperand();
        state.pushOperand(std::make_shared<XAroundPredicate>(state.current_radius, std::move(reference)));
    }
};

template<>
struct Action<Grammar::beyond_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        // The reference selection is already on the stack from parsing primary
        auto reference = state.popOperand();
        state.pushOperand(std::make_shared<BeyondPredicate>(state.current_radius, std::move(reference)));
    }
};

// Expansion specifier actions
template<>
struct Action<Grammar::byres_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        // The child selection is already on the stack from parsing primary
        auto child = state.popOperand();
        state.pushOperand(std::make_shared<ByResPredicate>(std::move(child)));
    }
};

template<>
struct Action<Grammar::bychain_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        // The child selection is already on the stack from parsing primary
        auto child = state.popOperand();
        state.pushOperand(std::make_shared<ByChainPredicate>(std::move(child)));
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
