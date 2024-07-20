#pragma once

#include <cstdint>
#include <vector>


class Mapper0
{
public:
    Mapper0();
    Mapper0(std::vector<uint8_t>&& prg_rom, std::vector<uint8_t>&& chr_rom);
    Mapper0& operator=(Mapper0&& mapper);

    Mapper0(const Mapper0& mapper) = delete;
    Mapper0& operator=(const Mapper0& mapper) = delete;
    Mapper0(Mapper0&& mapper) = delete;

    uint8_t read(uint16_t address) const;
    void write(uint16_t address, uint8_t data);
private:
    std::vector<uint8_t> prg_rom;
    std::vector<uint8_t> chr_rom;
};
