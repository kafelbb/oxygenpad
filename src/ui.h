#pragma once

#include <iostream>
#include <memory>

#include "SFML/Graphics.hpp"

struct sound_bind {
	std::string path;
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

sound_bind* check_buttons(std::vector<std::unique_ptr<button>>& btns, int mx, int my, int yoffset);