#pragma once

#include <string>

bool httpPatchFirebase(const std::string& url, const std::string& method, const std::string& jsonData);
bool fb_update_time(int time);
