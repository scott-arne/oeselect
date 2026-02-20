/**
 * @file Parser.cpp
 * @brief PEGTL-based selection parser implementation.
 *
 * This file implements the grammar and actions for parsing PyMOL-style
 * selection strings into predicate trees. The parser uses operator
 * precedence: NOT > AND > OR > XOR.
 */

#include "oeselect/Parser.h"
#include "oeselect/Error.h"
#include "oeselect/predicates/NamePredicate.h"
#include "oeselect/predicates/LogicalPredicates.h"
#include "oeselect/predicates/AtomPropertyPredicates.h"
#include "oeselect/predicates/ComponentPredicates.h"
#include "oeselect/predicates/AtomTypePredicates.h"
#include "oeselect/predicates/DistancePredicates.h"
#include "oeselect/predicates/ExpansionPredicates.h"
#include "oeselect/predicates/SecondaryStructurePredicates.h"

#include <tao/pegtl.hpp>
#include <stack>

namespace pegtl = tao::pegtl;

namespace OESel {

// ============================================================================
// Grammar Rules
// ============================================================================
namespace Grammar {

// Whitespace handling
struct ws : pegtl::star<pegtl::space> {};
struct ws_required : pegtl::plus<pegtl::space> {};

// Pattern matching for names and residues
struct glob_char : pegtl::sor<pegtl::alnum, pegtl::one<'*', '?', '_', '-'>> {};
struct glob_pattern : pegtl::plus<glob_char> {};
struct quoted_string : pegtl::seq<
    pegtl::one<'"'>,
    pegtl::star<pegtl::not_one<'"'>>,
    pegtl::one<'"'>
> {};
struct value : pegtl::sor<quoted_string, glob_pattern> {};
struct value_list : pegtl::list<value, pegtl::one<'+'>> {};

// Property keywords (case-insensitive)
struct kw_name : TAO_PEGTL_ISTRING("name") {};
struct kw_resn : TAO_PEGTL_ISTRING("resn") {};
struct kw_resi : TAO_PEGTL_ISTRING("resi") {};
struct kw_chain : TAO_PEGTL_ISTRING("chain") {};
struct kw_elem : TAO_PEGTL_ISTRING("elem") {};
struct kw_index : TAO_PEGTL_ISTRING("index") {};

// Logical operators
struct kw_and : TAO_PEGTL_ISTRING("and") {};
struct kw_or : TAO_PEGTL_ISTRING("or") {};
struct kw_not : TAO_PEGTL_ISTRING("not") {};
struct kw_xor : TAO_PEGTL_ISTRING("xor") {};

// Special keywords
struct kw_all : TAO_PEGTL_ISTRING("all") {};
struct kw_none : TAO_PEGTL_ISTRING("none") {};

// Component keywords
struct kw_protein : TAO_PEGTL_ISTRING("protein") {};
struct kw_ligand : TAO_PEGTL_ISTRING("ligand") {};
struct kw_water : TAO_PEGTL_ISTRING("water") {};
struct kw_solvent : TAO_PEGTL_ISTRING("solvent") {};
struct kw_organic : TAO_PEGTL_ISTRING("organic") {};
struct kw_backbone : pegtl::sor<TAO_PEGTL_ISTRING("backbone"), TAO_PEGTL_ISTRING("bb")> {};
struct kw_sidechain : pegtl::sor<TAO_PEGTL_ISTRING("sidechain"), TAO_PEGTL_ISTRING("sc")> {};
struct kw_metal : pegtl::sor<TAO_PEGTL_ISTRING("metals"), TAO_PEGTL_ISTRING("metal")> {};

// Atom type keywords (order matters for disambiguation)
struct kw_heavy : TAO_PEGTL_ISTRING("heavy") {};
struct kw_polar_hydrogen : pegtl::sor<TAO_PEGTL_ISTRING("polar_hydrogen"), TAO_PEGTL_ISTRING("polarh")> {};
struct kw_nonpolar_hydrogen : pegtl::sor<TAO_PEGTL_ISTRING("nonpolar_hydrogen"), TAO_PEGTL_ISTRING("apolarh")> {};
struct kw_hydrogen : pegtl::sor<TAO_PEGTL_ISTRING("hydrogen"), TAO_PEGTL_ISTRING("h")> {};

// Numeric patterns
struct number : pegtl::plus<pegtl::digit> {};
struct range : pegtl::seq<number, pegtl::one<'-'>, number> {};
struct float_num : pegtl::seq<
    pegtl::opt<pegtl::one<'-'>>,
    pegtl::plus<pegtl::digit>,
    pegtl::opt<pegtl::seq<pegtl::one<'.'>, pegtl::star<pegtl::digit>>>
> {};

// Distance and expansion keywords
struct kw_around : TAO_PEGTL_ISTRING("around") {};
struct kw_xaround : TAO_PEGTL_ISTRING("xaround") {};
struct kw_beyond : TAO_PEGTL_ISTRING("beyond") {};
struct kw_byres : TAO_PEGTL_ISTRING("byres") {};
struct kw_bychain : TAO_PEGTL_ISTRING("bychain") {};

// Secondary structure keywords
struct kw_helix : TAO_PEGTL_ISTRING("helix") {};
struct kw_sheet : TAO_PEGTL_ISTRING("sheet") {};
struct kw_turn : TAO_PEGTL_ISTRING("turn") {};
struct kw_loop : TAO_PEGTL_ISTRING("loop") {};

// Comparison operators
struct comp_ge : pegtl::string<'>', '='> {};
struct comp_le : pegtl::string<'<', '='> {};
struct comp_gt : pegtl::one<'>'> {};
struct comp_lt : pegtl::one<'<'> {};
struct comp_op : pegtl::sor<comp_ge, comp_le, comp_gt, comp_lt> {};

// Atomic patterns
struct element_symbol : pegtl::seq<pegtl::alpha, pegtl::opt<pegtl::alpha>> {};
struct chain_id : pegtl::alpha {};

// Property specifiers with values
struct name_spec : pegtl::seq<kw_name, ws_required, value_list> {};
struct resn_spec : pegtl::seq<kw_resn, ws_required, value_list> {};
struct resi_comp_spec : pegtl::seq<kw_resi, ws_required, comp_op, ws, number> {};
struct resi_range_spec : pegtl::seq<kw_resi, ws_required, range> {};
struct resi_exact_spec : pegtl::seq<kw_resi, ws_required, number> {};
struct resi_spec : pegtl::sor<resi_comp_spec, resi_range_spec, resi_exact_spec> {};
struct chain_spec : pegtl::seq<kw_chain, ws_required, chain_id> {};
struct elem_spec : pegtl::seq<kw_elem, ws_required, element_symbol> {};
struct index_comp_spec : pegtl::seq<kw_index, ws_required, comp_op, ws, number> {};
struct index_range_spec : pegtl::seq<kw_index, ws_required, range> {};
struct index_exact_spec : pegtl::seq<kw_index, ws_required, number> {};
struct index_spec : pegtl::sor<index_comp_spec, index_range_spec, index_exact_spec> {};

// Keyword-only specifiers
struct protein_spec : kw_protein {};
struct ligand_spec : kw_ligand {};
struct water_spec : kw_water {};
struct solvent_spec : kw_solvent {};
struct organic_spec : kw_organic {};
struct backbone_spec : kw_backbone {};
struct sidechain_spec : kw_sidechain {};
struct metal_spec : kw_metal {};

struct heavy_spec : kw_heavy {};
struct polar_hydrogen_spec : kw_polar_hydrogen {};
struct nonpolar_hydrogen_spec : kw_nonpolar_hydrogen {};
struct hydrogen_spec : kw_hydrogen {};

struct helix_spec : kw_helix {};
struct sheet_spec : kw_sheet {};
struct turn_spec : kw_turn {};
struct loop_spec : kw_loop {};

struct all_spec : kw_all {};
struct none_spec : kw_none {};

// Hierarchical macro: //chain/resi/name
struct macro_prefix : pegtl::string<'/', '/'> {};
struct macro_chain : pegtl::opt<chain_id> {};
struct macro_resi : pegtl::opt<number> {};
struct macro_name : pegtl::opt<glob_pattern> {};
struct macro_spec : pegtl::seq<
    macro_prefix,
    macro_chain,
    pegtl::one<'/'>,
    macro_resi,
    pegtl::one<'/'>,
    macro_name
> {};

// Forward declarations for recursive grammar
struct expression;
struct primary;

// Parentheses markers for action tracking
struct open_paren : pegtl::one<'('> {};
struct close_paren : pegtl::one<')'> {};

struct paren_expr : pegtl::seq<
    open_paren,
    ws,
    expression,
    ws,
    close_paren
> {};

// Distance specifiers with nested selection
struct around_spec : pegtl::seq<kw_around, ws_required, float_num, ws_required, primary> {};
struct xaround_spec : pegtl::seq<kw_xaround, ws_required, float_num, ws_required, primary> {};
struct beyond_spec : pegtl::seq<kw_beyond, ws_required, float_num, ws_required, primary> {};

// Expansion specifiers
struct byres_spec : pegtl::seq<kw_byres, ws_required, primary> {};
struct bychain_spec : pegtl::seq<kw_bychain, ws_required, primary> {};

// All specifiers (order matters - longer matches first)
struct specifier : pegtl::sor<
    macro_spec,
    name_spec, resn_spec, resi_spec, chain_spec, elem_spec, index_spec,
    protein_spec, ligand_spec, water_spec, solvent_spec, organic_spec,
    backbone_spec, sidechain_spec, metal_spec,
    helix_spec, sheet_spec, turn_spec, loop_spec,
    heavy_spec, polar_hydrogen_spec, nonpolar_hydrogen_spec, hydrogen_spec,
    xaround_spec, around_spec, beyond_spec,
    bychain_spec, byres_spec,
    all_spec, none_spec
> {};

struct primary : pegtl::sor<paren_expr, specifier> {};

// Operator markers for action tracking
struct not_op : kw_not {};
struct and_op : kw_and {};
struct or_op : kw_or {};
struct xor_op : kw_xor {};

// Expression grammar with precedence
struct not_expr : pegtl::sor<
    pegtl::seq<not_op, ws_required, not_expr>,
    primary
> {};

struct and_expr : pegtl::seq<
    not_expr,
    pegtl::star<pegtl::seq<ws_required, and_op, ws_required, not_expr>>
> {};

struct or_expr : pegtl::seq<
    and_expr,
    pegtl::star<pegtl::seq<ws_required, or_op, ws_required, and_expr>>
> {};

struct xor_expr : pegtl::seq<
    or_expr,
    pegtl::star<pegtl::seq<ws_required, xor_op, ws_required, or_expr>>
> {};

struct expression : xor_expr {};
struct selection : pegtl::seq<ws, expression, ws, pegtl::eof> {};

}  // namespace Grammar

// ============================================================================
// Parser State
// ============================================================================

/// Tracks operator counts at a given nesting level
struct OpContext {
    int and_count = 0;
    int or_count = 0;
    int xor_count = 0;
    int not_count = 0;
};

/// Parser state with operand stack and operator context
struct ParserState {
    std::stack<Predicate::Ptr> operands;
    std::string current_value;
    std::vector<std::string> value_list;

    // Numeric state for resi/index
    int first_number = 0;
    int second_number = 0;
    ResiPredicate::Op comp_op = ResiPredicate::Op::Eq;

    // Distance predicate state
    float current_radius = 0.0f;

    // Hierarchical macro state
    std::string macro_chain;
    int macro_resi = -1;
    std::string macro_name;

    // Operator context stack (one per nesting level)
    std::stack<OpContext> contexts;

    ParserState() {
        contexts.push(OpContext{});
    }

    OpContext& currentContext() { return contexts.top(); }

    void pushContext() { contexts.push(OpContext{}); }

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

// ============================================================================
// Internal Predicate Implementations
// ============================================================================

/// Always-true predicate for empty selections and 'all' keyword
class TruePredicateImpl : public Predicate {
public:
    bool Evaluate(Context&, const OEChem::OEAtomBase&) const override { return true; }
    [[nodiscard]] std::string ToCanonical() const override { return "all"; }
    [[nodiscard]] PredicateType Type() const override { return PredicateType::True; }
};

/// Always-false predicate for 'none' keyword
class FalsePredicateImpl : public Predicate {
public:
    bool Evaluate(Context&, const OEChem::OEAtomBase&) const override { return false; }
    [[nodiscard]] std::string ToCanonical() const override { return "none"; }
    [[nodiscard]] PredicateType Type() const override { return PredicateType::False; }
};

// ============================================================================
// Parser Actions
// ============================================================================

template<typename Rule>
struct Action : pegtl::nothing<Rule> {};

// Value capture
template<>
struct Action<Grammar::value> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, ParserState& state) {
        std::string val = in.string();
        // Strip quotes if present
        if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
            val = val.substr(1, val.size() - 2);
        }
        state.current_value = val;
        state.value_list.push_back(val);
    }
};

// Clear value list on keyword
template<>
struct Action<Grammar::kw_name> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.value_list.clear();
    }
};

template<>
struct Action<Grammar::kw_resn> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.value_list.clear();
    }
};

// Name specifier with multi-value support
template<>
struct Action<Grammar::name_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        if (state.value_list.size() == 1) {
            state.pushOperand(std::make_shared<NamePredicate>(state.value_list[0]));
        } else {
            // Multiple values become OR predicate
            std::vector<Predicate::Ptr> children;
            for (const auto& val : state.value_list) {
                children.push_back(std::make_shared<NamePredicate>(val));
            }
            state.pushOperand(std::make_shared<OrPredicate>(std::move(children)));
        }
        state.value_list.clear();
    }
};

template<>
struct Action<Grammar::resn_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        if (state.value_list.size() == 1) {
            state.pushOperand(std::make_shared<ResnPredicate>(state.value_list[0]));
        } else {
            std::vector<Predicate::Ptr> children;
            for (const auto& val : state.value_list) {
                children.push_back(std::make_shared<ResnPredicate>(val));
            }
            state.pushOperand(std::make_shared<OrPredicate>(std::move(children)));
        }
        state.value_list.clear();
    }
};

// Number capture
template<>
struct Action<Grammar::number> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, ParserState& state) {
        state.second_number = state.first_number;
        state.first_number = std::stoi(in.string());
    }
};

// Comparison operators
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

// Residue specifiers
template<>
struct Action<Grammar::resi_comp_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<ResiPredicate>(state.first_number, state.comp_op));
        state.comp_op = ResiPredicate::Op::Eq;
    }
};

template<>
struct Action<Grammar::resi_range_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
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

// Chain and element
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
struct Action<Grammar::index_comp_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        IndexPredicate::Op index_op;
        switch (state.comp_op) {
            case ResiPredicate::Op::Lt: index_op = IndexPredicate::Op::Lt; break;
            case ResiPredicate::Op::Le: index_op = IndexPredicate::Op::Le; break;
            case ResiPredicate::Op::Gt: index_op = IndexPredicate::Op::Gt; break;
            case ResiPredicate::Op::Ge: index_op = IndexPredicate::Op::Ge; break;
            default: index_op = IndexPredicate::Op::Eq; break;
        }
        state.pushOperand(std::make_shared<IndexPredicate>(
            static_cast<unsigned int>(state.first_number), index_op));
        state.comp_op = ResiPredicate::Op::Eq;
    }
};

template<>
struct Action<Grammar::index_range_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<IndexPredicate>(
            static_cast<unsigned int>(state.second_number),
            static_cast<unsigned int>(state.first_number)));
    }
};

template<>
struct Action<Grammar::index_exact_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<IndexPredicate>(
            static_cast<unsigned int>(state.first_number), IndexPredicate::Op::Eq));
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

// Secondary structure specifiers
template<>
struct Action<Grammar::helix_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<HelixPredicate>());
    }
};

template<>
struct Action<Grammar::sheet_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<SheetPredicate>());
    }
};

template<>
struct Action<Grammar::turn_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<TurnPredicate>());
    }
};

template<>
struct Action<Grammar::loop_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<LoopPredicate>());
    }
};

// Special specifiers
template<>
struct Action<Grammar::all_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<TruePredicateImpl>());
    }
};

template<>
struct Action<Grammar::none_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushOperand(std::make_shared<FalsePredicateImpl>());
    }
};

// Hierarchical macro actions
template<>
struct Action<Grammar::macro_prefix> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.macro_chain.clear();
        state.macro_resi = -1;
        state.macro_name.clear();
    }
};

template<>
struct Action<Grammar::macro_chain> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, ParserState& state) {
        state.macro_chain = in.string();
    }
};

template<>
struct Action<Grammar::macro_resi> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, ParserState& state) {
        if (std::string s = in.string(); !s.empty()) {
            state.macro_resi = std::stoi(s);
        }
    }
};

template<>
struct Action<Grammar::macro_name> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, ParserState& state) {
        state.macro_name = in.string();
    }
};

template<>
struct Action<Grammar::macro_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        std::vector<Predicate::Ptr> conditions;

        if (!state.macro_chain.empty()) {
            conditions.push_back(std::make_shared<ChainPredicate>(state.macro_chain));
        }
        if (state.macro_resi >= 0) {
            conditions.push_back(std::make_shared<ResiPredicate>(
                state.macro_resi, ResiPredicate::Op::Eq));
        }
        if (!state.macro_name.empty()) {
            conditions.push_back(std::make_shared<NamePredicate>(state.macro_name));
        }

        if (conditions.empty()) {
            state.pushOperand(std::make_shared<TruePredicateImpl>());
        } else if (conditions.size() == 1) {
            state.pushOperand(std::move(conditions[0]));
        } else {
            state.pushOperand(std::make_shared<AndPredicate>(std::move(conditions)));
        }
    }
};

// Float capture for distance predicates
template<>
struct Action<Grammar::float_num> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, ParserState& state) {
        state.current_radius = std::stof(in.string());
    }
};

// Distance specifiers
template<>
struct Action<Grammar::around_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        auto reference = state.popOperand();
        state.pushOperand(std::make_shared<AroundPredicate>(state.current_radius, std::move(reference)));
    }
};

template<>
struct Action<Grammar::xaround_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        auto reference = state.popOperand();
        state.pushOperand(std::make_shared<XAroundPredicate>(state.current_radius, std::move(reference)));
    }
};

template<>
struct Action<Grammar::beyond_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        auto reference = state.popOperand();
        state.pushOperand(std::make_shared<BeyondPredicate>(state.current_radius, std::move(reference)));
    }
};

// Expansion specifiers
template<>
struct Action<Grammar::byres_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        auto child = state.popOperand();
        state.pushOperand(std::make_shared<ByResPredicate>(std::move(child)));
    }
};

template<>
struct Action<Grammar::bychain_spec> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        auto child = state.popOperand();
        state.pushOperand(std::make_shared<ByChainPredicate>(std::move(child)));
    }
};

// Parentheses - context management
template<>
struct Action<Grammar::open_paren> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.pushContext();
    }
};

template<>
struct Action<Grammar::close_paren> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.popContext();
    }
};

// Operator tracking
template<>
struct Action<Grammar::not_op> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.currentContext().not_count++;
    }
};

template<>
struct Action<Grammar::and_op> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.currentContext().and_count++;
    }
};

template<>
struct Action<Grammar::or_op> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.currentContext().or_count++;
    }
};

template<>
struct Action<Grammar::xor_op> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        state.currentContext().xor_count++;
    }
};

// Apply NOT
template<>
struct Action<Grammar::not_expr> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        if (auto& ctx = state.currentContext(); ctx.not_count > 0) {
            const int not_count = ctx.not_count;
            ctx.not_count = 0;

            if (!state.operands.empty()) {
                for (int i = 0; i < not_count; ++i) {
                    auto operand = state.popOperand();
                    state.pushOperand(std::make_shared<NotPredicate>(std::move(operand)));
                }
            }
        }
    }
};

// Apply AND
template<>
struct Action<Grammar::and_expr> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        if (auto& ctx = state.currentContext(); ctx.and_count > 0) {
            const int and_count = ctx.and_count;
            ctx.and_count = 0;

            if (state.operands.size() >= static_cast<size_t>(and_count) + 1) {
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

// Apply OR
template<>
struct Action<Grammar::or_expr> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        if (auto& ctx = state.currentContext(); ctx.or_count > 0) {
            const int or_count = ctx.or_count;
            ctx.or_count = 0;

            if (state.operands.size() >= static_cast<size_t>(or_count) + 1) {
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

// Apply XOR
template<>
struct Action<Grammar::xor_expr> {
    template<typename ActionInput>
    static void apply(const ActionInput&, ParserState& state) {
        if (auto& ctx = state.currentContext(); ctx.xor_count > 0) {
            const int xor_count = ctx.xor_count;
            ctx.xor_count = 0;

            if (state.operands.size() >= static_cast<size_t>(xor_count) + 1) {
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

// ============================================================================
// Public API
// ============================================================================

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
