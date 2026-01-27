#pragma once
#include <memory>
#include <utility>
#include <vector>

namespace stardraw
{
    template <typename stored_type>
    class polymorphic_list_builder;

    template <typename stored_type>
    class polymorphic_list
    {
    public:
        template <typename... stored_types>
        static std::unique_ptr<polymorphic_list> create(stored_types... values)
        {
            return std::unique_ptr<polymorphic_list>(new polymorphic_list(values...));
        }

        polymorphic_list(polymorphic_list& other) = delete;                      //COPY CONSTRUCTOR
        polymorphic_list(polymorphic_list&& other) = delete;                     //MOVE CONSTRUCTOR
        polymorphic_list& operator=(polymorphic_list& other) = delete;           //COPY ASSIGNMENT
        polymorphic_list& operator=(polymorphic_list&& other) noexcept = delete; //MOVE ASSIGNMENT

        ~polymorphic_list()
        {
            for (const stored_type* value : list)
            {
                delete value;
            }
        }

        auto begin() const { return list.cbegin(); }
        auto end() const { return list.cend(); }

    private:
        friend class polymorphic_list_builder<stored_type>;

        template <typename... stored_types>
        explicit polymorphic_list(const stored_types&... values)
        {
            (add(std::forward<const stored_types&>(values)), ...);
        }

        template <typename stored_derived_type>
        void add(const stored_derived_type& value)
        {
            static_assert(std::is_base_of_v<stored_type, stored_derived_type>, "Not a valid type for this list");
            stored_derived_type* stored_heap = new stored_derived_type(value);
            list.push_back(stored_heap);
        }

        std::vector<stored_type*> list;
    };

    template <typename stored_type>
    class polymorphic_list_builder
    {
    public:
        polymorphic_list_builder() : poly_list(new polymorphic_list<stored_type>()) {}

        polymorphic_list_builder(polymorphic_list_builder& other) = delete;                      //COPY CONSTRUCTOR
        polymorphic_list_builder(polymorphic_list_builder&& other) = delete;                     //MOVE CONSTRUCTOR
        polymorphic_list_builder& operator=(polymorphic_list_builder& other) = delete;           //COPY ASSIGNMENT
        polymorphic_list_builder& operator=(polymorphic_list_builder&& other) noexcept = delete; //MOVE ASSIGNMENT

        ~polymorphic_list_builder()
        {
            delete poly_list;
        }

        template <typename stored_derived_type>
        void add(const stored_derived_type& value)
        {
            poly_list->add(value);
        }

        std::unique_ptr<polymorphic_list<stored_type>> finish()
        {
            polymorphic_list<stored_type>* built = poly_list;
            poly_list = new polymorphic_list<stored_type>();
            return std::unique_ptr<polymorphic_list<stored_type>>(built);
        }

    private:
        polymorphic_list<stored_type>* poly_list;
    };
}
