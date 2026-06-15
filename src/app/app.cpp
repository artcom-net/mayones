#include <cstdio>
#include <format>
#include <print>
#include <utility>
#include <vector>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/VideoMode.hpp>

#include "mayones/app/app.hpp"
#include "mayones/app/config.hpp"
#include "mayones/app/formatters.hpp"
#include "mayones/app/render.hpp"
#include "mayones/app/version.hpp"
#include "mayones/core/core.hpp"
#include "mayones/core/rom.hpp"
#include "mayones/io/file.hpp"

namespace mayones::app {

MayoNes::MayoNes(Config config) :
    nes_core_{ frame_buffer_ },
    renderer_{ 4 }, // TODO: take from config
    config_{ std::move(config) }
{
}

int MayoNes::run()
{
    std::println("MayoNES {}\n{}", mayones::VERSION, config_);

    auto read_result = mayones::io::read_binary_file(config_.rom_path);
    if (!read_result)
    {
        println(stderr, "Error read ROM: {}", read_result.error());
        return 1;
    }

    if (auto load_result = nes_core_.load_rom(std::move(read_result).value()); !load_result)
    {
        std::println(stderr, "Error load ROM: {}", load_result.error());
        return 1;
    }

    const core::rom::RomInfo& rom_info = nes_core_.rom_info();
    std::println("{}", rom_info);

    bool pause_core{};
    nes_core_.reset();
    sf::RenderWindow window(sf::VideoMode({ WINDOW_WIDTH, WINDOW_HEIGHT }), "MyoNES");

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
                {
                    pause_core = !pause_core;
                }
            }
        }

        if (!pause_core)
        {
            nes_core_.tick_frame();
            renderer_.draw(window, frame_buffer_);
        }

        window.display();
    }

    return 0;
}

} // namespace mayones::app
