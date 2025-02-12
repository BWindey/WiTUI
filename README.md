# WiTUI

> [!IMPORTANT]
> This is still a work in progress, but it is nearing a first release!
> Currently still hunting bugs and working on demos to see what features are
> still missing. See this [issue](https://github.com/BWindey/WiTUI/issues/2)
> for info on what is planned before releasing a first release.
> From this moment though, `main` should only contain useable code, features
> and bugs will be worked on in seperate branches.

A library to draw Terminal User Interfaces,
specialising in showing multiple "windows".

This shines in the so-called "depending windows", where a window can have
multiple content-strings, and it will show the content according to where the
cursor is in the window that it depends on.
The inspiration for this was the need to display extra info for each row in a
table.


## Content-table
- [Currently implemented](#currently-implemented)
- [Plans](#plans)
- [Library-flow](#library-flow)
- [Installation and platform support](#installation-and-platform-support)
- [Demos](#demos)
- [Documentation](#documentation)


## Currently implemented
Basic features:
- windows in rows with configurable sizes
- flexible width available, recalculates immediatly on terminal resize
- content in windows
- navigating content with a cursor
- moving focus between windows
- fully customisable borders

Advanced features:
- custom powerful keymaps with callback function (defaults available)
- show different content depending on cursor-position ("depending windows")


## Plans
With the basics covered, there are a few feature that I would like to introduce:
- input fields
- better sizing options (window percentage)
- keeping track of ansi escape codes when wrapping text
- scrollbar
- rework of drawing windows to allow redrawing only 1 window
- session borders
- asynchronously run `wi_show_session(...)`

More can be found in this issue: https://github.com/BWindey/WiTUI/issues/3 .
You are welcome to post your ideas there as well.


## Library-flow
This section shows the basic building blocks and explains what they are
conceptually. Exact properties and definitions can be found in the
[Documentation](#documentation).

### Windows
Windows are areas in which text can be displayed. They are a container for
contents. A window can have more then one content-string, each on a seperate
place in the grid. This is useful for windows which depend on another window.
This window will look at the cursor position inside its "parent" to determine
which content to display on the screen.

> [!warning]
> Currently, looking at the current cursor-position is not working very
> intuitively for wrapped windows. See the behaviour in [Documentation/Cursor](#cursor).

A window is also always encased inside a border. This border consists of four
sides (left, top, right and bottom), the four corners, a colour when the window
is in focus, and a colour when the window is out of focus,
and a title and footer. Together they provide great customisability.

### Session
Sessions are containers grouping windows. Windows can be placed on different
rows inside the session, and the session provides the keymaps to move inside
or between those windos.
A session is also the object that the library renders.

### Custom keybindings
The programmer can implement keybinds through a combination of a key, modifier
and function-pointer. The library provides some good premade functions, like
some that can move the cursor or focus.
This allows the programmer to run arbitrary code when a user presses a button.

> [!warning] Catching pressed buttons
> Terminals do not give pressed modifiers to a running program. This limits the
> possible keymaps. The worst one is that `CTRL + j` is the same as pressing
> `enter`. They both return ascii code 10 (`\n`). Terminals which support
> [Kitty's keyboard handling protocol](https://sw.kovidgoyal.net/kitty/keyboard-protocol/)
> would be able to correctly show all different key-modifiers, but this protocol
> is currently not (yet) supported by WiTUI.

### Multithreading
Currently the rendering and input-handling happen on 2 seperate threads.
The rendering-entrypoint (`wi_show_session(wi_session* session)`) spawns these
two threads and waits for them to finish. Keymaps will be executed on the input
thread, the rendering thread just renders and checks if the terminal size
changed.
This means that the program calling `wi_show_session(...)` will halt until
rendering is done, but extra logic can be implemented via keymaps.

In the future the library will probably support a way to easily run
`wi_show_session(...)` asynchronously.



## Documentation
> [!note]
> Work in progress...

Before I begin, I'll start of by saying everything is 0-indexed.

WiTUI works with 2 main objects (structs):
- wi_session
- wi_window

A `wi_session` is the container holding `wi_windows`.

#### `wi_session`
A session is a struct that consists of the following values:
- `windows` (`wi_window***`):
    This is the 2D array on the heap with pointers to the windows attached
    to the session. I would strongly discourage modifying this directly.
    Use the function `wi_add_window_to_session(..)` instead to handle everything.
- `fullscreen` (`bool`):
    This setting indicates whether a `clear` should be called before each
    rendered frame. If this is set to `false`, the session will be rendered
    just below the shell-prompt.
- `cursor_pos` (`struct wi_posisition`):
    This setting indicates which window should be the first to be in focus.
    The struct holds 2 integers: `.row` and `.col`.
- `movement_keys` (`struct wi_movement_keys`):
    This setting dictates which keys are used to move between windows,
    and inside windows.
    The struct holds 5 chars: `.left`, `.right`, `.up`, `.down` and `.quit`,
    and it holds the modifier-key that needs to be pressed with the normal
    key to jump between windows.
    The `.modifier_key` is an enum with following options: `CTRL`, `ALT`, `SHIFT`.
- `internal` (anonymous `struct`);
    This field holds internal data about the amount of `wi_windows` in the
    session. I strongly advice to not edit this field, unless you're adding/
    removing windows from `.windows` without using library-functions.

A session also has the following functions made for them:
- `void wi_free_session_completely(wi_session*)`:
    This function calls `wi_free_window()` (see its documentation later)
    on all the `wi_window`s inside it,
    calls `free()` on all the rows of the 2D array and the array itself,
    and calls `free()` on the extra internal array for keeping track of size.

- `int wi_render_frame(wi_session*)`:
    This will a single frame of the session, and return the height of the
    printed out frame.
    This can be useful while developing, or maybe you just don't want that
    interactive stuff.
    This function is called by `wi_show_session(...)` every time the user
    presses a key.

- `wi_result wi_show_session(wi_session*)`:
    This is the function that the library was designed for.
    Show that session!
    This will wait for the user to quit by pressing `session.movement_keys.quit`.
    Then it will return the result: a `wi_result` struct.
    This struct consists of 2 `wi_positsion`, the first one showing which window
    was last selected, and the second one showing the position of the cursor
    in that window. This behaviour can change depending on the window settings
    (see that documentation later).

- `wi_session* wi_make_session(void)`:
    This is the recommended way to create a session. It sets defaults, and
    initialises all the values in the way the other functions expect.
    The defaults are:
        - `.windows` - empty
        - `.start_clear_screen` = `false`
        - `.cursor_pos` = `{ 0, 0 }`
        - `.movement_keys` = `{ 'h', 'j', 'k', 'l', CTRL }`
    It returns the created session.

- `wi_session* wi_add_window_to_session(wi_session*, wi_window*, int row)`:
    This function adds a `wi_window*` to the session, on the given row.
    It handles all the memory-management and updates internal sizes.
    When the row is bigger then the current amount of rows, a new row is
    created at the end, and the window is placed there.
    So when there are 2 rows, and you want to add it on row 4,
    it will actually be at the new 3rd row, not the 4th.
    It will also attach windows at the end of the given row, so the order
    of adding windows is important.


#### `wi_window`
A window is a struct that consists of the following values:
- `width` (int):
    The width a window should have, excluding a potential border.
    The width can be set to `-1`. This means it will take in all the available
    screenspace. When multiple windows on the same row have their width set to
    `-1`, the available space will be distributed equally between them.
    If the space is not equally divisible, the remainder `r` will be distributed
    among the first `r` windows with width `-1`. Yep, that math checks out =)
- `height` (int):
    The height a window should have, excluding a potential border.
- `contents` (`wi_content***`):
    A 2D array on the heap with `wi_content` pointers. I advice to not modify
    this directly, but to use `wi_add_content_to_window(...)`.
- `border` (`wi_border`):
    A struct with the border-information, including title and footer.
    To have empty parts of the border, set them as empty strings. To disable
    the whole border at once, set it to `{ 0 }`, or more specifically setting
    `.corner_bottom_left` to `NULL` will do the trick.
- `wrap_text` (`bool`):
    Whether to wrap long lines inside this window or not.
    Leaving this off will enable side-scrolling.
- `store_cursor_position` (`bool`):
    Whether to store the cursor-position inside the parent-session or not.
- `cursor_rendering` (`wi_cursor_rendering`):
    An enum (`INVISIBLE`, `LINEBASED`, `POINTBASED`) that tells how to render
    the cursor inside this window.
- `depending_windows` (`wi_window**`):
    An array with windows, will be used to redraw the right windows later, not
    in use yet.
- `depends_on` (`wi_window*`):
    A pointer to the window this window depends on.
- `internal` (anonymous `struct`):
    A struct with values you should not touch, they get updated by the library.


#### Content
Ansi escape codes in the contents (or borders) are supported, but it has to be
noted that the library chooses to stop them at a `\n`. This is to limit the
amount of "keeping-track" the library has to do, and is, in my opinion, not that
bad of choice to make.
When content is wrapped, the effects will last for the entire line, even when
wrapped.

The reason I keep track of effects when wrapping, but not when not wrapping,
is that when lines do not wrap, the library should go look at content that is
on the right-hand side of the window too to look for effects, which would be
not good for performance. The library does however look to the left of the
window for effects, as it has to count the width to correctly calculate offset.
