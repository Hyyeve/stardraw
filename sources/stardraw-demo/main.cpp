#include "stardraw/api/window.hpp"
using namespace stardraw;

int main()
{

    window* wind = window::create({graphics_api::GL45});



    while (true)
    {
        wind->TEMP_UPDATE_WINDOW();
    }


    return 0;
}