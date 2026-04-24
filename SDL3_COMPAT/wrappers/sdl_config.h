#ifndef RA_SDL_CONFIG_H
#define RA_SDL_CONFIG_H

#include <cstdint>


bool RA_ReadConfigString(const char* section_name, const char* value_name, char* buffer, uint32_t* buffer_size);
bool RA_ReadConfigUint32(const char* section_name, const char* value_name, uint32_t* value);
bool RA_DeleteConfigValue(const char* section_name, const char* value_name);


#endif
