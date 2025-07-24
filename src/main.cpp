#include "tiled_window_manager.h"
#include <miral/append_event_filter.h>
#include <miral/configuration_option.h>
#include <miral/external_client.h>
#include <miral/runner.h>
#include <miral/set_window_management_policy.h>
#include <miral/toolkit_event.h>
#include <string>
#include <vector>

using namespace miral;
using namespace miral::toolkit;

int main(int argc, char const* argv[])
{
    MirRunner runner{argc, argv};
    
    std::string terminal_cmd{"xfce4-terminal"};
    miral::ExternalClientLauncher launcher;

    auto const open_terminal_keybinds = [&](MirEvent const* event)
    {
        // Skip non-input events
        if (mir_event_get_type(event) != mir_event_type_input)
            return false;

        // Skip non-key input events
        MirInputEvent const* input_event = mir_event_get_input_event(event);
        if (mir_input_event_get_type(input_event) != mir_input_event_type_key)
            return false;

        // Skip anything but down presses
        MirKeyboardEvent const* kev =
            mir_input_event_get_keyboard_event(input_event);
        if (mir_keyboard_event_action(kev) != mir_keyboard_action_down)
            return false;

        // CTRL  ALT must be pressed
        MirInputEventModifiers mods = mir_keyboard_event_modifiers(kev);
        if (!(mods & mir_input_event_modifier_alt) ||
            !(mods & mir_input_event_modifier_ctrl))
            return false;

        switch (mir_keyboard_event_keysym(kev))
        {
            case XKB_KEY_BackSpace:
                runner.stop();
                return true;
            case XKB_KEY_k:
            case XKB_KEY_K:
                launcher.launch({terminal_cmd});
                return true;
            default:
                return false;
        };
    };

    auto run_startup_apps = [&](std::vector<std::string> const& apps)
    {
        for (auto const& app : apps)
        {
            launcher.launch(std::vector{app});
        }
    };

    return runner.run_with(
    {
        set_window_management_policy<TiledWindowManager>(), launcher,
         miral::ConfigurationOption{
             run_startup_apps, "startup-app",
             "App to run at startup (can be specified multiple times)"},
         miral::AppendEventFilter{open_terminal_keybinds}
    });
}
