#pragma once
#include <list>

namespace stardraw
{
    class simple_block_allocator
    {
    public:
        explicit simple_block_allocator(const uint64_t size);

        bool try_allocate(const uint64_t size, uint64_t& address_out);
        void deallocate(const uint64_t address);
        [[nodiscard]] uint64_t get_last_used_address() const;
        void resize(const uint64_t size);
        void clear();

    private:
        [[nodiscard]] bool find_free(const uint64_t size, uint64_t& start, uint64_t& block_idx, bool& is_end_allocation) const;

        struct allocation_block
        {
            uint64_t start;
            uint64_t size;
        };

        std::list<allocation_block> allocations {};
        uint64_t total_pool_size;
        uint64_t remaining_available_size;
    };
}
