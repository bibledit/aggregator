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
#include <cstddef>
#include <optional>
#include <vector>
#include <string>

namespace books {

enum class BookType { OldTestament, NewTestament, Apocrypha, FrontBack, Other };

void load();
std::size_t count();
std::vector<std::string> get_all_osis();
std::vector<std::string> get_all_osis(BookType);
std::string get_osis(int id);
int get_id_fuzzy(const std::string&);

struct Passage
{
    int book_id{};    // The book ID.
    std::string book_name; //
    int chapter{}; // Chapter number.
    int verse{};   // Verse number.
};
std::optional<Passage> get_passage(std::string_view);



}
