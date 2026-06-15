#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "mayones/core/ppu_bus.hpp"

namespace mayones::core {

inline constexpr std::size_t FRAME_WIDTH{ 256 };
inline constexpr std::size_t FRAME_HEIGHT{ 240 };
inline constexpr std::size_t FRAME_BUFFER_SIZE{ FRAME_WIDTH * FRAME_HEIGHT };

struct PixelColor {
    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};
    std::uint8_t a{ 0xFF };
};

class Ppu {
public:
    explicit Ppu(PpuBus& bus, std::span<PixelColor, FRAME_BUFFER_SIZE> frame_buffer);

    void reset() noexcept;

    std::uint8_t read(std::uint16_t address);
    void write(std::uint16_t address, std::uint8_t data);

    void tick();
    bool is_nmi_pending();

private:
    static constexpr std::size_t OAM_SIZE{ 256 };
    static constexpr std::size_t PALETTE_RAM_SIZE{ 32 };

    static constexpr std::uint8_t REGISTER_ADDRESS_MASK{ 0x0007 };
    static constexpr std::uint8_t PALETTE_RAM_ADDRESS_MASK{ PALETTE_RAM_SIZE - 1 };
    static constexpr std::uint16_t VRAM_ADDRESS_MASK{ 0x3FFF };

    static constexpr std::uint16_t NAMETABLE_BASE_ADDRESS{ 0x2000 };
    static constexpr std::uint16_t PALETTE_BASE_ADDRESS{ 0x3F00 };
    static constexpr std::uint16_t ATTRIBUTE_BASE_ADDRESS{ 0x23C0 };

    static constexpr std::uint16_t FIRST_PATTERN_TABLE{ 0x0000 };
    static constexpr std::uint16_t SECOND_PATTERN_TABLE{ 0x1000 };

    enum RegisterAddress : std::uint8_t {
        CONTROL = 0x00,
        MASK = 0x01,
        STATUS = 0x02,
        OAM_ADDRESS = 0x03,
        OAM_DATA = 0x04,
        SCROLL = 0x05,
        VRAM_ADDRESS = 0x06,
        VRAM_DATA = 0x07
    };

    enum ControlFlag : std::uint8_t {
        NAMETABLE_INDEX = 1 << 1 | 1 << 0,
        VRAM_INCREMENT = 1 << 2,
        SPRITE_PATTERN_TABLE = 1 << 3,
        BACKGROUND_PATTERN_TABLE = 1 << 4,
        SPRITE_SIZE = 1 << 5,
        MASTER_SLAVE = 1 << 6,
        ENABLE_NMI = 1 << 7
    };

    enum MaskFlag : std::uint8_t {
        GREYSCALE = 1 << 0,
        SHOW_BG_LEFTMOST = 1 << 1,
        SHOW_FG_LEFTMOST = 1 << 2,
        ENABLE_BG_RENDERING = 1 << 3,
        ENABLE_FG_RENDERING = 1 << 4,
        EMPHASIZE_RED = 1 << 5,
        EMPHASIZE_GREEN = 1 << 6,
        EMPHASIZE_BLUE = 1 << 7
    };

    enum StatusFlag : std::uint8_t {
        SPRITE_OVERFLOW = 1 << 5,
        SPRITE0_HIT = 1 << 6,
        VBLANK = 1 << 7
    };

    struct Registers {
        std::uint8_t control{};
        std::uint8_t mask{};
        std::uint8_t status{};
        std::uint8_t oam_address{};
    };

    static constexpr std::array<PixelColor, 64> PALETTE{
        { { .r = 85, .g = 85, .b = 85 },    { .r = 0, .g = 23, .b = 115 },
          { .r = 0, .g = 7, .b = 134 },     { .r = 46, .g = 5, .b = 120 },
          { .r = 89, .g = 2, .b = 77 },     { .r = 114, .g = 0, .b = 17 },
          { .r = 110, .g = 0, .b = 0 },     { .r = 76, .g = 8, .b = 0 },
          { .r = 23, .g = 27, .b = 0 },     { .r = 0, .g = 42, .b = 0 },
          { .r = 0, .g = 49, .b = 0 },      { .r = 0, .g = 46, .b = 8 },
          { .r = 0, .g = 38, .b = 69 },     { .r = 0, .g = 0, .b = 0 },
          { .r = 0, .g = 0, .b = 0 },       { .r = 0, .g = 0, .b = 0 },
          { .r = 165, .g = 165, .b = 165 }, { .r = 0, .g = 87, .b = 198 },
          { .r = 34, .g = 63, .b = 229 },   { .r = 110, .g = 40, .b = 217 },
          { .r = 174, .g = 26, .b = 166 },  { .r = 210, .g = 23, .b = 89 },
          { .r = 209, .g = 33, .b = 7 },    { .r = 167, .g = 55, .b = 0 },
          { .r = 99, .g = 81, .b = 0 },     { .r = 24, .g = 103, .b = 0 },
          { .r = 0, .g = 114, .b = 0 },     { .r = 0, .g = 115, .b = 49 },
          { .r = 0, .g = 106, .b = 132 },   { .r = 0, .g = 0, .b = 0 },
          { .r = 0, .g = 0, .b = 0 },       { .r = 0, .g = 0, .b = 0 },
          { .r = 254, .g = 255, .b = 255 }, { .r = 47, .g = 168, .b = 255 },
          { .r = 93, .g = 129, .b = 255 },  { .r = 156, .g = 112, .b = 255 },
          { .r = 247, .g = 114, .b = 255 }, { .r = 255, .g = 119, .b = 189 },
          { .r = 255, .g = 126, .b = 117 }, { .r = 255, .g = 138, .b = 43 },
          { .r = 205, .g = 160, .b = 0 },   { .r = 129, .g = 184, .b = 2 },
          { .r = 61, .g = 200, .b = 48 },   { .r = 18, .g = 205, .b = 123 },
          { .r = 13, .g = 197, .b = 208 },  { .r = 60, .g = 60, .b = 60 },
          { .r = 0, .g = 0, .b = 0 },       { .r = 0, .g = 0, .b = 0 },
          { .r = 254, .g = 255, .b = 255 }, { .r = 164, .g = 222, .b = 255 },
          { .r = 177, .g = 200, .b = 255 }, { .r = 204, .g = 190, .b = 255 },
          { .r = 244, .g = 194, .b = 255 }, { .r = 255, .g = 197, .b = 234 },
          { .r = 255, .g = 199, .b = 201 }, { .r = 255, .g = 205, .b = 170 },
          { .r = 239, .g = 214, .b = 150 }, { .r = 208, .g = 224, .b = 149 },
          { .r = 179, .g = 231, .b = 165 }, { .r = 159, .g = 234, .b = 195 },
          { .r = 154, .g = 232, .b = 230 }, { .r = 175, .g = 175, .b = 175 },
          { .r = 0, .g = 0, .b = 0 },       { .r = 0, .g = 0, .b = 0 } }
    };

    PpuBus& bus_;

    std::span<PixelColor, FRAME_BUFFER_SIZE> frame_buffer_;
    std::array<std::uint8_t, OAM_SIZE> oam_{};
    std::array<std::uint8_t, PALETTE_RAM_SIZE> palette_ram_{};

    Registers registers_{};
    std::uint16_t vram_address_{};
    std::uint16_t tmp_vram_address_{};

    std::uint16_t cycle_{};
    std::uint16_t scanline_{};
    std::uint16_t bg_pattern_address_{};
    std::uint16_t fg_pattern_address_{};

    std::uint16_t lsb_pattern_shift_{};
    std::uint16_t msb_pattern_shift_{};
    std::uint16_t lsb_attribute_shift_{};
    std::uint16_t msb_attribute_shift_{};

    std::uint8_t tile_id_latch_{};
    std::uint8_t lsb_pattern_latch_{};
    std::uint8_t msb_pattern_latch_{};
    std::uint8_t lsb_attribute_latch_{};
    std::uint8_t msb_attribute_latch_{};

    std::uint8_t x_scroll_{};
    std::uint8_t vram_read_buffer_{};
    std::uint8_t vram_address_increment_{};

    bool write_latch_{};
    bool is_nmi_pending_{};
    bool is_rendering_enabled_{};

    PixelColor render_background();
    void increment_x();
    void increment_y();
    void fetch_next_tile();
};

} // namespace mayones::core
