Decker
======
Decker is a multimedia platform for creating and sharing interactive documents, with sound, images, hypertext, and scripted behavior.

![Decker, complete with toolbars](images/wings.gif)

You can learn more about Decker on [my website](http://beyondloom.com/decker/), or just dive in and [try it online](http://beyondloom.com/decker/tour.html).

There is also a [community forum](https://internet-janitor.itch.io/decker/community) on itch.io.


Building
--------
The [web version](http://beyondloom.com/decker/tour.html) of Decker (web-decker) can be built with a `make` script. The test suite uses [Node.js](https://nodejs.org/en/):

```
make testjs
make web-decker
make runweb      # (optional) open in your default browser
```

Periodic binary releases of the native version of Decker (c-decker) for OSX and Windows are available on [itch.io](https://internet-janitor.itch.io/decker). Building from source requires:

- a c compiler and libc
- the `xxd` utility (standard with OSX and most \*nix distros)
- [SDL2](https://www.libsdl.org/download-2.0.php)
- [SDL_image](https://github.com/libsdl-org/SDL_image)

On OSX, BSD, or Linux, fetch the appropriate SDL2 packages and then build with `make`. This has also been reported to build and run successfully under WSL.

```
brew install sdl2 sdl2_image                                   # OSX/Homebrew
sudo apt install libsdl2-2.0-0 libsdl2-dev libsdl2-image-dev   # Debian

make lilt            # (optional) command-line tools
make decker          # build decker itself
make test            # (optional) regression test suite
sudo make install    # (optional) install lilt, decker, and lil syntax profiles
```

Building the documentation requires [multimarkdown](http://fletcher.github.io/MultiMarkdown-5/installation):
```
brew install multimarkdown

make docs
```

As a fun bonus, you can also build Lilt against [Cosmopolitan Libc](https://github.com/jart/cosmopolitan), producing a single binary that will run on most popular operating systems:
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
- When modifying the JavaScript version of Decker, _please_ test your changes in multiple web browsers and avoid using bleeding-edge features. As a rule of thumb, if it didn't exist 5 years ago, don't use it now. If it _only_ works in Chrome, it's better not to do it at all.
- When modifying the C version of Decker, avoid generating warnings and _do not use_ compiler-specific features such as GCC extensions.

Please refrain from submitting Pull Requests to this repository containing new features without first discussing their inclusion in an Issue. Decker is intended to be small, simple, and cozy. There are an infinite number of features that could potentially be added, but creative constraints are also valuable. If you have a differing vision, feel empowered to explore it in your own fork of the project- that's what permissive licenses are for.
