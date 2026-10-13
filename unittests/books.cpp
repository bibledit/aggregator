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

class test_books  : public testing::Test
{
public:
    test_books()
    {
        books::load();
    }
};

TEST_F(test_books, basic)
{
    EXPECT_EQ(books::count(), 109);
    const auto osis = books::get_all_osis();
    EXPECT_EQ(osis.size(), 84);
    EXPECT_TRUE(std::ranges::find(osis, "John") != osis.cend());
}
