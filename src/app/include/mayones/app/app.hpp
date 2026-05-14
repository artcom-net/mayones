#pragma once

#include "config.hpp"
#include "mayones/core/core.hpp"

namespace mayones::app {

class MayoNes {
public:
    explicit MayoNes(Config config);

    int run();

private:
    core::NesCore m_nes_core;
    Config m_config;
};

} // namespace mayones::app
