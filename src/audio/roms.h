#ifndef roms_h_
#define roms_h_

constexpr size_t rom_basic_len = 8192;
constexpr size_t rom_kernal_len = 8192;
constexpr size_t rom_characters_len = 4096;

extern const unsigned char rom_basic[8192];
extern const unsigned char rom_kernal[8192];
extern const unsigned char rom_characters[4096];

#endif