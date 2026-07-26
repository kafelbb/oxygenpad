/*
 * oxygenpad
 *
 * Copyright (C) 2026 kafelbb
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <iostream>

int check_for_updates(const char* prog_ver);
int initiate_update();

extern bool there_is_an_update;
extern std::string update_meta_url;
extern std::string new_prog_ver;
extern std::string new_release_url;
extern std::string new_changelog;