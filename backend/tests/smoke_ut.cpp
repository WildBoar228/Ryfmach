#include "language.hpp"

#include <gtest/gtest.h>

namespace {

TEST(Language, ParsesBelarusianWord) {
    const auto letters = ryfmach::bel::ParseWord("Беларусь");

    ASSERT_TRUE(letters.has_value());
    ASSERT_EQ(letters->size(), 8);
    EXPECT_EQ((*letters)[0], ryfmach::bel::Letter::kBe);
    EXPECT_EQ((*letters)[7], ryfmach::bel::Letter::kSoftSign);
}

TEST(Language, ParsesDashInsideBelarusianWord) {
    const auto letters = ryfmach::bel::ParseWord("што-небудзь");

    ASSERT_TRUE(letters.has_value());
    ASSERT_EQ(letters->size(), 11);
    EXPECT_EQ((*letters)[3], ryfmach::bel::Letter::kDash);
    EXPECT_TRUE(ryfmach::bel::IsDash((*letters)[3]));
}

TEST(Language, RejectsDashAtWordBoundary) {
    EXPECT_FALSE(ryfmach::bel::ParseWord("-небудзь").has_value());
    EXPECT_FALSE(ryfmach::bel::ParseWord("што-").has_value());
    EXPECT_FALSE(ryfmach::bel::ParseWord("што--небудзь").has_value());
}

TEST(Language, ClassifiesLetters) {
    EXPECT_TRUE(ryfmach::bel::IsVowel(ryfmach::bel::Letter::kA));
    EXPECT_TRUE(ryfmach::bel::IsConsonant(ryfmach::bel::Letter::kBe));
    EXPECT_TRUE(ryfmach::bel::IsSofteningVowel(ryfmach::bel::Letter::kYa));
    EXPECT_FALSE(ryfmach::bel::IsAlphabetLetter(
        ryfmach::bel::Letter::kApostrophe));
    EXPECT_FALSE(ryfmach::bel::IsAlphabetLetter(ryfmach::bel::Letter::kDash));
}

TEST(Language, SpellsPhonemesWithCyrillicText) {
    EXPECT_EQ(ryfmach::bel::PhonemeSpelling(ryfmach::bel::Phoneme::kBSoft), "б'");
    EXPECT_EQ(ryfmach::bel::PhonemeSpelling(ryfmach::bel::Phoneme::kW), "ў");
}

} // namespace
