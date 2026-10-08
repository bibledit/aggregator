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
#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST (sword, parse)
{
    {
        constexpr std::string_view line {"*[ymp2025eb]  	(2.7)  	- Yamap"};
        const std::optional<sword::info> info = sword::parse(line);
        EXPECT_TRUE(info);
        if (info)
        {
            EXPECT_EQ(info->id, "ymp2025eb");
            EXPECT_EQ(info->version, "2.7");
            EXPECT_EQ(info->name, "Yamap");
        }
    }
    {
        constexpr std::string_view line {"*[peg2020eb]  	(4.7)  	- ସତ୍‌ ବଚନ୍"};
        const std::optional<sword::info> info = sword::parse(line);
        EXPECT_TRUE(info);
        if (info)
        {
            EXPECT_EQ(info->id, "peg2020eb");
            EXPECT_EQ(info->version, "4.7");
            EXPECT_EQ(info->name, "ସତ୍‌ ବଚନ୍");
        }
    }
    {
        constexpr std::string_view line {"*[pan2017eb]  	(21.37)  	- ਇੰਡਿਅਨ ਰਿਵਾਇਜ਼ਡ ਵਰਜ਼ਨ (IRV) - ਪੰਜਾਬੀ"};
        const std::optional<sword::info> info = sword::parse(line);
        EXPECT_TRUE(info);
        if (info)
        {
            EXPECT_EQ(info->id, "pan2017eb");
            EXPECT_EQ(info->version, "21.37");
            EXPECT_EQ(info->name, "ਇੰਡਿਅਨ ਰਿਵਾਇਜ਼ਡ ਵਰਜ਼ਨ (IRV) - ਪੰਜਾਬੀ");
        }
    }
    {
        constexpr std::string_view line {"*[ABSMaps]  	(1.071229)  	- Maps by American Bible Society (1888)"};
        const std::optional<sword::info> info = sword::parse(line);
        EXPECT_TRUE(info);
        if (info)
        {
            EXPECT_EQ(info->id, "ABSMaps");
            EXPECT_EQ(info->version, "1.071229");
            EXPECT_EQ(info->name, "Maps by American Bible Society (1888)");
        }
    }
}
