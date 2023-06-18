Drumrox
=======

[OpenSUSE](https://build.opensuse.org/package/show/multimedia:proaudio/drumrox) *
[AUR](https://aur.archlinux.org/packages/drumrox) * [Drumrox kits repo](https://github.com/psemiletov/drumrox-kits)

=======

Drumrox is an easy way to write drum tracks as MIDI at your DAW, using Hydrogen and Drumrox kits.

Drumrox is LV2 drum machine (based on DrMr) to load Hydrogen and Drumrox drumkits. Drumrox is compatible with MODERN Hydrogen kit format. More of that, Drumrox has own simple drumkit format and the repository with [Drumrox kits](https://github.com/psemiletov/drumrox-kits)


Some history: Drumrox is based on Nicklan's DrMr (https://github.com/nicklan/drmr) and Filipe Coelho's DrMr (https://github.com/falkTX/drmr). The first one can save/load the preset by index, so when you install new kits or delete some, indexes are messing up. falkTX's DrMr deals with presets in more comfortable way, via the names. I (Peter Semiletov) used it everyday, but at the some point of time it becomes incompatible with new Hydrogen kits, and I've edited them manually to fix (removing some section in XML). Fresh install of Hydrogen converts my edited kits to the modern kit format again, and it was simplier to add some code than to edit the kits again. And when I've started to do that, I understand that I want to clean up the code, etc, etc. That how Drumrox continues DrMr...

![image](https://user-images.githubusercontent.com/8168336/246684340-0b81c208-a9e5-4c75-b843-8840223d13ac.png)


About:
-----

- Loads as LV2 in the host (Ardour)
- Scan for and load Hydrogen drum kits
- Multi-layer hydrogen kits (will pick layer based on that samples set gain)
- LV2 controls for gain on first 32 samples of kit
- LV2 controls for pan on first 32 samples of kit
- GTK ui that can select a kit and control gain/pan on each sample
- Custom knob widget for GTK ui based on phatknob that is both functional and awesome looking

From falkTX's DrMr "Regarding This Branch":
-------------------------------------------

- Kits are now selected via their path and not the hacky integer port as before
- The kit path is now saved and restored using lv2-state, so you can install/remove/modify hydrogen kits and your state won't be messed up.
- No need to scan kits in the plugin core so the plugin loads faster
- Led indicator in the UI when a sample is triggered
- Led indicator can be clicked to trigger sample for preview


Drumrox additions:
------------------

- Compatibility with modern Hydrogen kit format
- Presets are sorted in alphabetical order
- Drumrox format drumkits and [kits repo](https://github.com/psemiletov/drumrox-kits)
- Plugin window is more fits to the screen
- Drumkit picture support
- Panning modes (linear panner, law: -6 dB //default; linear panner, law: 0 dB;                    square root panner, law: -3 dB; sin/cos panner, law: -3 dB
- best resampler mode at sample load (SRC_SINC_BEST_QUALITY from libsamplerate)
- Partial conversion from C to C++ to further development



Compilation and Install
-----------------------
Drumrox is built with [CMake](http://www.cmake.org). Or you can install Drumrox from AUR (``yay -S drumrox``)

To build from the source simply do (from this dir):

    mkdir b
    cd b
    cmake ..    (or "cmake -DUSE_NKNOB=OFF .." if you want old style sliders)

Then do:

    make
    make install

to install.  There are some customizable variables for cmake.  To see them do "cmake -L".  The important ones are:

USE_NKNOB - Use custom knob widget for controls instead of the default gtk sliders.  This defaults to ON.  Try turning it off if you are experiencing problems, or just prefer the sliders.

LV2_INSTALL_DIR - The directory to install the Drumrox plugin to. To install to your home directory, use "~/.lv2" and clear the CMAKE_INSTALL_PREFIX. This defaults to "lib/lv2" (this is relative to CMAKE_INSTALL_PREFIX, which is usually /usr/local)

You can also use "ccmake .." or "cmake-gui .." for a more interactive configuration process.

A legacy Makefile is included, that will possibly work for you if you don't want to use cmake.  To use it just do (from this dir):

    make -f Makefile.legacy
    make -f Makefile.legacy install

You'll need the following libraries to build and install DrMr:

- [libsndfile](http://www.mega-nerd.com/libsndfile/)
- [libsamplerate](http://www.mega-nerd.com/SRC/index.html)
- [lv2](http://lv2plug.in/)
- [gtk+2](http://www.gtk.org)

Drumrox scans the following directories for hydrogen drum kits:

- /usr/share/hydrogen/data/drumkits/
- /usr/local/share/hydrogen/data/drumkits/
- /usr/share/drmr/drumkits/
- ~/.hydrogen/data/drumkits/
- ~/.drmr/drumkits/
- ~/.drumrox/drumkits/

If you want to add others, add them to the default_drumkit_locations array at the top of drumrox_hydrogen.c

### Note 1
Drumrox is currently using a static ttl file.  This means I have to decide statically how many gain/pan controls to expose.  I've settled on 32 for the moment, but that is arbitrary.  At some point DrMr will probably move to using the LV2 Dynamic Manifest feature to expose the appropriate number of gain controls for the current sample set, although how force a host update of the manifest when the kit is changed is unclear (if you know how, please let me know)

### Note 2
Drumrox only currently supports a subset of things that can be specified in a hydrogen drumkit.xml file.  Specifically, Drumrox will not use gain/pan/pitch/asdr information.  Drumrox basically only uses the filename and layer min/max information to build it's internal sample representation.
