#pragma once
#include "glfw_window.hpp"

namespace starwin
{
    using namespace starlib_stdint;
    class gl45_window final : public glfw_window
    {
    public:
        static status create(const window_config& config, window*& out_window);

        [[nodiscard]] status set_vsync(const bool sync) override;

        ~gl45_window() override;

        status refresh() override;
    };

}