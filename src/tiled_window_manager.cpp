#include "tiled_window_manager.h"
#include "mir/geometry/forward.h"
#include "mir_toolkit/common.h"
#include "miral/application.h"
#include "miral/application_info.h"
#include "miral/minimal_window_manager.h"
#include "miral/window.h"
#include "miral/window_management_policy.h"
#include "miral/window_specification.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
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
    
    return mir::geometry::Point{
        std::get<0>(position) * width,
        std::get<1>(position) * height
    };
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
    
    static int8_t start_index[kMaxWindows] = { 0, 1, 3, 6 };  
    
    std::tuple<float, float> size = sizes[start_index[total - 1] + index];
    
    return mir::geometry::Size{
        std::get<0>(size) * width,
        std::get<1>(size) * height
    };
}

TiledWindowManager::TiledWindowManager(const WindowManagerTools& tools)
    : MinimalWindowManager(tools)
{
}

miral::WindowSpecification TiledWindowManager::place_new_window(
    miral::ApplicationInfo const& app_info,
    miral::WindowSpecification const& requested_specification)
{
    miral::WindowSpecification spec = requested_specification;

    auto *windows = &app_info.windows();
    size_t n_windows = windows->size();

    if (n_windows == kMaxWindows)
    {
        throw std::runtime_error("Cannot place more than 4 windows");
    }

    int width = tools.active_output().size.width.as_value();
    int height = tools.active_output().size.height.as_value();

    for (int i = 0; i < n_windows; ++i) {
        windows->at(i).resize(size_for(i, n_windows + 1, width, height));
    }
    
    spec.top_left() = position_for(n_windows, width, height);
    spec.size() = size_for(n_windows, n_windows + 1, width, height);

    return spec;
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

void TiledWindowManager::handle_modify_window(WindowInfo& window_info, WindowSpecification const& modifications) 
{
    if (modifications.state() == MirWindowState::mir_window_state_maximized) {
        return;
    }
    
    MinimalWindowManager::handle_modify_window(window_info, modifications);
}

void TiledWindowManager::advise_delete_window(WindowInfo const& window_info)
{
    miral::MinimalWindowManager::advise_delete_window(window_info);
    
    auto filter = [&](miral::ApplicationInfo const& app_info) -> bool
    {
        // TODO: Implement logic to filter applications
        return true;
        return window_info.application_id() == app_info.name();
    };

    miral::Application app = tools.find_application(filter);
    miral::ApplicationInfo app_info = tools.info_for(app);

    std::vector<miral::Window> ignorable_windows = {window_info.window()};
    positionate_windows(app_info, ignorable_windows);
}

void TiledWindowManager::positionate_windows(
    miral::ApplicationInfo const& app_info,
    std::vector<miral::Window> ignorable_windows)
{
    int n_windows = app_info.windows().size();
    int width = tools.active_output().size.width.as_value();
    int height = tools.active_output().size.height.as_value();

    std::vector<miral::Window>* windows = &app_info.windows();
    
    int real_index = 0;
    for (int i = 0; i < n_windows; ++i)
    {
        auto it = std::find(ignorable_windows.begin(), ignorable_windows.end(), windows->at(i));
        if (it != ignorable_windows.end()) continue;
        
        windows->at(i).resize(size_for(real_index, n_windows - ignorable_windows.size(), width, height));
        windows->at(i).move_to(position_for(real_index, width, height));
        real_index = real_index + 1;
    }
}
