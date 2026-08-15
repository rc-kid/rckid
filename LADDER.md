# Programming Ladder

This describes the core ideas behind the programming ladder built into RCKId's SDK that enables on-device application programming and algorithmics across wide age & skill ranges. This is powered by a single DSL that is exposed to the users via different editor levels and capability tiers. Think of this as more fine grained and on-device oriented MakeCode (which already switches between blocks and TypeScript) combined with Hedy levels. Not very editor supports every level, but the exact level & editor boundaries are not in the current scope of this document. 

The different editors are the cornerstone of the entire design. They support the users across the wide range of cognitive and literacy skills. The editors and their on-device capabilities shape the language design itself (and its levels) and give children agency as especially younger kids generally lack unconstrained access to a PC: 

    Editor        | Host   | Literacy | Description
    --------------|--------|----------|-------------------------------------------
    asset editor  | device | pre      | Edits app static assets (images, sounds, tilemaps, etc.)
    iconic editor | device | pre      | Edits parts of code with iconic representation via chevrons
    visual editor | device | lit      | Edits code via chevron based visual blocks (Scratch for small screens)
    token editor  | device | lit      | Edits code with input terminal by terminal (for on-device text editing)
    text editor   | pc (*) | lit      | Edits code as normal text editor does (can produce invalid programs)

> (*) This is mostly because editing character by character on device without qwerty keyboard is really painful. I am however considering an RCKid mod where something like blackberry keyboard can be overlaid over the PCB for a full keyboard experience after which the text editor can easily run on-device. 

All editors are just projections into single language and runtime shared across all levels. This ensures that as the children move to more complex levels and editors, they can take their creations with them, not leave them behind. 

The design also emphasizes extensibility and encapsulation. Children can naturally wrap more advanced functionality into simpler snippets that can be physically shared with peers (via cartridges) and used in their designs. As a simple example, a control flow requiring mechanics can be encapsulated into action and used by people at lower levels where the action hides the control flow. When the users advance to control flow, the so far hidden control flow will become visible and editable. 

> Another example can be adding an icon to the action can make the action (or existing action without an icon) available in the pre-literate iconic editor as well. This allows literate child to empower pre-literate friends.

# Runtime

The prgram is a collection of *game objects*, which are either provided by the SDK itself, or can be created in the DSL itself at more advanced levels. Some game objects are always present (such as Device), while others must be created explicitly. 

Game objects have events and actions (like say slots & signals in QT) and free functions that do not belong to any game object are also possible. The bulk of the program is then connecting game object events to desired game object actions (or ad hoc functions). 

Each game object also defines its *assets*, such as music, sprites, tilemaps and tilesets, etc. Assets themselves are game objects and so new assets can be instantiated (so that sprite game object can swap bitmap assets, etc.). 

The language is always interpreted so that device re-flashing is not necessary and special care is taken in the runtime design to place as much of the immutable data as possible to flash memory as opposed to relatively scarce ram. 

> Note to self: I have proof of concept of RTTI descriptors in flash, but need to verify this works for extensions and objects created at runtime in the DSL itself as well (here they will be obviously in RAM, but should work the same).

The runtime is event based where game objects can also define their *events* and *actions*, akin to signals & slots in QT. An action is simply a method of the game object that can be called as a function. An event can either be assigned a handler function, or be called like a method, which raises the event and calls the handler (*).

> (*) should events be single handler, or should multiple handlers be allowed?

# Editors

All programs are stored in their canonical form with is the text editor's character by character representation (normal text). The editors operate on a grammatical and semantic subset of the full language, which they visualize to fit their needs (such as prefix expression notation in the visual editor, etc.)

This has some important consequences for the game development:

(1) The work done at the lower levels does not have to be lost when the user advances to next representation - each layer is a superset of the layer beneath it, e.g. the visual editor can show everything the iconic editor can, etc. This means the games grow with the children, such as the beaver game used in the examples below. 

(2) Single game does not have to exist at single level. Of course the assets will always be assets, but even the iconic/visual/text can change from event to event based on the complexity of the events, or merely the time at which it was touched.

(3) Work done at higher abstraction levels (and presumably editors) can be encapsulates into blocks available at lower language levels/editors. This can have multiple forms, such as for instance, providing an icon (via the asset editors) for some action/event that does not have one, and thus enabling the even in the iconic editor, encapsulating control flow into an action so that it can be used in non-control flow enabled levels, etc.

> The ultimate extension is creating new game objects using the C++ and the SDK itself. While I am not expecting the students will do that, it is very useful for extending the runtime in capabilities both software (new rendering techniques), and in hardware (game objects for peripherals, etc.).

Unlike the traditional languages, due to RCKid's limited hardware capabilities (especially screen estate, and number of input keys), the editors shape the language itself. Furthermore the editors are designed so that every new editor level requires more abstract thinking from its users, while being able to express more complex code.

## On-Device Editing

RCKid with its small non-touch screen and very limited input controls cannot use the default text editing with character by character typing on the keyboard, nor can it support the drag & drop user interface known from visual editors running on PC or tablets such as Scratch or Makecode. 

Instead, the iconic, visual and token editors all work around the same principle of showing the code in blocks (icons, visual blocks or tokens) with holes (denoted as `...` in the examples below) in the visualized code at places where new stuff can be added. Particular block or hole can be selected via dpad and it can be changed/filled by invoking a context menu, which will show options valid for that block (and possibly change the blocks around it as necessary).  

The hole based on-device editing works the best in prefix notation, whereas the hole is filled with the function/operator, which also knows how many new holes are necessary. This also avoids the clutter of parentheses, which are now not necessary. 

> I am not sure if this is the best way to go forward, but I am nore and more willing to try it out. The language itself does not have to ne prefixed and the text editor (latest tier) can be infix based with the lower tiers automatically converting to prefix.

## Asset Editor

Each game object in the program defines assets it uses. Editing this collection requires no programming/algorithmics skills. Yet it gently teaches basic decomposition strategies (sprite is collection of pixels, sprite animation is sequence of images, music is sequence of notes, tilemap is 2D array of tiles, etc.). In terms of the language, assets can be thought of as static constexpr data (*), which can be edited in the corresponding editors. 

This is a *safe* tier as it only allows changing the looks of the game, not the code itself. Furthermore the tier is pre-literate in its nature.

> (*) Instead of polluting the codebase with static arrays describing the binary data, I am thinking of either some specialized constructors (like say the SDK allows editing pixels in fonts & tilemaps), or even offsetting to external files, which the asset editors can edit. 

> For a specific example, consider a simple platformer. The game mechanics (what happens when buttons are pressed, that a player exists, enemies exist, interaction between them, the platforms in the level themselves) are all defined at the asset level, which also defines which sprite animations and other effects & visuals are required to run the game. Kids can only change those visuals, so instead of say "Mario", you can have a Beaver, instead of platforms you can have lakes with logs and dams, and instead of turtles you can have wasps. The music and sound effects can be different too. But the beaver still jumps and walks like mario and the enemies still walk the same and they all kill the player upon touch

## Iconic Editor

The iconic editor is identical in form and function to the visual editor with single difference and limitation: Instead of text, all actions, events and types are visualized using their icons. This also limits the scope of the editor as actions, events, types, values, etc. that do not have corresponding icons defined cannot be used at this level.

The iconic editor is basically a simple *when-do* style programming based on the game object's event system that should be very easy for children to understand even at pre-literate stage.

For more details on the iconic editor working see the visual editor.

> I'm thinking that control flow (other than perhaps a sequence) will not be available for this editor - but this is a deliberate decision of not to provide icon for the construct rather than fundamental limitation. I am also thinking that perhaps some values can be instantiated, such as simple text editing and numeric values. This is optional and should bridge more softly the gap between pre- and literate- children (often kids can write and read their names which they are likely to use in their creations before they can really read and write, and they understand small numbers)

> For our example, the children can now make sure the beaver does not die and wasps can be its friends. Instead a fox may appear that can kill the beaver, but the random moving wasps can sting the fox instead. For a more advanced uses, another tilemap can be provided for say parallax scrolling.

## Visual Editor

The visual editor is inspired by Scratch, Makecode, Blockly, Kodu and others where different shapes can be connected together and different shapes enforce legal composition. The visualized blocks can be navigated via the dpad and highlighted block/hole can be changed as decripbed in the _On Device Editing_ section.

As the visual level, kids should already be familiar with the when-do style of the iconic editor and will be introduced to actual programming concepts, such as expressions, control flow statements, variables, functions, etc.

The prefix nature of the language makes the editing and visualization straightforward. The visual editor uses simple chevron powerline-like visualization that saves screen real estate. Each statement starts and ends with the `|` character. If any expression part requires operands, it is followed by `>` as many times as there are operands. This is shown on the sequence below that creates a simple function call `foo.bar(4 + 5, 67)`:

    | ... |                               # Initial hole
    | foo > ... |                         # selected object foo, reqires action specification
    | foo > bar > ... > ... |             # selected action, we know it takes 2 arguments
    | foo > bar >> + > ... > ... |> ... | # added +, takes two arguments
    | foo > bar >> + > 4 > 5 |> ... |     # addition arguments added
    | foo > bar >> + > 4 > 5 |> 67 |      # whole expression done

Note the `>>` chevron above, which indicates nesting. While in most visual editors, nesting is done by graphically increasing the height of the statements so that nesting can be visualized, on RCKid's small screen, nesting is visualized by color change of the parts, which is indicated by the `>>` chevron which indicates color change. Note that this chevron does notreally exist and is only in the ASCII "art" representing the notation. Users will instead see a color change of the blocks. 

When the nested call is done, it can either be followed by another operand of the parent block, in which case we see `|>` at the end, where the pipe signals the end of the nested call, and the `>` sigals continuation of the parent call, and will be drawn in the parent call. Or it can be `||` where the nested *and* parent calls end at the same block. Note that a combination of those is possible, i.e. `|||>` which closes 3 expressions and continues the fourth one. The `||` can be rendered as a small bar block of the corresponding color. Examples below:

    | foo > bar >> + >> - > b > a |> c |> d ||
    # foo.bar(+ - b a c, d) in prefix
    # foo.bar((b - a)) + c, d) in infix

Visualized nesting happens when newly inserted element expects at least one argument, i.e. :

    | foo > bar > 5 |           # no nesting, 5 is literal value
    | foo > bar >> + > 5 > 6 ||   # nesting, + expects 2 operands
    | foo > bar >> foo > baz || # nesting (foo expects action specifier, baz expects no arguments)

An extension of the simple visual editor is the use of control flow. The simplest control flow statement is the sequential execution of multiple statements (code block). This is visualized by a small `||` narrow block to the left with `\/` joins (visualized in ASCIIart as the joins alone):

    \/ | log > print > "first" |          # first statement
    \/ | log > print > "second" |         # second statement
       | ... |                            # hole for the next, third statement, or empty

The `\/` short side block can also be selected and will offer control-flow specific actions such as adding before, after, removing, etc.

More complex control flow such as if statements or loops is visualized similarly:

    \/ | log > print > "pre-if" |            # log.print("pre-if");
    | if > == >> foo > bar >> 56 |           # if (foo.bar == 56) {
    || \/ | log > print > "its 56" |         #     log.print("its 56");
    ||    | ... |                            #     // hole for adding statements to true case
    | else |                                 # } else {
    |  \/ | log > print > "something else" | #     log.print("something else")
    |     | ... |                            #     // gole for adding statements to false case
    | end if |                               # }
       | ... |                               # // hole for after if statements

Further extensions to the visual editor can be introducing function definitions and perhaps even entire game object classes. Those will follow the same princples. 

It is important to note that the connection shape (always `>`) has nothing to do with types as opposed to the more conventional computer/tablet based block systems. Instead, types are used to limit the availability of options that can be selected from the input menus to ensure that a well-typed program will be produced when all `> ... ` hole types (i.e. holes in required positions) are filled in. To demonstrate, consider this example:

    | ... |       # first hole to add a statement
                  # this can be all available game objects (say log, device, player)
                  # all control flow statements (if, switch, loop, etc.)
                  # function definition and other global statements, etc.
    | log > ... | # we have selected log, but simply using game object in value position is not allowed here
                  # so action is required, where available options will be *all* actions of log
    | log > print > ... | # we have selected print, which takes 1 argument of type string
                          # so now *any* objects that have actions that produce string
                          # and any string creating functions will be available
                          # as well as string literals, variables that are strings, etc.
    | log > print >> device > ... || # we selected device, device is not string, must cal action
                                     # so now the options will only be device actions that return string
    | log > print >> device > id ||

The program in the visual editor is a collection of mapping between game object's events and their handlers, which form the actual source code. When events have arguments, pattern matching can be employed to only react to specific conditions, so for instance the following is possible:

    | device > onKeyDown > ButtonUp > : | ... | # where we react only to button up

> For the beaver game we can add a lot now, including for instance the ability of the beaver to hit back at its enemies with its powerful tail. Beaver can swim when in water and walk when on land. Instead of unique lives, we can have an HP gauge, etc. The movement of the wasps and foxes can be changed, so that foxes fear the wasps, etc.  

## Token Editor

The token editor works very similarly to the visual and iconic editors, with same hole editing principles. The difference here is that program code is entered as text in a token by token basis. So for instance the previous if example can be visualized as this:

    log.print("pre-if")
    if == foo.bar() 56
        log.print("its 56")
        ...
    else
        log.print("something else)
        ...
    ...

> Note that the `.` dot operator for membership has changed to infix now. I am not sure yet if that is a feature (leaning towards yes), but enough to say that there its infix-ness is not a problem as it is still mandatory in places where types would require it, you can think of the tokenizationof the above as `log` `.print` etc. The infix dot being part of the member selector. 

Thanks to the prefix notation and the DSL gramar used, this should look *very* similar to the visual representation already, just written purely as text and with operators & keywords that were missing from the visual description (such as the `.` operator, parentheses, etc.). 

Despite showing the actuall DSL code as text, the editing still happens at the hole insertion. When hole (`...`) is selected, the editor shows the available options that can be used to fill the hole, determined on the place of the hole both syntactically and type-wise, in the same manner it was used in the iconic and visual editors. 

Formatting will be dobe by the editor itself and the chevron style guides are no longer visible. But when selected, the call and its arguments can be highlighted in colors.

> This of course depends on the exact syntax, subject to be refined later. 

> For the beaver game, not that many new cool things can happen - the visual editor should already be turning complete. But the more complex the game mechanics get, the more useful the text editor can be by fitting more text on the screen and by utilizing more advanced language features (but those can in theory all exist also at the visual level). 

## Text Editor

This is the final tier that is identical to programming editors as we know them where the program code is entered character by character. Not expected to run on the device as the lack of qwerty keyboard makes character by character editing annoying. Yet the editor tier is pedagogically important because:

- it introduces the notion of invalid programs due to the free form editing, whereas the previous editing methods never allowed inserting invalid token in place.
- by the definition, can support the language in its entire complexity as full language simply means all productions and nonterminals are allowed.

> For the beaver, the only thig that changes is that very complex game interactions are possible now with more ease.

# Language Reference

I am thinking event based, simple syntax that will read nicely, infix syntax in pure text (but rendered as prefix in the visual & token editors). No curly braces because of off-side rule like Python. Dot operator for membership. Pattern matching. `:=` for assignment and `==` for comparison. Static typing with inference, algebraic data types.

> TODO to be done later

# Levels

The language itself can be viewed in *levels*, similarly to the Hedy and Racket, but more similarly to Hedy where the levels are more fine grained and introduce much more basic concepts, unlike the Racket's Beginner language which already does quite a lot. 

Furthermore, certain levels can only be expressed in particular editors. It still holds that any higher-level editor can express everything te editor below it can, but only the text editor at the very end is required to be complete. 

> But I hope the token-based editor can be complete, or near complete too. At the moment I am thinking the visual editor should eventually support everything up to creating functions and extending existing game objects, while maybe creating completely new game objects can be omitted. The iconic editor on the other hand will be rather restrictied, only show things for which icons will be defined and will likely lack control flow (maybe sequence ok) and variables (maybe passing them ok).

The below is not in any particular order, just ideas for features that can define levels:

- assets only (no code editing only static data)
- game objects and their events & actions, explicit values
- types, complex constructors (such as point, etc.)
- variables
- sequential execution
- conditional statements
- loops
- writing own functions
- extending game objects with events and actions
- writing own game objects
- memory management
- collections

> TODO to be done later, can get inspire dby hedy & racket stuff

# History

Language has switched to prefix notation to simplify the editing paradigm. The `<` chevron is no longer used to show that an expression returns value - this is not needed as the type based selectors will only show valid options anyways.

In the olden days I was assuming full C++ SDK as the latest level of the mastery ladder. I have abandoned this as the full C++ SDK is arguably a diffrent programming language and experience at *all* (flashing, really complex memory management, hardware interaction, etc.). 