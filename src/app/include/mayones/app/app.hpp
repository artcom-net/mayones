#pragma once

#include <array>

#include "config.hpp"
#include "mayones/app/render.hpp"
#include "mayones/core/core.hpp"
#include "mayones/core/ppu.hpp"

namespace mayones::app {

class MayoNes {
public:
    explicit MayoNes(Config config);

    int run();

private:
    static constexpr std::uint16_t WINDOW_WIDTH{ 1024 };
    static constexpr std::uint16_t WINDOW_HEIGHT{ 960 };

    std::array<core::PixelColor, core::FRAME_BUFFER_SIZE> frame_buffer_{};
    core::NesCore nes_core_;
    Renderer renderer_;
    Config config_;
};

} // namespace mayones::app
