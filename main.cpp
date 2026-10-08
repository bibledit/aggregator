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

#include <cstdlib>
#include <iostream>
#include <ostream>
#include "arguments.h"
#include "logger.h"
#include "sword/sword.h"

int main(int argc, char* argv[])
{
    try
    {
        logger::plain("Aggregator", VERSION);
        const Arguments arguments(argc, argv);

        if (not arguments.sword().empty())
        {
            Sword sword;
            sword.initialize();
            sword.fetch_remote_sources();
            sword.fetch_modules();
        }

        return EXIT_SUCCESS;
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << std::endl;
    }
    return EXIT_FAILURE;
}
