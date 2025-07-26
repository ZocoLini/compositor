#include "mir/geometry/forward.h"
#include "mir_toolkit/common.h"
#include "miral/application_info.h"
#include "miral/minimal_window_manager.h"
#include "miral/window.h"
#include "miral/window_management_policy.h"
#include "miral/window_manager_tools.h"
#include "miral/window_specification.h"
#include <miral/toolkit_event.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <sys/stat.h>
#include <tuple>
#include <vector>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "tiled_window_manager.h"

using namespace miral::toolkit;

const int kMaxWindows = 4;
const int kSizeCombinations = 4 * 3 * 2 * 1; // kMaxWindows!

mir::geometry::Point position_for(int index, int width, int height)
{
    static std::tuple<float, float> positions[kMaxWindows] = {
        std::tuple<float, float>{0.0f, 0.0f},
        std::tuple<float, float>{0.5f, 0.0f},
        std::tuple<float, float>{0.5f, 0.5f},
        std::tuple<float, float>{0.0f, 0.5f},
    };

    std::tuple<float, float> position = positions[index];

    return mir::geometry::Point{std::get<0>(position) * width, std::get<1>(position) * height};
}

mir::geometry::Size size_for(int index, int total, int width, int height)
{
    static std::tuple<float, float> sizes[kSizeCombinations] = {
        std::tuple<float, float>{1.0f, 1.0f}, // 1 window  - Window 0

        std::tuple<float, float>{0.5f, 1.0f}, // 2 windows - Window 0
        std::tuple<float, float>{0.5f, 1.0f}, // 2 windows - Window 1

        std::tuple<float, float>{0.5f, 1.0f}, // 3 windows - Window 0
        std::tuple<float, float>{0.5f, 0.5f}, // 3 windows - Window 1
        std::tuple<float, float>{0.5f, 0.5f}, // 3 windows - Window 2

        std::tuple<float, float>{0.5f, 0.5f}, // 4 windows - Window 0
        std::tuple<float, float>{0.5f, 0.5f}, // 4 windows - Window 1
        std::tuple<float, float>{0.5f, 0.5f}, // 4 windows - Window 2
        std::tuple<float, float>{0.5f, 0.5f}, // 4 windows - Window 3
    };

    static int8_t start_index[kMaxWindows] = {0, 1, 3, 6};

    std::tuple<float, float> size = sizes[start_index[total - 1] + index];

    return mir::geometry::Size{std::get<0>(size) * width, std::get<1>(size) * height};
}

TiledWindowManager::TiledWindowManager(const miral::WindowManagerTools& tools)
    : miral::MinimalWindowManager(tools)
{
    workspaces.push_back(this->tools.create_workspace());
}

TiledWindowManager::~TiledWindowManager()
{
    for (auto workspace : this->workspaces)
    {
        tools.for_each_window_in_workspace(
            workspace, [&](const miral::Window& window) { tools.ask_client_to_close(window); });
    }

    this->workspaces.clear();
}

miral::WindowSpecification TiledWindowManager::place_new_window(
    miral::ApplicationInfo const& app_info,
    miral::WindowSpecification const& requested_specification)
{
    miral::WindowSpecification spec = requested_specification;

    return spec;
}

void TiledWindowManager::advise_new_window(miral::WindowInfo const& window_info)
{
    auto current_workspace = get_current_workspace();
    int windows_count = count_windows_in_workspace(current_workspace);

    if (windows_count >= kMaxWindows)
    {
        auto new_workspace = tools.create_workspace();
        this->workspaces.push_back(new_workspace);
        this->current_workspace_index = workspaces.size() - 1;
        current_workspace = new_workspace;
    }

    MinimalWindowManager::advise_new_window(window_info);

    tools.add_tree_to_workspace(window_info.window(), current_workspace);
}

void TiledWindowManager::advise_adding_to_workspace(
    std::shared_ptr<miral::Workspace> const& workspace, std::vector<miral::Window> const& windows)
{
    std::cout << "Adding windows to workspace" << std::endl;
    std::cout << "Added windows count: " << windows.size() << std::endl;
    std::cout << "Windows count: " << count_windows_in_workspace(workspace) << std::endl;

    update_windows({});
}

void TiledWindowManager::advise_removing_from_workspace(
    std::shared_ptr<miral::Workspace> const& workspace, std::vector<miral::Window> const& windows)
{
    std::cout << "Removing windows from workspace" << std::endl;
    std::cout << "Deleted windows count: " << windows.size() << std::endl;
    std::cout << "Windows count: " << count_windows_in_workspace(workspace) << std::endl;

    if (this->workspaces.size() != 1 &&
        (get_workspace_windows(workspace).size() - windows.size()) == 0)
    {
        std::erase(this->workspaces, workspace);

        if (this->current_workspace_index == this->workspaces.size())
        {
            --this->current_workspace_index;
            update_windows({});
            return;
        }
    }

    update_windows(windows);
}

void TiledWindowManager::handle_request_resize(
    miral::WindowInfo& window_info, MirInputEvent const* input_event, MirResizeEdge edge)
{
}

void TiledWindowManager::handle_request_move(
    miral::WindowInfo& window_info, MirInputEvent const* input_event)
{
}

void TiledWindowManager::handle_modify_window(
    miral::WindowInfo& window_info, miral::WindowSpecification const& modifications)
{
    if (modifications.state() == MirWindowState::mir_window_state_maximized ||
        modifications.state() == MirWindowState::mir_window_state_minimized ||
        modifications.state() == MirWindowState::mir_window_state_restored)
    {
        return;
    }

    MinimalWindowManager::handle_modify_window(window_info, modifications);
}

bool TiledWindowManager::handle_keyboard_event(MirKeyboardEvent const* event)
{
    if (mir_keyboard_event_action(event) != mir_keyboard_action_down) return false;

    MirInputEventModifiers mods = mir_keyboard_event_modifiers(event);
    if (!(mods & mir_input_event_modifier_alt) || !(mods & mir_input_event_modifier_ctrl))
        return false;

    auto current_workspace = get_current_workspace();
    auto active_window = tools.active_window();
    std::shared_ptr<miral::Workspace> new_workspace;

    switch (mir_keyboard_event_keysym(event))
    {
        case XKB_KEY_V:
        case XKB_KEY_v:
            tools.for_each_window_in_workspace(
                current_workspace,
                [&](const miral::Window& window) { tools.ask_client_to_close(window); });
            return true;
        case XKB_KEY_C:
        case XKB_KEY_c:
            tools.ask_client_to_close(active_window);
            return true;
        case XKB_KEY_Tab:
        {
            std::vector<miral::Window> windows = get_workspace_windows(current_workspace);

            int active_window_index = -1;
            for (size_t i = 0; i < windows.size(); ++i)
            {
                if (windows[i] == active_window)
                {
                    active_window_index = i;
                    break;
                }
            }

            ++active_window_index;

            if (active_window_index == windows.size())
            {
                active_window_index = 0;
            }

            miral::Window next_window = windows[active_window_index];

            tools.select_active_window(next_window);

            return true;
        }
        case XKB_KEY_m:
        case XKB_KEY_M:
            if (count_windows_in_workspace(current_workspace) == 1) return false;
            tools.remove_tree_from_workspace(active_window, current_workspace);

            new_workspace = tools.create_workspace();
            this->workspaces.push_back(new_workspace);
            this->current_workspace_index = this->workspaces.size() - 1;

            tools.add_tree_to_workspace(active_window, new_workspace);

            return true;
        case XKB_KEY_q:
        case XKB_KEY_e:
            this->current_workspace_index =
                (this->current_workspace_index + (mir_keyboard_event_keysym(event) == XKB_KEY_q
                                                      ? this->workspaces.size() - 1
                                                      : 1)) %
                this->workspaces.size();
            update_windows({});
            return true;
    }

    return false;
}

void TiledWindowManager::update_windows(std::vector<miral::Window> const& ignorable_windows)
{
    std::cout << "Updating windows..." << std::endl;

    int width = tools.active_output().size.width.as_value();
    int height = tools.active_output().size.height.as_value();

    int i = 0;
    for (; i < this->current_workspace_index; ++i)
    {
        auto workspace = workspaces.at(i);
        tools.for_each_window_in_workspace(
            workspace, [&](const miral::Window& window) { hide_window(window); });
    }

    std::cout << "Showing workspace " << this->current_workspace_index << std::endl;

    int windows_count =
        count_windows_in_workspace(get_current_workspace()) - ignorable_windows.size();

    int window_index = 0;

    std::vector<miral::Window> windows = get_workspace_windows(get_current_workspace());
    miral::Window* first_valid_window = NULL;

    for (auto window : windows)
    {
        auto it = std::find(ignorable_windows.begin(), ignorable_windows.end(), window);
        if (it != ignorable_windows.end()) continue;

        if (first_valid_window == NULL) first_valid_window = &window;

        show_window(window);

        window.resize(size_for(window_index, windows_count, width, height));
        window.move_to(position_for(window_index, width, height));

        ++window_index;
    }

    if (first_valid_window != NULL)
    {
        tools.select_active_window(*first_valid_window);
    }

    ++i;
    for (; i < this->workspaces.size(); ++i)
    {
        auto workspace = workspaces.at(i);
        tools.for_each_window_in_workspace(
            workspace, [&](const miral::Window& window) { hide_window(window); });
    }
}

void TiledWindowManager::hide_window(miral::Window const& window)
{
    miral::WindowSpecification spec;
    spec.state() = MirWindowState::mir_window_state_minimized;

    tools.modify_window(window, spec);
};

void TiledWindowManager::show_window(miral::Window const& window)
{
    miral::WindowSpecification spec;
    spec.state() = MirWindowState::mir_window_state_restored;

    tools.modify_window(window, spec);
};

int TiledWindowManager::count_windows_in_workspace(
    std::shared_ptr<miral::Workspace> const& workspace)
{
    int count = 0;
    tools.for_each_window_in_workspace(workspace, [&](const miral::Window& window) { ++count; });
    return count;
}

std::vector<miral::Window>
TiledWindowManager::get_workspace_windows(std::shared_ptr<miral::Workspace> const& workspace)
{
    int windows_count = count_windows_in_workspace(workspace);
    std::vector<miral::Window> windows;
    windows.reserve(windows_count);
    tools.for_each_window_in_workspace(
        workspace, [&](const miral::Window& window) { windows.push_back(window); });
    return windows;
}

std::shared_ptr<miral::Workspace> TiledWindowManager::get_current_workspace()
{
    return workspaces.at(this->current_workspace_index);
}
