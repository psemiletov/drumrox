Drumrox
=======

[OpenSUSE](https://build.opensuse.org/package/show/multimedia:proaudio/drumrox) *
[AUR](https://aur.archlinux.org/packages/drumrox) * [Drumrox kits repo](https://github.com/psemiletov/drum_sklad)

=======

Drumrox is an easy way to write drum tracks as MIDI at your DAW, using Hydrogen and Drumrox kits.

Drumrox is LV2 drum machine (based on DrMr) to load Hydrogen and Drumrox drumkits. Drumrox is compatible with MODERN Hydrogen kit format. More of that, Drumrox has own simple drumkit format and the repository with [Drumrox kits](https://github.com/psemiletov/drum_sklad)


Some history: Drumrox is based on Nicklan's DrMr (https://github.com/nicklan/drmr) and Filipe Coelho's DrMr (https://github.com/falkTX/drmr). The first one can save/load the preset by index, so when you install new kits or delete some, indexes are messing up. falkTX's DrMr deals with presets in more comfortable way, via the names. I (Peter Semiletov) used it everyday, but at the some point of time it becomes incompatible with new Hydrogen kits, and I've edited them manually to fix (removing some section in XML). Fresh install of Hydrogen converts my edited kits to the modern kit format again, and it was simplier to add some code than to edit the kits again. And when I've started to do that, I understand that I want to clean up the code, etc, etc. That how Drumrox continues DrMr...

![image](https://user-images.githubusercontent.com/8168336/250968814-1b15341c-f59e-413b-8276-807a05089021.png)


## Features

* LV2 plugin format (currently with GTK2 GUI, works with Ardour and Mixbus only)

* Stereo (with built-in mixer) and 32-channels versions

* Supported sample kit formats: Hydrogen, Drumrox, SFZ

* Up to 32 instruments with layers

* Automatic open hihat mute on hihat close

* Drumkit image at plugin window

* Built-in mixed with panning options


Compilation and Install
-----------------------

Drumrox can be built with [CMake](http://www.cmake.org). Or you can install Drumrox from AUR (``yay -S drumrox``)

To build from the source simply do (from this dir):

    mkdir b
    cd b
    cmake ..    (or "cmake -DUSE_NKNOB=OFF .." if you want old style sliders)

Then do, as root or with sudo:

    make
    make install

There are some customizable variables for cmake.  To see them do "cmake -L".  The important ones are:

```USE_NKNOB``` - Use custom knob widget for controls instead of the default gtk sliders.  This defaults to ON.  Try turning it off if you are experiencing problems, or just prefer the sliders.

```LV2_INSTALL_DIR``` - The directory to install the Drumrox plugin to. To install to your home directory, use "~/.lv2" and clear the CMAKE_INSTALL_PREFIX. This defaults to "lib/lv2" (this is relative to CMAKE_INSTALL_PREFIX, which is usually /usr/local)


You need the following libraries to build Drumrox from the source:

- [libsndfile](http://www.mega-nerd.com/libsndfile/)
- [libsamplerate](http://www.mega-nerd.com/SRC/index.html)
- [lv2](http://lv2plug.in/)
- [gtk+2](http://www.gtk.org)

Drumrox scans the following directories for Hydrogen and Drumrox drum kits:

```/usr/share/hydrogen/data/drumkits
/usr/local/share/hydrogen/data/drumkits
/usr/share/drmr/drumkits
$HOME/.hydrogen/data/drumkits
$HOME/.drmr/drumkits
/usr/share/drumrox-kits
$HOME/.drumrox/drumkits
$HOME/drumrox-kits```
