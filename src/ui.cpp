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

#include <cmath>
#include <algorithm>
#include <iostream>
#include <memory>

#include "ui.h"

#include "SFML/Graphics.hpp"

std::vector<std::unique_ptr<button>> btns;
std::vector<sound_bind> binds;

void button::create(sound_bind& bind) {
    btns.push_back(std::make_unique<button>(bind));
}
button::button(sound_bind& bind)
    : bind(bind)
{
}

sound_bind* check_buttons(std::vector<std::unique_ptr<button>>& btns, int mx, int my, int yoffset) {
    for (auto& b : btns) {
        if (mx >= b->bind.spr.getPosition().x && mx <= b->bind.spr.getPosition().x + b->bind.tex.getSize().x && my >= b->bind.spr.getPosition().y - yoffset && my <= b->bind.spr.getPosition().y - yoffset + b->bind.tex.getSize().y) {
            return &b->bind;
        }
    }
    return nullptr;
}

sf::Image icon;

//sadly but this func was vibe coded
void draw_round_rect(sf::RenderTarget& win, int roundness, sf::Vector2i pos, sf::Vector2i size, sf::Color clr1, sf::Color clr2) {
    float r = static_cast<float>(std::min({ static_cast<int>(roundness), size.x / 2, size.y / 2 }));

    float x = static_cast<float>(pos.x);
    float y = static_cast<float>(pos.y);
    float w = static_cast<float>(size.x);
    float h = static_cast<float>(size.y);

    if (r <= 0.f) {
        sf::VertexArray rect(sf::TriangleStrip, 4);
        rect[0] = sf::Vertex(sf::Vector2f(x, y), clr1);
        rect[1] = sf::Vertex(sf::Vector2f(x + w, y), clr1);
        rect[2] = sf::Vertex(sf::Vector2f(x, y + h), clr2);
        rect[3] = sf::Vertex(sf::Vector2f(x + w, y + h), clr2);
        win.draw(rect);
        return;
    }

    auto get_gradient_color = [&](float current_y) -> sf::Color {
        float t = (current_y - y) / h;
        t = std::clamp(t, 0.f, 1.f);
        return sf::Color(
            static_cast<sf::Uint8>(clr1.r + t * (clr2.r - clr1.r)),
            static_cast<sf::Uint8>(clr1.g + t * (clr2.g - clr1.g)),
            static_cast<sf::Uint8>(clr1.b + t * (clr2.b - clr1.b)),
            static_cast<sf::Uint8>(clr1.a + t * (clr2.a - clr1.a))
        );
        };

    sf::ConvexShape shape;

    const int points_per_corner = 12;
    shape.setPointCount(points_per_corner * 4);

    const float PI = 3.14159265f;
    int point_index = 0;

    sf::Vector2f center_tr(x + w - r, y + r);
    for (int i = 0; i < points_per_corner; ++i) {
        float angle = -PI / 2.f + (PI / 2.f) * i / (points_per_corner - 1);
        shape.setPoint(point_index++, sf::Vector2f(center_tr.x + r * std::cos(angle), center_tr.y + r * std::sin(angle)));
    }

    sf::Vector2f center_br(x + w - r, y + h - r);
    for (int i = 0; i < points_per_corner; ++i) {
        float angle = 0.f + (PI / 2.f) * i / (points_per_corner - 1);
        shape.setPoint(point_index++, sf::Vector2f(center_br.x + r * std::cos(angle), center_br.y + r * std::sin(angle)));
    }

    sf::Vector2f center_bl(x + r, y + h - r);
    for (int i = 0; i < points_per_corner; ++i) {
        float angle = PI / 2.f + (PI / 2.f) * i / (points_per_corner - 1);
        shape.setPoint(point_index++, sf::Vector2f(center_bl.x + r * std::cos(angle), center_bl.y + r * std::sin(angle)));
    }

    sf::Vector2f center_tl(x + r, y + r);
    for (int i = 0; i < points_per_corner; ++i) {
        float angle = PI + (PI / 2.f) * i / (points_per_corner - 1);
        shape.setPoint(point_index++, sf::Vector2f(center_tl.x + r * std::cos(angle), center_tl.y + r * std::sin(angle)));
    }

    sf::VertexArray fill_vertices(sf::TrianglesFan, shape.getPointCount() + 1);

    sf::Vector2f center(x + w / 2.f, y + h / 2.f);
    fill_vertices[0] = sf::Vertex(center, get_gradient_color(center.y));

    for (size_t i = 0; i < shape.getPointCount(); ++i) {
        sf::Vector2f pt = shape.getPoint(i);
        fill_vertices[i + 1] = sf::Vertex(pt, get_gradient_color(pt.y));
    }

    sf::Vector2f first_pt = shape.getPoint(0);
    fill_vertices[shape.getPointCount()] = sf::Vertex(first_pt, get_gradient_color(first_pt.y));
    win.draw(fill_vertices);
}

void draw_list(sf::RenderWindow& win, int offset_y) {
    int index = 0;
    int w = 416;
    int h = 32;
    int padding = 24;

    int initial_offset = (512 - w) / 2;
    int prev_y = initial_offset + offset_y;

    for (auto& b : binds) {
        int x = 256 - w / 2;
        int y = prev_y + (index * (h + padding));
        b.spr.setPosition(x, y);
        win.draw(b.spr);
        index++;
    }
}
void draw_gui(sf::RenderWindow& win, float curr_scroll, sf::Sprite hud_s, sf::Sprite hud2_s, float hud2_y, float dt) {
    win.clear(sf::Color(1, 0, 0));

    draw_list(win, static_cast<int>(curr_scroll));

    win.draw(hud_s);

    hud2_s.setPosition(0, hud2_y);
    win.draw(hud2_s);
    win.display();
}
void init_binds_sprites(sf::Font& font) {
    int index = 0;
    for (auto& b : binds) {
        sf::ContextSettings st;
        st.antialiasingLevel = 8;
        sf::RenderTexture temp_rtex;

        if (!temp_rtex.create(416, 32, st)) continue;
        temp_rtex.clear(sf::Color::Transparent);
        temp_rtex.setSmooth(true);

        button::create(b);

        if (b.path != "add new one") {
            sf::Color clr1;
            sf::Color clr2;
            sf::Color clr3;
            sf::Color clr4;
            sf::Color clr5;
            sf::Color clr6;
            if (b.path != "toggle" && b.path != "vc toggle" && b.path != "stop" && b.path != "add new one" && b.path != "voice chat") {
                clr1 = sf::Color(255, 255, 255);
                clr2 = sf::Color(150, 150, 150);
                clr3 = sf::Color(150, 150, 150);
                clr4 = sf::Color(50, 50, 50);
                clr5 = sf::Color(50, 50, 50);
                clr6 = sf::Color(230, 230, 230);
            }
            else {
                clr3 = sf::Color(255, 255, 255);
                clr4 = sf::Color(150, 150, 150);
                clr1 = sf::Color(150, 150, 150);
                clr2 = sf::Color(50, 50, 50);
                clr6 = sf::Color(50, 50, 50);
                clr5 = sf::Color(230, 230, 230);
            }
            draw_round_rect(temp_rtex, 16, sf::Vector2i(0, 0), sf::Vector2i(416, 32), clr1, clr2);

            sf::Text text;
            text.setFont(font);
            text.setCharacterSize(16);

            sf::FloatRect tx_rect;

            text.setString(b.binds_str);

            tx_rect = text.getGlobalBounds();

            int key_w = tx_rect.width + 16;
            text.setPosition(416 - key_w, 6);
            text.setFillColor(clr6);
            draw_round_rect(temp_rtex, 16, sf::Vector2i(416 - 12 - key_w, 4), sf::Vector2i(key_w + 8, 24), clr3, clr4);
            temp_rtex.draw(text);

            text.setFillColor(clr5);
            std::string title = sf::String::fromUtf8(b.name.begin(), b.name.end());
            text.setString(title);
            tx_rect = text.getGlobalBounds();

            float max_text_width = 416.0f - 8.0f - 20.0f - key_w;

            if (tx_rect.width > max_text_width) {
                while (!title.empty() && text.getGlobalBounds().width > max_text_width) {
                    title.pop_back();
                    text.setString(title + "...");
                }
                title += "...";
            }

            text.setString(title);
            text.setPosition(8, 6);
            temp_rtex.draw(text);
        }
        else {
            draw_round_rect(temp_rtex, 16, sf::Vector2i(0, 0), sf::Vector2i(416, 32), sf::Color(255, 255, 255), sf::Color(150, 150, 150));
            draw_round_rect(temp_rtex, 12, sf::Vector2i(4, 4), sf::Vector2i(408, 24), sf::Color(0, 0, 0), sf::Color(0, 0, 0));

            sf::Text text;
            text.setFont(font);
            text.setString("add a new one");
            text.setCharacterSize(16);
            text.setFillColor(sf::Color::White);

            sf::FloatRect tx_rect;
            tx_rect = text.getGlobalBounds();
            text.setPosition(416 / 2 - tx_rect.width / 2, 6);
            temp_rtex.draw(text);
        }

        temp_rtex.display();

        b.tex = temp_rtex.getTexture();
        b.tex.setSmooth(true);

        b.spr.setTexture(b.tex);
        index += 1;
    }
}

sf::Font mont_sb;