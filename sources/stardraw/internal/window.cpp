
#include "stardraw/gl45/gl45_window.hpp"
#include "stardraw/api/window.hpp"

namespace stardraw
{
    status window::create(const window_config& config, window** out_window)
    {
        switch (config.api)
        {
            case graphics_api::GL45: return gl45::gl45_window::create_gl45_window(config, out_window);
            default: return { status_type::UNSUPPORTED, "Provided graphics api is not supported." };
        }
    }
}
