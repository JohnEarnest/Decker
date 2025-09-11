title:Lilt

Lilt
====
_Lil Terminal_ is a command-line interface for the Lil programming language, allowing easy experimentation outside of Decker. Lilt also includes bindings for basic CLI and filesystem IO, making it potentially usable for shell scripting and other applications.

Installation
------------
The source code for Lilt is available [On GitHub](https://github.com/JohnEarnest/Decker).

Lilt depends upon the C standard library and some POSIX extensions. It should work on MacOS, BSD, or most Linux distros. To build and install Lilt, run the included `make` script. By default, a binary is installed in `/usr/local/bin`.
```
make lilt && make install
```

Invoking Lilt
-------------
```
$ lilt [FILE.lil...] [-e EXPR...]
	if present, execute a FILE and exit
	-e : evaluate EXPR and exit
	-h : display this information
```

Executing a `FILE` or `EXPR` argument will not automatically produce any output. Use `show[]` or `print[]` to produce results on _stdout_:
```
$ lilt -e "show[100+range 5]"
(100,101,102,103,104)

$ lilt -e "print[sys.version]"
0.6
```

If a `FILE` or `EXPR` argument has not been provided, Lilt will run in interactive "REPL" mode, which can be exited with Ctrl+C or the `exit[]` function.
```
$ lilt
  2+3
5
  exit[]
$
```

For convenience, after each line is executed at the REPL, the result will be stored in a variable named `_`:
```
  1,2,3
(1,2,3)
  10+_
(11,12,13)
  _/5
(2.2,2.4,2.6)
```

You can write reasonably portable shell scripts by starting a file with a "shebang" something like:
```
#!/usr/bin/env lilt
```

If an environment variable named `LIL_HOME` is set, Lilt will search that directory path at startup, executing any `.lil` files. These could in turn easily load datasets or other useful definitions every time you open a REPL. Startup scripts are always loaded prior to executing `FILE` or `EXPR` arguments.

Global Variables
----------------
| Name            | Description                                                                                           |
| :-------------- | :---------------------------------------------------------------------------------------------------- |
| `args`          | List of CLI arguments to Lilt as strings.                                                             |
| `env`           | Dictionary of POSIX Environment variables.                                                            |
| `sys`           | An Interface which exposes information about the Lilt runtime.                                        |
| `rtext`         | An Interface with routines for working with rich text.                                                |
| `bits`          | An Interface with routines for bit-wise manipulation of numbers.                                      |
| `pi`            | The ratio of a circle's circumference to its diameter. Roughly 3.141.                                 |
| `e`             | Euler's number; the base of the natural logarithm. Roughly 2.718.                                     |
| `colors`        | A dictionary of named pattern indices.                                                                |

Built-in Functions
------------------
| Name             | Description                                                                                                                 | Purpose |
| :--------------- | :-------------------------------------------------------------------------------------------------------------------------- | :------ |
| `input[x]`       | Read a line from _stdin_ as a string, optionally displaying `x` as if with `print[]`, without the newline. Gets nil on EOF. | Console |
| `show[...x]`     | Print a human-comprehensible representation of the value `x` to _stdout_ followed by a newline, and return `x`.             | Console |
| `print[...x]`    | Print a string `x` to _stdout_ followed by a newline. If more args are provided, `format` all but the first using `x`.(0)   | Console |
| `error[...x]`    | Print a string `x` to _stderr_ followed by a newline. If more args are provided, `format` all but the first using `x`.(0)   | Console |
| `dir[x]`         | List the content of a directory as a table.(1)                                                                              | Files   |
| `path[x y]`      | Canonical path `x` (joined with `y`, if given) via [realpath()](https://www.man7.org/linux/man-pages/man3/realpath.3.html). | Files   |
| `read[x hint]`   | Read a file `x` using `hint` as necessary to control its interpretation.(2)                                                 | Files   |
| `write[x y hint]`| Write a value `y` to a file `x`. Returns `1` on success.(3)                                                                 | Files   |
| `exit[x]`        | Stop execution with exit code `x`.                                                                                          | System  |
| `shell[x]`       | Execute string `x` as a shell command and block for its completion.(4)                                                      | System  |
| `eval[x y z]`    | Parse and execute a string `x` as a Lil program, using any variable bindings in dictionary `y`.(5)                          | System  |
| `import[x]`      | Execute a `.lil` script `x` in an isolated scope and return a dictionary of definitions made within that script. (6)        | System  |
| `random[x y]`    | Choose `y` random elements from `x`. In Lilt, `sys.seed` is always pre-initialized to a constant.                           | System  |
| `readcsv[x y d]` | Turn a [RFC-4180](https://datatracker.ietf.org/doc/html/rfc4180) CSV string `x` into a Lil table with column spec `y`.(5)   | Data    |
| `writecsv[x y d]`| Turn a Lil table `x` into a CSV string with column spec `y`.(5)                                                             | Data    |
| `readxml[x]`     | Turn a useful subset of XML/HTML into a Lil structure.(5)                                                                   | Data    |
| `writexml[x fmt]`| Turn a Lil structure `x` into an XML string, formatted with whitespace if `fmt` is truthy.(5)                               | Data    |
| `array[x y]`     | Create a new _array_ with size `x` and cast string `y`, or decode an encoded array string `x`.                              | Decker  |
| `image[x]`       | Create a new _image_ interface with size `x` (`(width,height)`) or decode an encoded image string.                          | Decker  |
| `sound[x]`       | Create a new _sound_ interface with size `x` (sample count) or decode an encoded sound string.                              | Decker  |
| `newdeck[x]`     | Create a new _deck_ interface from scratch, or decode an encoded deck string.                                               | Decker  |

0) If `print[]` or `error[]` are given a single _array interface_ as an argument, the raw bytes of that array will be sent to _stdout_ or _stderr_, respectively, with no trailing newline added. In this way it is possible to print characters which do not have a valid representation as Lil strings, like Unicode block characters.

1) `dir[]` of a file results in an empty table. Directory tables contain:
- `dir`:  if an item is a directory `1`, and otherwise `0`.
- `name`: the filename of the item.
- `type`: the extension including a dot (like `.txt`), if any, always converted to lowercase.

2) `read[x hint]` recognizes several types of file by extension and will interpret each appropriately:

- if the `hint` argument is the string `"array"`, the file will be read as an _array interface_ with a default `cast` of `u8`.
- `.gif` files are read as _image interfaces_ (or a dictionary containing _image interfaces_, as noted below).
- `.wav` files are read as _sound interfaces_.
- `.deck` files are read as _deck interfaces_. If you want to read a `.html` deck export, you can read it as a text file (below) and then use `newdeck[]` to decode it.
- anything else is treated as a UTF-8 text file and read as a string. A Byte-Order Mark, if present, is skipped. ASCII `\r` (Carriage-Return) characters are removed, tabs become a single space, "smart-quotes" are straightened, and anything else outside the range of valid Lil characters becomes "unknown" (`�`).

There are several possible `hint` arguments to control the interpretation of colors in an image:

- `"color"` (or no hint): convert to Decker's 16-color palette (patterns 32-47). Read only the first frame of an animated GIF.
- `"gray"`: convert to 256 grays based on a perceptual weighting of the RGB channels. Read only the first frame of an animated GIF.
- `"frames"`: 16 colors, but read all frames of an animated GIF.
- `"gray_frames"`: 256 grays, but read all frames of an animated GIF.

The `"frames"` or `"gray_frames"` hints will cause `read[]` of a GIF to return a dictionary containing the following keys:
- `frames`: a list of images.
- `delays`: a list of integers representing interframe delays in 1/100ths of a second.

If an image contains transparent pixels, they will be read as pattern 0.

The [WAV file format](https://en.wikipedia.org/wiki/WAV) is much more complex than one might imagine. For this reason, and in order to avoid drawing in large dependencies, `read[]` in Lilt accepts only a very specific subset of valid WAV files corresponding to the output of `write[]`: monophonic, 8khz, with 8-bit unsigned PCM samples and no optional headers. Any other format (or an altogether invalid audio file) will be read as a `sound` with a `size` of 0. For reference, you can convert nearly any audio file into a compatible format using [ffmpeg](https://ffmpeg.org) like so:
```
ffmpeg -i input.mp3 -bitexact -map_metadata -1 -ac 1 -ar 8000 -acodec pcm_u8 output.wav
```

3) `write[path x hint]` recognizes several types of Lil value and will serialize each appropriately:
- _array interfaces_ are written as binary files.
- _sound interfaces_ are written as a .WAV audio file.
- _image interfaces_ are written as GIF89a images. By default, writing out an image will use Decker's default display palette. If `hint` is provided, it can specify a palette for the image as a list of up to 256 24-bit `RRGGBB` colors represented as integers. An explicit palette will translate the pixel values of the image "raw" through the palette, instead of interpreting 1-bit patterns and animated patterns as they are normally displayed within Decker. A value of `-1` in the palette can be used to represent transparency.
- _deck interfaces_ are written as a "standalone" deck with a bundled HTML+JS runtime if the path ends in a `.html` suffix; otherwise the deck will be written as a "bare" deck, which is smaller.
- a list of _image interfaces_ is written as an animated GIF89a image, with each image in the list written as one frame.
- A dictionary is written as an animated GIF89a image. The dictionary should contain the keys `frames` (a list of _image interfaces_) and `delays` (a list of integers representing interframe delays in 1/100ths of a second).
- anything else is converted to a string and written as a text file.

4) `shell[]` returns a dictionary containing:
- `exit`: the exit code of the process, as a number. If the process halted abnormally (i.e. due to a signal), this will be -1.
- `out`: _stdout_ of the process, as a string.

5) See the Decker Manual for details of `eval[]`, `readcsv[]`, `writecsv[]`, `readxml[]`, and `writexml[]`.

6) Scripts loaded with `import[]` will not have access to `args` or `env`. Scripts may use `args~0` as an idiom to detect when they have been imported as a library. If you `import[]` a `.deck` or `.html` file, the function will return a dictionary of the module values within that deck, keyed by module name, as a convenience:
```lil
bn:read["examples/decks/bignums.deck"].modules.bn.value   # without import[]
bn:import["examples/decks/bignums.deck"].bn               # with import[]
```

Working With Decks
------------------
The `newdeck[]`, `read[]` and `write[]` functions allow Lilt to operate on Decker documents. Lilt can load, create, and manipulate multiple decks simultaneously, providing options for automated testing, data import/export, accessibility, and interacting with other technology from outside the Decker ecosystem.

Just as in Decker, you can simulate "injecting" events into widgets, cards, or the deck with the `x.event[name ...args]` function they provide, running the appropriate scripts to completion and producing any appropriate side-effects on the deck. For example, clicking on the first widget on the active card:

```lil
d:read["demo.deck"]
(first d.card.widgets).event["click"]
```

Changelog
---------
v1.0:
- initial release.

v1.9:
- integrated readline functionality into Lilt, removing the need for `rlwrap`.

v1.10:
- deprecated specialized widget event injectors in favor of `x.event[]`.

v1.14:
- introduced `read[]` hints for decoding the frames of animated GIF images.

v1.18:
- generalized `print[]` and `error[]` to accept a array interfaces as arguments.

v1.22:
- introduced `import[]` to simplify building multi-file scripts.

v1.54:
- introduced `newdeck[]` as a way of decoding decks from a string representation.
- generalized `read[]` to recognize `.deck` files and decode them as deck interfaces.
- generalized `write[]` to accept deck interfaces and write them as `.deck` or `.html` files.
- deprecated the specialized `readdeck[]` and `writedeck[]` functions in favor of the above.
