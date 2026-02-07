#include <array>
#include <format>
#include <unordered_map>

#include <slang-com-ptr.h>
#include "slang.hpp"

#include <ranges>

namespace stardraw
{
    static slang::IGlobalSession* global_slang_context;
    static slang::ISession* active_slang_session;
    static std::unordered_map<std::string, Slang::ComPtr<slang::IModule>> loaded_modules;
    static std::unordered_map<std::string, Slang::ComPtr<slang::IComponentType>> linked_shaders;

    int get_target_index_for_api(const graphics_api& api)
    {
        switch (api)
        {
            case graphics_api::GL45: return 0;
        }

        return -1;
    }


    status init_slang_session()
    {
        const std::vector<shader_macro> shader_macros;
        const std::vector<std::string> search_paths;

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
        for (const shader_macro& macro : shader_macros)
        {
            compiler_options.push_back(slang::CompilerOptionEntry {
                slang::CompilerOptionName::MacroDefine,
                slang::CompilerOptionValue {
                    slang::CompilerOptionValueKind::String,
                    0, 0, macro.name.data(), macro.value.data()
                }
            });
        }

        const std::array supported_targets = {
            slang::TargetDesc {
                .format = SLANG_GLSL,
                .profile = global_slang_context->findProfile("glsl_450"),
            },
        };

        slang::SessionDesc session_desc;

        session_desc.targets = supported_targets.data();
        session_desc.targetCount = supported_targets.size();

        session_desc.compilerOptionEntries = compiler_options.data();
        session_desc.compilerOptionEntryCount = compiler_options.size();

        std::vector<const char*> search_path_ptrs;
        for (const std::string& path : search_paths)
        {
            search_path_ptrs.push_back(path.c_str());
        }

        session_desc.searchPaths = search_path_ptrs.data();
        session_desc.searchPathCount = search_path_ptrs.size();

        const SlangResult session_creation = global_slang_context->createSession(session_desc, &active_slang_session);
        if (SLANG_FAILED(session_creation)) return {status_type::BACKEND_ERROR, "Slang session creation failed"};

        return status_type::SUCCESS;
    }

    status load_slang_module(const std::string_view& name, const std::string_view& source)
    {
        const std::string fake_path = std::format("{0}.slang", name);
        Slang::ComPtr<slang::IBlob> diagnostics;
        const Slang::ComPtr module(active_slang_session->loadModuleFromSourceString(name.data(), fake_path.c_str(), source.data(), diagnostics.writeRef()));

        if (diagnostics)
        {
            std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
            return {status_type::BACKEND_ERROR, std::format("Slang module loading '{1}' failed with error: '{0}'", msg, name)};
        }

        if (!module)
        {
            std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
            return {status_type::BACKEND_ERROR, std::format("Slang module '{1}' loading failed with error: '{0}'", msg, name)};
        }

        loaded_modules[std::string(name)] = module;

        return status_type::SUCCESS;
    }

    status link_slang_shader(const std::string& shader_name, const std::string& entry_point_module, const std::string& entry_point_name, const std::vector<std::string>& additional_modules)
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

    status link_slang_shader(const std::string& shader_name, const std::string& entry_point_module, const std::string& entry_point_name)
    {
        return link_slang_shader(shader_name, entry_point_module, entry_point_name, std::vector<std::string>());
    }

    status get_shader_data(const std::string& shader_name, const graphics_api& api, shader_data& out_shader_data)
    {
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

            out_shader_data.data_size = shader_blob->getBufferSize();
            out_shader_data.data = malloc(out_shader_data.data_size);
            memcpy(out_shader_data.data, shader_blob->getBufferPointer(), out_shader_data.data_size);
        }

        {
            Slang::ComPtr<slang::IBlob> diagnostics;
            slang::ShaderReflection* layout = linked_shader->getLayout(target_index, diagnostics.writeRef());

            if (diagnostics)
            {
                std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
                return {status_type::BACKEND_ERROR, std::format("Slang shader layout for '{1}' failed with error: '{0}'", msg, shader_name)};
            }

            out_shader_data.reflection_data = layout;
        }

        return status_type::SUCCESS;
    }

    shader_param_location navigate_to_index(const shader_param_location& from, const uint32_t index)
    {
        slang::TypeLayoutReflection* element_layout = from.type_layout->getElementTypeLayout();
        if (element_layout == nullptr) return invalid_location;

        shader_param_location result = from;
        result.type_layout = element_layout;
        result.byte_address += index * element_layout->getStride();

        result.binding_index *= from.type_layout->getElementCount();
        result.binding_index += index;

        return result;
    }

    shader_param_location navigate(const shader_param_location& from, const char* name_beign, const char* name_end)
    {
        const int32_t index = from.type_layout->findFieldIndexByName(name_beign, name_end);
        if (index < 0) return invalid_location;

        slang::VariableLayoutReflection* field = from.type_layout->getFieldByIndex(index);

        shader_param_location result = from;
        result.type_layout = field->getTypeLayout();
        result.byte_address += field->getOffset();
        result.binding_range += from.type_layout->getFieldBindingRangeOffset(index);

        return result;
    }

    shader_param_location locate_shader_param(slang::ShaderReflection* shader_layout, const std::string_view& name, const uint32_t index)
    {
        auto name_parts = name | std::ranges::views::split('.') | std::ranges::views::drop(1);
        auto root_name = name.substr(0, name.find_first_of('.'));

        shader_param_location result;

        slang::TypeLayoutReflection* globals = shader_layout->getGlobalParamsTypeLayout();
        slang::VariableLayoutReflection* root_param = globals->getFieldByIndex(globals->findFieldIndexByName(root_name.data(), root_name.data() + root_name.size()));
        if (root_param == nullptr) return invalid_location;

        result.type_layout = root_param->getTypeLayout()->getElementTypeLayout();
        uint32_t descriptorSetIndex = root_param->getOffset(slang::ParameterCategory::RegisterSpace);
        uint32_t descriptorSetIndex1 = root_param->getOffset(slang::ParameterCategory::DescriptorTableSlot);
        uint32_t descriptorSetIndex2 = root_param->getOffset(slang::ParameterCategory::SubElementRegisterSpace);

        for (auto elem : name_parts) result = navigate(result, elem.data(), elem.data() + elem.size());
        if (index != -1u) result = navigate_to_index(result, index);

        return result;
    }
}
