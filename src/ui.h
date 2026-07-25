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
#include <memory>

#include "SFML/Graphics.hpp"

struct sound_bind {
	std::string path;
	std::string name;
	std::vector<sf::Keyboard::Key> binds;
	std::string binds_str;
	bool debounce = false;
	sf::Sprite spr;
	sf::Texture tex;
};
extern std::vector<sound_bind> binds;

void draw_round_rect(sf::RenderTarget& win, int roundness, sf::Vector2i pos, sf::Vector2i size, sf::Color clr1, sf::Color clr2);

class button {
public:
	sound_bind& bind;
	bool released = false;

	button(sound_bind& bind);
	static void create(sound_bind& bind);
	//~button();
};

extern std::vector<std::unique_ptr<button>> btns;
extern sf::Font mont_sb;
extern sf::Image icon;

void draw_list(sf::RenderWindow& win, int offset_y);
void draw_gui(sf::RenderWindow& win, float curr_scroll, sf::Sprite hud_s, sf::Sprite hud2_s, float hud2_y, float dt);
void init_binds_sprites(sf::Font& font);

sound_bind* check_buttons(std::vector<std::unique_ptr<button>>& btns, int mx, int my, int yoffset);