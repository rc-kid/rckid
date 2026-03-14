# Development Ladder

This describes the core ideas behind the development ladder that RCKid will expose to the children for learning. Overall this is a 5 tier ladder of progressively more complex ways to creatively interact with the device to make things (games mostly:). Every next level is a superset of the previous one in two ways. It allows more control over the device while also extending the capability of the lower tiers. Some of those tiers can be used *on* the device so that extra computer access is not necessary, while others require external computer, for both UX and technical details (it is really hard to program code on device with no keyboard, *and* there is no compiler on RCKid to compile code to begin with). That said, pushing as much of the tiers to the device is important - the younger the audience, the less likely a stable access to a PC and understanding the complexity of cartridge flashing is.

Here is the brief overview of the development ladder tiers, they are then followed with dedicated sections with more detail:

- *asset editors* - at this point kids are not even expected to be able to read and write, they do not change the game mechanics, but may change the visual aspects of it by changing the assets themselves. This can be quite powerful, tilemaps, sprites, level editors, music and sound editors combined can fundamentally change the feel of simple games
- *visual editor* - the first time when kids can customize the behavior of the applications. New objects (player, enemiers, levels, etc.) can be added and their events & actions customized via simple uniform blocks (i.e. not syntax problems, any block can be used anywhere). Literacy should still not be required at this level.
- *blocks editor* - allows conditional execution in actions & events and more forms of composition & state. Blocks can be used to create visual components that can be used on the lower level.
- *C++ editor* - similarly to makecode, this simply allows expressing the blocks in C++, providing more ways of control flow & code composition, as well as teaching how to express algorithms textually. Also C++ can be used to define new blocks that can be used at the block level below
- *SDK* - the ultimate level, where the full RCKid SDK is accessible. Users can create entirely new game types, new hardware drivers, new applications.  

I plan this all to basically be C++ objects defined in the SDK with each level giving access to more API. The lower on device levels will be interpreted, while the C++ style tiers will be compiled and will extend the device's firmware.

Some design principles of the whole ladder, so that I keep reminding myself:

- every tier is a superset of the previous - games grow with the child
- higher tiers extend lower ones - blocks create visual components, C++ creates blocks, etc. 
- push towards on device editing - younger kids, less computer access. Also one part of the original success was that you could pick it up on the go for a few moments, and do things - on device editing preserves this
- the game is the continuity - moving to higher tier should not mean abandoning the game

## Game Apps

Before we jump to the details of the respective tiers, we should discuss the general requirements of the apps that will be supported by this. 

- start with just tilemap & sprite games, which can be tweaked to a large number of visually different games
- different physics can be provided on the visual level & above for stuff like platformers, 2d top down walkers, etc.

The game C++ code provides the rendering, but the actual functionality will be provided via function callbacks that can later be populated by visual & block interpreters, c++ functions created by the higher tiers, etc. 

## Tier Details

Sections below describe the tiers in slightly more detail, at least conceptually. They deal with the tier changes that are fundamental to the nature of the tiers, but more differentiators are possible. After the description I try to show it on a simple example of how a game made by the kids can be evolved throughout the tiers. 

But I would like to stress that the tier system is deliberately structured as going both ways, the key is that the game will grow with the kid as they progress in the ladder. Understanding new tier even slightly can unlock new things possible in the lower tier, where the child is comfortable now and thus the transition to the higher tiers will likely be gradual and it is entirely possible that when some parts of the game are expressed in C++, there will still be some visual action leftovers from the early days that did not need any changes till now. I believe the ability of the system to support this will be a sign of good internal design.

> Note this is work in progress.

### Assets Editor

The lowest tier, will provide asset editors for predefined types. At the minimum, those will be tilesets, tilemaps, sprites, sprite sequences, pictures, sounds and music. This is "safe" as it allows the kids to only change how the game looks. 

Each compatible game will have a manifest that will expose all the asset resources it uses and those will be editable

> For a specific example, consider a simple platformer. The game mechanics (what happens when buttons are pressed, that a player exists, enemies exist, interaction between them, the platforms in the level themselves) are all defined at the visual level, which also defines which sprite animations and other effects & visuals are required to run the game. Kids can only change those visuals, so instead of say "Mario", you can have a Beaver, instead of platforms you can have lakes with logs and dams, and instead of turtles you can have wasps. The music and sound effects can be different too. But the beaver still jumps and walks like mario and the enemies still walk the same and they all kill the player.

### Visual Editor

The visual editor exposes game objects and their events. Game objects are things like a level, player, enemy, etc. Each object internally knows how to render/play itself (this is not exposed at this level) and defines events. Each event can be a sequence of *visual blocks*. The visual blocks can be thought of as a simplified versions of the blocks known from Scratch, etc. Say they are just squares, so they compose simply, square after square with no syntax errors. But this also means there is no communication between them in a sense and no control flow other than uncodintional execution (aka basic blocks). 

Kids at this level can add/remove objects and can add visual blocks to the events exposed by the objects. This allows fundamentally changing the game behavior. Furthermore, the visual blocks reference assets. And crucially, they can reference *new* assets, i.e. the kids can at this point add/remove assets from the game manifest. Imagine adding a visual block movement to an event, in its properties, you can choose the animation, which can be any of the existing animtion assets. You can also add a new animation asset, extending the game. 

> For example, the beaver now cannot jump, but instead may swim and beat enemies with its tail (this adds need for new assets). The enemies may now stop being enemies, some of them can be friends (they do not kill the beaver on contact). For a more advanced uses, another tilemap can be provided for say parallax scrolling. 

### Blocks editor

Think scratch or makecode. The objects and events stay, but whereas in the previous level all actions were rectangles without any interaction between them, limiting the actions, now we have real control flow with conditional executoon and crucially a state. At this point the objects also expose properties through which their inner state can be modified even further. 

That said, the block editor adds one more functionality: A special "visual block" that contains blocks inside, but provides the interface identical to visual blocks to the outside. This means that kids can create visual blocks using the scratch blocks. Those visual blocks can then be used at the visual editor level. This is inspired by things like Delphi, where you could in Object Pascal create components that Delphi recognized as components and you could then use those without the need to understand their internals. 

While historically this has been done on computers, I would like to push this layer to the device as well - but without a mouse & dragging the blocks around, some smarter and not so frustrating way of building the blocks will be necessary.

> Now beaver can do cool things. It can build castles by changing the tilemap dynamically, it can cut trees and create dams. The interactions between enemies & friends can now be far more comples - friends can follow and help, enemies can hunt. Their movements can be expressed in much greater detail. In game state such as high scores, lives, etc. can be expressed at this level. 

### C++ Editor

The main purpose of this is to teach the kids how to express the ideas. It likely does not expose any new fundamental aspects of the game engine, the fact that kids are dealing with programming, types, statements, etc, is enough. It does however expose new compositions where the blocks fall short - bot high level via functions, and low level via all the possible C++ statements available. 

That said, kids at this level can create functions/objects that will allow to be plugged into the blocks interface below, i.e. they can create new blocks and call those blocks from the lower levels again. 

Because this is C++ now, this tier very likely has to happen on a PC. The c++ functions will be baked into the firmware and will have to be flashed. That said, I am not completely giving up on the idea of exposing this at least in some way on the device as well. I recall how my high school graphic calculator allowed programming by predefined keywords, which sped the typing. RCKid version with some blackberry-like keyboard is indeed possible, but I will admit I gave this thought perhaps more time than it deserves by now;-D

> Beaver can't technically do coller things than in the blocks level, *but* cooler and more complex forms of interaction are possible because of code being a lot better suited to expressing them. 

### Full C++ SDK

This is the ultimate tier, where the kids (teenagers likely at this point) can break free from the game engine they have been given at the asset level editor and that has grown with them till now. At this level, you can do three fundamentally novel things:

1) customize the game engine itself - you can create completely new objects, can change their events and the event logic itself. Can talk directly to the hardware for even more stunning visuals, etc. 
2) you can write your *own* hardware drivers - at the previous levels, unless it was available as a block, you could not use it (or unless there was SDK for it already on the C++ editor level). Here you can write drivers properly. The RP2350 is uniquely capable at this with the pio peripherals and the cartridge connector ensures that you have enough of those pins to do great things (the full HSTX is exposed). 
3) by using the SDK itself, you can write not just games, but also apps.

> The humble beaver has travelled far. Breaking from the "shackles" of interpretation at some level, you can make the visuals next level with more sprites, more background layers, etc. To put this in perspective, my estimate is that the game engine can give you games roughly comparable to game boy color, with full C++ you can sit beween GBA and DS. But you can also write genuinely useful apps - or app engines for different game types & physics. Music players, I believe video player might be a reality too, utilities, you name it. 