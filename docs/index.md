# Drumrox: LV2 drum machine

Drumrox is LV2 drum machine (based on DrMr) to load Hydrogen drumkits. The main goal of Drumrox is to keep it compatible with MODERN Hydrogen kit format. More of that, Drumrox has own simple drumkit format and the repository with [Drumrox kits](https://github.com/psemiletov/drumrox-kits)


[Drumrox github](https://github.com/psemiletov/drumrox) *
[OpenSUSE](https://build.opensuse.org/package/show/multimedia:proaudio/drumrox) *
[AUR](https://aur.archlinux.org/packages/drumrox)


![image](https://user-images.githubusercontent.com/8168336/244692820-9aa0c6a3-27cd-451c-9c9f-6149c374bd63.png)

## Features

* LV2 plugin format (currenty GTK2 only, works with Ardour)

* Hydrogen drumkits support

* Drumrox drumkits support

* Up to 32 instruments with layers



## Some history

Drumrox is based on Nicklan's DrMr (https://github.com/nicklan/drmr) and Filipe Coelho's DrMr (https://github.com/falkTX/drmr). The first one can save/load the preset by index, so when you install new kits or delete some, indexes are messing up. falkTX's DrMr deals with presets in more comfortable way, via the names. I (Peter Semiletov) used it everyday, but at the some point of time it becomes incompatible with new Hydrogen kits, and I've edited them manually to fix (removing some section in XML). Fresh install of Hydrogen converts my edited kits to the modern kit format again, and it was simplier to add some code than to edit the kits again. And when I've started to do that, I understand that I want to clean up the code, etc, etc. That how Drumrox continues DrMr...


