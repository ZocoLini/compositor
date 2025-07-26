#ifndef FIBONACCI_WINDOW_MANAGER_H
#define FIBONACCI_WINDOW_MANAGER_H

#include "miral/window.h"
#include "miral/window_management_policy.h"
#include "miral/window_manager_tools.h"
#include <miral/minimal_window_manager.h>

class TiledWindowManager : public miral::MinimalWindowManager
{
  public:
    explicit TiledWindowManager(const miral::WindowManagerTools& tools);
    ~TiledWindowManager();

    miral::WindowSpecification place_new_window(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification const& requested_specification) override;

    void handle_request_resize(
        miral::WindowInfo& window_info,
        MirInputEvent const* input_event,
        MirResizeEdge edge) override;

    void advise_new_window(miral::WindowInfo const& window_info) override;
    void advise_adding_to_workspace(
        std::shared_ptr<miral::Workspace> const& workspace,
        std::vector<miral::Window> const& windows) override;
    void advise_removing_from_workspace(
        std::shared_ptr<miral::Workspace> const& workspace,
        std::vector<miral::Window> const& windows) override;
    void
    handle_request_move(miral::WindowInfo& window_info, MirInputEvent const* input_event) override;
    void handle_modify_window(
        miral::WindowInfo& window_info, miral::WindowSpecification const& modifications) override;
    bool handle_keyboard_event(MirKeyboardEvent const* event) override;

  private:
    void update_windows(std::vector<miral::Window> const& ignorable_windows);
    void hide_window(miral::Window const& window);
    void show_window(miral::Window const& window);
    std::vector<miral::Window>
    get_workspace_windows(std::shared_ptr<miral::Workspace> const& workspace);
    int count_windows_in_workspace(std::shared_ptr<miral::Workspace> const& workspace);
    std::shared_ptr<miral::Workspace> get_current_workspace();

  protected:
    std::vector<std::shared_ptr<miral::Workspace>> workspaces;
    size_t current_workspace_index = 0;
};

#endif // FIBONACCI_WINDOW_MANAGER_H
