// tests/cpp/test_glob_match.cpp
// Unit tests for the portable match_glob helper used by Name/Resn predicates.

#include <gtest/gtest.h>

#include "glob_match.h"

using OESel::match_glob;

TEST(GlobMatch, ExactStringMatches) {
    EXPECT_TRUE(match_glob("CA", "CA"));
    EXPECT_FALSE(match_glob("CA", "CB"));
    EXPECT_FALSE(match_glob("CA", "CAB"));
    EXPECT_FALSE(match_glob("CA", ""));
    EXPECT_FALSE(match_glob("", "CA"));
    EXPECT_TRUE(match_glob("", ""));
}

TEST(GlobMatch, StarMatchesZeroOrMoreChars) {
    EXPECT_TRUE(match_glob("*", ""));
    EXPECT_TRUE(match_glob("*", "anything"));
    EXPECT_TRUE(match_glob("CA*", "CA"));
    EXPECT_TRUE(match_glob("CA*", "CAB"));
    EXPECT_TRUE(match_glob("CA*", "CAlpha"));
    EXPECT_FALSE(match_glob("CA*", "C"));
    EXPECT_FALSE(match_glob("CA*", "XCA"));
    EXPECT_TRUE(match_glob("*CA", "CA"));
    EXPECT_TRUE(match_glob("*CA", "NCA"));
    EXPECT_TRUE(match_glob("*CA", "XXCA"));
    EXPECT_FALSE(match_glob("*CA", "CAX"));
}

TEST(GlobMatch, QuestionMarkMatchesExactlyOneChar) {
    EXPECT_TRUE(match_glob("H?", "H1"));
    EXPECT_TRUE(match_glob("H?", "HA"));
    EXPECT_FALSE(match_glob("H?", "H"));
    EXPECT_FALSE(match_glob("H?", "HAB"));
    EXPECT_TRUE(match_glob("?", "A"));
    EXPECT_FALSE(match_glob("?", ""));
    EXPECT_FALSE(match_glob("?", "AB"));
}

TEST(GlobMatch, MixedStarAndQuestion) {
    EXPECT_TRUE(match_glob("C*A", "CA"));
    EXPECT_TRUE(match_glob("C*A", "CBA"));
    EXPECT_TRUE(match_glob("C*A", "ChainA"));
    EXPECT_FALSE(match_glob("C*A", "CB"));
    EXPECT_TRUE(match_glob("?*", "A"));
    EXPECT_TRUE(match_glob("?*", "ABC"));
    EXPECT_FALSE(match_glob("?*", ""));
}

TEST(GlobMatch, BiologicalAtomNames) {
    // Real-world patterns the parser sees for PDB atom names.
    EXPECT_TRUE(match_glob("CA", "CA"));
    EXPECT_TRUE(match_glob("H*", "HA"));
    EXPECT_TRUE(match_glob("H*", "HB1"));
    EXPECT_TRUE(match_glob("HD?", "HD1"));
    EXPECT_TRUE(match_glob("HD?", "HD2"));
    EXPECT_FALSE(match_glob("HD?", "HE1"));
}
