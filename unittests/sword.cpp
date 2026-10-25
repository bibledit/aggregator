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


#include "sword/sword.h"
#include <filesystem>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "utilities.h"
#include "exception.h"


namespace {
class sword_lib : public testing::Test
{
public:
    std::filesystem::path store_path{};
protected:
    sword_lib() {
        // No interference with production store: Use temporal file.
        store_path = utilities::tmp_file();
        std::error_code ec;
        std::filesystem::remove(store_path, ec);
    }
    ~sword_lib() override {
        store_path.clear();
    }
};
}

TEST_F(sword_lib, parse_module)
{
    {
        constexpr std::string_view line {"*[ymp2025eb]  	(2.7)  	- Yamap"};
        EXPECT_EQ(sword::parse(line)->abbrev, "ymp2025eb");
        EXPECT_EQ(sword::parse(line)->version, "2.7");
        EXPECT_EQ(sword::parse(line)->name, "Yamap");
    }

    {
        constexpr std::string_view line {"*[peg2020eb]  	(4.7)  	- ସତ୍‌ ବଚନ୍"};
        EXPECT_EQ(sword::parse(line)->abbrev, "peg2020eb");
        EXPECT_EQ(sword::parse(line)->version, "4.7");
        EXPECT_EQ(sword::parse(line)->name, "ସତ୍‌ ବଚନ୍");
    }

    {
        constexpr std::string_view line {"*[pan2017eb]  	(21.37)  	- ਇੰਡਿਅਨ ਰਿਵਾਇਜ਼ਡ ਵਰਜ਼ਨ (IRV) - ਪੰਜਾਬੀ"};
        EXPECT_EQ(sword::parse(line)->abbrev, "pan2017eb");
        EXPECT_EQ(sword::parse(line)->version, "21.37");
        EXPECT_EQ(sword::parse(line)->name, "ਇੰਡਿਅਨ ਰਿਵਾਇਜ਼ਡ ਵਰਜ਼ਨ (IRV) - ਪੰਜਾਬੀ");
    }

    {
        constexpr std::string_view line {"*[ABSMaps]  	(1.071229)  	- Maps by American Bible Society (1888)"};
        EXPECT_EQ(sword::parse(line)->abbrev, "ABSMaps");
        EXPECT_EQ(sword::parse(line)->version, "1.071229");
        EXPECT_EQ(sword::parse(line)->name, "Maps by American Bible Society (1888)");
    }

    {
        constexpr std::string_view line {"id]  	(1.1)  	- Name"};
        EXPECT_FALSE(sword::parse(line));
    }
    {
        constexpr std::string_view line {"[id  	(1.1)  	- Name"};
        EXPECT_FALSE(sword::parse(line));
    }
    {
        constexpr std::string_view line {"[id]  	1.1)  	- Name"};
        EXPECT_FALSE(sword::parse(line));
    }
    {
        constexpr std::string_view line {"[id]  	(1.1  	- Name"};
        EXPECT_FALSE(sword::parse(line));
    }
}


TEST_F(sword_lib, download_required)
{
    constexpr auto remote_source = "source";

    store::Store store(store_path); // empty.

    struct TestSword : Sword
    {
        // Inherit the user-defined constructors.
        using Sword::Sword;
        // Make Sword's protected member accessible for testing.
        using Sword::download_required;
    };

    TestSword sword (store);
    sword.remote_source(remote_source);

    constexpr auto get_info = []
    {
        return sword::info {
            .abbrev = "abbrev",
            .version = "1.0",
            .name = "name"
        };
    };
    const auto get_module = [&]
    {
        return store::Module {
            .id = 1,
            .type = store::Type::sword,
            .source = remote_source,
            .abbrev = get_info().abbrev,
            .version = get_info().version,
            .name = get_info().name,
        };
    };

    // Module not in store: Download req.
    EXPECT_TRUE(sword.download_required(get_info()));

    // Put the module in the Store. Different version number -> Download req.
    {
        auto module = get_module();
        module.version = "different";
        store.add_or_update(module);
        EXPECT_TRUE(sword.download_required(get_info()));
    }
    store.add_or_update(get_module()); // restore.







}