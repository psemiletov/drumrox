# Drumrox: LV2 drum machine

Drumrox is LV2 drum machine (based on DrMr) to load Hydrogen drumkits. The main goal of Drumrox is to keep it compatible with MODERN Hydrogen kit format. More of that, Drumrox has own simple drumkit format and the repository with [Drumrox kits](https://github.com/psemiletov/drumrox-kits)


[Drumrox github](https://github.com/psemiletov/drumrox) *
[OpenSUSE](https://build.opensuse.org/package/show/multimedia:proaudio/drumrox) *
[AUR](https://aur.archlinux.org/packages/drumrox)

## News

Drumrox 2.1.0 + 3 drumkits (Lel PSR, Lel DR8, LinnDrum)

Hello!
Besides many fixes and code refactoring, there are some interesting new features.

1. First of all, I've made a [Drumrox site] (https://psemiletov.github.io/drumrox/) which is more clear as the github's project page.

2. Also, here is a repository for [Drumrox kits] (https://github.com/psemiletov/drumrox-kits). Yes, Drumrox 2.1 introduces, in addition to Hydrogen format support, the new drumkit format. Hydrogen is a larger program than Drumrox, so it needs more complex format with wide set of features. Instead of that, Drumrox provides simple format in the form of directory, which contains the samples and drumkit.txt file. The last one looks like that:

```Kick=kick.wav
Snare=snare.wav
Crash=crash.wav
```

So we can quickly create and use new kits.

3. Currently at the Drumrox kits repo are 3 kits, let me introduce them! Lel PSR - the legendary Soviet drum machine, used at early albums of famous USSR rock bands such as Kino, Strannye Igry, Aquarium. Lel DR8 - the continuation of Lel PSR. LinnDrum - drum machine manufactured by Linn Electronics between 1982 and 1985.
LinnDrum was used by Peter Gabriel, Stevie Wonder, Gary Numan, Michael Jackson, Devo, John Carpenter, Prince, Madonna, Elton John, Queen (at Radio Ga Ga).

4. Search paths for drumkits are:

```/usr/share/hydrogen/data/drumkits
/usr/local/share/hydrogen/data/drumkits
/usr/share/drmr/drumkits
/usr/share/drumrox-kits
$HOME/.hydrogen/data/drumkits
$HOME/.drmr/drumkits
$HOME/.drumrox/drumkits
$HOME/drumrox-kits
```

5. Drumrox kit can have the picture (named "image.png") of drum machine/kit, that appears at plugin window.

6. Future plans - porting GUI part from GTK2 to another kit (Nuklear? FLTK?), fixes, rewritting some code that I hardly understand, etc. More new kits!


![image](https://user-images.githubusercontent.com/8168336/244692820-9aa0c6a3-27cd-451c-9c9f-6149c374bd63.png)

## Features

* LV2 plugin format (currenty GTK2 only, works with Ardour)

* Hydrogen drumkits support

* Drumrox drumkits support

* Up to 32 instruments with layers


## Some history

Drumrox is based on Nicklan's DrMr (https://github.com/nicklan/drmr) and Filipe Coelho's DrMr (https://github.com/falkTX/drmr). The first one can save/load the preset by index, so when you install new kits or delete some, indexes are messing up. falkTX's DrMr deals with presets in more comfortable way, via the names. I (Peter Semiletov) used it everyday, but at the some point of time it becomes incompatible with new Hydrogen kits, and I've edited them manually to fix (removing some section in XML). Fresh install of Hydrogen converts my edited kits to the modern kit format again, and it was simplier to add some code than to edit the kits again. And when I've started to do that, I understand that I want to clean up the code, etc, etc. That how Drumrox continues DrMr...


