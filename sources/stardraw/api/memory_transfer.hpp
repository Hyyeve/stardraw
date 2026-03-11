#pragma once
#include "stardraw/api/common.hpp"
namespace stardraw
{
    using namespace starlib_stdint;
    enum class memory_transfer_status
    {
        READY, TRANSFERRING, COMPLETE
    };


    struct buffer_memory_transfer_info
    {
        enum class type : u8
        {
            UPLOAD_UNCHECKED, //Fastest upload, but not syncronization safe. Use if doing your own syncronization checks.
            UPLOAD_STREAMING, //Fast upload, allocates additional memory to stage uploads. Use for small repeated uploads.
            UPLOAD_CHUNK, //Slower upload, creates a single-use staging buffer. Use for large infrequent uploads.
        };

        object_identifier target;
        u64 address;
        u64 bytes;
        type transfer_type = type::UPLOAD_CHUNK;
    };


    struct texture_memory_transfer_info
    {
        enum class pixel_data_type
        {
            U8, U32, I8, I32, F32
        };

        enum class pixel_channels
        {
            R, RG, RGB, RGBA, DEPTH, STENCIL
        };

        object_identifier target;
        u32 x = 0;
        u32 y = 0;
        u32 z = 0;

        u32 width = 1;
        u32 height = 1;
        u32 depth = 1;

        u32 mipmap_level = 0;
        u32 layer = 0;
        u32 layers = 1;

        //The data type that the pixels being uploaded are provided as.
        pixel_data_type data_type = pixel_data_type::U8;
        //The channels that the pixels being provided include
        pixel_channels channels = pixel_channels::RGBA;
    };

    //Single-use threadsafe handle for performing a memory transfer.
    class memory_transfer_handle
    {
    public:
        virtual ~memory_transfer_handle() = default;

        //Transfer the requested memory amount in or out of data. Blocks calling thread until transfer is completed (or an error status is generated)
        //Call from a different thread if you want to avoid blocking your render thread during the transfer
        virtual status transfer(void* data) = 0;
        virtual memory_transfer_status transfer_status() = 0;
    };

}
