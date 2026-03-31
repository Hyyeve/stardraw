#pragma once
#include <vector>

#include "stardraw/api/common.hpp"
#include "starlib/types/status.hpp"

namespace stardraw
{
    ///Status for memory transfers
    enum class memory_transfer_status
    {
        READY, TRANSFERRING, COMPLETE
    };

    ///Information required to transfer data to/from buffers
    ///TODO: Readback operations are currently not implemented.
    struct buffer_memory_transfer_info
    {
        enum class type : starlib_stdint::u8
        {
            UPLOAD_UNCHECKED, //Fastest upload, but not syncronization safe. Use if doing your own syncronization checks.
            UPLOAD_STREAMING, //Fast upload, allocates additional memory to stage uploads. Use for small repeated uploads.
            UPLOAD_CHUNK, //Slower upload, creates a single-use staging buffer. Use for large infrequent uploads.
        };

        object_identifier target;
        starlib_stdint::u64 address;
        starlib_stdint::u64 bytes;
        type transfer_type = type::UPLOAD_CHUNK;
    };

    ///Information required to transfer data to/from textures
    ///TODO: Readback operations are currently not implemented.
    struct texture_memory_transfer_info
    {
        enum class type : starlib_stdint::u8
        {
            UPLOAD,
        };

        enum class pixel_data_type : starlib_stdint::u8
        {
            U8, U32, I8, I32, F32
        };

        enum class pixel_channels : starlib_stdint::u8
        {
            R, RG, RGB, RGBA, DEPTH, STENCIL
        };

        object_identifier target;
        starlib_stdint::u32 x = 0;
        starlib_stdint::u32 y = 0;
        starlib_stdint::u32 z = 0;

        starlib_stdint::u32 width = 1;
        starlib_stdint::u32 height = 1;
        starlib_stdint::u32 depth = 1;

        starlib_stdint::u32 mipmap_level = 0;
        starlib_stdint::u32 layer = 0;
        starlib_stdint::u32 layers = 1;

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
        virtual starlib::status transfer(void* data) = 0;
        virtual memory_transfer_status transfer_status() = 0;
    };

    ///Information required to layout packed data into a padded layout
    struct memory_layout_info
    {
        struct pad
        {
            starlib_stdint::u64 address;
            starlib_stdint::u64 size;
        };

        starlib_stdint::u64 packed_size = 0;
        starlib_stdint::u64 padded_size = 0;
        std::vector<pad> pads;
    };

    ///Allocates and returns a new block of memory with the data laid out according to the layout information.
    ///Inserted padding space is left uninitialized.
    ///NOTE: Input data is assumed to be tightly packed. Using non-tightly-packed input data will result in unexpected behaviour!
    [[nodiscard]] void* layout_memory(const memory_layout_info& layout, const void* data, const starlib_stdint::u64 data_size);
}
