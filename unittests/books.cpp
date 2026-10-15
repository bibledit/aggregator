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
#include "books/books.h"
#include "utilities.h"

namespace {
class test_books  : public testing::Test
{
public:
    test_books()
    {
        books::load();
    }
};
}

TEST_F(test_books, basic)
{
    EXPECT_EQ(books::count(), 109);
    const auto osis = books::get_all_osis();
    EXPECT_EQ(osis.size(), 84);
    EXPECT_TRUE(std::ranges::find(osis, "John") != osis.cend());

    const auto osis_ot = books::get_all_osis(books::BookType::OldTestament);
    EXPECT_EQ(osis_ot.size(), 39);
    EXPECT_TRUE(std::ranges::find(osis_ot, "Deut") != osis_ot.cend());

    const auto osis_nt = books::get_all_osis(books::BookType::NewTestament);
    EXPECT_EQ(osis_nt.size(), 27);
    EXPECT_TRUE(std::ranges::find(osis_nt, "John") != osis_nt.cend());

    EXPECT_EQ(books::get_osis(0), "");
    EXPECT_EQ(books::get_osis(1), "Gen");
    EXPECT_EQ(books::get_osis(66), "Rev");
}


TEST_F(test_books, discovery)
{
    EXPECT_EQ(books::get_id_fuzzy("Gene"), 1); // Genesis.
    EXPECT_EQ(books::get_id_fuzzy("Sng"), 22); // Songs.


    {
        const auto passage = books::get_passage("1 Samuel 1:2: Verse text...");
        EXPECT_TRUE(passage);
        if (passage)
        {
            EXPECT_EQ(passage->book_name, "1 Samuel");
            EXPECT_EQ(passage->book_id, 9);
            EXPECT_EQ(passage->chapter, 1);
            EXPECT_EQ(passage->verse, 2);
        }
    }

    {
        const auto passage = books::get_passage("John 1:1: In the beginning was the Word, and the Word was with God ...");
        EXPECT_TRUE(passage);
        if (passage)
        {
            EXPECT_EQ(passage->book_name, "John");
            EXPECT_EQ(passage->book_id, 43);
            EXPECT_EQ(passage->chapter, 1);
            EXPECT_EQ(passage->verse, 1);
        }
    }

    EXPECT_FALSE(books::get_passage("John In the beginning was the Word, and the Word was with God ..."));

    {
        const auto passage = books::get_passage("John 1 In the beginning was the Word, and the Word was with God ...");
        EXPECT_TRUE(passage);
        if (passage)
        {
            EXPECT_EQ(passage->book_name, "John");
            EXPECT_EQ(passage->book_id, 43);
            EXPECT_EQ(passage->chapter, 1);
            EXPECT_EQ(passage->verse, 0);
        }
    }
}