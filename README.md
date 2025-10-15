![Language](https://img.shields.io/badge/RETRO--DOS-orange)
![Language](https://img.shields.io/badge/C-green)
![Language](https://img.shields.io/badge/C%2B%2B-green)
![Language](https://img.shields.io/badge/WATCOM_11-blue)
![Language](https://img.shields.io/badge/x86-blue)
![Language](https://img.shields.io/badge/32bit-blue)
![Language](https://img.shields.io/badge/DOS4GW-blue)
![Language](https://img.shields.io/badge/code_quality-C-----red)
![Language](https://img.shields.io/badge/project_state-fubar-red)
![Language](https://img.shields.io/badge/mental_state-alarming-red)

# CYBERSNAKE 2000 TURBO

![CYBERSNAKE 2000 TURBO](_stuff/cybersnake.gif)
![CYBERSNAKE 2000 TURBO](_stuff/snake-game.gif)

by Marwellus <marwellus@disruption.global>

> If you know how to make MIDI music and are interested in creating some cool 
(dark electronic) tunes, besides seeing your name attached to a silly retro game,
feel free to contact me! :-)

## What the heck is this?
```
"Probaly the most (...) over-engineered snake clone in gaming history!"
- Frank Kabulski, Weird Bullshit Games, issue 11/95

Forget that shitty snake game from the past and dive deep into an apocalyptic 
future full of cyborg crap and such! You are CYBERSNAKE 2000 TURBO, the very
last hope of human kind, and have to save all by eating *cyber eggs* and
avoiding getting eaten yourself! But beware! Some of those eggs are tricky!

This game features:
* full blown VGA graphics with stunning 256 colors!
* massive amounts of effects!
* weirdly complex level design!
* digital sound effects!
* stolen MIDI soundtracks which are awesome!
* nerve wrecking, totally unfair game play!
* occasionally crashes and freezes that pump you up even more!

Jack in and crack the high score! An amazing experience you've never experienced
before awaits! Buy this game now or else! We've tons of bills to pay, the bank is 
already breathing down our necks because of the countless loans we had to take 
out for all the beer, pizzas and weed while we were developing this game!
```

## Okay, seriously.

Its a weird snake-like retro game with some extras based on a dead OS, written in
in age old C/C++ dialect, using ancient hardware, compiled with an obsolete
compiler. Thus, it runs only on PCs from the stone age.

**To be a bit more spezific:**

### Dev machine:
* Pentium 100 / 16mb / S3 Trio 64 / Soundblaster AWE 32 8mb
* DOS 6.22 (Win 3.11)
* build in C/C++ (some ASM) with Watcom 11 compiler

### Minimum target system:
* 486DX33 / 4mb / ET4000 (ISA16) / SB comp.
* DOS 6.22 (or compatible)

This game is (ridiculously) heavy on graphics for a snake clone, because of double buffering 
and certain effects, so at least a fast ISA16 graphics card is needed. A 386DX40 with a 
VLB graphics card will run this game quite well, actually - except for the special effects 
which actually need a FPU. If you ever wondered what difference a 387 FPU could make in a
snake game, this is your chance! ;-)

I will look into it later with more experience and knowledge, using more advanced/clever 
techniques, to squeeze out what I can. Like, using look-up tables or a simple 
approximation of the SIN()/COS() function, instead of actually using it :-p

The entire project is a learning experiment for myself. I just jumped into the
cold water and learned by just doing it, coming from an "entire different angle".
I didn't even touch C/C++ before, I "grew up" as a (more or less) self-taught
programmer starting with HTML/JS/PHP (late 90s) and later JAVA/C# (since ~2010).

Unfortunately, I don't have any real hardware (anymore), so I am actually using
86Box <https://86box.net/> to emulate these machines. Which is an awesome emulator
by the way, and combined with a proper Reshade <https://reshade.me/> filter it
will get you as close to the original experience as it can. BUT - who knows, I
might actually invest in some real old hardware some day, it would be great to
see this thing running on a real DOS machine.

### Features:
* DOS/4GW, 32bit
* Mode 13h VGA "engine" w/ double buffer
* DSP & OPL3 support
* AWE32 MIDI support (using original AWE32lib)
* GUS support (work in progess, using GUS SDK)
* PCX / GIF loader
* Asset loading / cache system
* framework like design 
  - C is used for code closer to HW
  - C++ for higher functions

Most code I've written myself, so almost no special libs (other than AWE32lib / GUS SDK)
are used. "Only" the midi engine is stolen and ported from the Allegro FW
<https://liballeg.org/>, an amazing piece of work by Shawn Hargreaves.
Other than this I'm really trying to get stuff done by myself, because that's 
a large part of the fun.

Huge thanks to all the people who saved a lot of the old documentations from 80s/90s HW/SW 
and put it online! And of course thanks to all who made their code public for everyone 
to study and learn. This little thingy here is my modest contribution to it, maybe more
follows. Coding like they did in the "old days" just feels more real tbh.

## Code style explanation

Coming from JS/TS & JAVA/C# I didn't really switch over to known C/C++ coding conventions, 
I went along with my own conventions (feel free to hate them):
```
C:
Snake_Case: public props & methods
_camelCase: private props
snake_case: private methods & local variables

CPP:
PascalCase: public props & methods
_camelCase: private props
camelCase : private methods
snake_case: local variables
```
Regarding the rest of the code, I just followed personal preferences and enjoyed the 
freedom, while still trying to keep it readable and clear, at least for myself. ;-)