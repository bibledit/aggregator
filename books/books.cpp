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
#include <charconv>
#include <fstream>
#include <iostream>
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
    int id = 0;
    std::string english;
    int order = 0;
    std::string osis;
    std::string usfm;
    BookType type = BookType::Other;
};

// The macro generates two conversion functions, similar to "from_json" and "to_json".
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Book, id, english, order, osis, usfm, type)

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


// Get the OSIS book name from the book ID.
std::string get_osis(const int id)
{
    const auto iter = std::ranges::find(books, id, &Book::id);
    return iter == books.cend() ? std::string{} : iter->osis;
}


// Tries to interpret the passed text as the name of a Bible book.
// Returns the book's identifier if it succeeds.
// If it fails, it returns 0.
int get_id_fuzzy(const std::string& text) // Todo add cache.
{
    struct candidate
    {
        int id;
        int similarity;
    };
    std::vector<candidate> candidates;
    candidates.reserve(books.size() * 5);

    const std::string case_folded_text {utilities::unicode_case_fold(text)};

    // Go through all known book names and abbreviations.
    // Store how much the passed case folded text differs from the known values.
    for (const auto & record : books)
    {
        candidates.emplace_back(record.id, utilities::similarity_percentage(case_folded_text, utilities::unicode_case_fold(record.english)));
        candidates.emplace_back(record.id, utilities::similarity_percentage(case_folded_text, utilities::unicode_case_fold(std::string{record.osis})));
        candidates.emplace_back(record.id, utilities::similarity_percentage(case_folded_text, utilities::unicode_case_fold(std::string{record.usfm})));
    }

    // Don't sort the entire vector, just take the maximum element.
    const auto best = std::ranges::max_element(candidates, {}, &candidate::similarity);
    return best->id;
}


std::optional<Passage> get_passage(std::string_view text)
{
    // Get the passage from texts like this:
    // Psalms 76:7: You are awesome! Yes, you!
    // John 1:1: In the beginning was the Word, and the Word was with God ...
    // 1 Samuel 2:3: Verse text...

    // Caching mechanism to speed discovery up a huge lot.
    // Discovering 1000 identical passages without the cache took 889327µs on a given machine.
    // Discovering the same passages with the cache took 79µs.
    // Speedup is more than 11'000 times.
    static Passage cached_passage{};
    static std::string cached_fragment{};
    if (not cached_fragment.empty())
        if (text.size() > cached_fragment.size())
            if (text.find(cached_fragment) == 0)
                return cached_passage;

    Passage passage{};

    size_t start = 0;
    if (not text.empty() and std::isdigit(static_cast<unsigned char>(text[0])))
        ++start; // Skip "1" in "1 Samuel" and the like.
    size_t parse_pos = (start == std::string_view::npos)
                           ? std::string_view::npos
                           : text.find_first_of("0123456789", start);
    passage.book_name = std::string{text.substr(0, parse_pos)};
    utilities::trim(passage.book_name); // "1 Samuel"
    passage.book_id = get_id_fuzzy(passage.book_name);

    // Parses an unsigned integer at s[pos...]; on success advances pos past it.
    const auto parse_int = [](const std::string_view s, size_t& pos, int& out) -> bool
    {
        const char* first = s.data() + pos;
        const char* last  = s.data() + s.size();
        const auto [ptr, ec] = std::from_chars(first, last, out);
        if (ec != std::errc{}) return false;
        pos += static_cast<size_t>(ptr - first);
        return true;
    };

    // Chapter, then verse.
    if (not parse_int(text, parse_pos, passage.chapter))
        return std::nullopt;
    if (parse_pos < text.size() and text[parse_pos] == ':') {
        ++parse_pos;
        parse_int(text, parse_pos, passage.verse); // Leaves verse at 0 if it fails.
    }

    if (passage.verse)
    {
        cached_passage = passage;
        cached_fragment = text.substr(0, parse_pos);
    }

    return passage;
}


}
