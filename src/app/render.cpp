#include <cstdint>

#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>

#include "mayones/app/render.hpp"
#include "mayones/core/ppu.hpp"

namespace mayones::app {

Renderer::Renderer(std::uint8_t scale_factor) :
    texture_{ sf::Vector2u{ core::FRAME_WIDTH, core::FRAME_HEIGHT } },
    vertex_array_{ sf::PrimitiveType::TriangleStrip, 4 }
{
    const auto scaled_witdh = static_cast<float>(core::FRAME_WIDTH * scale_factor);
    const auto scaled_height = static_cast<float>(core::FRAME_HEIGHT * scale_factor);

    vertex_array_[0].position = sf::Vector2f{ 0.0f, 0.0f };
    vertex_array_[1].position = sf::Vector2f{ scaled_witdh, 0.0f };
    vertex_array_[2].position = sf::Vector2f{ 0.0f, scaled_height };
    vertex_array_[3].position = sf::Vector2f{ scaled_witdh, scaled_height };

    vertex_array_[0].texCoords = sf::Vector2f{ 0.0f, 0.0f };
    vertex_array_[1].texCoords = sf::Vector2f{ core::FRAME_WIDTH, 0.0f };
    vertex_array_[2].texCoords = sf::Vector2f{ 0.0f, core::FRAME_HEIGHT };
    vertex_array_[3].texCoords = sf::Vector2f{ core::FRAME_WIDTH, core::FRAME_HEIGHT };
}

void Renderer::draw(sf::RenderWindow& window,
                    std::span<const core::PixelColor, core::FRAME_BUFFER_SIZE> frame_buffer)
{
    texture_.update(reinterpret_cast<const std::uint8_t*>(frame_buffer.data()));
    window.draw(vertex_array_, &texture_);
}

} // namespace mayones::app
