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

//this func was vibe coded actually
void draw_round_rect(sf::RenderTarget& win, int roundness, sf::Vector2i pos, sf::Vector2i size, sf::Color clr1, sf::Color clr2) {
    float r = static_cast<float>(std::min({ static_cast<int>(roundness), size.x / 2, size.y / 2 }));

    float x = static_cast<float>(pos.x);
    float y = static_cast<float>(pos.y);
    float w = static_cast<float>(size.x);
    float h = static_cast<float>(size.y);

    // Если закругления нет — рисуем обычный прямоугольник
    if (r <= 0.f) {
        sf::VertexArray rect(sf::TriangleStrip, 4);
        rect[0] = sf::Vertex(sf::Vector2f(x, y), clr1);
        rect[1] = sf::Vertex(sf::Vector2f(x + w, y), clr1);
        rect[2] = sf::Vertex(sf::Vector2f(x, y + h), clr2);
        rect[3] = sf::Vertex(sf::Vector2f(x + w, y + h), clr2);
        win.draw(rect);
        return;
    }

    // Лямбда для получения цвета градиента в любой точке по высоте
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

    // Создаем одну сплошную полигональную фигуру для всей кнопки
    sf::ConvexShape shape;

    const int points_per_corner = 12; // Качество закругления (всего будет 48 точек)
    shape.setPointCount(points_per_corner * 4);

    const float PI = 3.14159265f;
    int point_index = 0;

    // 1. Верхний правый угол
    sf::Vector2f center_tr(x + w - r, y + r);
    for (int i = 0; i < points_per_corner; ++i) {
        float angle = -PI / 2.f + (PI / 2.f) * i / (points_per_corner - 1);
        shape.setPoint(point_index++, sf::Vector2f(center_tr.x + r * std::cos(angle), center_tr.y + r * std::sin(angle)));
    }

    // 2. Нижний правый угол
    sf::Vector2f center_br(x + w - r, y + h - r);
    for (int i = 0; i < points_per_corner; ++i) {
        float angle = 0.f + (PI / 2.f) * i / (points_per_corner - 1);
        shape.setPoint(point_index++, sf::Vector2f(center_br.x + r * std::cos(angle), center_br.y + r * std::sin(angle)));
    }

    // 3. Нижний левый угол
    sf::Vector2f center_bl(x + r, y + h - r);
    for (int i = 0; i < points_per_corner; ++i) {
        float angle = PI / 2.f + (PI / 2.f) * i / (points_per_corner - 1);
        shape.setPoint(point_index++, sf::Vector2f(center_bl.x + r * std::cos(angle), center_bl.y + r * std::sin(angle)));
    }

    // 4. Верхний левый угол
    sf::Vector2f center_tl(x + r, y + r);
    for (int i = 0; i < points_per_corner; ++i) {
        float angle = PI + (PI / 2.f) * i / (points_per_corner - 1);
        shape.setPoint(point_index++, sf::Vector2f(center_tl.x + r * std::cos(angle), center_tl.y + r * std::sin(angle)));
    }

    // А ТЕПЕРЬ НАКЛАДЫВАЕМ ГРАДИЕНТ НА ВСЮ ФИГУРУ СРАЗУ
    // Для этого мы создаем RenderTexture и красим вершины нашей сплошной фигуры
    sf::VertexArray fill_vertices(sf::TrianglesFan, shape.getPointCount() + 1);

    // Центральная точка для TriangleFan (среднее арифметическое координат)
    sf::Vector2f center(x + w / 2.f, y + h / 2.f);
    fill_vertices[0] = sf::Vertex(center, get_gradient_color(center.y));

    for (size_t i = 0; i < shape.getPointCount(); ++i) {
        sf::Vector2f pt = shape.getPoint(i);
        fill_vertices[i + 1] = sf::Vertex(pt, get_gradient_color(pt.y));
    }

    // Замыкаем веер (последняя вершина дублирует первую точку контура)
    sf::Vector2f first_pt = shape.getPoint(0);
    fill_vertices[shape.getPointCount()] = sf::Vertex(first_pt, get_gradient_color(first_pt.y));

    // Отрисовываем монолитную закругленную фигуру БЕЗ ДЫР
    win.draw(fill_vertices);
}