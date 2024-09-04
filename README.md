# WiTUI
A library to draw Terminal User Interfaces,
specialising in showing multiple "windows".

Note that the displayed output will be "read-only". 
There are currently no plans to make input-text-fields.


## Goals
### Window
The programmer can define 1 or more "windows".
Each window has the following, customisable, settings:
- size
- wrap / cutoff text
- title
- footer
- border
- cursor behaviour

Text in a window can be too long for the window size. 
Too long lines can either be wrapped to a new line, or cut off.
Too many lines will result in a scrollable window.
This scrolling will leave a line visible around the cursor,
as long as the bottom or top isn't reached yet.
The text of a window is wrapped once beforehand,
to not have to recompute every time.

The title and footer can internally be the same struct, 
and can have their alignement set to left, right or center.
They will be displayed inside the border, and cut off when too large.

The border consists of an array of 8 characters: 4 corners and 4 sides.
A border also has different colours when active or not.

The cursor behaviour defines how the cursor is displayed.
This can be invisible, line-based or point-based.
The position of the cursor is shown by swapping fore- and backgroundcolours.

A window can also depend on other windows. 
More on that later in [Depending windows](<README#Depending windows>).

### Session
A session is the actual thing displayed.
It is a collection of windows, and defines their order to display.
It has:
- a 2D array with the windows
- a setting whether or not to take over the whole screen
- movement settings (see [movement](<README#Moving the cursor>)

The programmer using this library could define multiple sessions 
to create something like tmux can do.

### Depending windows
As mentioned before, windows can depend on eachother.
This means that the content of a window can change when the cursor moves 
inside another window that it depends on.

The content of these depending windows is stored in a 2D array.
When updating the content of the window, it will grab the content found at 
the coordinates of the cursor. When there is no content there,
it will use the content of the last cell left it, then up.
To have a depending window have new content per line, 
the rows in the contents-array should have only one item each.

For the program to know which windows to update, each window will have a list 
with the windows it should update.

When lines are being wrapped because they are too long,
the "new" lines will insert a null in the contents-array to keep the 
right order.


### Return value
The main rendering function will take control over the program until it has to
stop rendering. 
At that point, it would be useful to return some kind of position.
I decided for now that the position should be a struct with the 
window position in the session-2D-array, and the relative cursor position in the
content of the window.

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
blabla, stored in session, will write this section later
