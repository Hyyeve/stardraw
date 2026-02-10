#pragma once
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace stardraw
{
    template <typename base_type>
    class polymorphic_ptr
    {
    public:
        template <typename derived_type>
        // ReSharper disable once CppNonExplicitConvertingConstructor
        polymorphic_ptr(derived_type&& value) // NOLINT(*-forwarding-reference-overload)
        {
            typedef std::decay_t<derived_type> actual_derived;
            static_assert(std::is_base_of_v<base_type, actual_derived>, "Derived type must derive from base");
            static_assert(!std::is_base_of_v<polymorphic_ptr, base_type>, "Polymorphic ptr should not store itself");

            copy_func = [](const std::unique_ptr<base_type>& to_copy)
            {
                const actual_derived* original = static_cast<actual_derived*>(to_copy.get());
                return std::unique_ptr<actual_derived>(new actual_derived(*original));
            };

            stored_ptr.reset(new actual_derived(std::forward<derived_type>(value)));
        }

        polymorphic_ptr(const polymorphic_ptr& other)
        {
            copy_func = other.copy_func;
            stored_ptr = copy_func(other.stored_ptr);
        }

        polymorphic_ptr(polymorphic_ptr& other) : polymorphic_ptr(static_cast<const polymorphic_ptr&>(other)) {}

        polymorphic_ptr(polymorphic_ptr&& other) noexcept
        {
            copy_func = std::move(other.copy_func);
            stored_ptr = std::move(other.stored_ptr);
        }

        polymorphic_ptr& operator=(const polymorphic_ptr& other)
        {
            copy_func = other.copy_func;
            stored_ptr = copy_func(other.stored_ptr);
            return *this;
        }

        polymorphic_ptr& operator=(polymorphic_ptr&& other) noexcept
        {
            copy_func = std::move(other.copy_func);
            stored_ptr = std::move(other.stored_ptr);
            return *this;
        }

        const base_type& operator*() const
        {
            return *ptr();
        }

        base_type& operator*()
        {
            return *ptr();
        }

        const base_type* operator->() const
        {
            return ptr();
        }

        base_type* operator->()
        {
            return ptr();
        }

        const base_type* ptr() const
        {
            return stored_ptr.get();
        }

        base_type* ptr()
        {
            return stored_ptr.get();
        }

    private:
        std::function<std::unique_ptr<base_type>(const std::unique_ptr<base_type>&)> copy_func;
        std::unique_ptr<base_type> stored_ptr;
    };
}
