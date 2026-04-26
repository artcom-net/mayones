#pragma once

#include "config.hpp"

namespace mayones::app {

class MayoNES {
public:
    explicit MayoNES(Config config);

private:
    Config m_config;
};

} // namespace mayones::app
