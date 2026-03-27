# RCKid's Story

> I want a handheld game console targeted at kids that will be all about creation, not consumption. Something that will be their first personal digital device. Like a phone, but simpler, safer, and hackable. A device that would grow with them, both in software where they would be able to progress to more and more complex forms of creativity with it, and in hardware extensibility.

## How I Got There

On Christmas 2021 I finished building an mp3 player for my kids. I really enjoyed the work, and seeing my kids using the player gave something like a "meaning" to my life. But I was also left with lots of spare parts - screens, MCUs, speakers, microphones. To make use of these, I jumped into my next project: taking some old LEGO technic motors and creating a **R**emote **C**ontroller for **kid**s from the spare parts. But feature creep hit hard. With a dpad, MCU and radio, I could add a spare screen - kids love displays. Now I had a display, MCU and dpad - enough to play some simple games. Kids love sound, so let's add a speaker. If only I could add a microphone, I could make it a walkie talkie too, but there was no free pin left. Upgrade to bigger MCU, problem solved. Bigger CPU can drive a better display - kids love colors! Writing games for a color display would be hard without drawing skills, but I could use emulators and play old games! Kids love old games! But the MCU wasn't powerful enough. Well, swap it for an even beefier one - RPi Zero!

But programming for RPi Zero was no fun for me as a programmer. And when I built the device, I realized I didn't want to give it to my kids. It played lots of old games, music, movies. But it was all about consumption. So I scrapped the project and started fresh.

Out of this came the current version of RCKid (RC still stands for Remote Controller - too late to change the name :). It is small enough to fit in children's pockets, polished enough to look like a real product rather than a DIY hack or an obvious STEM educational tool. Powerful enough to be genuinely useful, but designed with creation first and foremost.

## Current Status

Let's be honest. This is an enormous undertaking and at this point I am working on it only in my free - mostly night - time. I have third generation prototypes that my two kids use pretty much daily. The device has a GB emulator so that games created in GB Studio can be played, an mp3 player, Telegram messenger (with WiFi cartridge), flashlight, TV remote, alarm clock, contacts and a bunch of other utility apps. I also have a simple asset editor for icons.

I used the self-imposed Christmas deadline and Baby Jesus' logistical services (he delivers presents with a bit of magic where I live) to deliver units to my two beta testers - and their cousin. That's not many data points, but:

- Both my kids take it with them everywhere and genuinely treat it as their personal device - with visual customizations, think early cellphone themes
- My 5 year old regularly uses the humble icon editor to create what he calls maps of houses - living rooms, towers, cellars, you name it
- My 8 year old is hooked on the games, wants to create her own (started learning Scratch to that end), and because reading is required, her reading skills have improved significantly thanks to the device

## Growing With the Kids

My vision for the future is far greater than this. I would not shy from calling it megalomaniac. But the fact that I have persisted with this for so long already gives me some hope that I can deliver on this big picture as well.
My plan is to evolve RCKid into an educational platform for kids and teens. There are plenty of such devices already, but RCKid is unique because:

Wait for it... It has cartridges! 

Cartridges add tangibility to creations. When you create a game in Scratch you save it on your parents' computer somewhere. When you create a game for RCKid, you save it on your cartridge - one you can give to a friend to play. The cartridge system also allows nearly endless hardware customization. I already have plain, WiFi and NRF24L01P cartridges, with LoRa, camera and FM radio planned.

I also plan a ladder of creative tools that kids across a wide age range can use (details in [Development Ladder](LADDER.md)):

- Small kids without any literacy can start with asset editing - tilesets, tilemaps, sprites, sounds, music trackers - that will allow them to customize existing games. This teaches decomposition and basic game concepts without requiring reading or writing.
- A visual editor where behavior, not just appearance, can be tweaked without literacy. This teaches decomposition of behavior into sub-actions in time.
- A block editor (like Scratch), which adds more agency, teaching control flow, modularity and so on.
- A language editor with a DSL that teaches describing algorithms textually and correctly.
- A C++ editor, which teaches more advanced concepts.
- And finally, a full C++ SDK that gives complete control over the device.

I think this is big. A 5 year old who cannot read or write can create games that feel genuinely their own. And since every step above is a superset of the previous one, the games they create will grow with them for years to come, becoming more uniquely theirs with every new skill they learn.

## Personal Ownership Matters

I built a device for consumption and then realized as a parent I didn't want that for my kids - so I rebuilt it for creation. As an educator I started thinking about how to nurture that creative instinct systematically, which led to the progression ladder and the personal ownership model:

RCKid is intended to be *personal* device -  in fact it will likely be the *first* personal digital device for the youngest kids. It is not supposed to be a kit they leave at school after the lessons are over. It should be *theirs* to take home. That way the personal ownership blurs the line between school and home. This increases participation, which leads to creating and learning: Kids can spend their free time improving their projects and then impressing their friends with their creations - and because cartridges are physical and shareable, they can give their games to their friends who will run them on their devices. There is nothing more rewarding than seeing *your* creation being used by somebody else.  

That is why I have designed RCKid to look as polished and product-like as I can and why I put emphasis on the device being generally useable - not just narrowly focused STEM educational kit. But none of the diy hackability is lost - it is literally a cartridge swap away!

But the story does not end here. I have realized that personal ownership matters most to the kids who have the least. A child with hundreds of toys gets one more device. A child with few possessions takes RCKid seriously in a way that makes the whole mission work better. The kids who need it most are also the kids for whom the personal ownership model is most powerful.

This is why one of RCKid's long term goals is to ensure that cost is never a barrier. A device given freely to a child who has little is not just a gift of a toy - if the premise works, it is a gift of a better future through learning. Validating that premise carefully and honestly, starting with what works and building toward who needs it most, is the real mission.

## Future Plans

I would like to finish the fundamentals - a solid SDK that allows painless creation of all sorts of apps, so that the device feels truly useful and coherent. I still need to iron out some hardware details (working microphone, DFM improvements). Then I want to validate the creation angle more concretely: implement asset editors, a simple visual editor, and basic game templates for the youngest target audience.

Two beta testers are better than zero, but I would like more. A small pilot in a local school - ideally with kids aged 7-9, where the progression from asset editing toward behavior customization can be observed naturally within a single school year - is the next concrete step. Starting with motivated kids in a supported environment, learning what works, and then taking those lessons toward the kids who need it most.

I hope to make this under the self-imposed deadline of my two beta testers growing out of the target group eventually:)

## What about AI?

We live in times when you might wonder why bother teaching kids STEM when most programming might soon be done by LLMs. But you do not teach 6 year olds how to program — you teach them the art of algorithm description. Most kids who will have RCKid will not become programmers. This was true before LLMs and remains true after. The whole point of STEM education is to teach universally useful thinking skills in ways that do not require deep technical knowledge. Decomposition, sequencing, abstraction — these are cognitive skills that predate programming and will outlast any specific technology.
