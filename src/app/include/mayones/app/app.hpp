#pragma once

#include "config.hpp"
#include "mayones/core/core.hpp"

namespace mayones::app {

class MayoNes {
public:
    explicit MayoNes(Config config);

    int run();

private:
    core::NesCore nes_core_;
    Config config_;
};

} // namespace mayones::app
