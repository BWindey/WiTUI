# WiTUI
A library to draw Terminal User Interfaces,
specialising in showing multiple "windows".

This shines in the so-called "depending windows", where a window can have
multiple content-strings, and it will show the content according to where the
cursor is in the window that it depends on.
This is particularly useful in a scenario where you have a window showing a
table, and a side-window showing extra information about the line that is
selected in the table.

Note that the displayed output will be "read-only".
There currently is no way to input text yet, though I'd like to implement that.

Also note that any violations against the rules laid out here,
or malformed strings, will result in the program aborting.
I will however do my best to make the error-messages clear.

### Current short-term TODO's:
- Improve content:
    - shouldn't have to wrap
    - scrolling
- Rethink movement in rendering, maybe I could use absolute movement by storing
    starting position as (0, 0); then use more go_to()'s.
    This could simplify making a border around the session itself... hmmm
- Implement session-border


## Goals
### Window
The programmer can define 1 or more "windows".
Each window has the following, customisable, settings:
- size
- title
- footer
- border
- wrap / sidescroll text
- content
- cursor representation
- dependent windows
- store latest position (see [Return value](#return-value))

The size of a window can have the value -1 for each value, which means it will
make the window fill up the entire width/height of the available space.
When multiple windows have their width set to -1, they will each get an equal
amount of space to fill up the terminal-width.
When the terminal width is not dividable by the amount of flexible windows,
the remainder 'r' will be divided among the first 'r' windows on the row.

I'm still thinking about how to do the vertical flexibility, I'll probably
not implement that behaviour for now.
<!--- TODO: how does this work for vertical expansion? --->

Scrolling and wrapping should behave the same as in vim.

The title and footer can internally be the same struct,
and can have their alignement set to left, right or center.
They will be displayed inside the border, and cut off when too large.

The border consists of an array of 8 characters: 4 corners and 4 sides.
It also can have a color that is applied to the whole border,
and that color can be defined for when the window is focussed or not.
By default this would be white and dim respectively.
IMPORTANT: this border effect should not contain visible characters when
printed, to not mess up any width calculations.
When the border is set to `NULL`, it won't be rendered.
Note that if you want an empty border, but not that 2 windows do not have any
separation, you will need to have a border consisting of spaces.

The cursor representation defines how the cursor is displayed.
This can be invisible, line-based or point-based.
The position of the cursor is shown by swapping fore- and backgroundcolours.

A window can also depend on other windows.
More on that later in [Depending windows](#depending-windows).
That section also defines how `content` looks.
(A single content is a normal string, with newline characters to split lines.)

### Session
A session is the actual thing displayed.
It is a collection of windows, and defines their order to display.
It has:
- a 2D array with the windows
- a setting whether or not to take over the whole screen
- a setting on which window to put the cursor initially (default top-left)
- movement settings (see [movement](#moving-the-cursor))
- border (same as in window, but default `NULL`)

Note that when there are too many windows on a row to fit on the terminal,
the "left-over" windows will be placed on a new line.

The programmer using this library could define multiple sessions
to create something like tmux can do.


### Depending windows
As mentioned before, windows can depend on eachother.
This means that the content of a window can change when the cursor moves
inside another window that it depends on.

The content of windows is stored in a 2D array.
When updating the content of the window, it will grab the content found at
the coordinates of the cursor. When there is no content there,
it will use the content of the last cell left off it, then up.
To have a depending window have new content per line,
the rows in the contents-array should have only one item each.

For the program to know which windows to update, each window will have a list
with the windows it should update, indicated with their session-window-position.
This presents a slight issue when you want to switch between multiple sessions
with (some of) the same windows that depend on other windows.
For now, I'll let the user of the library do the work of setting the right
variables so they get updated, but it migth be worth exploring ideas to solve
this in the library itself. A potential solution migth be a datastructure in the
session that stores depending windows, which could be a 2D array or a map.

<!-- TODO: I have not thought about this yet... -->
When lines get wrapped because they are too long, the contents of the wrapped
part are also moved to a new row. This could mean adding a NULL element so
that the content of the original line is showed when hovering over the wrapped
part.

A depending window will start empty, unless it depends on the window where the
cursor starts. Then it will behave like the cursor just moved to that position.


### Custom keybindings
The programmer can implement keybinds through a combination of a key, modifier
and function-pointer. The library provides some good premade functions, like
some that can move the cursor or focus.


## Aditional notes
- There is a potential for better performance by only redrawing parts of the
screen that have changed.

- Another feature could be to show a scrollbar when scrolling inside a window.
This scrollbar can be part of the border, and also be configured like the other
border-elements.


## Documentation
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
