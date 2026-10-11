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
