#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace stardraw
{
    struct shader_parameter_value
    {
        enum class value_type
        {
            BOOL, FLOAT, DOUBLE, INT, UINT, FLOAT_MATRIX, DOUBLE_MATRIX, OPAQUE_REFERENCE
        };

        enum class vector_size_type
        {
            _1, _2, _3, _4,
        };

        enum class matrix_dimensions_type
        {
            _2x2, _2x3, _2x4, _3x2, _3x3, _3x4, _4x2, _4x3, _4x4
        };

        template <uint32_t rows, uint32_t columns>
        struct matrix_dimensions_info
        {
            static_assert(rows > 1 && rows < 5, "Matrix row count must be 2-4");
            static_assert(columns > 1 && columns < 5, "Matrix column count must be 2-4");
            constexpr static matrix_dimensions_type type = matrix_dimensions_type::_2x2;
        };

        template <uint32_t vector_size>
        struct vector_size_info
        {
            static_assert(vector_size > 0 && vector_size < 5, "Vector size must be 2-4");
            constexpr static uint32_t num_elements = vector_size;
            constexpr static vector_size_type type = vector_size_type::_1;
        };

        template <typename data_type, typename... data_type_pack>
        static shader_parameter_value vector(data_type val0, data_type_pack... vals)
        {
            static_assert(is_valid_scalar_type<data_type>(), "Type must be a supported scalar type");
            static_assert((... && (typeid(data_type_pack) == typeid(data_type))), "All parameters must be the same type");
            const uint32_t vector_size = 1 + sizeof...(data_type_pack);
            static_assert(vector_size > 0 && vector_size <= 4, "Number of args (vector size) must be 1-4");

            return shader_parameter_value {
                get_scalar_type<data_type>(),
                matrix_dimensions_type::_2x2,
                vector_size_info<vector_size>::type,
                1,
                to_bytes<std::array<data_type, vector_size>>({val0, vals...}),
            };
        }

        template <uint64_t vector_size = 1, typename data_type>
        static shader_parameter_value vector(std::array<data_type, vector_size> array)
        {
            static_assert(vector_size > 0 && vector_size < 5, "Vector size must be 1-4");
            static_assert(is_valid_scalar_type<data_type>(), "Type must be a supported scalar type");

            return shader_parameter_value {
                get_scalar_type<data_type>(),
                matrix_dimensions_type::_2x2,
                vector_size_info<vector_size>::type,
                1,
                to_bytes<std::array<data_type, vector_size>>(array),
            };
        }

        template <uint64_t vector_size = 1, uint64_t total_size, typename data_type>
        static shader_parameter_value vectors(std::array<data_type, total_size> array)
        {
            static_assert(total_size % vector_size == 0, "Total elements must be a multiple of vector size");
            static_assert(total_size > 0, "Array size cannot be 0");
            static_assert(vector_size > 0 && vector_size < 5, "Vector size must be 1-4");
            static_assert(is_valid_scalar_type<data_type>(), "Type must be a supported scalar type");

            return shader_parameter_value {
                get_scalar_type<data_type>(),
                matrix_dimensions_type::_2x2,
                vector_size_info<vector_size>::type,
                total_size / vector_size,
                to_bytes<std::array<data_type, total_size>>(array),
            };
        }

        template <typename dimensions, uint64_t total_elements, typename data_type>
        static shader_parameter_value matrices(std::array<data_type, total_elements> matrix)
        {
            static_assert(is_valid_matrix_type<data_type>(), "Type must be a supported matrix type");
            static_assert(is_matrix_dimensions<dimensions>, "Dimensions must be type of matrix_dimensions<rows, columns>");
            static_assert(total_elements > 0, "Array size cannot be 0");
            static_assert(total_elements % dimensions::num_elements == 0, "Total elements must be a multiple of number of elements per array");
            return shader_parameter_value {
                get_matrix_type<data_type>(),
                dimensions::type,
                vector_size_type::_1,
                total_elements / dimensions::num_elements,
                to_bytes<std::array<data_type, total_elements>>(matrix),
            };
        }

        static shader_parameter_value opaque(const std::string& reference)
        {
            return shader_parameter_value {
                value_type::OPAQUE_REFERENCE,
                matrix_dimensions_type::_2x2,
                vector_size_type::_1,
                0,
                {},
                reference,
            };
        }

        value_type type = value_type::FLOAT;
        matrix_dimensions_type matrix_size = matrix_dimensions_type::_2x2;
        vector_size_type vector_size = vector_size_type::_1;
        uint32_t array_size = 0;
        std::vector<uint8_t> bytes = {};
        std::string opaque_reference = "???";

    private:
        template <class t>
        constexpr static bool is_matrix_dimensions = false;


        template <typename data_type>
        constexpr static value_type get_scalar_type()
        {
            if (typeid(data_type) == typeid(bool)) return value_type::BOOL;
            if (typeid(data_type) == typeid(float)) return value_type::FLOAT;
            if (typeid(data_type) == typeid(double)) return value_type::DOUBLE;
            if (typeid(data_type) == typeid(uint32_t)) return value_type::UINT;
            if (typeid(data_type) == typeid(int32_t)) return value_type::INT;
            return value_type::FLOAT;
        }

        template <typename data_type>
        constexpr static bool is_valid_scalar_type()
        {
            return
                (typeid(data_type) == typeid(bool)) ||
                (typeid(data_type) == typeid(float)) ||
                (typeid(data_type) == typeid(double)) ||
                (typeid(data_type) == typeid(int32_t)) ||
                (typeid(data_type) == typeid(uint32_t));
        }

        template <typename data_type>
        constexpr static value_type get_matrix_type()
        {
            if (typeid(data_type) == typeid(float)) return value_type::FLOAT_MATRIX;
            if (typeid(data_type) == typeid(double)) return value_type::DOUBLE_MATRIX;
            return value_type::FLOAT;
        }

        template <typename data_type>
        constexpr static bool is_valid_matrix_type()
        {
            return
                (typeid(data_type) == typeid(float)) ||
                (typeid(data_type) == typeid(double));
        }

        template <typename data_type>
        static std::vector<uint8_t> to_bytes(data_type value)
        {
            uint8_t* ptr = reinterpret_cast<uint8_t*>(&value);
            return {ptr, ptr + sizeof(data_type)};
        }
    };

    template <uint32_t rows, uint32_t columns>
    struct matrix_dimensions
    {
        static_assert(rows > 1 && rows < 5, "Matrix row count must be 2-4");
        static_assert(columns > 1 && columns < 5, "Matrix column count must be 2-4");
        constexpr static uint32_t num_elements = rows * columns;
        constexpr static uint32_t num_rows = rows;
        constexpr static uint32_t num_columns = columns;
        constexpr static shader_parameter_value::matrix_dimensions_type type = shader_parameter_value::matrix_dimensions_info<rows, columns>::type;
    };

    template <uint32_t a, uint32_t b>
    constexpr bool shader_parameter_value::is_matrix_dimensions<matrix_dimensions<a, b>> = true;

    template <>
    struct shader_parameter_value::matrix_dimensions_info<2, 2>
    {
        constexpr static matrix_dimensions_type type = matrix_dimensions_type::_2x2;
    };

    template <>
    struct shader_parameter_value::matrix_dimensions_info<2, 3>
    {
        constexpr static matrix_dimensions_type type = matrix_dimensions_type::_2x3;
    };

    template <>
    struct shader_parameter_value::matrix_dimensions_info<2, 4>
    {
        constexpr static matrix_dimensions_type type = matrix_dimensions_type::_2x4;
    };

    template <>
    struct shader_parameter_value::matrix_dimensions_info<3, 2>
    {
        constexpr static matrix_dimensions_type type = matrix_dimensions_type::_3x2;
    };

    template <>
    struct shader_parameter_value::matrix_dimensions_info<3, 3>
    {
        constexpr static matrix_dimensions_type type = matrix_dimensions_type::_3x3;
    };

    template <>
    struct shader_parameter_value::matrix_dimensions_info<3, 4>
    {
        constexpr static matrix_dimensions_type type = matrix_dimensions_type::_3x4;
    };

    template <>
    struct shader_parameter_value::matrix_dimensions_info<4, 2>
    {
        constexpr static matrix_dimensions_type type = matrix_dimensions_type::_4x2;
    };

    template <>
    struct shader_parameter_value::matrix_dimensions_info<4, 3>
    {
        constexpr static matrix_dimensions_type type = matrix_dimensions_type::_4x3;
    };

    template <>
    struct shader_parameter_value::matrix_dimensions_info<4, 4>
    {
        constexpr static matrix_dimensions_type type = matrix_dimensions_type::_4x4;
    };

    template <>
    struct shader_parameter_value::vector_size_info<1>
    {
        constexpr static vector_size_type type = vector_size_type::_1;
    };

    template <>
    struct shader_parameter_value::vector_size_info<2>
    {
        constexpr static vector_size_type type = vector_size_type::_2;
    };

    template <>
    struct shader_parameter_value::vector_size_info<3>
    {
        constexpr static vector_size_type type = vector_size_type::_3;
    };

    template <>
    struct shader_parameter_value::vector_size_info<4>
    {
        constexpr static vector_size_type type = vector_size_type::_4;
    };
}
