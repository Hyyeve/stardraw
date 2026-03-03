#pragma once
#include "types.hpp"
namespace stardraw
{
    enum class memory_transfer_type : uint8_t
    {
        UNCHECKED_UPLOAD, //Fastest upload, but not syncronization safe. Use if doing your own syncronization checks.
        STREAMING_UPLOAD, //Fast upload, allocates additional memory to stage uploads. Use for small repeated uploads.
        CHUNKED_UPLOAD //Slower upload, creates a single-use staging buffer. Use for large infrequent uploads.
    };

    enum class memory_transfer_status
    {
        READY, TRANSFERRING, COMPLETE
    };

    enum class memory_transfer_target_type
    {
        BUFFER, TEXTURE
    };

    struct memory_transfer_info
    {
        static inline memory_transfer_info buffer_upload(const std::string& buffer, const uint64_t address, const uint64_t size, const memory_transfer_type transfer_type = memory_transfer_type::CHUNKED_UPLOAD)
        {
            return {buffer, address, size, transfer_type, memory_transfer_target_type::BUFFER};
        }

        static inline memory_transfer_info texture_upload(const std::string& texture, const uint64_t address, const uint64_t size, const memory_transfer_type transfer_type = memory_transfer_type::CHUNKED_UPLOAD)
        {
            return {texture, address, size, transfer_type, memory_transfer_target_type::TEXTURE};
        }

        std::string target;
        uint64_t address;
        uint64_t bytes;
        memory_transfer_type transfer_type;
        memory_transfer_target_type target_type;
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
