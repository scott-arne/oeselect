// include/oeselect/oeselect.h
#ifndef OESELECT_OESELECT_H
#define OESELECT_OESELECT_H

#define OESELECT_VERSION_MAJOR 1
#define OESELECT_VERSION_MINOR 0
#define OESELECT_VERSION_PATCH 0

namespace OESel {

class OESelection;
class OESelect;
class Context;
class Predicate;
class Tagger;

}  // namespace OESel

#include "oeselect/Error.h"
#include "oeselect/Predicate.h"
#include "oeselect/Selection.h"
#include "oeselect/Selector.h"
#include "oeselect/Context.h"
#include "oeselect/Parser.h"
#include "oeselect/Tagger.h"

#endif  // OESELECT_OESELECT_H
