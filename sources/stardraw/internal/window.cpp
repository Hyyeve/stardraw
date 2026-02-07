#include "stardraw/api/window.hpp"

#include "stardraw/gl45/gl_45_window.hpp"

namespace stardraw
{
    window* window::create(const window_config& config)
    {
        switch (config.api)
        {
            case graphics_api::GL45: return new gl45_window(config);
        }

        return nullptr;
    }
}
