#pragma once

#include <cstdint>
#include <span>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include "mayones/core/ppu.hpp"

namespace mayones::app {

class Renderer {
public:
    Renderer(std::uint8_t scale_factor);

    void draw(sf::RenderWindow& window,
              std::span<const core::PixelColor, core::FRAME_BUFFER_SIZE> frame_buffer);

private:
    sf::Texture texture_;
    sf::VertexArray vertex_array_;
};

} // namespace mayones::app
