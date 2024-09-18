# WiTUI
A library to draw Terminal User Interfaces,
specialising in showing multiple "windows".

Note that the displayed output will be "read-only". 
There are currently no plans to make input-text-fields.

Also note that any violations against the rules laid out here, 
or malformed strings, will result in the program aborting.
I will however do my best to make the error-messages clear.


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
Note that when there are multiple windows, only 1 of them per row can have that 
-1 value. When this is not the case, the program wil abort. Test your code ;P
<!--- TODO: how does this work for vertical expansion? --->

Text in a window can be too long for the window size. 
Too long lines can either be wrapped to a new line, or cut off.
Too many lines will result in a scrollable window.
This scrolling will leave a line visible around the cursor,
as long as the bottom or top isn't reached yet.
When the cursor is line-based, scrolling left and right will be the intuitive
way, instead of waiting till the invisible cursor reached the end of the line.
Scrolling past the text-boundaries will do nothing, the cursor will halt.

The text of a window is wrapped once beforehand,
to not have to recompute every time.

The title and footer can internally be the same struct, 
and can have their alignement set to left, right or center.
They will be displayed inside the border, and cut off when too large.

The border consists of an array of 8 characters: 4 corners and 4 sides.
It also can have a color that is applied to the whole border,
and that color can be defined for when the window is selected or not.
By default this would be white and grey respectively.

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
it will use the content of the last cell left it, then up.
To have a depending window have new content per line, 
the rows in the contents-array should have only one item each.

For the program to know which windows to update, each window will have a list 
with the windows it should update, indicated with their session-window-position.

When lines get wrapped because they are too long, the contents of the wrapped
part are also moved to a new row. This could mean adding a NULL element so 
that the content of the original line is showed when hovering over the wrapped
part.

A depending window will start empty, unless it depends on the window where the 
cursor starts. Then it will behave like the cursor just moved to that position.


### Return value
The main rendering function will take control over the program until it has to
stop rendering. 
At that point, it would be useful to return some kind of position.
I decided for now that the position should be a struct with the 
window position in the session-2D-array, and the relative cursor position in the
content of the window, and the last key pressed (see [Moving the cursor](#moving-the-cursor)).

To solve the issue that sometimes you don't want to know the position of the 
cursor in latest window, each window has a setting that dictates whether or not 
it should overwrite the latest position.

> This is necessary in a situation with depending windows. 
Imagine a session with 2 windows next to eachother. 
One holds a table, the second one (depending on the first) has a text field to
display more info about the current selected row. 
This second window may however not be big enough to show the whole text, 
so the user may switch to that second window and scroll down. 
Upon having read the whole text, the rendering may stop. 
At that point you don't want the position inside the second window,
but the current selected row in the first window.


### Moving the cursor
The user can move around in each window with the arrow-keys, 
or `hjkl` (from vim). The latter can be changed by the programmer.

To stop the rendering, escape will always work. Other keybinds can be added.
This exit-keypress will be returned alonside the cursor-position. 
This can be useful to determine whether a user wanted to confirm something,
or just exit.

To move between the different windows, the same keys are used, 
but with a modifier-key held. By default this is the control-key.
This key can be modified by the programmer.


## Current restrictions
This is a list of things that don't have to be completed before calling this 
working, but are things I would like to implement:



## Aditional notes
- There is a potential for better performance by only redrawing parts of the 
screen that have changed. 
This is quite advanced, and should only be implemented if tests indicate that
performance is troubling.

- Another feature could be to show a scrollbar when scrolling inside a window.
This scrollbar can be part of the border, and also be configured like the other
border-elements.

- Currently, resizing isn't handled. To effectively resize when the terminal 
size changes, the wrapping should be recalculated.
