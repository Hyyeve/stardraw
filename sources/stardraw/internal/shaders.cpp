#include "internal.hpp"

#include <array>
#include <format>
#include <queue>
#include <slang-com-ptr.h>
#include <slang.h>
#include <spirv_glsl.hpp>
#include <stack>

namespace stardraw
{
    struct shader_module::shader_module_internal
    {
        Slang::ComPtr<slang::IModule> slang_module;
        std::string name;
    };
}

template <>
struct std::hash<stardraw::shader_entry_point>
{
    std::size_t operator()(const stardraw::shader_entry_point& key) const noexcept
    {
        return hash<string>()(key.entry_point_name + key.module.internal->name);
    }
};

namespace stardraw
{
    using namespace starlib_stdint;
    using namespace starlib;

    struct shader_program::shader_program_internal
    {
        Slang::ComPtr<slang::IComponentType> linked_components;
        std::unordered_map<shader_entry_point, u32> entry_point_indexes;
    };

    static slang::IGlobalSession* global_slang_context;
    static slang::ISession* active_slang_session;
    static std::vector<Slang::ComPtr<slang::IComponentType>> linked_programs;


    int get_target_index_for_api(const graphics_api& api)
    {
        switch (api)
        {
            case graphics_api::GL45: return 0;
            case graphics_api::VK13: break;
            case graphics_api::DX11: break;
            case graphics_api::DX12: break;
            case graphics_api::METAL: break;
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

    starlib::status cleanup_shader_compiler()
    {
        linked_programs.clear();
        active_slang_session->release();
        global_slang_context->release();
        delete active_slang_session;
        delete global_slang_context;
        return status_type::SUCCESS;
    }

    status load_shader_module(const std::string_view& source, shader_module& out_shader_module)
    {
        constexpr std::hash<std::string_view> hash;
        const std::string fake_path = std::format("module_{0}.fakepath", hash(source));
        Slang::ComPtr<slang::IBlob> diagnostics;
        const Slang::ComPtr module(active_slang_session->loadModuleFromSourceString(fake_path.c_str(), fake_path.c_str(), source.data(), diagnostics.writeRef()));

        if (diagnostics)
        {
            std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
            return {status_type::BACKEND_ERROR, std::format("Slang module loading failed with error: '{0}'", msg)};
        }

        if (!module)
        {
            std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
            return {status_type::BACKEND_ERROR, std::format("Slang module loading failed with error: '{0}'", msg)};
        }

        out_shader_module = shader_module(std::make_unique<shader_module::shader_module_internal>(module, fake_path));

        return status_type::SUCCESS;
    }

    status load_shader_module(const void* cache_ptr, const u64 cache_size, shader_module& out_shader_module)
    {
        const std::string fake_path = std::format("module_{0}.fakepath", std::hash<const void*>()(cache_ptr));
        Slang::ComPtr<slang::IBlob> diagnostics;

        const Slang::ComPtr module(active_slang_session->loadModuleFromIRBlob(fake_path.c_str(), fake_path.c_str(), slang_createBlob(cache_ptr, cache_size), diagnostics.writeRef()));

        if (diagnostics)
        {
            std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
            return {status_type::BACKEND_ERROR, std::format("Slang module loading failed with error: '{0}'", msg)};
        }

        if (!module)
        {
            return {status_type::BACKEND_ERROR, std::format("Slang module loading failed with unknwon error")};
        }

        out_shader_module = shader_module(std::make_unique<shader_module::shader_module_internal>(module, fake_path));

        return status_type::SUCCESS;
    }

    status cache_shader_module(const shader_module& module, void*& out_cache_ptr, u64& out_cache_size)
    {
        const Slang::ComPtr<slang::IModule> inner_module = module.internal->slang_module;

        Slang::ComPtr<ISlangBlob> serialized_blob;
        const SlangResult serialize_result = inner_module->serialize(serialized_blob.writeRef());
        if (SLANG_FAILED(serialize_result)) return {status_type::BACKEND_ERROR, "Failed to serialize module"};

        const u64 cache_size = serialized_blob->getBufferSize();

        out_cache_ptr = malloc(cache_size);
        memcpy(out_cache_ptr, serialized_blob->getBufferPointer(), cache_size);
        out_cache_size = cache_size;

        return status_type::SUCCESS;
    }


    status link_shader_program(const std::vector<shader_entry_point>& entry_points, shader_program& out_linked_shader, const std::vector<shader_module>& additional_modules)
    {
        std::vector<slang::IComponentType*> shader_components;
        std::unordered_map<shader_entry_point, u32> entry_point_index_map;

        for (u32 idx = 0; idx < entry_points.size(); idx++)
        {
            const shader_entry_point& entry_point = entry_points[idx];

            const Slang::ComPtr<slang::IModule> inner_module = entry_point.module.internal->slang_module;

            Slang::ComPtr<slang::IEntryPoint> slang_entry_point;

            const SlangResult found_entry_point = inner_module->findEntryPointByName(entry_point.entry_point_name.c_str(), slang_entry_point.writeRef());
            if (SLANG_FAILED(found_entry_point)) return {status_type::BACKEND_ERROR, std::format("Couldn't find entry point named '{0}' in module", entry_point.entry_point_name)};

            shader_components.push_back(slang_entry_point);
            entry_point_index_map[entry_point] = idx;
        }

        for (const shader_module& additional_module : additional_modules)
        {
            shader_components.push_back(additional_module.internal->slang_module);
        }

        Slang::ComPtr<slang::IComponentType> composite;

        {
            Slang::ComPtr<slang::IBlob> diagnostics;
            active_slang_session->createCompositeComponentType(shader_components.data(), shader_components.size(), composite.writeRef(), diagnostics.writeRef());

            if (diagnostics)
            {
                std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
                return {status_type::BACKEND_ERROR, std::format("Slang shader linking failed with error: '{0}'", msg)};
            }
        }

        Slang::ComPtr<slang::IComponentType> linked_program;

        {
            Slang::ComPtr<slang::IBlob> diagnostics;
            composite->link(linked_program.writeRef(), diagnostics.writeRef());

            if (diagnostics)
            {
                std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
                return {status_type::BACKEND_ERROR, std::format("Slang shader linking failed with error: '{0}'", msg)};
            }
        }

        linked_programs.push_back(linked_program); //We need to hang on to the references so the lifetimes of pointers obtained via the linked program are kept alive until the shader compiler is cleaned up.
        out_linked_shader = shader_program(std::make_unique<shader_program::shader_program_internal>(linked_program, std::move(entry_point_index_map)));
        return status_type::SUCCESS;
    }

    shader_stage_type to_shader_stage_type(const SlangStage slang_stage)
    {
        switch (slang_stage)
        {
            case SLANG_STAGE_VERTEX: return shader_stage_type::VERTEX;
            case SLANG_STAGE_GEOMETRY: return shader_stage_type::GEOMETRY;
            case SLANG_STAGE_FRAGMENT: return shader_stage_type::FRAGMENT;
            case SLANG_STAGE_COMPUTE: return shader_stage_type::COMPUTE;
            default: return shader_stage_type::UNKNOWN;
        }
    }

    status create_shader_stage(const shader_program& linked_shader, const shader_entry_point& entry_point, const graphics_api& api, shader_stage& out_shader_stage)
    {
        shader_stage result = shader_stage(std::make_shared<shader_stage::shader_stage_internal>());

        const Slang::ComPtr<slang::IComponentType> linked_shader_component = linked_shader.internal->linked_components;

        if (!linked_shader.internal->entry_point_indexes.contains(entry_point)) return {status_type::UNKNOWN, "Entry point not found in linked shader"};
        const u32 entry_point_idx = linked_shader.internal->entry_point_indexes.at(entry_point);

        const int target_index = get_target_index_for_api(api);
        if (target_index == -1) return {status_type::UNSUPPORTED, "API selected is not currently supported for slang shaders"};

        {
            Slang::ComPtr<slang::IBlob> diagnostics;
            slang::ShaderReflection* layout = linked_shader_component->getLayout(target_index, diagnostics.writeRef());

            if (diagnostics)
            {
                std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
                return {status_type::BACKEND_ERROR, std::format("Slang shader layout reflection failed with error: '{0}'", msg)};
            }

            result.internal->reflection = layout;
            result.internal->type = to_shader_stage_type(layout->getEntryPointByIndex(entry_point_idx)->getStage());
        }

        {
            Slang::ComPtr<slang::IBlob> shader_blob;
            Slang::ComPtr<slang::IBlob> diagnostics;

            linked_shader_component->getEntryPointCode(entry_point_idx, target_index, shader_blob.writeRef(), diagnostics.writeRef());

            if (diagnostics)
            {
                std::string msg = std::string(static_cast<const char*>(diagnostics->getBufferPointer()));
                return {status_type::BACKEND_ERROR, std::format("Slang shader data generation failed with error: '{0}'", msg)};
            }

            if (!shader_blob)
            {
                return {status_type::BACKEND_ERROR, std::format("Slang shader data generation failed with unknown error")};
            }

            result.internal->api = api;
            result.internal->data_size = shader_blob->getBufferSize();
            result.internal->data = malloc(result.internal->data_size);
            memcpy(result.internal->data, shader_blob->getBufferPointer(), result.internal->data_size);
        }

        out_shader_stage = std::move(result);

        return status_type::SUCCESS;
    }

    struct struct_field_location
    {
        u64 offset;
        u64 size;
    };

    std::vector<struct_field_location> flatten_structure(slang::TypeLayoutReflection* structure)
    {
        struct stack_frame
        {
            slang::TypeLayoutReflection* type;
            u64 parent_offset;
            u64 current_field_index;
        };

        std::vector<struct_field_location> results;
        std::stack<stack_frame> layout_stack;
        u64 current_offset = 0;

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
                for (i32 idx = field_type->getElementCount() - 1; idx >= 0; idx--)
                {
                    const u64 element_offset = idx * field_type->getElementStride(SLANG_PARAMETER_CATEGORY_UNIFORM);
                    layout_stack.push({field_type->getElementTypeLayout(), current_offset + element_offset, 0});
                }

                continue;
            }

            const u64 size = field_type->getSize();
            const u64 stride = field_type->getStride();
            const u64 offset = field->getOffset();

            if (size == 0) continue;

            results.push_back({offset + current.parent_offset, size});
            current_offset = offset + stride + current.parent_offset;
        }

        return results;
    }

    shader_module::~shader_module() = default;
    shader_program::~shader_program() = default;

    shader_parameter_location shader_parameter_location::index(const u32 index) const
    {
        slang::TypeLayoutReflection* type_layout = internal->offset_ptr;
        slang::TypeLayoutReflection* element_layout = type_layout->getElementTypeLayout();
        if (element_layout == nullptr)
        {
            shader_parameter_location result = shader_parameter_location(*this);
            result.internal->is_valid = false;
            result.internal->path_string += std::format("[{0}]", index);
            return result;
        }

        shader_parameter_location result = shader_parameter_location(*this);
        result.internal->offset_ptr = element_layout;
        result.internal->byte_address += index * element_layout->getStride();
        result.internal->binding_range_index *= type_layout->getElementCount();
        result.internal->binding_range_index += index;
        result.internal->path_string += std::format("[{0}]", index);
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
        slang::TypeLayoutReflection* type_layout = internal->offset_ptr;

        if (is_single_element_container_kind(type_layout->getKind())) type_layout = type_layout->getElementTypeLayout();

        const i32 index = type_layout->findFieldIndexByName(name.data(), name.data() + name.size());
        if (index < 0) {
            shader_parameter_location result = shader_parameter_location(*this);
            result.internal->is_valid = false;
            result.internal->path_string += std::format(".{0}", name);
            return result;
        }

        slang::VariableLayoutReflection* field = type_layout->getFieldByIndex(index);

        shader_parameter_location result = shader_parameter_location(*this);
        result.internal->offset_ptr = field->getTypeLayout();
        result.internal->byte_address += field->getOffset();
        result.internal->binding_range += type_layout->getFieldBindingRangeOffset(index);
        result.internal->path_string += std::format(".{0}", name);
        return result;
    }

    shader_parameter_location::shader_parameter_location(const shader_parameter_location& other) : internal(std::make_unique<shader_parameter_location_internal>(*other.internal)) {}

    shader_parameter_location& shader_parameter_location::operator=(const shader_parameter_location& other)
    {
        if (this == &other) return *this;
        internal = std::make_unique<shader_parameter_location_internal>(*other.internal);
        return *this;
    }

    shader_parameter_location::shader_parameter_location(shader_parameter_location&& other) noexcept : internal(std::move(other.internal)) {}

    shader_parameter_location& shader_parameter_location::operator=(shader_parameter_location&& other) noexcept
    {
        if (this == &other) return *this;
        internal = std::move(other.internal);
        return *this;
    }

    bool shader_parameter_location::operator==(const shader_parameter_location& other) const
    {
        return *internal == *other.internal;
    }

    shader_parameter_location::shader_parameter_location(std::unique_ptr<shader_parameter_location_internal>&& internal) : internal(std::move(internal)) {}
    shader_parameter_location::~shader_parameter_location() = default;

    shader_parameter_location shader_stage::locate(const std::string_view& name) const
    {
        slang::VariableLayoutReflection* globals_as_var = internal->reflection->getGlobalParamsVarLayout();
        slang::TypeLayoutReflection* globals = globals_as_var->getTypeLayout();
        const i64 field_idx = globals->findFieldIndexByName(name.data(), name.data() + name.size());
        slang::VariableLayoutReflection* root_param = field_idx < 0 ? nullptr : globals->getFieldByIndex(field_idx);
        if (root_param == nullptr)
        {
            shader_parameter_location result {
                std::make_unique<shader_parameter_location::shader_parameter_location_internal>(
                    shader_parameter_location::shader_parameter_location_internal {
                        .path_string = std::string(name),
                        .is_valid = false,
                    }
                )
            };

            return result;
        }

        shader_parameter_location result {
            std::make_unique<shader_parameter_location::shader_parameter_location_internal>(
                shader_parameter_location::shader_parameter_location_internal {
                    .root_ptr = root_param,
                    .offset_ptr = root_param->getTypeLayout(),
                    .root_idx = static_cast<u64>(field_idx),
                    .byte_address = 0,
                    .binding_range = 0,
                    .binding_range_index = 0,
                    .path_string = std::string(name),
                }
            )
        };

        return result;
    }

    i64 shader_stage::buffer_size(const std::string_view& name) const
    {
        slang::TypeLayoutReflection* globals = internal->reflection->getGlobalParamsTypeLayout();
        const i64 global_idx = globals->findFieldIndexByName(name.data(), name.data() + name.size());
        if (global_idx < 0) return -1;

        slang::VariableLayoutReflection* root_param = globals->getFieldByIndex(global_idx);
        if (root_param == nullptr) return -1;
        const size_t size = root_param->getTypeLayout()->getElementTypeLayout()->getSize();
        return size == ~static_cast<size_t>(0) ? -1 : size; //Slang encodes unsized types as max value, we convert that to -1.
    }

    shader_stage_type shader_stage::get_stage_type() const
    {
        return internal->type;
    }

    shader_stage::~shader_stage() = default;

    status determine_shader_buffer_layout(const shader_stage& program, const std::string_view& buffer_name, memory_layout_info& out_buffer_layout)
    {
        slang::ShaderReflection* shader_layout = program.internal->reflection;

        slang::TypeLayoutReflection* globals = shader_layout->getGlobalParamsTypeLayout();
        const i64 global_idx = globals->findFieldIndexByName(buffer_name.data(), buffer_name.data() + buffer_name.size());
        if (global_idx < 0) return {status_type::UNKNOWN, std::format("Couldn't find buffer by name '{0}'", buffer_name)};
        slang::VariableLayoutReflection* root_param = globals->getFieldByIndex(global_idx);
        if (root_param == nullptr) return {status_type::UNKNOWN, std::format("Couldn't find buffer by name '{0}'", buffer_name)};

        slang::TypeLayoutReflection* base_layout = root_param->getTypeLayout()->getElementTypeLayout();
        out_buffer_layout.padded_size = base_layout->getStride();

        u64 current_offset = 0;
        u64 packed_size = 0;

        const std::vector<struct_field_location> fields = flatten_structure(base_layout);

        for (const struct_field_location& field : fields)
        {
            if (field.offset > current_offset)
            {
                out_buffer_layout.pads.push_back({current_offset, field.offset - current_offset});
                current_offset = field.offset;
            }

            packed_size += field.size;
            current_offset += field.size;
        }

        //Padding at the end of the structure?
        if (current_offset < out_buffer_layout.padded_size)
        {
            out_buffer_layout.pads.push_back({current_offset, out_buffer_layout.padded_size - current_offset});
        }

        out_buffer_layout.packed_size = packed_size;

        return status_type::SUCCESS;
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
        slang::VariableLayoutReflection* root_var = location.internal->root_ptr;
        slang::TypeLayoutReflection* root_layout = root_var->getTypeLayout();
        slang::TypeLayoutReflection* selected_layout = location.internal->offset_ptr;

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
        const SlangInt slang_binding_set = root_element_layout->getBindingRangeDescriptorSetIndex(location.internal->binding_range);
        const SlangInt slang_binding_slot = root_element_layout->getBindingRangeFirstDescriptorRangeIndex(location.internal->binding_range);

        //Slang descriptor set indexes -> actual VK descriptor set / slot.
        const SlangInt set = root_element_layout->getDescriptorSetSpaceOffset(slang_binding_set) + set_offset;
        const SlangInt slot = root_element_layout->getDescriptorSetDescriptorRangeIndexOffset(slang_binding_set, slang_binding_slot);

        return {set, slot, selected_layout};
    }
}
