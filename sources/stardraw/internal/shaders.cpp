#include "../api/shaders.hpp"
#include "internal_types.hpp"

#include <array>
#include <format>
#include <queue>
#include <slang-com-ptr.h>
#include <slang.h>
#include <stack>

namespace stardraw
{
    static slang::IGlobalSession* global_slang_context;
    static slang::ISession* active_slang_session;
    static std::unordered_map<std::string, Slang::ComPtr<slang::IModule>> loaded_modules;
    static std::unordered_map<std::string, Slang::ComPtr<slang::IComponentType>> linked_shaders;

    status delete_shader_buffer_layout(shader_buffer_layout** buffer_layout)
    {
        if (buffer_layout == nullptr) return status_type::UNEXPECTED_NULL;
        delete *buffer_layout;
        return status_type::SUCCESS;
    }

    void* layout_shader_buffer_memory(const shader_buffer_layout* layout, const void* data, const uint64_t data_size) {

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
                memcpy(output+ base_write_address + current_write_offset, in_bytes + base_read_address + current_read_offset, size_to_pad_start);
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
                .format = SLANG_GLSL,
                .profile = global_slang_context->findProfile("glsl_450"),
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
            std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
            return {status_type::BACKEND_ERROR, std::format("Slang module '{1}' loading failed with error: '{0}'", msg, module_name)};
        }

        loaded_modules[std::string(module_name)] = module;

        return status_type::SUCCESS;
    }

    status cache_shader_module(const std::string& module_name, void** out_cache_ptr, uint64_t& out_cache_size)
    {
        if (!loaded_modules.contains(module_name)) return {status_type::UNKNOWN_NAME, std::format("No loaded slang module called '{0}' found.", module_name)};
        const Slang::ComPtr<slang::IModule> module = loaded_modules[module_name];

        ISlangBlob* serialized_blob;
        const SlangResult serialize_result = module->serialize(&serialized_blob);
        if (SLANG_FAILED(serialize_result)) return {status_type::BACKEND_ERROR, std::format("Failed to serialize module '{0}'", module_name)};

        *out_cache_ptr = malloc(serialized_blob->getBufferSize());
        memcpy(*out_cache_ptr, serialized_blob->getBufferPointer(), serialized_blob->getBufferSize());

        delete serialized_blob;

        return status_type::SUCCESS;
    }

    status link_shader(const std::string& shader_name, const std::string& entry_point_module, const std::string& entry_point_name, const std::vector<std::string>& additional_modules)
    {
        std::vector<slang::IComponentType*> shader_components;

        if (!loaded_modules.contains(entry_point_module)) return {status_type::UNKNOWN_NAME, std::format("No loaded slang module called '{0}' found.", entry_point_module)};
        const Slang::ComPtr<slang::IModule> module = loaded_modules[entry_point_module];

        Slang::ComPtr<slang::IEntryPoint> slang_entry_point;

        const SlangResult found_entry_point = module->findEntryPointByName(entry_point_name.c_str(), slang_entry_point.writeRef());
        if (SLANG_FAILED(found_entry_point)) return {status_type::BACKEND_ERROR, std::format("Couldn't find entry point named '{0}' in module '{1}'", entry_point_name, entry_point_module)};

        shader_components.push_back(module);
        shader_components.push_back(slang_entry_point);

        for (const std::string& module_name : additional_modules)
        {
            if (!loaded_modules.contains(module_name)) return {status_type::UNKNOWN_NAME, std::format("No loaded slang module called '{0}' found.", module_name)};
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
                return {status_type::BACKEND_ERROR, std::format("Slang shader linking for '{1}' failed with error: '{0}'", msg, shader_name)};
            }
        }

        Slang::ComPtr<slang::IComponentType> linked_program;

        {
            Slang::ComPtr<slang::IBlob> diagnostics;
            composite->link(linked_program.writeRef(), diagnostics.writeRef());

            if (diagnostics)
            {
                std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
                return {status_type::BACKEND_ERROR, std::format("Slang shader linking for '{1}' failed with error: '{0}'", msg, shader_name)};
            }
        }

        linked_shaders[shader_name] = linked_program;

        return status_type::SUCCESS;
    }

    status create_shader_program(const std::string& shader_name, const graphics_api& api, shader_program** out_shader_program)
    {
        if (out_shader_program == nullptr) return status_type::UNEXPECTED_NULL;
        *out_shader_program = new shader_program();
        shader_program* result = *out_shader_program;

        if (!linked_shaders.contains(shader_name)) return {status_type::UNKNOWN_NAME, std::format("No linked slang shader called '{0}' exists.", shader_name)};
        const Slang::ComPtr<slang::IComponentType> linked_shader = linked_shaders[shader_name];

        const int target_index = get_target_index_for_api(api);
        if (target_index == -1) return {status_type::UNSUPPORTED, "API selected is not currently supported for slang shaders"};

        {
            Slang::ComPtr<slang::IBlob> shader_blob;
            Slang::ComPtr<slang::IBlob> diagnostics;

            linked_shader->getEntryPointCode(0, target_index, shader_blob.writeRef(), diagnostics.writeRef());

            if (diagnostics)
            {
                std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
                return {status_type::BACKEND_ERROR, std::format("Slang shader data for '{1}' failed with error: '{0}'", msg, shader_name)};
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
                return {status_type::BACKEND_ERROR, std::format("Slang shader layout for '{1}' failed with error: '{0}'", msg, shader_name)};
            }

            result->internal_ptr = layout;
        }

        *out_shader_program = result;
        return status_type::SUCCESS;
    }

    status delete_shader_program(shader_program** shader_program)
    {
        if (shader_program == nullptr || *shader_program == nullptr) return status_type::UNEXPECTED_NULL;
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
        slang::TypeLayoutReflection* type_layout = type_layout_of(*this);
        slang::TypeLayoutReflection* element_layout = type_layout->getElementTypeLayout();
        if (element_layout == nullptr) return invalid_shader_paramter_location;

        shader_parameter_location result = shader_parameter_location(*this);
        result.internal_ptr = element_layout;
        result.byte_address += index * element_layout->getStride();

        result.binding_slot_index *= type_layout->getElementCount();
        result.binding_slot_index += index;

        return result;
    }

    shader_parameter_location shader_parameter_location::field(const std::string_view& name) const
    {
        slang::TypeLayoutReflection* type_layout = type_layout_of(*this);
        const int32_t index = type_layout->findFieldIndexByName(name.data(), name.data() + name.size());
        if (index < 0) return invalid_shader_paramter_location;

        slang::VariableLayoutReflection* field = type_layout->getFieldByIndex(index);

        shader_parameter_location result = shader_parameter_location(*this);
        result.internal_ptr = field->getTypeLayout();
        result.byte_address += field->getOffset();
        result.binding_slot += type_layout->getFieldBindingRangeOffset(index);

        return result;
    }

    shader_parameter_location shader_program::locate(const std::string_view& name) const
    {
        shader_parameter_location result;

        slang::TypeLayoutReflection* globals = shader_reflection_of(this)->getGlobalParamsTypeLayout();
        const int64_t global_idx = globals->findFieldIndexByName(name.data(), name.data() + name.size());
        if (global_idx <= 0) return invalid_shader_paramter_location;

        slang::VariableLayoutReflection* root_param = globals->getFieldByIndex(global_idx);
        if (root_param == nullptr) return invalid_shader_paramter_location;

        result.root_param = name;
        result.internal_ptr = root_param->getTypeLayout()->getElementTypeLayout();
        result.byte_address = 0;
        result.binding_set = root_param->getOffset(slang::ParameterCategory::RegisterSpace);
        result.binding_slot = globals->getFieldBindingRangeOffset(global_idx);
        result.binding_slot_index = 0;

        return result;
    }

    status create_shader_buffer_layout(const shader_program* program, const std::string_view& buffer_name, shader_buffer_layout** out_buffer_layout)
    {
        if (program == nullptr || out_buffer_layout == nullptr) return status_type::UNEXPECTED_NULL;

        slang::ShaderReflection* shader_layout = shader_reflection_of(program);
        shader_buffer_layout* result = new shader_buffer_layout();

        slang::TypeLayoutReflection* globals = shader_layout->getGlobalParamsTypeLayout();
        const uint64_t global_idx = globals->findFieldIndexByName(buffer_name.data(), buffer_name.data() + buffer_name.size());
        slang::VariableLayoutReflection* root_param = globals->getFieldByIndex(global_idx);
        if (root_param == nullptr) return { status_type::UNKNOWN_NAME, std::format("Couldn't find buffer by name {0}",buffer_name )};

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
}
