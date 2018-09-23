# Laser Color

A simple color picker for Windows written in C++.

![App window](https://user-images.githubusercontent.com/23151624/45925194-5b33a900-bf19-11e8-84f0-b17ab586191e.png)

You can choose colors, copy them in three different formats into the clipboard and save the colors into the library.

## Controls
The application has __no mouse controls__ except for the close and minimize buttons on the title bar. This is so that
you wouldn't have to move your hand away from the keyboard keys which makes you work slower.
Keybindings were inspired by vim keybindings therefore knowledge and being comfortable with vim keybindings can be helpful when using this application.

##### Keybindings:
* __J__ - move to the next color component (e.g., from R to G or from G to B)
* __K__ - move to the previous color component (e.g., from B to G or from G to R)
* __W__ - increase the value of the component by 1
  * __Shift + W__ - increase the value of the component by 10
* __B__ - decrease the value of the component by 1
  * __Shift + B__ - decrease the value of the component by 10
* __0__ - toggle color component (switch from 0 to 255 and vice versa)
* __Ctrl + D__ - copy the color as three byte values: `255, 128, 128`
* __Ctrl + F__ - copy the color as three percentages values (range from 0 to 1): `1.0f, 0.5f, 0.5f`
* __Ctrl + X__ - copy the color as a hexadecimal value: `#ff8080`
* __I__ - add the color to the library
* __X__ - remove the color from the library
* __R__ - replace the currently selected color in the library with the chosen color
* __U__ - undo changes to the color in the library
* __H__ - select previous color sample
* __L__ - select next color sample
* __Ctrl + S__ - save the library

## Installation

To install the app, simply go to Releases page of the repository and download the LaserColor.exe installer.
In the installer, follow instructions as you would in any other installer. That's it.
