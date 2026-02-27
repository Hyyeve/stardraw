#include "../api/shaders.hpp"
#include "internal.hpp"

#include <array>
#include <format>
#include <queue>
#include <set>
#include <slang-com-ptr.h>
#include <slang.h>
#include <spirv_glsl.hpp>
#include <stack>

template <>
struct std::hash<stardraw::shader_entry_point>
{
    std::size_t operator()(const stardraw::shader_entry_point& key) const noexcept
    {
        return hash<string>()(key.module_name + key.entry_point_name);
    }
};

namespace stardraw
{
    struct linked_set
    {
        Slang::ComPtr<slang::IComponentType> linked_components;
        std::unordered_map<shader_entry_point, uint32_t> entry_point_indexes;
    };

    static slang::IGlobalSession* global_slang_context;
    static slang::ISession* active_slang_session;
    static std::unordered_map<std::string, Slang::ComPtr<slang::IModule>> loaded_modules;
    static std::unordered_map<std::string, linked_set> linked_sets;

    status delete_shader_buffer_layout(shader_buffer_layout** buffer_layout)
    {
        if (buffer_layout == nullptr) return status_type::UNEXPECTED;
        delete *buffer_layout;
        return status_type::SUCCESS;
    }

    void* layout_shader_buffer_memory(const shader_buffer_layout* layout, const void* data, const uint64_t data_size)
    {
        if (layout == nullptr || data == nullptr) return nullptr;

        const uint64_t element_count = data_size / layout->packed_size;
        const uint8_t* in_bytes = static_cast<const uint8_t*>(data);
        uint8_t* output = static_cast<uint8_t*>(malloc(layout->padded_size * element_count));
        if (layout->padded_size == layout->packed_size)
        {
            memcpy(output, in_bytes, layout->padded_size * element_count);
            return output;
        }

        for (uint64_t idx = 0; idx < element_count; idx++)
        {
            const uint64_t base_read_address = layout->packed_size * idx;
            const uint64_t base_write_address = layout->padded_size * idx;

            uint64_t current_write_offset = 0;
            uint64_t current_read_offset = 0;

            for (const shader_buffer_layout::pad& pad : layout->pads)
            {
                const uint64_t size_to_pad_start = pad.address - current_write_offset;
                memcpy(output + base_write_address + current_write_offset, in_bytes + base_read_address + current_read_offset, size_to_pad_start);
                current_write_offset = pad.address + pad.size;
                current_read_offset += size_to_pad_start;
            }
        }

        return output;
    }


    int get_target_index_for_api(const graphics_api& api)
    {
        switch (api)
        {
            case graphics_api::GL45: return 0;
        }

        return -1;
    }

    status setup_shader_compiler(const std::vector<shader_macro>& macro_defines)
    {
        if (global_slang_context == nullptr)
        {
            const SlangResult result = slang::createGlobalSession(&global_slang_context);
            if (SLANG_FAILED(result)) return {status_type::BACKEND_ERROR, "Slang context creation failed"};
        }

        if (active_slang_session != nullptr)
        {
            const SlangResult delete_result = active_slang_session->release();
            delete active_slang_session;

            loaded_modules.clear();

            if (SLANG_FAILED(delete_result)) return {status_type::BACKEND_ERROR, "Deleting previous slang session failed"};
        }

        std::vector<slang::CompilerOptionEntry> compiler_options;
        for (const shader_macro& macro : macro_defines)
        {
            compiler_options.push_back(slang::CompilerOptionEntry {
                slang::CompilerOptionName::MacroDefine,
                slang::CompilerOptionValue {
                    slang::CompilerOptionValueKind::String,
                    0, 0, macro.name.data(), macro.value.data()
                }
            });
        }

        slang::SessionDesc session_desc;

        const static std::array slang_targets = {
            slang::TargetDesc {
                .format = SlangCompileTarget::SLANG_SPIRV,
                .profile = global_slang_context->findProfile("spirv_latest"),
            },
        };

        session_desc.targets = slang_targets.data();
        session_desc.targetCount = slang_targets.size();

        session_desc.compilerOptionEntries = compiler_options.data();
        session_desc.compilerOptionEntryCount = compiler_options.size();

        session_desc.searchPaths = nullptr;
        session_desc.searchPathCount = 0;

        const SlangResult session_creation = global_slang_context->createSession(session_desc, &active_slang_session);
        if (SLANG_FAILED(session_creation)) return {status_type::BACKEND_ERROR, "Slang session creation failed"};

        return status_type::SUCCESS;
    }

    status load_shader_module(const std::string_view& module_name, const std::string_view& source)
    {
        const std::string fake_path = std::format("{0}_fakepath.slang", module_name);
        Slang::ComPtr<slang::IBlob> diagnostics;
        const Slang::ComPtr module(active_slang_session->loadModuleFromSourceString(module_name.data(), fake_path.c_str(), source.data(), diagnostics.writeRef()));

        if (diagnostics)
        {
            std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
            return {status_type::BACKEND_ERROR, std::format("Slang module loading '{1}' failed with error: '{0}'", msg, module_name)};
        }

        if (!module)
        {
            std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
            return {status_type::BACKEND_ERROR, std::format("Slang module '{1}' loading failed with error: '{0}'", msg, module_name)};
        }

        loaded_modules[std::string(module_name)] = module;

        return status_type::SUCCESS;
    }

    status load_shader_module(const std::string_view& module_name, const void* cache_ptr, const uint64_t cache_size)
    {
        const std::string fake_path = std::format("{0}_fakepath.slang", module_name);
        Slang::ComPtr<slang::IBlob> diagnostics;

        const Slang::ComPtr module(active_slang_session->loadModuleFromIRBlob(module_name.data(), fake_path.c_str(), slang_createBlob(cache_ptr, cache_size), diagnostics.writeRef()));

        if (diagnostics)
        {
            std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
            return {status_type::BACKEND_ERROR, std::format("Slang module loading '{1}' failed with error: '{0}'", msg, module_name)};
        }

        if (!module)
        {
            return {status_type::BACKEND_ERROR, std::format("Slang module '{0}' loading failed with unknwon error", module_name)};
        }

        loaded_modules[std::string(module_name)] = module;

        return status_type::SUCCESS;
    }

    status cache_shader_module(const std::string& module_name, void** out_cache_ptr, uint64_t& out_cache_size)
    {
        if (!loaded_modules.contains(module_name)) return {status_type::UNKNOWN, std::format("No loaded slang module called '{0}' found.", module_name)};
        const Slang::ComPtr<slang::IModule> module = loaded_modules[module_name];

        Slang::ComPtr<ISlangBlob> serialized_blob;
        const SlangResult serialize_result = module->serialize(serialized_blob.writeRef());
        if (SLANG_FAILED(serialize_result)) return {status_type::BACKEND_ERROR, std::format("Failed to serialize module '{0}'", module_name)};

        const uint64_t cache_size = serialized_blob->getBufferSize();

        *out_cache_ptr = malloc(cache_size);
        memcpy(*out_cache_ptr, serialized_blob->getBufferPointer(), cache_size);
        out_cache_size = cache_size;

        return status_type::SUCCESS;
    }


    status link_shader_modules(const std::string& linked_set_name, const std::vector<shader_entry_point>& entry_points, const std::vector<std::string>& additional_modules)
    {
        std::vector<slang::IComponentType*> shader_components;
        std::unordered_map<shader_entry_point, uint32_t> entry_point_index_map;

        for (uint32_t idx = 0; idx < entry_points.size(); idx++)
        {
            const shader_entry_point& entry_point = entry_points[idx];

            if (!loaded_modules.contains(entry_point.module_name)) return {status_type::UNKNOWN, std::format("No loaded slang module called '{0}' found.", entry_point.module_name)};
            const Slang::ComPtr<slang::IModule> module = loaded_modules[entry_point.module_name];

            Slang::ComPtr<slang::IEntryPoint> slang_entry_point;

            const SlangResult found_entry_point = module->findEntryPointByName(entry_point.entry_point_name.c_str(), slang_entry_point.writeRef());
            if (SLANG_FAILED(found_entry_point)) return {status_type::BACKEND_ERROR, std::format("Couldn't find entry point named '{0}' in module '{1}'", entry_point.entry_point_name, entry_point.module_name)};

            shader_components.push_back(slang_entry_point);
            entry_point_index_map[entry_point] = idx;
        }

        for (const std::string& module_name : additional_modules)
        {
            if (!loaded_modules.contains(module_name)) return {status_type::UNKNOWN, std::format("No loaded slang module called '{0}' found.", module_name)};
            const Slang::ComPtr<slang::IModule> additional_module = loaded_modules[module_name];
            shader_components.push_back(additional_module);
        }

        Slang::ComPtr<slang::IComponentType> composite;

        {
            Slang::ComPtr<slang::IBlob> diagnostics;
            active_slang_session->createCompositeComponentType(shader_components.data(), shader_components.size(), composite.writeRef(), diagnostics.writeRef());

            if (diagnostics)
            {
                std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
                return {status_type::BACKEND_ERROR, std::format("Slang shader linking for '{1}' failed with error: '{0}'", msg, linked_set_name)};
            }
        }

        Slang::ComPtr<slang::IComponentType> linked_program;

        {
            Slang::ComPtr<slang::IBlob> diagnostics;
            composite->link(linked_program.writeRef(), diagnostics.writeRef());

            if (diagnostics)
            {
                std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
                return {status_type::BACKEND_ERROR, std::format("Slang shader linking for '{1}' failed with error: '{0}'", msg, linked_set_name)};
            }
        }


        linked_sets[linked_set_name] = {
            linked_program,
            std::move(entry_point_index_map)
        };

        return status_type::SUCCESS;
    }

    status create_shader_program(const std::string& linked_set_name, const shader_entry_point& entry_point, const graphics_api& api, shader_program** out_shader_program)
    {
        if (out_shader_program == nullptr) return status_type::UNEXPECTED;
        *out_shader_program = new shader_program();
        shader_program* result = *out_shader_program;

        if (!linked_sets.contains(linked_set_name)) return {status_type::UNKNOWN, std::format("No linked slang shader called '{0}' exists.", linked_set_name)};
        const linked_set& linked_set = linked_sets[linked_set_name];
        const Slang::ComPtr<slang::IComponentType> linked_shader = linked_set.linked_components;

        if (!linked_set.entry_point_indexes.contains(entry_point)) return {status_type::UNKNOWN, "Entry point not found in linked set"};
        const uint32_t entry_point_idx = linked_set.entry_point_indexes.at(entry_point);

        const int target_index = get_target_index_for_api(api);
        if (target_index == -1) return {status_type::UNSUPPORTED, "API selected is not currently supported for slang shaders"};

        {
            Slang::ComPtr<slang::IBlob> shader_blob;
            Slang::ComPtr<slang::IBlob> diagnostics;

            linked_shader->getEntryPointCode(entry_point_idx, target_index, shader_blob.writeRef(), diagnostics.writeRef());

            if (diagnostics)
            {
                std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
                return {status_type::BACKEND_ERROR, std::format("Slang shader data for '{1}' failed with error: '{0}'", msg, linked_set_name)};
            }

            if (!shader_blob)
            {
                return {status_type::BACKEND_ERROR, std::format("Slang shader data for '{0}' failed with unknown error", linked_set_name)};
            }

            result->data_size = shader_blob->getBufferSize();
            result->data = malloc(result->data_size);
            memcpy(result->data, shader_blob->getBufferPointer(), result->data_size);
        }

        {
            Slang::ComPtr<slang::IBlob> diagnostics;
            slang::ShaderReflection* layout = linked_shader->getLayout(target_index, diagnostics.writeRef());

            if (diagnostics)
            {
                std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
                return {status_type::BACKEND_ERROR, std::format("Slang shader layout for '{1}' failed with error: '{0}'", msg, linked_set_name)};
            }

            result->internal_ptr = layout;
        }

        *out_shader_program = result;
        return status_type::SUCCESS;
    }

    status delete_shader_program(shader_program** shader_program)
    {
        if (shader_program == nullptr || *shader_program == nullptr) return status_type::UNEXPECTED;
        free((*shader_program)->data);
        delete *shader_program;
        *shader_program = nullptr;
        return status_type::SUCCESS;
    }

    struct struct_field_location
    {
        uint64_t offset;
        uint64_t size;
    };

    std::vector<struct_field_location> flatten_structure(slang::TypeLayoutReflection* structure)
    {
        struct stack_frame
        {
            slang::TypeLayoutReflection* type;
            uint64_t parent_offset;
            uint64_t current_field_index;
        };

        std::vector<struct_field_location> results;
        std::stack<stack_frame> layout_stack;
        uint64_t current_offset = 0;

        layout_stack.push({structure, 0, 0});

        while (!layout_stack.empty())
        {
            stack_frame& current = layout_stack.top();

            if (current.current_field_index >= current.type->getFieldCount())
            {
                layout_stack.pop();
                continue;
            }

            slang::VariableLayoutReflection* field = current.type->getFieldByIndex(current.current_field_index);
            slang::TypeLayoutReflection* field_type = field->getTypeLayout();

            current.current_field_index++;

            if (field_type->getKind() == slang::TypeReflection::Kind::Struct)
            {
                layout_stack.push({field_type, current_offset, 0});
                continue;
            }

            if (field_type->getKind() == slang::TypeReflection::Kind::Array)
            {
                for (int32_t idx = field_type->getElementCount() - 1; idx >= 0; idx--)
                {
                    const uint64_t element_offset = idx * field_type->getElementStride(SLANG_PARAMETER_CATEGORY_UNIFORM);
                    layout_stack.push({field_type->getElementTypeLayout(), current_offset + element_offset, 0});
                }

                continue;
            }

            const uint64_t size = field_type->getSize();
            const uint64_t stride = field_type->getStride();
            const uint64_t offset = field->getOffset();

            if (size == 0) continue;

            results.push_back({offset + current.parent_offset, size});
            current_offset = offset + stride + current.parent_offset;
        }

        return results;
    }

    shader_parameter_location shader_parameter_location::index(const uint32_t index) const
    {
        slang::TypeLayoutReflection* type_layout = slang_type_reflection(*this);
        slang::TypeLayoutReflection* element_layout = type_layout->getElementTypeLayout();
        if (element_layout == nullptr) return invalid_shader_paramter_location;

        shader_parameter_location result = shader_parameter_location(*this);
        result.offset_ptr = element_layout;
        result.byte_address += index * element_layout->getStride();

        result.binding_range_index *= type_layout->getElementCount();
        result.binding_range_index += index;

        return result;
    }

    bool is_single_element_container_kind(const slang::TypeReflection::Kind kind)
    {
        switch (kind)
        {
            case slang::TypeReflection::Kind::ConstantBuffer:
            case slang::TypeReflection::Kind::TextureBuffer:
            case slang::TypeReflection::Kind::ShaderStorageBuffer:
            case slang::TypeReflection::Kind::ParameterBlock:
            {
                return true;
            }

            default:
            {
                return false;
            }
        }
    }

    shader_parameter_location shader_parameter_location::field(const std::string_view& name) const
    {
        slang::TypeLayoutReflection* type_layout = slang_type_reflection(*this);

        if (is_single_element_container_kind(type_layout->getKind())) type_layout = type_layout->getElementTypeLayout();

        const int32_t index = type_layout->findFieldIndexByName(name.data(), name.data() + name.size());
        if (index < 0) return invalid_shader_paramter_location;

        slang::VariableLayoutReflection* field = type_layout->getFieldByIndex(index);

        shader_parameter_location result = shader_parameter_location(*this);
        result.offset_ptr = field->getTypeLayout();
        result.byte_address += field->getOffset();
        result.binding_range += type_layout->getFieldBindingRangeOffset(index);

        return result;
    }

    shader_parameter_location shader_program::locate(const std::string_view& name) const
    {
        shader_parameter_location result;

        slang::VariableLayoutReflection* globals_as_var = slang_shader_reflection(this)->getGlobalParamsVarLayout();
        slang::TypeLayoutReflection* globals = globals_as_var->getTypeLayout();
        const int64_t field_idx = globals->findFieldIndexByName(name.data(), name.data() + name.size());
        if (field_idx < 0) return invalid_shader_paramter_location;

        slang::VariableLayoutReflection* root_param = globals->getFieldByIndex(field_idx);
        if (root_param == nullptr) return invalid_shader_paramter_location;

        result.root_ptr = root_param;
        result.offset_ptr = root_param->getTypeLayout();
        result.root_idx = field_idx;
        result.byte_address = 0;
        result.binding_range = 0;
        result.binding_range_index = 0;

        return result;
    }

    int64_t shader_program::buffer_size(const std::string_view& name) const
    {
        slang::TypeLayoutReflection* globals = slang_shader_reflection(this)->getGlobalParamsTypeLayout();
        const int64_t global_idx = globals->findFieldIndexByName(name.data(), name.data() + name.size());
        if (global_idx < 0) return -1;

        slang::VariableLayoutReflection* root_param = globals->getFieldByIndex(global_idx);
        if (root_param == nullptr) return -1;
        const size_t size = root_param->getTypeLayout()->getElementTypeLayout()->getSize();
        return size == ~static_cast<size_t>(0) ? -1 : size; //Slang encodes unsized types as max value, we convert that to -1.
    }

    status create_shader_buffer_layout(const shader_program* program, const std::string_view& buffer_name, shader_buffer_layout** out_buffer_layout)
    {
        if (program == nullptr || out_buffer_layout == nullptr) return status_type::UNEXPECTED;

        slang::ShaderReflection* shader_layout = slang_shader_reflection(program);
        shader_buffer_layout* result = new shader_buffer_layout();

        slang::TypeLayoutReflection* globals = shader_layout->getGlobalParamsTypeLayout();
        const int64_t global_idx = globals->findFieldIndexByName(buffer_name.data(), buffer_name.data() + buffer_name.size());
        if (global_idx < 0) return {status_type::UNKNOWN, std::format("Couldn't find buffer by name {0}", buffer_name)};
        slang::VariableLayoutReflection* root_param = globals->getFieldByIndex(global_idx);
        if (root_param == nullptr) return {status_type::UNKNOWN, std::format("Couldn't find buffer by name {0}", buffer_name)};

        slang::TypeLayoutReflection* base_layout = root_param->getTypeLayout()->getElementTypeLayout();
        result->padded_size = base_layout->getStride();

        uint64_t current_offset = 0;
        uint64_t packed_size = 0;

        const std::vector<struct_field_location> fields = flatten_structure(base_layout);

        for (const struct_field_location& field : fields)
        {
            if (field.offset > current_offset)
            {
                result->pads.push_back({current_offset, field.offset - current_offset});
                current_offset = field.offset;
            }

            packed_size += field.size;
            current_offset += field.size;
        }

        //Padding at the end of the structure?
        if (current_offset < result->padded_size)
        {
            result->pads.push_back({current_offset, result->padded_size - current_offset});
        }

        result->packed_size = packed_size;

        *out_buffer_layout = result;

        return status_type::SUCCESS;
    }

    slang::TypeLayoutReflection* slang_type_reflection(const shader_parameter_location& location)
    {
        return static_cast<slang::TypeLayoutReflection*>(location.offset_ptr);
    }

    slang::VariableLayoutReflection* slang_root_var_reflection(const shader_parameter_location& location)
    {
        return static_cast<slang::VariableLayoutReflection*>(location.root_ptr);
    }

    slang::ShaderReflection* slang_shader_reflection(const shader_program* program)
    {
        return static_cast<slang::ShaderReflection*>(program->internal_ptr);
    }

    bool does_slang_type_consume_bindings(slang::TypeLayoutReflection* type)
    {
        switch (type->getKind())
        {
            case slang::TypeReflection::Kind::Array:
            {
                return does_slang_type_consume_bindings(type->getElementTypeLayout());
            }

            case slang::TypeReflection::Kind::None:
            case slang::TypeReflection::Kind::Struct:
            case slang::TypeReflection::Kind::Matrix:
            case slang::TypeReflection::Kind::Vector:
            case slang::TypeReflection::Kind::Scalar:
            {
                return false;
            }

            default:
            {
                return true;
            }
        }
    }

    binding_location_info vk_binding_for_location(const shader_parameter_location& location)
    {
        slang::VariableLayoutReflection* root_var = slang_root_var_reflection(location);
        slang::TypeLayoutReflection* root_layout = root_var->getTypeLayout();
        slang::TypeLayoutReflection* selected_layout = slang_type_reflection(location);

        const bool inside_parameter_block = root_layout->getKind() == slang::TypeReflection::Kind::ParameterBlock;
        const bool is_parameter_block = root_var->getTypeLayout() == selected_layout && inside_parameter_block;
        const bool does_consume_bindings = does_slang_type_consume_bindings(selected_layout); //Does this variable have it's own binding?

        if (!inside_parameter_block && does_consume_bindings)
        {
            //Stuff that's not inside a parameter block doesn't have binding range
            //NOTE: Is there any case where the selected var could have it's own bindings separate to the root?
            //This won't work for that case if it exists
            const SlangInt set = root_var->getBindingSpace(slang::ParameterCategory::DescriptorTableSlot);
            const SlangInt slot = root_var->getOffset(slang::DescriptorTableSlot);
            return {set, slot, root_layout};
        }

        if (!does_consume_bindings || is_parameter_block)
        {
            if (inside_parameter_block)
            {
                //Parameter blocks can have an explicit attribute for the set, which is exposed as 'sub element register space'
                //They cannot have an explicit attribute for the binding slot, since they can contain multiple opaque types.
                //The binding slot for plain data within a parameter block is always 0 (automatically introduced constant buffer)
                const SlangInt set_offset = root_var->getOffset(slang::ParameterCategory::SubElementRegisterSpace);
                return {set_offset, 0, root_layout};
            }

            //Plain data outside of a parameter block (probably within a constant/structured/etc buffer) is part of the root binding
            const SlangInt set_offset = root_var->getBindingSpace(slang::ParameterCategory::DescriptorTableSlot);
            const SlangInt slot_offset = root_var->getOffset(slang::DescriptorTableSlot);

            return {set_offset, slot_offset, root_layout};
        }

        //Inside a parameter block and DOES have its own binding - get binding data by binding range.

        slang::TypeLayoutReflection* root_element_layout = root_layout->getElementTypeLayout();

        //Parameter blocks can have an explicit attribute for the set, which is exposed as 'sub element register space'
        //They cannot have an explicit attribute for the binding slot, since they can contain multiple opaque types.
        const SlangInt set_offset = root_var->getOffset(slang::ParameterCategory::SubElementRegisterSpace);

        //Slang binding range -> Slang descriptor set indexes
        const SlangInt slang_binding_set = root_element_layout->getBindingRangeDescriptorSetIndex(location.binding_range);
        const SlangInt slang_binding_slot = root_element_layout->getBindingRangeFirstDescriptorRangeIndex(location.binding_range);

        //Slang descriptor set indexes -> actual VK descriptor set / slot.
        const SlangInt set = root_element_layout->getDescriptorSetSpaceOffset(slang_binding_set) + set_offset;
        const SlangInt slot = root_element_layout->getDescriptorSetDescriptorRangeIndexOffset(slang_binding_set, slang_binding_slot);

        return {set, slot, selected_layout};
    }
}
