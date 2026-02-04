// src/Selector.cpp
#include "oeselect/Selector.h"
#include "oeselect/Context.h"

#include <oechem.h>

namespace OESel {

struct OESelect::Impl {
    std::unique_ptr<Context> ctx;
    OESelection sele;

    Impl(OEChem::OEMolBase& mol, const OESelection& s)
        : ctx(std::make_unique<Context>(mol, s)), sele(s) {}
};

OESelect::OESelect(OEChem::OEMolBase& mol, const OESelection& sele)
    : pimpl_(std::make_unique<Impl>(mol, sele)) {}

OESelect::OESelect(OEChem::OEMolBase& mol, const std::string& sele)
    : OESelect(mol, OESelection::Parse(sele)) {}

OESelect::OESelect(const OESelect& other)
    : pimpl_(std::make_unique<Impl>(other.pimpl_->ctx->Mol(), other.pimpl_->sele)) {}

OESelect& OESelect::operator=(const OESelect& other) {
    if (this != &other) {
        pimpl_ = std::make_unique<Impl>(other.pimpl_->ctx->Mol(), other.pimpl_->sele);
    }
    return *this;
}

OESelect::~OESelect() = default;

bool OESelect::operator()(const OEChem::OEAtomBase& atom) const {
    return pimpl_->sele.Root().Evaluate(*pimpl_->ctx, atom);
}

OESystem::OEUnaryFunction<OEChem::OEAtomBase, bool>* OESelect::CreateCopy() const {
    return new OESelect(*this);
}

const OESelection& OESelect::GetSelection() const {
    return pimpl_->sele;
}

}  // namespace OESel
