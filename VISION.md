# RCKid's Story

> I want a handheld game console targetted at kids that will be all about creation, not consumption. Something that will be their first *personal* digital device. Like a phone, but simpler, safer, and hackable. A device that would grow with them, both in software where they would be able to progress to more and more complex forms of creativity with it, and in hardware extensibility. 

# How I got there

On xmas 2021 I have finished building an [mp3 player](https://github.com/zduka/mp3-player) for my kids. I have really enjoyed the work, and seeing my kids using the player gave something like a "meaning" to my life. But I was also left with lots of spare parts, in form of screens, MCUs, speakers, microphones, etc. To make use of this, and my free time, I jumped into my next project - I wanted to take some old LEGO technic motors and create a *R*emote *C*ontroller for them from the spare parts. But feature creep did hit hard: with dpad, mcu and a radio, I can add spare screen, kids love displays. Now I have display, MCU and dpad - that is enough to play some simple games and we have a console. Kids love sound, so let's add a speaker too. Now I have Speaker, MCU, display, and or course the radio module for the remote control. If only I can add a microphone, I can also make this into a walkie talkie, but there is no free pin left. But I can uprade to bigger MCU, problem solved. But bigger CPU can drive better display, kids love colors! But writing games for the beautiful color display would be hard as I am not good with drawing. But I can use emulators and play old games! Kids love old games! But the MCU is not powerful enough for this. Well we know what to do, I can swap it for even beefier one - RPi Zero! 

But programming for RPi Zero is no fun for me as a programmer. Also, when I built the [device](), I realized I do not want to give itto my kids - it plays lots of old games (and newer ones), it plays music, plays movies. But it is all about consumption. So I scrapped the project and started fresh, this time with a clear goal in mind. Out of this came the current version of RCKid (RC still stands for Remote Controller, too late to change name:). It is small enough to fit in children's pockets, polished enough so that it looks like a real product, not a DIY hack, or STEM educational tool. Powerful enough to be useful even for some consumption, but designed with creation first & foremost. 

# Current Status

Let's be honest. This is an enormous undertaking and at this point I am working on this only in me free (mostly night) time. I have a third generation prototypes that my two kids use pretty much daily. I have GB emulator so that games created in GB studio can be played. The device also has mp3 player, telegram messenger (with wifi cartridge), flashlight, TV remote (prototype), alarm clock, contacts and a bunch of other utility apps. I also have a single simple asset editor for icons.

I have used the self-imposed xmas deadline and Baby Jesus' logistical services (he delivers presents with a bit of magic where I live) to deliver the units to my two betatesters (and their cousin:) That's not many datapoints, but:

- both my kids take it with them *everywhere* and genuinely treat it as their *personal* device (with visual customizations of course, think early cellphone themes:)
- my 5y old uses regularly the humble icon editor to create what they call a map of their houses with livig rooms, towers, cellars, you name it.
- my 8y old is hooked on the games, wants to create their own (started learning scratch to that end) and because reading is required, her reading skills improved significantly thanks to the device

Currently I am rewriting the C++ SDK into a more coherent and ellegant API with fewer problems and hacks and I am almost done with that. 

# Growing with the Kids

But my vision for the future is far greater than this. I would not shy from calling it megalomaniac:) But the fact that I have persisted with this for so long already gives me some hope that I can deliver on this *big* picture as well. 

My plan is to evolve RCkid into an educational platform for kids and teens. There is ton of such devices already, but RCKid is unique because of:

- it has cartridges!!
- no seriously, cartirdges are great, because they add tangibility to the creations. When you create game is scratch you save it on your parents computer somewhere. When you create game for RCKid, you save it on *your* cartridge, that you can give to your friend to play. It also allows nearly endless hardware customization. I already have plain, WifI and NRF24L01P cartridges with LoRa, camera and FM radio in the future. 
- it has an edge over similar devices by being also useful outside of the class. Kids pick it for the consumption, but stay for the creation

I also plan for a ladder of creative tools that kids of very wide age ranges can use to create on the device:

- small kids w/o any literacy can start with asset editing (tilesets, tilemaps, sprites, sounds, music trackers) that will allow them to customize existing games I'll crete for the device (platformers, 2d top view, etc.). This teaches them decompositon and basic game aspects. 
- visual editor when behavior, not just looks of the games, can be tweaked without the need to be able to read & write. This again teaches decomposition, but this time of behavior into subactions in time
- block editor (like scratch), which adds yeth more agency to the development, teaches control flow, modularity so on. 
- simple language editor (C++ like syntactically DSL) that teaches describing algorithms in textual and syntactically correct way
- C++ simple editor, which teaches more advanced concepts
- and finally, a full C++ SDK that gives full control over the device

I think this is *big* - a 5y old kid that cannot read or write can create games that they feel are their own. And since every step above is a superset of the previous one, the games they create will grow with them as well, becoming more unique with every skill they learn for years to come. 

# Future Plans

I would like to finish the fundamentals, a simple SDK that allows painless creation of all sorts of apps, so that the device feels truly useful and coherent. I still need to iron out some last hardware glitches (microphone recording, DFM issues and getting rid of fthe omnipresent feature creep - FM radio moves from device to optional cartridge:). 

Then I would like validate the creation angle a bit more - implement assents editor, simple visual editor and basic game templates for 2D gaming for the youngest target audience. I hope I can make this under the self-imposed deadline of my two betatesters growing out of the target group eventually. 

Finally, 2 betatesters are better than 0, but I would like more - a small pilot in a local school would be something that truly evaluates the idea & its execution.

