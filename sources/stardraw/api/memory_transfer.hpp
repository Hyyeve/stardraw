#pragma once
#include <optional>
#include <vector>

#include "stardraw/api/common.hpp"
#include "starlib/general/status.hpp"

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
        enum class type : starlib::u8
        {
            UPLOAD_UNCHECKED, //Uploads directly to the buffer (if possible), does not do any syncronization. Usually you don't want this.
            UPLOAD_TRANSFER_BUFFER, //Uploads to a provided intermediary buffer, then copies to the destination, fully syncronization safe. Usually you want this.
        };

        object_identifier target;
        std::optional<object_identifier> transfer_buffer = std::nullopt;
        starlib::u64 address = 0;
        starlib::u64 bytes = 0;
        type transfer_type = type::UPLOAD_TRANSFER_BUFFER;
    };

    ///Information required to transfer data to/from textures
    ///TODO: Readback operations are currently not implemented.
    struct texture_memory_transfer_info
    {
        enum class type : starlib::u8
        {
            UPLOAD,
        };

        enum class pixel_data_type : starlib::u8
        {
            U8, U32, I8, I32, F32
        };

        enum class pixel_channels : starlib::u8
        {
            R, RG, RGB, RGBA, DEPTH, STENCIL
        };

        //The target texture to upload to
        object_identifier target;

        //The transfer buffer to use to upload texture data
        object_identifier transfer_buffer;

        starlib::u32 x = 0;
        starlib::u32 y = 0;
        starlib::u32 z = 0;

        starlib::u32 width = 1;
        starlib::u32 height = 1;
        starlib::u32 depth = 1;

        starlib::u32 mipmap_level = 0;
        starlib::u32 layer = 0;
        starlib::u32 layers = 1;

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
            starlib::u64 address;
            starlib::u64 size;
        };

        starlib::u64 packed_size = 0;
        starlib::u64 padded_size = 0;
        std::vector<pad> pads;
    };

    ///Allocates and returns a new block of memory with the data laid out according to the layout information.
    ///Inserted padding space is left uninitialized.
    ///NOTE: Input data is assumed to be tightly packed. Using non-tightly-packed input data will result in unexpected behaviour!
    [[nodiscard]] void* layout_memory(const memory_layout_info& layout, const void* data, const starlib::u64 data_size);
}
