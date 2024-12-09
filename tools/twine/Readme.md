Ply: A Decker-Compatible Twine Story Format
===========================================
[Twine](https://twinery.org) offers an extensible universe of "[story formats](https://twinery.org/cookbook/introduction/story_formats.html)" with their own markup and semantics. Most of the popular story formats include support for web browser features- like embedded JavaScript and CSS styling- which preclude full support for them within Decker.

In the interest of offering a lowest common denominator, I have defined an excruciatingly simple story format called _Ply_:

- Text surrounded by `*asterisks*` is *bolded*.
- Text surrounded by `` `backticks` `` is `fixed-width`.
- Text surrounded by curly braces `{2+3}` is a fragment of Lil source code, which is evaluated and inserted in-place.
- Links match Harlowe syntax: `[[Link Label->Destination Passage]]`.

You can reference a compiled version of this story format suitable for loading into Twine at:
```
https://beyondloom.com/decker/ply/format.js
```

To build this story format from source, you will need Lilt:
```
lilt build_ply.lil
```

For more information, see [the twee module](https://beyondloom.com/decker/twee.html).
