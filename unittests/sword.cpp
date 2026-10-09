/*
Copyright (©) 2006-2026 Teus Benschop.

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


#include "sword/sword.h"
#include <filesystem>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "utilities.h"
#include "sword/store.h"

TEST(sword, store)
{
    // No interference with production store: Use temporal file.
    std::filesystem::path path = utilities::tmp_file();
    std::error_code ec;
    std::filesystem::remove(path, ec);

    // This loads the store with content from the given path.
    sword::Store store(path);

    // Testing data.
    constexpr auto source1 {"source1"};
    constexpr auto source2 {"source2"};
    constexpr auto identifier1 {"identifier1"};
    constexpr auto identifier2 {"identifier2"};
    constexpr auto version1 {"version1"};
    constexpr auto version2 {"version2"};
    constexpr auto name1 {"name1"};
    constexpr auto name2 {"name2"};
    constexpr sword::Module module1 {.source = source1, .identifier = identifier1, .version = version1, .name = name1};
    constexpr sword::Module module2 {.source = source1, .identifier = identifier2, .version = version2, .name = name2};
    constexpr sword::Module module3 {.source = source2, .identifier = identifier1, .version = version1, .name = name1};
    constexpr sword::Module module4 {.source = source2, .identifier = identifier2, .version = version2, .name = name2};

    // Test adding modules, no duplicate modules.
    EXPECT_EQ (store.size(), 0);
    store.add_or_replace(module1); // Adds.
    store.add_or_replace(module2);
    EXPECT_EQ (store.size(), 2);
    store.add_or_replace(module3);
    store.add_or_replace(module4);
    EXPECT_EQ (store.size(), 4);
    store.add_or_replace(module1); // Replaces.
    store.add_or_replace(module2);
    EXPECT_EQ (store.size(), 4);

    // Fetch item if exists, or null optional.
    EXPECT_FALSE(store.get_module(module1.source, "non-existing-identifier"));
    EXPECT_EQ(store.get_module(module1.source, module1.identifier), module1);
    EXPECT_NE(store.get_module(module2.source, module2.identifier), module1);
    EXPECT_EQ(store.get_module(module3.source, module3.identifier), module3);

    // Update item.
    sword::Module updated_module = module1;
    updated_module.version = "updated_version";
    updated_module.name = "updated_name";
    store.add_or_replace(updated_module);
    EXPECT_EQ(store.get_module(module1.source, module1.identifier), updated_module);

    // Save store. Load JSON in a second store: Both should be the same.
    store.save_to_file();
    sword::Store store2(path);
    EXPECT_EQ(store, store2);
}
