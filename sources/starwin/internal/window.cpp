
#include "starwin/api/window.hpp"

#include "gl45_window.hpp"
#include "starlib/types/status.hpp"

namespace starwin
{
    status window::create(const window_config& config, window*& out_window)
    {
        switch (config.api)
        {
            case graphics_api::GL45: return gl45_window::create(config, out_window);
            default: return { status_type::UNSUPPORTED, "Provided graphics api is not supported." };
        }
    }
}
