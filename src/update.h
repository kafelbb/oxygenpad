#pragma once

#include <iostream>

int check_for_updates(const char* prog_ver);
int initiate_update();

extern bool there_is_an_update;
extern std::string update_meta_url;
extern std::string new_prog_ver;
extern std::string new_release_url;
extern std::string new_changelog;