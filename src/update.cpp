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

#include "update.h"

#include <iostream>
#include <string>
#include "logger.h"
#include <curl/curl.h>

std::string update_meta_url = "https://raw.githubusercontent.com/kafelbb/oxygenpad/refs/heads/testing/updatemeta";
bool there_is_an_update = false;
std::string new_prog_ver;
std::string new_release_url;
std::string new_changelog;

size_t write_callback(void* contents, size_t s, size_t nmemb, std::string* udata) {
	size_t total_s = s * nmemb;
	udata->append(static_cast<char*>(contents), total_s);
	return total_s;
}

int fetch_file(const std::string& url, std::string& out_buffer) {
	CURL* curl = curl_easy_init();
	if (!curl) return false;

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out_buffer);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	//curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	return (res == CURLE_OK);
}

int check_for_updates(const char* prog_ver) {
	CURL* curl;
	CURLcode res;
	std::string buf;

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, update_meta_url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			log_err("core - update", "curl err: " + std::string(curl_easy_strerror(res)));
			return 1;
		}
		else {
			log_info("core - update", "got updatemeta contents");
		}
		curl_easy_cleanup(curl);
	}

	int state = 0;
	for (int i = 0; i < buf.length(); i++) {
		char c = buf[i];
		if (c == '\n' && state != 2) {
			state += 1;
			continue;
		}
		if (state == 0) {
			new_prog_ver += c;
		}
		else if (state == 1) {
			new_release_url += c;
		}
		else if (state == 2) {
			new_changelog += c;
		}
	}
	std::cout << "new ver: " << new_prog_ver << std::endl;
	std::cout << "new url: " << new_release_url << std::endl;
	std::cout << "new changelog: " << new_changelog << std::endl;

	if (std::string(prog_ver) != new_prog_ver) {
		log_info("core - update", "new ver is different, asking for update");
		there_is_an_update = true;
	}

	return 0;
}
int initiate_update() {
	return 0;
}