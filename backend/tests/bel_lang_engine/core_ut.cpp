#include "language.hpp"
#include "sounds.hpp"

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
    EXPECT_EQ(
        ryfmach::bel::PhonemeSpelling(ryfmach::bel::Phoneme::kBSoft), "б'");
    EXPECT_EQ(ryfmach::bel::PhonemeSpelling(ryfmach::bel::Phoneme::kW), "ў");
}

TEST(Sounds, FindsRingAndThudPairs) {
    EXPECT_TRUE(ryfmach::bel::IsRing(ryfmach::bel::Phoneme::kB));
    EXPECT_TRUE(ryfmach::bel::IsThud(ryfmach::bel::Phoneme::kP));
    EXPECT_EQ(ryfmach::bel::RingPair(ryfmach::bel::Phoneme::kP),
              ryfmach::bel::Phoneme::kB);
    EXPECT_EQ(ryfmach::bel::ThudPair(ryfmach::bel::Phoneme::kBSoft),
              ryfmach::bel::Phoneme::kPSoft);
}

TEST(Sounds, FindsHardAndSoftPairs) {
    EXPECT_TRUE(ryfmach::bel::IsHard(ryfmach::bel::Phoneme::kS));
    EXPECT_TRUE(ryfmach::bel::IsSoft(ryfmach::bel::Phoneme::kSSoft));
    EXPECT_EQ(ryfmach::bel::SoftPair(ryfmach::bel::Phoneme::kS),
              ryfmach::bel::Phoneme::kSSoft);
    EXPECT_EQ(ryfmach::bel::HardPair(ryfmach::bel::Phoneme::kSSoft),
              ryfmach::bel::Phoneme::kS);
}

TEST(Sounds, HandlesMissingPairs) {
    EXPECT_TRUE(ryfmach::bel::IsSonor(ryfmach::bel::Phoneme::kW));
    EXPECT_FALSE(ryfmach::bel::ThudPair(ryfmach::bel::Phoneme::kW).has_value());
    EXPECT_FALSE(ryfmach::bel::SoftPair(ryfmach::bel::Phoneme::kR).has_value());
    EXPECT_FALSE(ryfmach::bel::RingPair(ryfmach::bel::Phoneme::kA).has_value());
}

TEST(Sounds, FindsWhistlingAndHissingPairs) {
    EXPECT_TRUE(ryfmach::bel::IsWhistling(ryfmach::bel::Phoneme::kDzSoft));
    EXPECT_TRUE(ryfmach::bel::IsHissing(ryfmach::bel::Phoneme::kDzh));
    EXPECT_EQ(ryfmach::bel::WhistlingPair(ryfmach::bel::Phoneme::kDzh),
              ryfmach::bel::Phoneme::kDz);
    EXPECT_EQ(ryfmach::bel::HissingPair(ryfmach::bel::Phoneme::kTsSoft),
              ryfmach::bel::Phoneme::kCh);
}

TEST(Sounds, DescribesVowelsAndConsonantsInBelarusian) {
    EXPECT_EQ(
        ryfmach::bel::SoundDescription(
            {.phoneme = ryfmach::bel::Phoneme::kA, .stressed = false}),
        "галосны, ненаціскны");
    EXPECT_EQ(
        ryfmach::bel::SoundDescription(
            {.phoneme = ryfmach::bel::Phoneme::kA, .stressed = true}),
        "галосны, націскны");
    EXPECT_EQ(
        ryfmach::bel::SoundDescription(
            {.phoneme = ryfmach::bel::Phoneme::kB, .stressed = false}),
        "зычны, звонкі парны [п], цвёрды парны [б']");
    EXPECT_EQ(
        ryfmach::bel::SoundDescription(
            {.phoneme = ryfmach::bel::Phoneme::kW, .stressed = false}),
        "зычны, звонкі няпарны, цвёрды няпарны");
}


} // namespace
