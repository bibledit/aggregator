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

#include "books.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include "utilities.h"


namespace books {

// The macro does not throw on an unknown string.
// It silently maps to the first entry in the list, which is why Other comes first.
NLOHMANN_JSON_SERIALIZE_ENUM(BookType, {
    {BookType::Other, "other"},
    {BookType::OldTestament, "ot"},
    {BookType::NewTestament, "nt"},
    {BookType::Apocrypha, "ap"},
    {BookType::FrontBack, "frontback"},
})

// ReSharper disable once CppUseInternalLinkage
struct Book {
    std::string english;
    int order = 0;
    std::string osis;
    std::string usfm;
    BookType type = BookType::Other;
};

// The macro generates two conversion functions, similar to "from_json" and "to_json".
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Book, english, order, osis, usfm, type)

static std::vector<Book> books;

void load()
{
    const std::filesystem::path books_json_path = std::filesystem::path(SOURCE_DIR) / "books" / "books.json";
    const std::string contents = utilities::file_get_contents(books_json_path);
    books = nlohmann::json::parse(contents).get<std::vector<Book>>();
    // Sort the books in canonical order.
    std::ranges::sort(books, [](const Book& a, const Book& b) noexcept { return a.order < b.order; });
}


std::size_t count()
{
    return books.size();
}


std::vector<std::string> get_all_osis()
{
    auto&& view = books | std::views::transform(&Book::osis) | std::views::filter([](const std::string& s) { return not s.empty(); });
    return {view.begin(), view.end()};
}


std::vector<std::string> get_all_osis(const BookType book_type)
{
    auto&& view = books
        | std::views::filter([book_type](const Book& book) { return book.type == book_type; })
        | std::views::transform(&Book::osis)
        | std::views::filter([](const std::string& s) { return not s.empty(); });
    return {view.begin(), view.end()};
}


}
