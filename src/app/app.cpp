#include <print>
#include <utility>

#include "mayones/app/app.hpp"
#include "mayones/app/config.hpp"
#include "mayones/app/version.hpp"

namespace mayones::app {

MayoNES::MayoNES(Config config) :
    m_config{ std::move(config) }
{
    std::println("MyoNES v{}\nConfig:\n  rom_path={}\n  debug={}",
                 mayones::VERSION,
                 m_config.rom_path.generic_string(),
                 m_config.debug);
    std::println("tmp changes");
}

} // namespace mayones::app
