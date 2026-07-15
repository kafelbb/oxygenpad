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

//oxygenpad a001 by kafel
//9.07.2026 | 14.07.2026

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <atomic>

using namespace std::string_literals;


#include <SFML/Window/Keyboard.hpp>
#include <SFML/Graphics.hpp>


#include "logger.h"
#include "ui.h"


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

int sf_key_to_vk(sf::Keyboard::Key key) {
    if (key >= sf::Keyboard::A && key <= sf::Keyboard::Z) return 'A' + (key - sf::Keyboard::A);
    if (key >= sf::Keyboard::Num0 && key <= sf::Keyboard::Num9) return '0' + (key - sf::Keyboard::Num0);
    if (key >= sf::Keyboard::F1 && key <= sf::Keyboard::F12) return VK_F1 + (key - sf::Keyboard::F1);

    switch (key) {
    case sf::Keyboard::LControl:  return VK_LCONTROL;
    case sf::Keyboard::RControl:  return VK_RCONTROL;
    case sf::Keyboard::LShift:    return VK_LSHIFT;
    case sf::Keyboard::RShift:    return VK_RSHIFT;
    case sf::Keyboard::LAlt:      return VK_LMENU;
    case sf::Keyboard::RAlt:      return VK_RMENU;
    case sf::Keyboard::Space:     return VK_SPACE;
    case sf::Keyboard::Enter:     return VK_RETURN;
    case sf::Keyboard::Escape:    return VK_ESCAPE;
    case sf::Keyboard::Delete:    return VK_DELETE;
    default: return 0;
    }
}

void simulate_key_event(sf::Keyboard::Key sf_key, bool is_down) {
    int vk = sf_key_to_vk(sf_key);
    if (vk == 0) return;

    UINT scan_code = MapVirtualKey(vk, MAPVK_VK_TO_VSC);

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = 0;
    input.ki.wScan = static_cast<WORD>(scan_code);

    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    if (!is_down) {
        input.ki.dwFlags |= KEYEVENTF_KEYUP;
    }

    if (vk == VK_RMENU || vk == VK_RCONTROL || vk == VK_LEFT || vk == VK_UP || vk == VK_RIGHT || vk == VK_DOWN) {
        input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }

    SendInput(1, &input, sizeof(INPUT));
}
#else
//to be made (func for mac/linux
void simulate_key_event(sf::Keyboard::Key sf_key, bool is_down) {}
#endif


#define MINIAUDIO_IMPLEMENTATION
#include "libs/miniaudio.h"

#define TRAY_WINAPI 1
#include "libs/tray.h"

constexpr const char* PROG_VER = "a001";

namespace fs = std::filesystem;

bool core_enabled = true;
bool core_vc_enabled = true;
sound_bind* bind_to_edit = nullptr;
sf::Keyboard::Key vc_key = sf::Keyboard::Unknown;

struct snd {
    std::vector<float> audio_data;
    ma_audio_buffer buffer1;
    ma_audio_buffer buffer2;
    ma_device device1;
    ma_device device2;

    ~snd() {
        ma_device_uninit(&device2);
        ma_device_uninit(&device1);
        ma_audio_buffer_uninit(&buffer2);
        ma_audio_buffer_uninit(&buffer1);
    }
};
std::vector<std::shared_ptr<snd>> snds;

ma_audio_buffer g_buffer1;
ma_audio_buffer g_buffer2;

ma_device_id* voicemtr_id;

ma_context g_context;

void callback1(ma_device* d, void* out, const void* input, ma_uint32 frm_count) {
    ma_audio_buffer* buf = (ma_audio_buffer*)d->pUserData;
    ma_uint64 frames_to_read = frm_count;

    ma_audio_buffer_read_pcm_frames(buf, out, frames_to_read, MA_FALSE);

    if (frames_to_read < frm_count) {
        float* pOutputChannels = (float*)out;
        for (ma_uint64 i = frames_to_read * 2; i < frm_count * 2; ++i) {
            pOutputChannels[i] = 0.0f;
        }
    }
    (void)input;
}

void callback2(ma_device* d, void* out, const void* input, ma_uint32 frm_count) {
    ma_audio_buffer* buf = (ma_audio_buffer*)d->pUserData;
    ma_uint64 frames_to_read = frm_count;

    ma_audio_buffer_read_pcm_frames(buf, out, frames_to_read, MA_FALSE);

    if (frames_to_read < frm_count) {
        float* pOutputChannels = (float*)out;
        for (ma_uint64 i = frames_to_read * 2; i < frm_count * 2; ++i) {
            pOutputChannels[i] = 0.0f;
        }
    }
    (void)input;
}

int play_sound_to_both_devices(ma_context* context, const std::string& path) {
    if (voicemtr_id == NULL) {
        log_err("audio", "no voicemeeter device found", true);
        return 1;
    }

    ma_decoder decoder;
    ma_decoder_config dec_cfg = ma_decoder_config_init(ma_format_f32, 2, 0);

    ma_result result;

#ifdef _WIN32
    fs::path wide_path(reinterpret_cast<const char8_t*>(path.c_str()));
    result = ma_decoder_init_file_w(wide_path.wstring().c_str(), &dec_cfg, &decoder);
#else
    result = ma_decoder_init_file(path.c_str(), &dec_cfg, &decoder);
#endif

    if (result != MA_SUCCESS) {
        log_err("audio", "failed to load " + path);
        return 1;
    }

    ma_uint32 fileSampleRate = decoder.outputSampleRate;
    ma_uint64 total_frames = 0;
    ma_decoder_get_length_in_pcm_frames(&decoder, &total_frames);

    auto sound = std::make_shared<snd>();
    sound->audio_data.resize(total_frames * 2);

    ma_decoder_read_pcm_frames(&decoder, sound->audio_data.data(), total_frames, NULL);
    ma_decoder_uninit(&decoder);

    ma_audio_buffer_config buf_cfg = ma_audio_buffer_config_init(ma_format_f32, 2, total_frames, sound->audio_data.data(), NULL);
    ma_audio_buffer_init(&buf_cfg, &sound->buffer1);
    ma_audio_buffer_init(&buf_cfg, &sound->buffer2);

    ma_device_config deviceConfig1 = ma_device_config_init(ma_device_type_playback);
    deviceConfig1.playback.pDeviceID = NULL;
    deviceConfig1.playback.format = ma_format_f32;
    deviceConfig1.playback.channels = 2;

    deviceConfig1.sampleRate = fileSampleRate;
    deviceConfig1.dataCallback = callback1;
    deviceConfig1.pUserData = &sound->buffer1;

    if (ma_device_init(context, &deviceConfig1, &sound->device1) != MA_SUCCESS) {
        log_err("audio", "failed to open primary device for " + path);
        return 1;
    }

    ma_device_config deviceConfig2 = ma_device_config_init(ma_device_type_playback);
    deviceConfig2.playback.pDeviceID = voicemtr_id;
    deviceConfig2.playback.format = ma_format_f32;
    deviceConfig2.playback.channels = 2;
    deviceConfig2.sampleRate = fileSampleRate;
    deviceConfig2.dataCallback = callback2;
    deviceConfig2.pUserData = &sound->buffer2;

    if (ma_device_init(context, &deviceConfig2, &sound->device2) != MA_SUCCESS) {
        log_err("audio", "failed to open vm device for " + path);
        return 1;
    }

    ma_device_start(&sound->device1);
    ma_device_start(&sound->device2);

    snds.push_back(sound);
    log_info("audio", "playing " + path);
    return 0;
}

void stop_last_sound() {
    if (!snds.empty()) {
        snds.pop_back();
    }
}

void tray_stop(struct tray_menu* item) {
    stop_last_sound();
    (void)item;
}
void tray_play(struct tray_menu* item) {
    play_sound_to_both_devices(&g_context, "res/sounds/sound.wav");
    (void)item;
}
void tray_quit(struct tray_menu* item) {
    log_info("core", "exit from tray");
    tray_exit();
    (void)item;
}

sf::Keyboard::Key string_to_sf_key(const std::string& str) {
    std::string upper_str = str;
    for (char& c : upper_str) c = toupper(c);

    if (upper_str.rfind("KEY_", 0) == 0) {
        try {
            int key_code = std::stoi(upper_str.substr(4));
            return static_cast<sf::Keyboard::Key>(key_code);
        }
        catch (...) {
            return sf::Keyboard::Unknown;
        }
    }

    if (upper_str.length() == 1 && upper_str[0] >= 'A' && upper_str[0] <= 'Z') {
        return static_cast<sf::Keyboard::Key>(upper_str[0] - 'A');
    }
    if (upper_str.length() == 1 && upper_str[0] >= '0' && upper_str[0] <= '9') {
        return static_cast<sf::Keyboard::Key>(static_cast<int>(sf::Keyboard::Num0) + (upper_str[0] - '0'));
    }

    if (upper_str == "F1")  return sf::Keyboard::F1;
    if (upper_str == "F2")  return sf::Keyboard::F2;
    if (upper_str == "F3")  return sf::Keyboard::F3;
    if (upper_str == "F4")  return sf::Keyboard::F4;
    if (upper_str == "F5")  return sf::Keyboard::F5;
    if (upper_str == "F6")  return sf::Keyboard::F6;
    if (upper_str == "F7")  return sf::Keyboard::F7;
    if (upper_str == "F8")  return sf::Keyboard::F8;
    if (upper_str == "F9")  return sf::Keyboard::F9;
    if (upper_str == "F10") return sf::Keyboard::F10;
    if (upper_str == "F11") return sf::Keyboard::F11;
    if (upper_str == "F12") return sf::Keyboard::F12;

    if (upper_str == "LEFT")  return sf::Keyboard::Left;
    if (upper_str == "RIGHT") return sf::Keyboard::Right;
    if (upper_str == "UP")    return sf::Keyboard::Up;
    if (upper_str == "DOWN")  return sf::Keyboard::Down;

    if (upper_str == "SPACE")  return sf::Keyboard::Space;
    if (upper_str == "ENTER")  return sf::Keyboard::Enter;
    if (upper_str == "LCTRL")  return sf::Keyboard::LControl;
    if (upper_str == "RCTRL")  return sf::Keyboard::RControl;
    if (upper_str == "LSHIFT") return sf::Keyboard::LShift;
    if (upper_str == "RSHIFT") return sf::Keyboard::RShift;
    if (upper_str == "LALT")   return sf::Keyboard::LAlt;
    if (upper_str == "RALT")   return sf::Keyboard::RAlt;
    if (upper_str == "TAB")    return sf::Keyboard::Tab;
    if (upper_str == "ESCAPE") return sf::Keyboard::Escape;
    if (upper_str == "BACKSPACE") return sf::Keyboard::Backspace;
    if (upper_str == "DELETE") return sf::Keyboard::Delete;

    return sf::Keyboard::Unknown;
}
std::string sf_key_to_string(sf::Keyboard::Key key) {
    if (key >= sf::Keyboard::A && key <= sf::Keyboard::Z) {
        return std::string(1, 'A' + (key - sf::Keyboard::A));
    }
    if (key >= sf::Keyboard::Num0 && key <= sf::Keyboard::Num9) {
        return std::string(1, '0' + (key - sf::Keyboard::Num0));
    }
    if (key >= sf::Keyboard::F1 && key <= sf::Keyboard::F15) {
        return "F" + std::to_string(key - sf::Keyboard::F1 + 1);
    }
    switch (key) {
    case sf::Keyboard::LControl:  return "LCtrl";
    case sf::Keyboard::RControl:  return "RCtrl";
    case sf::Keyboard::LShift:    return "LShift";
    case sf::Keyboard::RShift:    return "RShiftT";
    case sf::Keyboard::LAlt:      return "LAlt";
    case sf::Keyboard::RAlt:      return "RAlt";
    case sf::Keyboard::Space:     return "Space";
    case sf::Keyboard::Enter:     return "Enter";
    case sf::Keyboard::Left:      return "Left";
    case sf::Keyboard::Right:     return "Right";
    case sf::Keyboard::Up:        return "Up";
    case sf::Keyboard::Down:      return "Down";
    case sf::Keyboard::Escape:    return "Escape";
    case sf::Keyboard::Delete:    return "Delete";
    case sf::Keyboard::Tab:       return "Tab";
    case sf::Keyboard::BackSpace: return "Backspace";
    default: return "KEY_" + std::to_string(static_cast<int>(key));
    }
}

std::vector<std::string> split_string(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        token.erase(0, token.find_first_not_of(" \t"));
        token.erase(token.find_last_not_of(" \t") + 1);
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

void save_soundmap() {
    std::ofstream out("res/sounds.map", std::ios::out | std::ios::trunc);

    if (out.is_open()) {
        out << PROG_VER << "\n";

        for (const auto& b : binds) {
            if (b.path != "add new one") {
                out << "path=" << b.path << "\n";
                out << "bind=" << b.binds_str << "\n";
                out << ";\n";
            }
        }

        out.close();
        log_info("core", "saved new sounds.map");
    }
    else {
        log_err("core", "can't for some reason save new sounds.map", true);
    }
}

void check_for_soundmap() {
    fs::create_directories("res/sounds");
    if (!fs::exists("res/sounds.map")) {
        log_info("startup", "can't find sounds.map creating a new one");
        std::ofstream out("res/sounds.map");
        if (out.is_open()) {
            out << PROG_VER << "\n";
            out << "path=toggle\n";
            out << "bind=LCtrl+LAlt+Backspace\n";
            out << ";\n";
            out << "path=vc toggle\n";
            out << "bind=RCtrl+LAlt+Backspace\n";
            out << ";\n";
            out << "path=stop\n";
            out << "bind=Delete\n";
            out << ";\n";
            out << "path=voice chat\n";
            out << "bind=V\n";
            out << ";\n";
            out.close();
        }
        else {
            log_err("startup", "for some reason can't create sounds.map", true);
            std::exit(1);
        }
    }
    else {
        log_info("startup", "found sounds.map");
    }

    std::ifstream f("res/sounds.map");
    std::string line;
    std::string npath;
    std::string nbindss;
    std::vector<sf::Keyboard::Key> nbinds;

    std::getline(f, line);

    while (std::getline(f, line)) {
        if (line.empty()) continue;
        if (line[0] == ';') {
            sound_bind n;
            n.binds = nbinds;
            n.path = npath;
            n.binds_str = nbindss;
            binds.push_back(n);

            log_info("startup - bindmap parser", "got " + std::to_string(nbinds.size()) + " keybinds for " + npath);

            npath = "";
            nbinds.clear();

            if (n.path == "voice chat") {
                vc_key = string_to_sf_key(nbindss);
            }
            continue;
        }
        if (line.find("path=") == 0) {
            npath = line.substr(5);
            continue;
        }
        if (line.find("bind=") == 0) {
            std::string bind_val = line.substr(5);
            nbindss = bind_val;
            std::vector<std::string> key_names = split_string(bind_val, '+');

            bool valid_combo = true;
            for (const auto& name : key_names) {
                sf::Keyboard::Key key = string_to_sf_key(name);
                if (key != sf::Keyboard::Unknown) {
                    nbinds.push_back(key);
                }
                else {
                    log_err("startup - bindmap parser", "unknown key name in combination: " + name);
                    valid_combo = false;
                }
            }

            if (!valid_combo) {
                nbinds.clear();
            }
            continue;
        }
    }
    f.close();
    sound_bind ass;
    ass.path = "add new one";
    binds.push_back(ass);
}


std::atomic<bool> g_win_requested{ false };
std::atomic<bool> g_gui_thread_running{ true };


void add_new_sound() {
    bind_to_edit = nullptr;
    log_info("core", "adding new file");

    auto selected = pfd::open_file(
        "select audio file",
        "",
        { "audio files (.wav, .mp3, .flac)", "*.wav *.mp3 *.flac", "any file", "*" }
    ).result();

    if (selected.empty()) {
        log_info("core", "brother cancelled");
        return;
    }

#ifdef _WIN32
    fs::path src_path(reinterpret_cast<const char8_t*>(selected[0].c_str()));
#else
    fs::path src_path(selection[0]);
#endif

    std::u8string u8_filename = src_path.filename().u8string();
    std::string filename(u8_filename.begin(), u8_filename.end());

    fs::path dest_dir("res/sounds");
    fs::path dest_path = dest_dir / filename;

    try {
        if (!fs::exists(dest_dir)) {
            fs::create_directories(dest_dir);
        }

        fs::copy_file(src_path, dest_path, fs::copy_options::overwrite_existing);
        log_info("core", "copied " + dest_path.string());

        sound_bind n;
        n.path = filename;
        n.binds_str = "None";
        n.debounce = false;

        auto insert_pos = binds.end();

        if (!binds.empty()) {
            insert_pos = binds.end() - 1;
        }

        auto new_elem_it = binds.insert(insert_pos, std::move(n));
        btns.clear();
        for (auto& b : binds) {
            button::create(b);
        }

        save_soundmap();

        log_info("core", "added " + filename + " to binds");
    }
    catch (const fs::filesystem_error& e) {
        log_err("core", "error: " + std::string(e.what()), true);
    }
}


void draw_list(sf::RenderWindow& win, int offset_y) {
    int index = 0;
    int w = 416;
    int h = 32;
    int padding = 24;

    int initial_offset = (512-w)/2;
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

        if (!temp_rtex.create(416, 32,st)) continue;
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
            std::string title = sf::String::fromUtf8(b.path.begin(), b.path.end());
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
            text.setPosition(416/2 - tx_rect.width/2, 6);
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

void gui_thread_worker() {
    float scroll_target = 0;
    float curr_scroll = 300;
    int scroll_step = 56;
    float scroll_speed = 15.f;

    sf::Clock clock;
    float dt = clock.restart().asSeconds();

    float hud2_y = -118.f;

    sf::Texture hud; hud.loadFromFile("res/hud.png");
    sf::Sprite hud_s(hud);

    sf::Texture hud2; hud2.loadFromFile("res/hud2.png");
    sf::Sprite hud2_s(hud2);
    hud2_s.setPosition(0, hud2_y);

    mont_sb.loadFromFile("res/fonts/mont_sb.ttf");

    bool win_debounce = false;

    sf::ContextSettings st;
    st.antialiasingLevel = 8;
    sf::RenderWindow win;

    init_binds_sprites(mont_sb);

    while (g_gui_thread_running) {
        if (!g_win_requested) {
            std::this_thread::sleep_for(std::chrono::milliseconds(350));
            continue;
        }
        else if (!win_debounce) {
            win.create(sf::VideoMode(512, 512), "oxygenpad", sf::Style::Close, st);

            //win.setFramerateLimit(100);
            win.setVerticalSyncEnabled(true);
            win_debounce = true;

            win_debounce = true;
            scroll_target = 0;
            curr_scroll = 300;

            win.requestFocus();

            draw_gui(win, curr_scroll, hud_s, hud2_s, hud2_y, dt);
        }

        sf::Image icon;
        if (icon.loadFromFile("res/icon.png")) {
            win.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
        }

        while (win.isOpen() && g_gui_thread_running) {
            sf::Event ev;
            while (win.pollEvent(ev)) {
                if (ev.type == sf::Event::Closed) {
                    win.close();
                }

                if (ev.type == sf::Event::MouseWheelScrolled) {
                    if (ev.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                        if (ev.mouseWheelScroll.delta > 0) {
                            scroll_target += scroll_step * 1.5f;
                        }
                        else {
                            scroll_target -= scroll_step * 1.5f;
                        }
                        if (scroll_target > 0.f) scroll_target = 0.f;

                        int window_height = 512;
                        int initial_offset = 48;
                        int total_content_height = (binds.size() + 5) * scroll_step;
                        int max_scroll = -(total_content_height - (window_height - initial_offset));

                        if (max_scroll > 0) max_scroll = 0;
                        if (scroll_target < max_scroll) scroll_target = static_cast<float>(max_scroll);
                    }
                }

                if (ev.type == sf::Event::MouseButtonPressed) {
                    int mx = ev.mouseButton.x;
                    int my = ev.mouseButton.y;

                    sound_bind* temp = check_buttons(btns, mx, my - curr_scroll, static_cast<int>(curr_scroll));

                    if (temp) {
                        if (ev.mouseButton.button == sf::Mouse::Left) {
                            if (temp->path != "add new one") {
                                if (bind_to_edit != temp) {
                                    bind_to_edit = temp;
                                    temp->binds_str = "...";
                                    bind_to_edit->binds.clear();
                                    core_enabled = false;
                                }
                            }
                            else {
                                add_new_sound();
                                init_binds_sprites(mont_sb);
                            }
                        }
                        if (temp->path != "voice chat" && temp->path != "vc toggle" && temp->path != "toggle" && temp->path != "stop" && temp->path != "add new one") {
                            if (ev.mouseButton.button == sf::Mouse::Right) {
                                bind_to_edit = nullptr;
                                btns.erase(
                                    std::remove_if(btns.begin(), btns.end(), [temp](const std::unique_ptr<button>& b) {
                                        return &(b->bind) == temp;
                                        }),
                                    btns.end()
                                );
                                binds.erase(
                                    std::remove_if(binds.begin(), binds.end(), [temp](const sound_bind& b) {
                                        return &b == temp;
                                        }),
                                    binds.end()
                                );
                                save_soundmap();
                            }
                        }
                        init_binds_sprites(mont_sb);
                        draw_gui(win, curr_scroll, hud_s, hud2_s, hud2_y, dt);
                    }
                }

                if (bind_to_edit != nullptr && ev.type == sf::Event::KeyPressed) {
                    if (bind_to_edit->binds_str == "...") {
                        bind_to_edit->binds_str = "";
                    }
                    auto& current_keys = bind_to_edit->binds;
                    if (std::find(current_keys.begin(), current_keys.end(), ev.key.code) == current_keys.end()) {
                        if (!bind_to_edit->binds_str.empty()) {
                            bind_to_edit->binds_str += "+";
                        }

                        current_keys.push_back(ev.key.code);

                        bind_to_edit->binds_str += sf_key_to_string(ev.key.code);
                    }

                    init_binds_sprites(mont_sb);
                    draw_gui(win, curr_scroll, hud_s, hud2_s, hud2_y, dt);
                }
                if (bind_to_edit != nullptr && ev.type == sf::Event::KeyReleased) {
                    if (bind_to_edit->path == "voice chat") {
                        vc_key = string_to_sf_key(bind_to_edit->binds_str);
                    }
                    bind_to_edit = nullptr;
                    core_enabled = true;
                    save_soundmap();
                }
            }

            dt = clock.restart().asSeconds();
            if (dt > 0.05f) dt = 0.016f;

            curr_scroll += (scroll_target - curr_scroll) * scroll_speed * dt;
            if (std::abs(scroll_target - curr_scroll) < 0.1f) {
                curr_scroll = scroll_target;
            }
            float hud2_target = (-curr_scroll > 30.f) ? 0.f : -118.f;
            hud2_y += (hud2_target - hud2_y) * 10.f * dt;
            if (std::abs(hud2_target - hud2_y) < 0.1f) {
                hud2_y = hud2_target;
            }
            
            if (curr_scroll != scroll_target) {
                draw_gui(win, curr_scroll, hud_s, hud2_s, hud2_y, dt);
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(80));
            }
        }
        g_win_requested = false;
        win_debounce = false;
    }
}
void tray_gui(struct tray_menu* item) {
    log_info("core", "gui open request from tray");
    g_win_requested = true;
    (void)item;
}

int main(int argc, char* argv[]) {
    std::setlocale(LC_ALL, ".UTF-8");

#ifdef _WIN32 //this one was also vibecoded
#ifdef NDEBUG
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$", "r", stdin);
        std::ios::sync_with_stdio(true);
        log_info("startup - init", "attached to cmd");
    }
#else
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        AllocConsole();
    }
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);
    std::ios::sync_with_stdio(true);
    log_info("startup - init", "attached to vs debug thing");
#endif
#endif


    log_info("startup", "starting oxygenpad "s + PROG_VER);

    check_for_soundmap();

    struct tray_menu menu[] = {
        { (char*)"gui", 0, 0, tray_gui, NULL },
        { (char*)"-", 0, 0, NULL, NULL },
        { (char*)"stop", 0, 0, tray_stop, NULL },
        { (char*)"-", 0, 0, NULL, NULL },
        { (char*)"quit", 0, 0, tray_quit, NULL },
        { NULL, 0, 0, NULL, NULL }
    };

    struct tray tray_app = {
        (char*)"res/icon.ico",
        menu
    };

    if (tray_init(&tray_app) < 0) {
        log_err("startup", "failed to init tray thing", true);
        ma_context_uninit(&g_context);
        return -1;
    }

    ma_result result;

    result = ma_context_init(NULL, 0, NULL, &g_context);
    if (result != MA_SUCCESS) {
        log_err("startup", "miniaudio context initialization fail", true);
        return -1;
    }

    ma_device_info* device_infos;
    ma_uint32 device_count;
    ma_context_get_devices(&g_context, &device_infos, &device_count, NULL, NULL);

    voicemtr_id = NULL;
    std::string target_name = "Voicemeeter Input (VB-Audio Voicemeeter VAIO)";

    log_info("startup", "detected devices:");
    for (ma_uint32 i = 0; i < device_count; ++i) {
        log_info("startup - detected devices", "(" + std::to_string(i) + ") " + device_infos[i].name);
        if (device_infos[i].name == target_name) {
            voicemtr_id = &device_infos[i].id;
            log_info("startup", "found Voicemeeter Input thing");
        }
    }

    if (voicemtr_id == NULL) {
        log_err("startup", "no " + target_name + " thing found. is VB-AUDIO Voicemeeter installed?", true);
        ma_context_uninit(&g_context);
        return -1;
    }

    bool hotkey_debounce = false;

    std::thread gui_thread(gui_thread_worker);

    while (tray_loop(0) == 0) {
        for (auto& k : binds) {
            if (k.binds.empty()) continue;
            bool all_keys_pressed = std::all_of(k.binds.begin(), k.binds.end(), [](sf::Keyboard::Key key) {
                return sf::Keyboard::isKeyPressed(key);
                });
            if (k.path != "toggle" && k.path != "voice chat" && k.path != "stop" && k.path != "vc toggle") {
                if (all_keys_pressed) {
                    if (!k.debounce) {
                        if (core_enabled) {
                            play_sound_to_both_devices(&g_context, "res/sounds/" + k.path);
                            k.debounce = true;
                            if (core_vc_enabled) {
                                simulate_key_event(vc_key, true);
                            }
                        }
                    }
                }
                else {
                    k.debounce = false;
                }
            }
            else {
                if (all_keys_pressed) {
                    if (!k.debounce) {
                        if (k.path == "stop" && core_enabled) {
                            stop_last_sound();
                            if (snds.size() > 0){
                                simulate_key_event(vc_key, false);
                            }
                            log_info("core", "stop from keyboard");
                        }
                        if (k.path == "toggle") {
                            core_enabled = !core_enabled;
                            log_info("core", "set core_enabled var to " + std::to_string(core_enabled));
                        }
                        if (k.path == "vc toggle") {
                            core_vc_enabled = !core_vc_enabled;
                            log_info("core", "set core_vc_enabled var to " + std::to_string(core_vc_enabled));
                            if (core_vc_enabled) {
                                if (snds.size() > 0) {
                                    simulate_key_event(vc_key, true);
                                }
                            }
                            else {
                                simulate_key_event(vc_key, false);
                            }
                        }
                        k.debounce = true;
                    }
                }
                else {
                    k.debounce = false;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    g_gui_thread_running = false;
    if (gui_thread.joinable()) {
        gui_thread.join();
    }

    snds.clear();
    ma_context_uninit(&g_context);

    return 0;
}
