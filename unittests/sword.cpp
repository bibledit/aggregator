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
#include "../store.h"
#include "exception.h"


namespace {
class storage : public testing::Test
{
public:
    std::filesystem::path store_path{};
protected:
    void SetUp() override {
        // No interference with production store: Use temporal file.
        store_path = utilities::tmp_file();
        std::error_code ec;
        std::filesystem::remove(store_path, ec);
    }
    void TearDown() override {
    }
};
}


// Testing data.
constexpr auto type_none  {store::Type::none};
constexpr auto type_sword {store::Type::sword};
constexpr auto id1        {1};
constexpr auto id2        {2};
constexpr auto source1    {"source1"};
constexpr auto source2    {"source2"};
constexpr auto abbrev1    {"abbrev1"};
constexpr auto abbrev2    {"abbrev2"};
constexpr auto version1   {"1.0"};
constexpr auto version2   {"2.0"};
constexpr auto name1      {"name1"};
constexpr auto name2      {"name2"};

constexpr store::Module module1 {
    .id = id1,
    .type = type_none,
    .source = source1,
    .abbrev = abbrev1,
    .version = version1,
    .name = name1
};
constexpr store::Module module2 {
    .id = id2,
    .type = type_sword,
    .source = source2,
    .abbrev = abbrev2,
    .version = version2,
    .name = name2
};


TEST_F(storage, initial_testing_store_is_empty)
{
    const store::Store store(store_path);
    EXPECT_EQ (store.count(), 0);
}


TEST_F(storage, save_load)
{
    store::Store store(store_path);

    store.add_or_update(module1);
    store.add_or_update(module2);

    {
        store::Store store2(store_path);
        EXPECT_NE(store, store2);
    }

    // Save store. Load JSON in a second store: Both should be the same.
    store.save_to_file();
    {
        store::Store store2(store_path);
        EXPECT_EQ(store, store2);
    }
}


TEST_F(storage, add_or_replace)
{
    constexpr store::Module module3 {.id = id1 + 2, .source = source2, .abbrev = abbrev1, .version = version1, .name = name1};
    constexpr store::Module module4 {.id = id2 + 2, .source = source2, .abbrev = abbrev2, .version = version2, .name = name2};

    store::Store store(store_path);

    // Test adding modules.
    store.add_or_update(module1);
    store.add_or_update(module2);
    EXPECT_EQ (store.count(), 2);
    store.add_or_update(module3);
    store.add_or_update(module4);
    EXPECT_EQ (store.count(), 4);

    // Test replacing modules.
    store.add_or_update(module1);
    store.add_or_update(module2);
    EXPECT_EQ (store.count(), 4);

    // Test that if replacing a module, if the replacement id differs, it throws.
    auto invalid_module = module1;
    invalid_module.id = 10;
    EXPECT_THROW(store.add_or_update(invalid_module), Base);
}


TEST_F(storage, only_type_differs)
{
    store::Store store(store_path);

    constexpr store::Module module10 {
        .id = id1,
        .type = type_none,
        .source = source1,
        .abbrev = abbrev1,
        .version = version1,
        .name = name1
    };
    store::Module module11 = module10;
    module11.type = type_sword;

    // If only the type differs, and the rest is the same, this is still a different module.
    store.add_or_update(module10);
    store.add_or_update(module11);
    EXPECT_EQ (store.count(), 2);
}


TEST_F(storage, update_module)
{
    store::Store store(store_path);

    store.add_or_update(module1);
    store.add_or_update(module2);
    EXPECT_EQ (store.count(), 2);

    auto updated_module1 = module1;
    updated_module1.version = "v2";
    store.add_or_update(updated_module1);
    EXPECT_EQ (store.count(), 2);

    updated_module1.name = "New Name";
    store.add_or_update(updated_module1);
    EXPECT_EQ (store.count(), 2);

    // On module update, but with different identifier, it throws.
    updated_module1.id = id1 + 1;
    EXPECT_THROW(store.add_or_update(updated_module1), Base);
}


TEST_F(storage, get_module)
{
    store::Store store(store_path);
    EXPECT_FALSE(store.get_module(module1.id));
    EXPECT_FALSE(store.get_module(module1.type, module1.source, module1.abbrev));
    constexpr std::array modules = {module1, module2};
    std::ranges::for_each(modules, [&](const auto& module)
    {
        store.add_or_update(module);
    });
    std::ranges::for_each(modules, [&](const auto& module)
    {
        EXPECT_EQ(store.get_module(module.id), module);
        EXPECT_EQ(store.get_module(module.type, module.source, module.abbrev), module);
    });
}

TEST_F(storage, count)
{
    store::Store store(store_path);
    store.add_or_update(module1);
    store.add_or_update(module2);
    EXPECT_EQ(store.count(), 2);
    EXPECT_EQ(store.count(type_none), 1);
    EXPECT_EQ(store.count(type_none, source1), 1);
    auto test_module = module1;
    test_module.abbrev = "abbrev";
    store.add_or_update(test_module);
    EXPECT_EQ(store.count(type_none), 2);
    EXPECT_EQ(store.count(type_none, source1), 2);
}
