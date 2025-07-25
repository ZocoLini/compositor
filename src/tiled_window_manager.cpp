#include "tiled_window_manager.h"
#include "mir/geometry/forward.h"
#include "mir_toolkit/common.h"
#include "miral/application_info.h"
#include "miral/minimal_window_manager.h"
#include "miral/window.h"
#include "miral/window_management_policy.h"
#include "miral/window_manager_tools.h"
#include "miral/window_specification.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <sys/stat.h>
#include <tuple>
#include <vector>

const int kMaxWindows = 4;
const int kSizeCombinations = 4 * 3 * 2 * 1; // kMaxWindows!

mir::geometry::Point position_for(int index, int width, int height)
{
    static std::tuple<float, float> positions[4] = {
        std::tuple<float, float>{0.0f, 0.0f},
        std::tuple<float, float>{0.5f, 0.0f},
        std::tuple<float, float>{0.5f, 0.5f},
        std::tuple<float, float>{0.0f, 0.5f},
    };

    std::tuple<float, float> position = positions[index];

    return mir::geometry::Point{std::get<0>(position) * width,
                                std::get<1>(position) * height};
}

mir::geometry::Size size_for(int index, int total, int width, int height)
{
    static std::tuple<float, float> sizes[kSizeCombinations] = {
        std::tuple<float, float>{1.0f, 1.0f}, // 1 window
        std::tuple<float, float>{0.5f, 1.0f}, // 2 windows
        std::tuple<float, float>{0.5f, 1.0f},
        std::tuple<float, float>{0.5f, 1.0f}, // 3 windows
        std::tuple<float, float>{0.5f, 0.5f},
        std::tuple<float, float>{0.5f, 0.5f},
        std::tuple<float, float>{0.5f, 0.5f}, // 4 windows
        std::tuple<float, float>{0.5f, 0.5f},
        std::tuple<float, float>{0.5f, 0.5f},
        std::tuple<float, float>{0.5f, 0.5f},
    };

    static int8_t start_index[kMaxWindows] = {0, 1, 3, 6};

    std::tuple<float, float> size = sizes[start_index[total - 1] + index];

    return mir::geometry::Size{std::get<0>(size) * width,
                               std::get<1>(size) * height};
}

TiledWindowManager::TiledWindowManager(const WindowManagerTools& tools)
    : MinimalWindowManager(tools)
{
    workspaces.push_back(this->tools.create_workspace());
}

TiledWindowManager::~TiledWindowManager()
{
    for (auto& workspace : workspaces)
    {
        workspace.reset();
    }
}

miral::WindowSpecification TiledWindowManager::place_new_window(
    miral::ApplicationInfo const& app_info,
    miral::WindowSpecification const& requested_specification)
{
    return requested_specification;
}

void TiledWindowManager::advise_new_window(WindowInfo const& window_info)
{
    auto current_workspace = this->workspaces.at(this->current_workspace_index);
    int windows_count = count_windows_in_workspace(current_workspace);

    if (windows_count >= kMaxWindows)
    {
        auto new_workspace = tools.create_workspace();
        tools.add_tree_to_workspace(window_info.window(), new_workspace);
        this->workspaces.push_back(new_workspace);
        this->current_workspace_index = workspaces.size() - 1;
        current_workspace = new_workspace;
    }

    tools.add_tree_to_workspace(window_info.window(), current_workspace);

    MinimalWindowManager::advise_new_window(window_info);
}

void TiledWindowManager::advise_adding_to_workspace(
    std::shared_ptr<Workspace> const& workspace,
    std::vector<Window> const& windows)
{
    std::cout << "Adding windows to workspace" << std::endl;
    std::cout << "Workspace ID: " << workspace.get() << std::endl;
    std::cout << "Windows count: " << windows.size() << std::endl;
    std::cout << "Windows count: " << count_windows_in_workspace(workspace)
              << std::endl;

    update_windows();
}

void TiledWindowManager::advise_removing_from_workspace(
    std::shared_ptr<Workspace> const& workspace,
    std::vector<Window> const& windows)
{
    std::cout << "Removing windows from workspace" << std::endl;
    std::cout << "Workspace ID: " << workspace.get() << std::endl;
    std::cout << "Windows count: " << windows.size() << std::endl;
    std::cout << "Windows count: " << count_windows_in_workspace(workspace)
              << std::endl;

    update_windows();
}

void TiledWindowManager::handle_request_resize(miral::WindowInfo& window_info,
                                               MirInputEvent const* input_event,
                                               MirResizeEdge edge)
{
}

void TiledWindowManager::handle_request_move(WindowInfo& window_info,
                                             MirInputEvent const* input_event)
{
}

void TiledWindowManager::handle_modify_window(
    WindowInfo& window_info, WindowSpecification const& modifications)
{
    if (modifications.state() == MirWindowState::mir_window_state_maximized)
    {
        return;
    }

    MinimalWindowManager::handle_modify_window(window_info, modifications);
}

void TiledWindowManager::advise_delete_window(WindowInfo const& window_info)
{
    miral::MinimalWindowManager::advise_delete_window(window_info);
}

void TiledWindowManager::update_windows()
{
    std::cout << "Updating windows..." << std::endl;

    int width = tools.active_output().size.width.as_value();
    int height = tools.active_output().size.height.as_value();

    int i = 0;
    for (; i < this->current_workspace_index; ++i)
    {
        auto workspace = workspaces.at(i);
        tools.for_each_window_in_workspace(
            workspace,
            [&](const Window& window)
            {
                miral::WindowSpecification spec;
                spec.state() = MirWindowState::mir_window_state_minimized;
                tools.modify_window(window, spec);
            });
    }

    std::cout << "Showing workspace " << this->current_workspace_index
              << std::endl;

    int windows_count = count_windows_in_workspace(
        workspaces.at(this->current_workspace_index));

    int window_index = 0;
    tools.for_each_window_in_workspace(
        workspaces.at(this->current_workspace_index),
        [&](const Window& window)
        {
            miral::Window _window = window;

            _window.resize(
                size_for(window_index, windows_count, width, height));
            _window.move_to(position_for(window_index, width, height));

            miral::WindowSpecification spec{};
            spec.state() = MirWindowState::mir_window_state_restored;
            tools.modify_window(window, spec);

            ++window_index;
        });

    ++i;
    for (; i < this->workspaces.size(); ++i)
    {
        auto workspace = workspaces.at(i);
        tools.for_each_window_in_workspace(
            workspace,
            [&](const Window& window)
            {
                miral::WindowSpecification spec;
                spec.state() = MirWindowState::mir_window_state_hidden;
                tools.modify_window(window, spec);
            });
    }
}

int TiledWindowManager::count_windows_in_workspace(
    std::shared_ptr<miral::Workspace> workspace)
{
    int count = 0;
    tools.for_each_window_in_workspace(workspace,
                                       [&](const Window& window) { ++count; });
    return count;
}
