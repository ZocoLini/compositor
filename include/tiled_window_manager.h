#ifndef FIBONACCI_WINDOW_MANAGER_H
#define FIBONACCI_WINDOW_MANAGER_H

#include <miral/minimal_window_manager.h>

using namespace ::miral;

class TiledWindowManager : public miral::MinimalWindowManager
{
  public:
    explicit TiledWindowManager(const WindowManagerTools& tools);

    miral::WindowSpecification place_new_window(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification const& requested_specification) override;

    void handle_request_resize(miral::WindowInfo& window_info,
                               MirInputEvent const* input_event,
                               MirResizeEdge edge) override;
    
    void handle_request_move(WindowInfo& window_info, MirInputEvent const* input_event) override;
    void advise_delete_window(WindowInfo const& app_info) override;
    void handle_modify_window(WindowInfo& window_info, WindowSpecification const& modifications) override;
    
    private:
        void positionate_windows(miral::ApplicationInfo const& window_info, std::vector<miral::Window> deleted_windows);
};

#endif // FIBONACCI_WINDOW_MANAGER_H
