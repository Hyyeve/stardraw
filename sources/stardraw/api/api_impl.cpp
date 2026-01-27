#include "api_impl.hpp"
#include "stardraw/gl45/gl45_impl.hpp"

namespace stardraw
{
    api_impl* api_impl::create(const graphics_api api)
    {
        switch (api)
        {
            case graphics_api::GL45: return new gl45::gl45_impl();
        }
        return nullptr;
    }
}
