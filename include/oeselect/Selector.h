// include/oeselect/Selector.h
#ifndef OESELECT_SELECTOR_H
#define OESELECT_SELECTOR_H

#include <memory>
#include <string>

#include <oechem.h>

#include "oeselect/Selection.h"

namespace OESel {

/// Molecule-bound selection evaluator (compatible with OpenEye predicates)
class OESelect : public OESystem::OEUnaryPredicate<OEChem::OEAtomBase> {
public:
    /// Construct from molecule and parsed selection
    OESelect(OEChem::OEMolBase& mol, const OESelection& sele);

    /// Construct from molecule and selection string
    OESelect(OEChem::OEMolBase& mol, const std::string& sele);

    /// Copy constructor
    OESelect(const OESelect& other);
    OESelect& operator=(const OESelect& other);
    ~OESelect() override;

    /// Evaluate predicate for atom (OEUnaryAtomPred interface)
    bool operator()(const OEChem::OEAtomBase& atom) const override;

    /// Create copy for OpenEye compatibility
    OESystem::OEUnaryFunction<OEChem::OEAtomBase, bool>* CreateCopy() const override;

    /// Access underlying selection
    const OESelection& GetSelection() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

}  // namespace OESel

#endif  // OESELECT_SELECTOR_H
