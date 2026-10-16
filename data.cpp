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
#include <cstdint>
#include <fstream>
#include "exception.h"
#include <utility>
#include <nlohmann/json.hpp>

#include "utilities.h"
using json = nlohmann::json;

namespace data {

Data::Data()
{
}

Data::~Data()
{
}


static json verse_to_json(const VerseText& vt)
{
    return json {
        { "book",      vt.book },
        { "chapter",   vt.chapter },
        { "verse",     vt.verse },
        { "text",      vt.text },
    };
}


static VerseText verse_from_json(const json& j)
{
    return VerseText {
        .book      = j.at("book").get<short>(),
        .chapter   = j.at("chapter").get<short>(),
        .verse     = j.at("verse").get<short>(),
        .text      = j.at("text").get<std::string>(),
    };
}


void Data::load(const std::optional<std::filesystem::path>& path)
{
    std::ifstream in(get_path(path), std::ios::binary);
    if (not in)
        throw Base("Loading data: Cannot open", get_path(path).string());

    const json j = json::parse(in);   // throws json::parse_error on bad input

    // Parse into locals first: Malformed file leaves the object unchanged.
    const auto id   = j.at("id").get<int>();
    const auto uuid = j.at("uuid").get<std::string>();

    std::vector<VerseText> verses;
    const auto& arr = j.at("data");
    verses.reserve(arr.size());
    for (const auto& item : arr)
        verses.push_back(verse_from_json(item));

    // Commit.
    m_id = id;
    m_uuid = uuid;
    m_data = std::move(verses);
}


void Data::save(std::optional<std::filesystem::path> path) const
{
    json verses = json::array();
    for (const auto& v : m_data)
        verses.push_back(verse_to_json(v));

    const json j {
        { "id",   m_id },
        { "uuid", m_uuid },
        { "data", std::move(verses) },
    };

    // Write to a temporary file first.
    // A failed save never leaves a truncated file at the real path.
    const auto tmp = utilities::tmp_file();
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (not out)
            throw Base("Saving data: Cannot open", tmp.string());
        out << j.dump(2);
        out.flush();
        if (not out)
            throw Base("Saving data: Write failed for", tmp.string());
    }
    std::filesystem::rename(tmp, get_path(std::move(path))); // Replaces existing file.
}


std::filesystem::path Data::get_path(std::optional<std::filesystem::path> path) const
{
    // Path may be passed (for unit tests).
    // If omitted, it uses a standard location.
    if (path)
        return *path;
    return std::filesystem::path(SOURCE_DIR) / "data" / std::string(std::to_string(m_id) + ".json");
}


void Data::add_or_replace(VerseText verse_text)
{
    // Identity of verse text.
    const auto same_verse = [&verse_text](const VerseText& v) noexcept
    {
        return v.book == verse_text.book and v.chapter == verse_text.chapter and v.verse == verse_text.verse;
    };
    if (const auto iter = std::ranges::find_if(m_data, same_verse); iter != m_data.cend())
        *iter = std::move(verse_text);
    else
        m_data.push_back(std::move(verse_text));
}


std::size_t Data::verse_count() const noexcept
{
    return m_data.size();
}


std::size_t Data::verse_count(const unsigned short book) const noexcept
{
    const auto same_book = [&](const VerseText& vt) noexcept
    {
        return vt.book == book;
    };
    return static_cast<std::size_t>(std::ranges::count_if(m_data, same_book));
}


std::size_t Data::verse_count(const unsigned short book, const unsigned short chapter) const noexcept
{
    const auto same_book_chapter = [&](const VerseText& vt) noexcept
    {
        return vt.book == book and vt.chapter == chapter;
    };
    return static_cast<std::size_t>(std::ranges::count_if(m_data, same_book_chapter));
}


std::size_t Data::verse_count(const unsigned short book, const unsigned short chapter, const unsigned short verse) const noexcept
{
    const auto same_book_chapter_verse = [&](const VerseText& vt) noexcept
    {
        return vt.book == book and vt.chapter == chapter and vt.verse == verse;
    };
    return static_cast<std::size_t>(std::ranges::count_if(m_data, same_book_chapter_verse));
}


[[nodiscard]] const VerseText* Data::find(const unsigned short book, const unsigned short chapter, const unsigned short verse) const noexcept
{
    const auto same_verse = [&](const VerseText& vt) noexcept
    {
        return vt.book == book and vt.chapter == chapter and vt.verse == verse;
    };
    const auto iter = std::ranges::find_if(m_data, same_verse);
    return iter != m_data.cend() ? &*iter : nullptr;
}


// This can return a std::string_view:
// In normal use, which is reading, the backing std::vector is not updated / reallocated.
std::string_view Data::get_text(const unsigned short book, const unsigned short chapter,
                                const unsigned short verse) const noexcept
{
    const auto* v = find(book, chapter, verse);
    return v ? std::string_view{v->text} : std::string_view{};
}


[[nodiscard]] std::filesystem::path Data::storage_path() const
{
    return std::filesystem::path(SOURCE_DIR) / "data" / std::string(std::to_string(m_id) + ".json");
}


}
