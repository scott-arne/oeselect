/**
 * @file oeselect.h
 * @brief Main umbrella header for the OESelect library.
 *
 * Include this header to access all OESelect functionality. It provides
 * PyMOL-style atom selection for OpenEye Toolkit molecules.
 *
 * @code
 * #include <oeselect/oeselect.h>
 *
 * auto sele = OESel::OESelection::Parse("protein and chain A");
 * OESel::OESelect sel(mol, sele);
 * for (auto& atom : mol.GetAtoms(sel)) {
 *     // process selected atoms
 * }
 * @endcode
 */

#ifndef OESELECT_OESELECT_H
#define OESELECT_OESELECT_H

/** @brief Major version number */
#define OESELECT_VERSION_MAJOR 1
/** @brief Minor version number */
#define OESELECT_VERSION_MINOR 0
/** @brief Patch version number */
#define OESELECT_VERSION_PATCH 1

/**
 * @namespace OESel
 * @brief OESelect library namespace containing all selection classes and utilities.
 */
namespace OESel {

// Forward declarations
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
#include "oeselect/ResidueSelector.h"
#include "oeselect/CustomPredicates.h"

#endif  // OESELECT_OESELECT_H
