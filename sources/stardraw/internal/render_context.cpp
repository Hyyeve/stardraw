#include "stardraw/gl45/render_context.hpp"

namespace stardraw
{
    using namespace starlib;

    status render_context::create(const render_context_config& info, render_context*& out_ptr)
    {
        if (out_ptr == nullptr) return {status_type::INVALID, "Null out pointer passed to render context create"};

        status out_status = status_type::SUCCESS;

        switch (info.api)
        {
            case graphics_api::GL45:
            {
                out_ptr = new gl45::render_context(info, out_status);
                break;
            }
            default:
            {
                return {status_type::INVALID, "Unsupported graphics api"};
            }
        }

        return out_status;
    }
}
