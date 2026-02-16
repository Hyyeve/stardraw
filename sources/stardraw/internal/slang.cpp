#include <array>
#include <format>
#include <unordered_map>

#include <slang-com-ptr.h>
#include "slang.hpp"

#include <ranges>

namespace stardraw
{
    shader_cursor shader_cursor::index(const uint32_t index) const
    {
        slang::TypeLayoutReflection* element_layout = type_layout->getElementTypeLayout();
        if (element_layout == nullptr) return invalid_location;

        shader_cursor result = shader_cursor(*this);
        result.type_layout = element_layout;
        result.byte_address += index * element_layout->getStride();

        result.binding_slot_index *= type_layout->getElementCount();
        result.binding_slot_index += index;

        return result;
    }

    shader_cursor shader_cursor::field(const std::string_view& name) const
    {
        const int32_t index = type_layout->findFieldIndexByName(name.data(), name.data() + name.size());
        if (index < 0) return invalid_location;

        slang::VariableLayoutReflection* field = type_layout->getFieldByIndex(index);

        shader_cursor result = shader_cursor(*this);
        result.type_layout = field->getTypeLayout();
        result.byte_address += field->getOffset();
        result.binding_slot += type_layout->getFieldBindingRangeOffset(index);

        return result;
    }

    shader_cursor locate_shader_param(slang::ShaderReflection* shader_layout, const std::string_view& name)
    {
        shader_cursor result;

        slang::TypeLayoutReflection* globals = shader_layout->getGlobalParamsTypeLayout();
        const uint64_t global_idx = globals->findFieldIndexByName(name.data(), name.data() + name.size());
        slang::VariableLayoutReflection* root_param = globals->getFieldByIndex(global_idx);
        if (root_param == nullptr) return invalid_location;

        result.type_layout = root_param->getTypeLayout()->getElementTypeLayout();
        result.byte_address = 0;
        result.binding_slot = globals->getFieldBindingRangeOffset(global_idx);
        result.binding_slot_index = 0;

        return result;
    }


}
