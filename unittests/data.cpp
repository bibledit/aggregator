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


#include "data.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utilities.h"


class db_test : public testing::Test
{
public:
    std::filesystem::path db_path{};
protected:
    db_test() {
        // No interference with production data: Use temporal file.
        db_path = utilities::tmp_file();
        std::error_code ec;
        std::filesystem::remove(db_path, ec);
    }
    ~db_test() override {
        db_path.clear();
    }
};


// Save a Data object to file.
// Load another Data object from that same file.
// Both should be the same.
TEST_F(db_test, save_load)
{
    data::Data live_data;
    constexpr auto id {123};
    live_data.id(id);
    const auto uuid {utilities::get_uuid()};
    live_data.uuid(uuid);
    const data::VerseText vt1 {
        .book = 456,
        .chapter = 567,
        .verse = 678,
        .text = "text1",
    };
    live_data.add_or_replace(vt1);
    const data::VerseText vt2 {
        .book = 4567,
        .chapter = 5678,
        .verse = 6789,
        .text = "text2",
    };
    live_data.add_or_replace(vt2);
    live_data.save(db_path);

    data::Data loaded_data;
    loaded_data.load(db_path);

    EXPECT_EQ(loaded_data, live_data);
    EXPECT_EQ(loaded_data.id(), live_data.id());
    EXPECT_EQ(loaded_data.uuid(), live_data.uuid());
}


TEST_F(db_test, add_or_replace)
{
    data::Data data;
    EXPECT_EQ(data.verse_count(), 0);
    data::VerseText vt {
        .book = 1,
        .chapter = 2,
        .verse = 3,
        .text = "t",
    };
    data.add_or_replace(vt);
    data.add_or_replace(vt);
    EXPECT_EQ(data.verse_count(), 1);
    auto vt2 = vt;
    vt2.book = 2;
    data.add_or_replace(vt2);
    data.add_or_replace(vt2);
    EXPECT_EQ(data.verse_count(), 2);
}


TEST_F(db_test, count)
{
    data::Data data;
    for (short book = 1; book <= 2; ++book)
    {
        for (short chapter = 1; chapter <= 2; ++chapter)
        {
            for (short verse = 1; verse <= 2; ++verse)
            {
                const auto text = [&]
                {
                    return std::to_string(book) + std::to_string(chapter) + std::to_string(verse);
                };
                data::VerseText vt {
                    .book = book,
                    .chapter = chapter,
                    .verse = verse,
                    .text = text(),
                };
                data.add_or_replace(std::move(vt));
            }
        }
    }
    EXPECT_EQ(data.verse_count(), 8);
    EXPECT_EQ(data.verse_count(1), 4);
    EXPECT_EQ(data.verse_count(1, 1), 2);
    EXPECT_EQ(data.verse_count(1, 1, 1), 1);
}


TEST_F(db_test, get)
{
    data::Data data;
    constexpr auto book = 1;
    constexpr auto chapter = 2;
    constexpr auto verse = 3;
    constexpr auto text = "text";

    data::VerseText vt {
        .book = book,
        .chapter = chapter,
        .verse = verse,
        .text = text,
    };
    data.add_or_replace(vt);

    auto vt2 = vt;
    ++vt2.book;
    ++vt2.chapter;
    ++vt2.verse;
    vt2.text.append(text);
    data.add_or_replace(vt2);

    EXPECT_EQ(data.get_text(book, chapter, verse), text);

    EXPECT_EQ(data.get_text(book + 1, chapter + 1, verse + 1), std::string(text).append(text));

    EXPECT_EQ(data.get_text(3, 2, 1), "");
}
