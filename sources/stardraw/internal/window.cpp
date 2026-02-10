
#include "stardraw/gl45/gl45_window.hpp"
#include "stardraw/api/window.hpp"

namespace stardraw
{
    window* window::create(const window_config& config)
    {
        switch (config.api)
        {
            case graphics_api::GL45: return new gl45::gl45_window(config);
            default: return nullptr;
        }
    }
}
