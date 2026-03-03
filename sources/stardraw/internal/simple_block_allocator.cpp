#include "simple_block_allocator.hpp"
#include <tracy/Tracy.hpp>

namespace stardraw
{
    simple_block_allocator::simple_block_allocator(const uint64_t size) : total_pool_size(size), remaining_available_size(total_pool_size) {}

    bool simple_block_allocator::try_allocate(const uint64_t size, uint64_t& address_out)
    {
        ZoneScoped;
        uint64_t block_idx;
        bool is_end_allocation;
        const bool has_space = find_free(size, address_out, block_idx, is_end_allocation);

        if (!has_space)
        {
            return false;
        }

        if (is_end_allocation) allocations.push_back({address_out, size});
        else allocations.insert(std::next(allocations.begin(), block_idx), {address_out, size});

        remaining_available_size -= size;
        return true;
    }

    void simple_block_allocator::deallocate(const uint64_t address)
    {
        ZoneScoped;
        uint64_t size = 0;
        const auto loc = std::ranges::find_if(allocations, [address, &size](const allocation_block& block)
        {
            if (block.start == address)
            {
                size = block.size;
                return true;
            }
            return false;
        });

        if (loc != allocations.end())
        {
            remaining_available_size += size;
            allocations.erase(loc);
        }
    }

    uint64_t simple_block_allocator::get_last_used_address() const
    {
        if (allocations.empty()) return 0;
        const allocation_block& last_block = allocations.back();
        return last_block.start + last_block.size;
    }

    void simple_block_allocator::resize(const uint64_t size)
    {
        remaining_available_size += size - total_pool_size;
        total_pool_size = size;
    }

    void simple_block_allocator::clear()
    {
        allocations.clear();
        remaining_available_size = total_pool_size;
    }

    bool simple_block_allocator::find_free(const uint64_t size, uint64_t& start, uint64_t& block_idx, bool& is_end_allocation) const
    {
        ZoneScoped;
        //Can't fit!
        if (size > remaining_available_size)
        {
            return false;
        }

        //Definitely fits
        if (allocations.empty())
        {
            start = 0;
            block_idx = 0;
            return true;
        }

        //Might fit at the end of the pool?
        const allocation_block& last_block = allocations.back();
        start = last_block.start + last_block.size;
        if (start + size <= total_pool_size)
        {
            block_idx = allocations.size();
            is_end_allocation = true;
            return true;
        }

        //Doesn't fit at the end - do we have space elsewhere?
        uint64_t remaining_possible_space = remaining_available_size - last_block.size;
        if (size > remaining_possible_space)
        {
            return false;
        }


        //Might have an empty space big enough from deallocations - iterate through and check
        //Performs best if elements are allocated and deallocated in roughly FIFO order
        start = 0;
        block_idx = 0;
        is_end_allocation = false;
        for (const allocation_block block : allocations)
        {
            if (block.start < start + size)
            {
                remaining_possible_space -= block.size;
                start = block.start + block.size;
                block_idx++;
            }
            else
            {
                return true;
            }

            if (size > remaining_possible_space)
            {
                return false;
            }
        }

        return false;
    }
}
