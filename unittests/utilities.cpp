/*
Copyright (©) 2026-2026 Teus Benschop.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unicode/urename.h>
#include "utilities.h"


TEST(utilities, get_next_id)
{
    // No interference with production data: Use temporal file.
    const auto test_path = utilities::tmp_file();
    std::error_code ec;
    std::filesystem::remove(test_path, ec);

    for (unsigned id {1}; id < 10; ++id)
        EXPECT_EQ (utilities::get_next_id(test_path), id);
}


TEST(utilities, similarity)
{
    EXPECT_EQ(utilities::similarity_percentage("kitten",          "sitting"), 57);
    EXPECT_EQ(utilities::similarity_percentage("abc",             "def"),      0);
    EXPECT_EQ(utilities::similarity_percentage("",                ""),       100);
    EXPECT_EQ(utilities::similarity_percentage("A",               "A"),      100);
    EXPECT_EQ(utilities::similarity_percentage("Exodus",          "Exodus"), 100);
    EXPECT_EQ(utilities::similarity_percentage("Genesis",         "Genes"),   71);
    EXPECT_EQ(utilities::similarity_percentage("Song of Solomon", "Songs"),   26);
}


TEST(utilities, case_fold)
{
    EXPECT_EQ(utilities::unicode_case_fold("ABC"), "abc");
}


TEST(utilities, replace_xml)
{
    constexpr auto input1 = R"(This is the account of the heavens and<milestone type="line" /><milestone type="line" />the earth when they were created - when the LORD God made the earth and heavens. <milestone type="line" /><milestone type="line" />)";
    constexpr auto output1 = R"(This is the account of the heavens and  the earth when they were created - when the LORD God made the earth and heavens.   )";
    EXPECT_EQ(utilities::replace_xml_elements(input1), output1);
}


TEST(utilities, strip_xml)
{
    constexpr auto input1 = R"(And the Spirit and the bride say, "Come!" And let the one who hears say: "Come!" And let the one who is thirsty come; let the one who wants it take the water of life free of charge.<milestone type="line" /><milestone type="line" />)";
    constexpr auto output1 = R"(And the Spirit and the bride say, "Come!" And let the one who hears say: "Come!" And let the one who is thirsty come; let the one who wants it take the water of life free of charge.)";
    EXPECT_EQ(utilities::strip_xml_elements(input1), output1);

    constexpr auto input2 = R"(This is the account of the heavens and<milestone type="line" /><milestone type="line" />the earth when they were created - when the <divineName type="x-yhwh">LORD</divineName> God made the earth and heavens. <milestone type="line" /><milestone type="line" />)";
    constexpr auto output2 = R"(This is the account of the heavens andthe earth when they were created - when the LORD God made the earth and heavens. )";
    EXPECT_EQ(utilities::strip_xml_elements(input2), output2);
}


TEST(utilities, remove_double_spaces)
{
    std::string line {"A  B   C"};
    utilities::remove_double_spaces(line);
    EXPECT_EQ(line, "A B C");
}
