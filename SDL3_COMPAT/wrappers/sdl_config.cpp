#include "sdl_config.h"
#include "sdl_fs.h"

#include <cstring>
#include <mutex>
#include <string>
#include <unordered_set>

namespace {

std::mutex g_config_mutex;
std::unordered_set<std::string> g_deleted_config_values;

bool config_reports_expansion_installed(const char* mix_name)
{
    return mix_name && WWFS_GetPathInfo(mix_name, nullptr);
}
std::string make_config_value_id(const char* section_name, const char* value_name)
{
    const char* safe_section_name = section_name ? section_name : "";
    const char* safe_value_name = value_name ? value_name : "";
    std::string id(safe_section_name);
    id.push_back('\n');
    id += safe_value_name;
    return id;
}

bool is_deleted_config_value(const char* section_name, const char* value_name)
{
    std::scoped_lock lock(g_config_mutex);
    return g_deleted_config_values.find(make_config_value_id(section_name, value_name)) != g_deleted_config_values.end();
}

bool is_wolapi_config_section(const char* section_name)
{
    return section_name && std::strcmp(section_name, "Software\\Westwood\\WOLAPI") == 0;
}

bool lookup_config_string_value(const char* section_name, const char* value_name, std::string& value)
{
    if (!section_name || !value_name) {
        return false;
    }

    if (is_wolapi_config_section(section_name) && std::strcmp(value_name, "InstallPath") == 0) {
        const char* base_path = SDL_GetBasePath();
        value = base_path ? base_path : "./";
        return true;
    }

    return false;
}

bool lookup_config_uint32_value(const char*, const char* value_name, uint32_t* value)
{
    if (!value_name || !value) {
        return false;
    }

    if (std::strcmp(value_name, "WolapiInstallComplete") == 0 ||
        std::strcmp(value_name, "WOLAPI Find Enabled") == 0 ||
        std::strcmp(value_name, "WOLAPI Page Enabled") == 0 ||
        std::strcmp(value_name, "WOLAPI Lang Filter") == 0 ||
        std::strcmp(value_name, "WOLAPI Show All Games") == 0) {
        *value = 0;
        return true;
    }

    if (std::strcmp(value_name, "CStrikeInstalled") == 0) {
        *value = config_reports_expansion_installed("EXPAND.MIX") ? 1u : 0u;
        return true;
    }

    if (std::strcmp(value_name, "AftermathInstalled") == 0) {
        *value = config_reports_expansion_installed("EXPAND2.MIX") ? 1u : 0u;
        return true;
    }

    return false;
}

} // namespace


bool RA_ReadConfigString(const char* section_name, const char* value_name, char* buffer, uint32_t* buffer_size)
{
    if (!buffer_size || !section_name || !value_name) {
        return false;
    }

    if (is_deleted_config_value(section_name, value_name)) {
        return false;
    }

    std::string value;
    if (!lookup_config_string_value(section_name, value_name, value)) {
        return false;
    }

    const size_t required_size = value.size() + 1;
    if (!buffer || *buffer_size < required_size) {
        *buffer_size = static_cast<uint32_t>(required_size);
        return false;
    }

    std::memcpy(buffer, value.c_str(), required_size);
    *buffer_size = static_cast<uint32_t>(required_size);
    return true;
}

bool RA_ReadConfigUint32(const char* section_name, const char* value_name, uint32_t* value)
{
    if (!section_name || !value_name || !value) {
        return false;
    }

    if (is_deleted_config_value(section_name, value_name)) {
        return false;
    }

    return lookup_config_uint32_value(section_name, value_name, value);
}

bool RA_DeleteConfigValue(const char* section_name, const char* value_name)
{
    if (!section_name || !value_name) {
        return false;
    }

    std::scoped_lock lock(g_config_mutex);
    g_deleted_config_values.insert(make_config_value_id(section_name, value_name));
    return true;
}
