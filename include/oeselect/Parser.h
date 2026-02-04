// include/oeselect/Parser.h
#ifndef OESELECT_PARSER_H
#define OESELECT_PARSER_H

#include <string>

#include "oeselect/Predicate.h"

namespace OESel {

/// Parse a selection string into a predicate tree
Predicate::Ptr ParseSelection(const std::string& sele);

}  // namespace OESel

#endif  // OESELECT_PARSER_H
