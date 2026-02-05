/**
 * @file Context.h
 * @brief Evaluation context with caching for predicate evaluation.
 *
 * The Context class provides shared state and caches for efficient
 * predicate evaluation against a single molecule.
 */

#ifndef OESELECT_CONTEXT_H
#define OESELECT_CONTEXT_H

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace OEChem {
class OEMolBase;
class OEAtomBase;
}

namespace OESel {

class OESelection;
class SpatialIndex;

/**
 * @brief Evaluation context with caching for a single molecule.
 *
 * Context maintains shared state during predicate evaluation, including:
 * - Reference to the molecule being evaluated
 * - Spatial index for distance queries (lazily initialized)
 * - Caches for residue, chain, and distance-based selections
 *
 * Caches use string keys derived from predicate canonical forms to ensure
 * equivalent predicates share cached results.
 *
 * @note Context is non-copyable as it holds mutable caches.
 */
class Context {
public:
    /**
     * @brief Construct context for molecule and selection.
     *
     * @param mol The molecule to evaluate against.
     * @param sele The selection being evaluated.
     */
    Context(OEChem::OEMolBase& mol, const OESelection& sele);

    /// @brief Destructor.
    ~Context();

    // Non-copyable
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    /// @brief Access the molecule (mutable).
    OEChem::OEMolBase& Mol();

    /// @brief Access the molecule (const).
    const OEChem::OEMolBase& Mol() const;

    /// @brief Access the selection being evaluated.
    const OESelection& Sele() const;

    /**
     * @brief Get or create the spatial index.
     *
     * The spatial index is created lazily on first access. It provides
     * efficient radius queries for distance-based predicates.
     *
     * @return Reference to the spatial index.
     */
    SpatialIndex& GetSpatialIndex();

    /// @name Residue Cache
    /// Cache for byres predicate results.
    /// @{

    /**
     * @brief Get cached atom indices for a residue-based selection.
     * @param key Cache key (typically predicate canonical form).
     * @return Set of atom indices in matching residues.
     */
    const std::unordered_set<unsigned int>& GetResidueAtoms(const std::string& key);

    /**
     * @brief Store atom indices for a residue-based selection.
     * @param key Cache key.
     * @param atoms Set of atom indices.
     */
    void SetResidueAtoms(const std::string& key, std::unordered_set<unsigned int> atoms);

    /**
     * @brief Check if residue cache contains a key.
     * @param key Cache key to check.
     * @return true if the cache contains this key.
     */
    bool HasResidueCache(const std::string& key) const;

    /// @}

    /// @name Chain Cache
    /// Cache for bychain predicate results.
    /// @{

    /**
     * @brief Get cached atom indices for a chain-based selection.
     * @param key Cache key.
     * @return Set of atom indices in matching chains.
     */
    const std::unordered_set<unsigned int>& GetChainAtoms(const std::string& key);

    /**
     * @brief Store atom indices for a chain-based selection.
     * @param key Cache key.
     * @param atoms Set of atom indices.
     */
    void SetChainAtoms(const std::string& key, std::unordered_set<unsigned int> atoms);

    /**
     * @brief Check if chain cache contains a key.
     * @param key Cache key to check.
     * @return true if the cache contains this key.
     */
    bool HasChainCache(const std::string& key) const;

    /// @}

    /// @name Distance Cache
    /// Cache for around/beyond predicate results.
    /// @{

    /**
     * @brief Get cached atom mask for distance-based selection.
     * @param key Cache key.
     * @return Boolean mask indexed by atom index.
     */
    const std::vector<bool>& GetAroundCache(const std::string& key);

    /**
     * @brief Store atom mask for distance-based selection.
     * @param key Cache key.
     * @param cache Boolean mask.
     */
    void SetAroundCache(const std::string& key, std::vector<bool> cache);

    /**
     * @brief Check if distance cache contains a key.
     * @param key Cache key to check.
     * @return true if the cache contains this key.
     */
    bool HasAroundCache(const std::string& key) const;

    /// @}

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;  ///< PIMPL for binary compatibility
};

}  // namespace OESel

#endif  // OESELECT_CONTEXT_H
