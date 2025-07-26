# Process and choices made

## Official Mir documentation

The process began by consulting the official Mir documentation to identify a ‘getting started’ guide, tutorialS, and examples.

## Creating the project and setting up the environment

Using the official tutorial, I now know what I need to do to create a Wayland compositor so I start by creating
a new project directory and setting up the environment.

- Created a basic project structure with include, src and test folders.
- CMakeLists.txt files created.
- Configured the code editor and C++ tools for development.
- Init a git repository.
- Created a main.cpp file with the example code from the official tutorial and tested that it works.

## Basic functionality

Using the 'how to guides' I decided to implement some basic functionality for the compositor such as:

- Starting a process (xfce4-terminal) using the key combination Ctrl+Alt+K
- Adding the ability to start a process automatically when the compositor starts, using the '--startup-app' option.

## Deciding the behaviour of the compositor

Once I have a basic project structure and functionality, I can start deciding the behaviour of the compositor.
For this case, I will implement a simple tiling compositor that can handle multiple windows and workspaces.

## Creating the Window Manager Policy

Extending the MinimalWindowManager class to implement a Tiling policy overriding the behaviour of the
MinimalWindowManager class where needed. The first thing I want for my Window Manager is to disable window resizing
and movement by the user. The TiledWindowManager will decide where the windows are placed and what size they will have.
Reading the methods exposed by the WindowManagerPolicy class, I found that the ones I need to override are:

- handle_request_move
- handle_request_resize

Once the windows cannot be moved or resized by the user, I also need to disable the possibility of changing the window state
by overriding the 'handle_modify_window' method.

The next step is to implement the functions that will place and resize the windows. I'm implementing this by
overriding the 'advise_new_window' method and defining two new functions:

- mir::geometry::Point position_for(int index, int width, int height)
- mir::geometry::Size size_for(int index, int total, int width, int height)

The index represents the position of the window, and the total represents the total number of windows. The positions are
pretty straightforward, since no matter the total number of windows, the position for the window 'n' is always the same.

```cpp
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
```

For the sizes, I first thought about using a formula to calculate the size of each window based on the total number of windows
and the available screen space, or using a strategy pattern to determine the size for each window based on the index and
total value. However, after some experimentation, I decided to go with an array of sizes, simpler and more efficient.

```cpp
mir::geometry::Size size_for(int index, int total, int width, int height)
{
    static std::tuple<float, float> sizes[kSizeCombinations] = {
        std::tuple<float, float>{1.0f, 1.0f}, // 1 window - Window 0

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
```

Now we have a usable Window Manager but with one issue, we cannot place more than four windows.

## Workspaces

Having a limit of four windows isn't ideal. To overcome this limitation, I can introduce the concept of workspaces which
I already saw in the Mir API. Each workspace can hold up to four windows and, each time a fifth window is added, it will be placed
in the next workspace.

The first step is to track the created workspaces and the active one.

```cpp
std::vector<std::shared_ptr<Workspace>> workspaces;
size_t current_workspace_index = 0;
```

Now we can adjust the logic to handle workspaces.

- When creating the window manager, we create the first workspace.
- The 'advise_new_window' method will be responsible for creating a new workspace when needed and placing the window where it fits.
- By overriding the methods 'advise_adding_to_workspace' and 'advise_removing_from_workspace' we can handle the main workspace logic,
  mainly by calling the 'update_windows' method.
- A new method 'update_windows' will iterate over the workspaces and updating the windows inside of them.

## Extra functionality to interact with the workspaces

Reading the API, I found more methods to interact with the workspaces, such as the 'handle_keyboard_event' method, which is perfect for
what I wanted to implement, logic to interact with the workspaces using keybinds.

- Ctrl + Alt + Q: Move to previous workspace
- Ctrl + Alt + E: Move to next workspace
- Ctrl + Alt + M: Move the active window to a new workspace
- Ctrl + Alt + V: Close all windows in the actual workspace, remove it, and move to the previous one
- Ctrl + Alt + Tab: Switch the focus between windows in the current workspace

## Testing

As the last step, while testing the creation of windows and moving them between workspaces, I noticed that, when moving to a
workspace where there is a gnome-calculator window, the focus was being moved to a window in the previous workspace, which was not the expected behavior.

My first thought was to change the focus to the first window when the method 'update_windows' was called but it did not work as
expected. The compositor started crashing on startup. The error was the following:

```bash
Mir fatal error: advise_focus_gained: window was not found in the list!!!
```

I decided to override the method 'advise_focus_gained' and it started working as expected. To see what was going on, I decided to read the
source code and understand what MinimalWindowManager was doing. At this point, I started thinking that using MinimalWindowManager instead
of WindowManagerPolicy might not be the best approach, but, at the same time, I wanted the functionality already implemented in
MinimalWindowManager.

Diving into the source code, I found that 'ApplicationSelector::advise_focus_gained' was raising a fatal error because the window I was
trying to focus was not found in a list called 'focus_list'. Elements were being added to the list when
'ApplicationSelector::advise_new_window' was being called. MinimalWindowManager calls 'ApplicationSelector::advise_new_window' when
'MinimalWindowManager::advise_new_window' gets called so maybe I wasn't calling 'MinimalWindowManager::advise_new_window' after
overriding the method. I went to my code and found that I was calling it, but after the focus request.

```cpp
void TiledWindowManager::advise_new_window(WindowInfo const& window_info)
{
    auto current_workspace = this->workspaces.at(this->current_workspace_index);
    int windows_count = count_windows_in_workspace(current_workspace);

    if (windows_count >= kMaxWindows)
    {
        auto new_workspace = tools.create_workspace();
        this->workspaces.push_back(new_workspace);
        this->current_workspace_index = workspaces.size() - 1;
        current_workspace = new_workspace;
    }

    tools.add_tree_to_workspace(window_info.window(), current_workspace); // This triggers update_windows trying to give focus to the first window in the workspace

    MinimalWindowManager::advise_new_window(window_info); // This adds the window to the focus list
}
```

I moved 'MinimalWindowManager::advise_new_window' before the focus request, and that solved the issue.

## Last keybinding

I noticed that I didn't have a keybinding to close a window, something mandatory if I want a window manager to work as expected without
a mouse. At the same time, I also found that I wasn't removing the workspaces when empty, which caused the app to crash when updating the
windows because there was no first window to focus. I added a check to ensure there is at least one window in the workspace before trying
to focus it. I also added logic to remove the workspace when empty inside 'advise_removing_from_workspace' instead of
the logic previously implemented when using Ctrl + Alt + V.

- Ctrl + Alt + C: Close the focused window
