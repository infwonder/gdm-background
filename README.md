# gdm-background
A simple graphical tool made with GTK3 in C to change the GDM 3 background image of Ubuntu 20.04

## Warning
This tool won't work with older versions of Ubuntu. It was made specifically to work with Ubuntu
20.04 as it now bundles all configuration files inside a .gresource file. It also won't work if
your system is set to a custom gdm3 theme. You will have to reset to the default configuration of
gdm3 before using this tool.

If you feel confortable compiling linux programs from source code, you may proceed to next
section which tells you about the required packages necessary to build the program. And remember,
I will not be responsible for any damage that may be caused by the use of this software. So, use
it at your own risk.

## Dependencies
You will need the following packages installed in order to compile gdm-background:

* `make` - to build the program.
* `gcc` - to compile the source code.
* `libgtk-3-dev` - to be able to compile gtk3 c code.
* `libpolkit-gobject-1-dev` - to be able to compile polkit c code.

## Installation

After you have all packages mentioned above you may build and install this tool with the following
commands:
```
$ git clone https://github.com/thiggy01/gdm-background
$ cd gdm-background
$ make
$ sudo make install
```
If everything was successful, you will see the GDM Background purple icon by pressing <kbd>Super</kbd>
and then typing gdm background.

## Usage
If you click on the gdm-background shortcut, it will open a drag and drop window for you to drop an
image file.

When you drop the selected image in the droping area, the program will enable the `set` button.
If you click on it, it will show a popup window asking for the admin password because this action can
only be performed by the root user. After you correctly enter the required credencial, the tool
will show a successful dialog and will ask if you want to restart the gdm service to apply change made.

You can always return to your default Ubuntu purple theme by clicking on the `restore` button and
it will ask for admin credentials and if you want to restart GDM to apply change.

You can even uninstall it with `sudo make uninstall`.

