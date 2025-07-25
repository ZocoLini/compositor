#ifndef FIBONACCI_WINDOW_MANAGER_H
#define FIBONACCI_WINDOW_MANAGER_H

#include <miral/minimal_window_manager.h>

using namespace ::miral;

class TiledWindowManager : public miral::MinimalWindowManager
{
  public:
    explicit TiledWindowManager(const WindowManagerTools& tools);
    ~TiledWindowManager();

    miral::WindowSpecification place_new_window(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification const& requested_specification) override;

    void handle_request_resize(miral::WindowInfo& window_info,
                               MirInputEvent const* input_event,
                               MirResizeEdge edge) override;
    
    void advise_new_window(WindowInfo const& window_info) override;
    void advise_adding_to_workspace(
        std::shared_ptr<Workspace> const& workspace,
        std::vector<Window> const& windows) override;
    void advise_removing_from_workspace(
        std::shared_ptr<Workspace> const& workspace,
        std::vector<Window> const& windows) override;
    void handle_request_move(WindowInfo& window_info, MirInputEvent const* input_event) override;
    void advise_delete_window(WindowInfo const& app_info) override;
    void handle_modify_window(WindowInfo& window_info, WindowSpecification const& modifications) override;
    
    private:
        void update_windows();
        int count_windows_in_workspace(std::shared_ptr<miral::Workspace> workspace);
        
    protected:
        std::vector<std::shared_ptr<Workspace>> workspaces;
        size_t current_workspace_index = 0;
};

#endif // FIBONACCI_WINDOW_MANAGER_H
