Lila: A Lil interpreter in Awk
==============================
Lila is an interpreter for the [Lil](http://beyondloom.com/tools/trylil.html) programming language implemented (mostly) in POSIX Awk. To learn more, see [the blog post](http://beyondloom.com/blog/lila.html) describing this project.

*Note: As of Decker v1.54, Lila is no longer actively maintained and kept in sync with newer revisions of Lil; if there's demand, I may resume work on this sub-project.*

Invoked with a `.lil` filename, Lila will execute the script and exit. If no filename is provided, it will instead run in an interactive "[REPL](https://en.wikipedia.org/wiki/Read–eval–print_loop_)" mode which can be terminated with Ctrl+C. Using the optional `rlwrap` utility as an intermediary provides a much nicer REPL experience, with line-editing and command history:
```
% rlwrap awk -f lila.awk
 range 10
(0,1,2,3,4,5,6,7,8,9)

 select index*2 value from "ABCD"
+-------+-------+
| index | value |
+-------+-------+
| 0     | "A"   |
| 2     | "B"   |
| 4     | "C"   |
| 6     | "D"   |
+-------+-------+


% cat demo.lil    
print["Goodbye, cruel world."]

% awk -f lila.awk demo.lil 
Goodbye, cruel world.
%
```

You can run Lila against an appropriate subset of Decker's test suite from the top directory of this repository:
```
% make testawk
running tests against awk -f tools/awk/lila.awk...
all interpreter tests passed.
all array tests passed.
all image tests passed.
all integration tests passed.
```

Lila has been tested against several Awk implementations:

- `awk` 20200816
- `awk` 20070501
- `mawk` v1.3.4
- `goawk` v1.29.0
- `gawk` v5.3.0 (requires the additional `-b` flag to disable locale-specific character handling)
- `gawk` v4.0.1 (see above)

Lila includes a large subset of the standard library provided with [Lilt](http://beyondloom.com/decker/lilt.html), with the following omissions, limitations, and caveats:

- `readdeck[]`, `writedeck[]`, and the interfaces that make up the [Decker](http://beyondloom.com/decker/) document model are not present.
- `sound[]` and the `sound` interface are not present.
- `read[]` and `write[]` do not support import or export of `.WAV` audio or `.GIF` images.
- `write[]` always returns `1`.
- `shell[]` always reports exit codes as `-1`.
- `path[]` is not present.
- `sys.ms` only provides per-second granularity.
- `sys.platform` reports `"awk"`.

Lila shells out to common utilities for functionality that is not possible (portably) in pure Awk:

- `od` for reading raw bytes from files. 
- `ls` to enumerate directory contents for `dir[]`.
- `date` for performing some date-time formatting.
