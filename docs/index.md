# Drumrox: LV2 drum machine

Drumrox is an easy way to write drum tracks as MIDI at your DAW, using Hydrogen and Drumrox kits. Currently Drumrox works **normally with Ardour and Mixbus only**! Consider to use my another drum machine, [Drumlabooh](https://psemiletov.github.io/drumlabooh/), that compatible with all DAW's.

Drumrox is LV2 drum machine (based on DrMr) to load Hydrogen and Drumrox drumkits. Drumrox is compatible with MODERN Hydrogen kit format. More of that, Drumrox has own simple drumkit format and supports SFZ.


[User's manual](manual.md)

**Downloads**: [Drumrox github](https://github.com/psemiletov/drumrox) *
[OpenSUSE](https://build.opensuse.org/package/show/multimedia:proaudio/drumrox) *
[AUR](https://aur.archlinux.org/packages/drumrox)

**Links**: [Drumrox group at Telegram](https://t.me/drumrox) * [Drumrox kits](https://github.com/psemiletov/drum_sklad) * [More Drumrox kits at Telegram](https://t.me/drum_sklad)

## News

Drumrox 3.3.0 

* MIDI velocity at Multi-channel fix

Peter Semiletov, Kiev, 06 april 2024

## Features

* LV2 plugin format (currently with GTK2 GUI, works with Ardour and Mixbus only)

* Stereo (with built-in mixer) and 32-channels versions

* Supported sample kit formats: Hydrogen, Drumrox, SFZ

* Up to 32 instruments with layers

* Automatic open hihat mute on hihat close

* Drumkit image at plugin window


![image](https://user-images.githubusercontent.com/8168336/250968814-1b15341c-f59e-413b-8276-807a05089021.png)


## Some history

Drumrox is based on Nicklan's DrMr (https://github.com/nicklan/drmr) and Filipe Coelho's DrMr (https://github.com/falkTX/drmr). The first one can save/load the preset by index, so when you install new kits or delete some, indexes are messing up. falkTX's DrMr deals with presets in more comfortable way, via the names. I (Peter Semiletov) used it everyday, but at the some point of time it becomes incompatible with new Hydrogen kits, and I've edited them manually to fix (removing some section in XML). Fresh install of Hydrogen converts my edited kits to the modern kit format again, and it was simplier to add some code than to edit the kits again. And when I've started to do that, I understand that I want to clean up the code, etc, etc. That how Drumrox continues DrMr...

