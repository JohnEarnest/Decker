Decker
======
Decker is a multimedia platform for creating and sharing interactive documents, with sound, images, hypertext, and scripted behavior.

![Decker, complete with toolbars](images/wings.gif)

You can learn more about Decker on [my website](http://beyondloom.com/decker/), on the [community forum](https://internet-janitor.itch.io/decker/community), or you can just dive in and [try it online](http://beyondloom.com/decker/tour.html). Periodic binary releases of Decker for MacOS and Windows are available on [Itch.io](https://internet-janitor.itch.io/decker).

If you're interested in _Lil_, Decker's scripting language, you can access documentation and play with it in your browser at [trylil](http://beyondloom.com/tools/trylil.html).


Web-Decker
----------
Decker is available as [a web application](http://beyondloom.com/decker/tour.html) (written in vanilla JavaScript) which is distributed as a single freestanding HTML file. Web-Decker can be built with a `make` script. The test suite uses [Node.js](https://nodejs.org/en/):

```
make testjs
make web-decker
make runweb      # (optional) open in your default browser
```


Native-Decker
-------------
Decker is also available as a native application, written in C. Building Native-Decker from source requires:

- a c compiler and libc
- the `xxd` utility (standard with MacOS and most \*nix distros)
- [SDL2](https://www.libsdl.org/download-2.0.php)
- [SDL2_image](https://github.com/libsdl-org/SDL_image)

On MacOS, BSD, or Linux, fetch the appropriate SDL2 packages and then build with `make`. This has also been reported to build and run successfully under WSL:

```
brew install sdl2 sdl2_image                                   # MacOS/Homebrew
sudo apt install libsdl2-2.0-0 libsdl2-dev libsdl2-image-dev   # Debian
nix-shell                                                      # Nix

make lilt            # (optional) command-line tools
make docs            # (optional) build documentation (requires Lilt)
make decker          # build decker itself
make test            # (optional) regression test suite
sudo make install    # (optional) install lilt, decker, and lil syntax profiles
```

If SDL2 is not available, Native-Decker can also be built with [reduced functionality](c/io_sdl1.h) against SDL1.2 and a corresponding version of `SDL_image`. This compatibility shim is presently designed with the [OLPC XO-4](https://wiki.laptop.org/go/XO-4_Touch) and its default Fedora 18 OS image in mind; expect to do some tinkering with the makefile for other platforms:
```
sudo yum install SDL-devel SDL_image-devel

make decker
```


Lilt
----
Decker's scripting language, [Lil](http://beyondloom.com/tools/trylil.html), is available as a standalone interpreter, with extended IO functionality to make it suitable for general-purpose programming and scripting: this package is called [Lilt](http://beyondloom.com/decker/lilt.html). Lilt only requires libc and `xxd` to build from source:
```
make lilt
```

Lilt can be used to programmatically create, inspect, and manipulate decks, as well as package them as Web-Decker self-executing documents:
```
$ lilt
 d:read["examples/decks/color.deck"]
<deck>
 d.card:d.cards.colhex
<card>
 d.card.widgets.hex.text:"FFAA00"
"FFAA00"
 d.card.widgets.hex.event["change"]
0
 d.card.widgets.rgb.text
"16755200"
 write["color.html" d]
1
```

You can build Lilt against [Cosmopolitan Libc](https://github.com/jart/cosmopolitan), producing a single binary that will run on most popular operating systems:
```
$ ./apelilt.sh
successfully compiled lilt.com
running tests against ./lilt.com...
all interpreter tests passed.
all dom tests passed.
all roundtrip tests passed.

$ sh ./lilt.com
  range 10
(0,1,2,3,4,5,6,7,8,9)
```

The Danger Zone
---------------
Decker normally sandboxes the execution of scripts within decks to prevent low-level access to the host computer and ensure parity between the capabilities of Web-Decker and Native-Decker. Both implementations offer opt-in APIs for performing more "dangerous" or non-portable operations called [The Danger Zone](http://beyondloom.com/decker/decker.html#thedangerzone).

When building Native-Decker from source, you can enable _The Danger Zone_ by defining the `DANGER_ZONE` preprocessor flag:
```
FLAGS:=$(FLAGS) -DDANGER_ZONE
```

A "dangerous" build of Native-Decker can export "dangerous" Web-Decker builds. You can also temporarily enable _The Danger Zone_ for Web-Decker by calling the `endanger()` function from your browser's JavaScript console or modifying the `DANGEROUS=0` constant in the .html file to `DANGEROUS=1`. [The Forbidden Library](http://beyondloom.com/decker/forbidden.html) offers a suite of bindings for useful JavaScript APIs based on this interface.


Contributing
------------
The Decker project is released under the MIT license. Any contributions to this repository are understood to fall under the same license.

- Bug fixes and typo corrections are always welcome.
- Bug reports must include simple steps for reproduction and clearly indicate the OS and/or web browser where the bug arises.
- PRs should match the style of existing code.
- PRs should be as small as possible, and must not contain bundled unrelated changes.
- PRs must include updates for _both_ the C and JavaScript versions of Decker (or its associated tools) whenever relevant.
- PRs must include updates for documentation (see: the `docs` directory) wherever relevant.
- PRs must pass the entire test suite (see: `make test`/`make testjs`).
- PRs _must not_ incorporate _any_ material generated by or with the assistance of _any_ so-called "generative AI" tool or language model. Place your trash in an appropriate receptacle.
- When modifying the JavaScript version of Decker, _please_ test your changes in multiple web browsers and avoid using bleeding-edge features. As a rule of thumb, if it didn't exist 5 years ago, don't use it now. If it _only_ works in Chrome, it's better not to do it at all.
- When modifying the C version of Decker, avoid generating warnings and _do not use_ compiler-specific features such as GCC extensions.

Please refrain from submitting Pull Requests to this repository containing new features without first discussing their inclusion in an Issue. Decker is intended to be small, simple, and cozy. There are an infinite number of features that could potentially be added, but creative constraints are also valuable. If you have a differing vision, feel empowered to explore it in your own fork of the project- that's what permissive licenses are for.
