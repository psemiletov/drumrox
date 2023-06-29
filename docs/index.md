# Drumrox: LV2 drum machine

Drumrox is an easy way to write drum tracks as MIDI at your DAW, using Hydrogen and Drumrox kits.

Drumrox is LV2 drum machine (based on DrMr) to load Hydrogen and Drumrox drumkits. Drumrox is compatible with MODERN Hydrogen kit format. More of that, Drumrox has own simple drumkit format.


[User's manual](manual.md)

**Downloads**: [Drumrox github](https://github.com/psemiletov/drumrox) *
[OpenSUSE](https://build.opensuse.org/package/show/multimedia:proaudio/drumrox) *
[AUR](https://aur.archlinux.org/packages/drumrox)

**Links**: [Drumrox group at Telegram](https://t.me/drumrox) * [Drumrox kits](https://github.com/psemiletov/drumrox-kits) * [More Drumrox kits at Telegram](https://t.me/drumrox_kits)

## News

Drumrox 3.1.0 (multi-layers for Drumrox format)

Hello!

After the implementation of multi-layered samples support for Hydrogen kits, I made the similar thing for Drumrox own kit format. As the example you can try a new kit TamilMulti at [Drumrox kits](https://github.com/psemiletov/drumrox-kits). There also a simple "Tamil" kit without multi-layered samples. I'm big fan of Tamil movies, as well as Hindi :)

And speaking about multi-layered samples at Drumrox kits, the syntax at file format as usual simple. For the multi-layered samples, just separate their file names with comma, using the order from "quiet" sample to the "loudest" one (multi-layered samples are the set of samples those differs with the timbre, not the volume):

```kick=kick01.wav,kick02.wav,kick03.wav,kick04.wav
snare=share01.wav,share02.wav,share03.wav
hihat opened=hihat01.wav,hihat02.wav```

## Features

* LV2 plugin format (currently GTK2 only, works with Ardour)

* Stereo (with built-in mixer) and 32-channels versions

* Hydrogen drumkits support

* Drumrox drumkits support

* Up to 32 instruments with layers

* Automatic open hihat mute on hihat close

* Drumkit image at plugin window


![image](https://user-images.githubusercontent.com/8168336/246684340-0b81c208-a9e5-4c75-b843-8840223d13ac.png)


## Some history

Drumrox is based on Nicklan's DrMr (https://github.com/nicklan/drmr) and Filipe Coelho's DrMr (https://github.com/falkTX/drmr). The first one can save/load the preset by index, so when you install new kits or delete some, indexes are messing up. falkTX's DrMr deals with presets in more comfortable way, via the names. I (Peter Semiletov) used it everyday, but at the some point of time it becomes incompatible with new Hydrogen kits, and I've edited them manually to fix (removing some section in XML). Fresh install of Hydrogen converts my edited kits to the modern kit format again, and it was simplier to add some code than to edit the kits again. And when I've started to do that, I understand that I want to clean up the code, etc, etc. That how Drumrox continues DrMr...


