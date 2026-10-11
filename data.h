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


#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace data {

struct VerseText
{
    unsigned short book{};
    unsigned short chapter{};
    unsigned short verse{};
    std::string text;
    std::string case_fold{};
    constexpr std::strong_ordering operator<=>(const VerseText&) const = default;
};

class Data
{
    unsigned int m_id {};
    std::uint64_t m_timestamp {};
    std::vector<VerseText> m_data{};
public:
    Data();
    ~Data();
    void load(const std::optional<std::filesystem::path>& path);
    void save(std::optional<std::filesystem::path> path) const;
    [[nodiscard]] decltype(m_id) id() const { return m_id; }
    void id (const decltype(m_id) v) { m_id = v; }
    [[nodiscard]] decltype(m_timestamp) timestamp() const { return m_timestamp; }
    void timestamp (const decltype(m_timestamp) v) { m_timestamp = v; }
    void add_or_replace(VerseText);
    [[nodiscard]] std::size_t verse_count() const noexcept;
    [[nodiscard]] std::size_t verse_count(unsigned short book) const noexcept;
    [[nodiscard]] std::size_t verse_count(unsigned short book, unsigned short chapter) const noexcept;
    [[nodiscard]] std::size_t verse_count(unsigned short book, unsigned short chapter, unsigned short verse) const noexcept;
    [[nodiscard]] std::string_view get_text(unsigned short book, unsigned short chapter, unsigned short verse) const noexcept;
    [[nodiscard]] std::string_view get_case_fold(unsigned short book, unsigned short chapter,
                                                 unsigned short verse) const noexcept;
    constexpr std::strong_ordering operator<=>(const Data&) const = default;
private:
    [[nodiscard]] std::filesystem::path get_path(std::optional<std::filesystem::path> path = std::nullopt) const;
    [[nodiscard]] const VerseText* find(unsigned short book, unsigned short chapter, unsigned short verse) const noexcept;
};

}
