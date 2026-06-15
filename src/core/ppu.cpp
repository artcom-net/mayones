#include <cstdint>
#include <utility>

#include "mayones/core/ppu.hpp"
#include "mayones/core/ppu_bus.hpp"

namespace mayones::core {

Ppu::Ppu(PpuBus& bus, std::span<PixelColor, FRAME_BUFFER_SIZE> frame_buffer) :
    bus_{ bus },
    frame_buffer_{ frame_buffer }
{
}

void Ppu::reset() noexcept
{
    registers_ = Registers{};
    write_latch_ = false;
    cycle_ = 0;
    scanline_ = 0;
}

std::uint8_t Ppu::read(std::uint16_t address)
{
    switch (address & REGISTER_ADDRESS_MASK)
    {
        case RegisterAddress::STATUS: {
            std::uint8_t current_status{ registers_.status };
            registers_.status &= ~StatusFlag::VBLANK;
            write_latch_ = false;
            return current_status;
        }
        case RegisterAddress::OAM_DATA:
            return oam_[registers_.oam_address];
        case RegisterAddress::VRAM_DATA: {
            std::uint8_t value{};
            std::uint8_t current_vram_buffer = vram_read_buffer_;
            vram_read_buffer_ = bus_.read(vram_address_ & VRAM_ADDRESS_MASK);

            if ((vram_address_ & VRAM_ADDRESS_MASK) < PALETTE_BASE_ADDRESS)
            {
                value = current_vram_buffer;
            }
            else
            {
                value = palette_ram_[vram_address_ & PALETTE_RAM_ADDRESS_MASK];
            }

            vram_address_ += vram_address_increment_;
            return value;
        }
        default:
            std::unreachable();
    }
}

void Ppu::write(std::uint16_t address, std::uint8_t data)
{
    switch (address & REGISTER_ADDRESS_MASK)
    {
        case RegisterAddress::CONTROL:
            registers_.control = data;
            // t: ...GH.. ........ <- d: ......GH
            tmp_vram_address_ = (tmp_vram_address_ & 0x73FF) |
                                ((registers_.control & ControlFlag::NAMETABLE_INDEX) << 10);
            vram_address_increment_ =
              (registers_.control & ControlFlag::VRAM_INCREMENT) > 0 ? 32 : 1;
            fg_pattern_address_ = (registers_.control & ControlFlag::SPRITE_PATTERN_TABLE) > 0
                                    ? SECOND_PATTERN_TABLE
                                    : FIRST_PATTERN_TABLE;
            bg_pattern_address_ = (registers_.control & ControlFlag::BACKGROUND_PATTERN_TABLE) > 0
                                    ? SECOND_PATTERN_TABLE
                                    : FIRST_PATTERN_TABLE;
            // TODO: Implement handle sprite size flag.
            break;
        case RegisterAddress::MASK:
            registers_.mask = data;
            is_rendering_enabled_ = (registers_.mask & MaskFlag::ENABLE_BG_RENDERING) != 0 ||
                                    (registers_.mask & MaskFlag::ENABLE_FG_RENDERING) != 0;
            break;
        case RegisterAddress::OAM_ADDRESS:
            registers_.oam_address = data;
            break;
        case RegisterAddress::OAM_DATA:
            oam_[registers_.oam_address++] = data;
            break;
        case RegisterAddress::SCROLL:
            if (write_latch_)
            {
                // t: FGH..AB CDE..... <- d: ABCDEFGH
                // w:                  <- 0
                tmp_vram_address_ =
                  (tmp_vram_address_ & 0x0C1F) | (data & 0xF8) << 2 | (data & 0x07) << 12;
                write_latch_ = !write_latch_;
            }
            else
            {
                // t: ....... ...ABCDE <- d: ABCDE...
                // x:              FGH <- d: .....FGH
                // w:                  <- 1
                tmp_vram_address_ = (tmp_vram_address_ & 0x7FE0) | (data & 0xF8) >> 3;
                x_scroll_ = data & 0x07;
                write_latch_ = !write_latch_;
            }
            break;
        case RegisterAddress::VRAM_ADDRESS:
            if (write_latch_)
            {
                // t: ....... ABCDEFGH <- d: ABCDEFGH
                // w:                  <- 0
                //    (wait 1 to 1.5 dots after the write completes)
                // v: <...all bits...> <- t: <...all bits...>
                tmp_vram_address_ = (tmp_vram_address_ & 0x7F00) | data;
                vram_address_ = tmp_vram_address_;
                write_latch_ = !write_latch_;
            }
            else
            {
                // t: .CDEFGH ........ <- d: ..CDEFGH
                //        <unused>     <- d: AB......
                // t: Z...... ........ <- 0 (bit Z is cleared)
                // w:                  <- 1
                tmp_vram_address_ = (tmp_vram_address_ & 0x00FF) | (data & 0x3F) << 8;
                write_latch_ = !write_latch_;
            }
            break;
        case RegisterAddress::VRAM_DATA:
            if ((vram_address_ & VRAM_ADDRESS_MASK) < PALETTE_BASE_ADDRESS)
            {
                bus_.write(vram_address_ & VRAM_ADDRESS_MASK, data);
            }
            else
            {
                palette_ram_[vram_address_ & PALETTE_RAM_ADDRESS_MASK] = data;
            }
            vram_address_ += vram_address_increment_;
            break;
        default:
            std::unreachable();
    }
}

void Ppu::tick()
{
    if (scanline_ < 240 || scanline_ == 261)
    {
        if ((cycle_ > 1 && cycle_ < 258) || (cycle_ > 320 && cycle_ < 338))
        {
            if (is_rendering_enabled_)
            {
                lsb_pattern_shift_ <<= 1;
                msb_pattern_shift_ <<= 1;
                lsb_attribute_shift_ <<= 1;
                msb_attribute_shift_ <<= 1;
            }
            fetch_next_tile();
        }

        if (is_rendering_enabled_)
        {
            if (cycle_ == 256)
            {
                increment_y();
            }

            if (cycle_ == 257)
            {
                // copies all bits related to horizontal position from t to v
                vram_address_ = (vram_address_ & 0x7BE0) | (tmp_vram_address_ & 0x041F);
            }

            if (scanline_ == 261 && cycle_ >= 280 && cycle_ <= 304)
            {
                // copy the vertical bits from t to v from dots 280 to 304, completing the full
                // initialization of v from t
                vram_address_ = (vram_address_ & 0x041F) | (tmp_vram_address_ & 0x7BE0);
            }
        }

        if (cycle_ == 338 || cycle_ == 340)
        {
            tile_id_latch_ = bus_.read(NAMETABLE_BASE_ADDRESS | (vram_address_ & 0x0FFF));
        }
    }

    if (is_rendering_enabled_ && scanline_ < 240 && (cycle_ > 0 && cycle_ < 257))
    {
        auto bg_pixel = render_background();
        frame_buffer_[(scanline_ * FRAME_WIDTH) + (cycle_ - 1)] = bg_pixel;
    }

    if (scanline_ == 241 && cycle_ == 1)
    {
        registers_.status |= StatusFlag::VBLANK;
        if ((registers_.control & ControlFlag::ENABLE_NMI) != 0)
        {
            is_nmi_pending_ = true;
        }
    }

    if (scanline_ == 261 && cycle_ == 1)
    {
        registers_.status &=
          ~(StatusFlag::SPRITE0_HIT | StatusFlag::SPRITE_OVERFLOW | StatusFlag::VBLANK);
    }

    if (cycle_ == 340)
    {
        cycle_ = 0;
        if (scanline_ == 261)
        {
            scanline_ = 0;
        }
        else
        {
            ++scanline_;
        }
    }
    else
    {
        ++cycle_;
    }
}

bool Ppu::is_nmi_pending()
{
    bool tmp = is_nmi_pending_;
    is_nmi_pending_ = false;
    return tmp;
}

PixelColor Ppu::render_background()
{
    if ((registers_.mask & MaskFlag::ENABLE_BG_RENDERING) != 0)
    {
        std::uint8_t palette_idx = ((msb_attribute_shift_ >> (15 - x_scroll_)) & 1) << 1 |
                                   ((lsb_attribute_shift_ >> (15 - x_scroll_)) & 1);

        std::uint8_t palette_color_idx = ((msb_pattern_shift_ >> (15 - x_scroll_)) & 1) << 1 |
                                         ((lsb_pattern_shift_ >> (15 - x_scroll_)) & 1);

        std::uint8_t color_idx = palette_ram_[(palette_idx << 2) + palette_color_idx];

        return PALETTE[color_idx];
    }

    return PALETTE[palette_ram_[0]];
}

void Ppu::increment_x()
{
    if ((vram_address_ & 0x001F) == 31)
    {
        vram_address_ &= ~0x001F;
        // switch horizontal nametable
        vram_address_ ^= 0x0400;
    }
    else
    {
        ++vram_address_;
    }
}

void Ppu::increment_y()
{
    // https://www.nesdev.org/wiki/PPU_scrolling
    if ((vram_address_ & 0x7000) != 0x7000)
    {
        vram_address_ += 0x1000;
    }
    else
    {
        vram_address_ &= ~0x7000;
        std::uint16_t y = (vram_address_ & 0x03E0) >> 5;

        if (y == 29)
        {
            y = 0;
            // switch vertical nametable
            vram_address_ ^= 0x0800;
        }
        else if (y == 31)
        {
            y = 0;
        }
        else
        {
            y += 1;
        }

        vram_address_ = (vram_address_ & ~0x03E0) | (y << 5);
    }
}

void Ppu::fetch_next_tile()
{
    switch (cycle_ % 8)
    {
        case 1:
            lsb_pattern_shift_ = (lsb_pattern_shift_ & 0xFF00) | lsb_pattern_latch_;
            msb_pattern_shift_ = (msb_pattern_shift_ & 0xFF00) | msb_pattern_latch_;

            lsb_attribute_shift_ = (lsb_attribute_shift_ & 0xFF00) | lsb_attribute_latch_;
            msb_attribute_shift_ = (msb_attribute_shift_ & 0xFF00) | msb_attribute_latch_;

            break;
        case 2:
            tile_id_latch_ = bus_.read(NAMETABLE_BASE_ADDRESS | (vram_address_ & 0x0FFF));
            break;
        case 4: {
            std::uint8_t attribute_byte =
              bus_.read(ATTRIBUTE_BASE_ADDRESS | (vram_address_ & 0x0C00) |
                        ((vram_address_ >> 4) & 0x38) | ((vram_address_ >> 2) & 0x07));
            std::uint8_t coarse_x = vram_address_ & 0x1F;
            std::uint8_t coarse_y = (vram_address_ >> 5) & 0x1F;
            std::uint8_t attr_shift = (coarse_y & 0x02) << 1 | (coarse_x & 0x02);

            lsb_attribute_latch_ = ((attribute_byte >> attr_shift) & 0x01) == 0 ? 0x00 : 0xFF;
            msb_attribute_latch_ = ((attribute_byte >> attr_shift) & 0x02) == 0 ? 0x00 : 0xFF;

            break;
        }
        case 6:
            lsb_pattern_latch_ =
              bus_.read(bg_pattern_address_ + (tile_id_latch_ * 16) + (vram_address_ >> 12));
            break;
        case 0:
            msb_pattern_latch_ =
              bus_.read(bg_pattern_address_ + (tile_id_latch_ * 16) + (vram_address_ >> 12) + 8);

            if (is_rendering_enabled_)
            {
                increment_x();
            }
            break;
    }
}

} // namespace mayones::core
