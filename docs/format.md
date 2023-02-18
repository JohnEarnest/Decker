title:Decker File Format

<style>
/* general prose */
body{margin:1em 5em 5em 3em;}
h1,h2,figcaption{font-family:Helvetica Neue,Arial,sans-serif;}
h1{color:#21183c;font-weight:700;}
h2{color:#21183c;font-weight:300;margin-top:1.5em;}
pre,code{background-color:Gainsboro;tab-size:2;font-size:large;}
pre{margin:0 .5em;padding:.5em;border:1px solid #aaa;border-radius:5px;}
li{margin:1em 0;}
table{margin:0 .5em;border-collapse:collapse;border:1px solid #aaa;}
td{padding:5px;}th{padding:10px;border-bottom:1px solid #aaa;background-color:Gainsboro;}
td,th:not(:first-child){border-left:1px solid #aaa;}
figure{display:block;text-align:center;}
</style>

The Decker File Format
======================
Decker is a multimedia platform for creating and sharing interactive documents, with sound, images, hypertext, and scripted behavior. This guide describes the structure of Decker documents in detail, to facilitate the creation of compatible document viewers and editors.

General Concepts
----------------
Decker documents are called _Decks_, and consist of a series of _Cards_. Cards in turn contain _Widgets_. The Decker environment can broadly be in "editing" mode, where widget properties are manipulated interactively, and an "interaction" mode where widgets respond to clicks and other interactions by emitting _events_ and running scripts as applicable.

Widgets, cards, or the deck itself may contain scripts written in the [Lil programming language](lil.html). Scripts have a mechanism for "bubbling" events up along the container hierarchy from a source widget to its containing card, and perhaps to the deck itself. In response to events, Lil scripts can inspect and manipulate properties of the deck via a series of Interfaces furnished by the application. For more detail on this, see the [Decker manual](decker.html). Lil scripts are not _persistent_- their local variable bindings are discarded between events. Everything that is persistent about a deck- especially widget values- is captured in the representation described in this guide.

Lil code intended to be reusable across projects can be organized into _Modules_. From a programming perspective, modules execute once when a deck is loaded (or the module is modified) and each return a dictionary which will be mounted as a global for use by ordinary event-triggered scripts. Modules do not have direct access to the deck or its components, nor do they have direct access to one another. Each module has an independent key-value store which can furnish static data used by the module or allow the module to explicitly preserve its state.

In addition to the built-in selection of widgets, it is possible to define custom widgets called _Contraptions_. Internally, a contraption behaves like a card, containing widgets (representing its state and interactive elements) and a background image. Externally, it behaves like a single widget. Many contraption instances can be created from a single definition, which is called a _Prototype_.

Rationale
---------
The deck format is designed around several constraints and considerations:

- **Extensibility**: It should be straightforward to add new properties and metadata to widgets, cards, and the deck itself in future revisions of the format. For this reason, we use a text-based format with a key-value associative structure, and provide a `version` field for the document.
- **Embeddability**: Decks are usually distributed through the web, and to users who have a web browser. By making the document format amenable to embedding inside a valid HTML document's `<script>` tag (with the necessary considerations for escaping special character sequences), it is possible to distribute decks as operating system- and architecture-agnostic "self-executing" documents which can _also_ be read and manipulated without a complete HTML parser.
- **Exposed Scripts**: Advanced users may want to edit their Lil scripts using their favorite text editor. The document format puts special emphasis on representing scripts with their whitespace intact and a minimum of escaped characters to facilitate this. As a side benefit, the representation given here allows for automatic de-duplication of scripts which appear in many places.
- **Diff-Friendliness**: Building upon the above, advanced users may want to store and track decks using existing source-control systems. Documents therefore use a line-oriented format compatible with tools like `diff` and attempt, within reason, to reflect localized edits to a deck in localized changes to the file.

Wrapper
-------
Decks are stored in UTF-8 encoded text files (optionally with a UTF-8 [BOM](https://en.wikipedia.org/wiki/Byte_order_mark)), and have the following structure:
```
<body><script language="decker">
PAYLOAD
</script>
RUNTIME
```

Where `PAYLOAD` is the format described in the remainder of this document and `RUNTIME` is a self-contained sequence of HTML, CSS, and JavaScript sufficient for making the deck self-executing. Anything which is not a web browser can ignore the content of the document following the `</script>` tag that terminates `PAYLOAD`.

The `PAYLOAD` must _never_ contain the literal sequence of characters `</script`: a web browser would incorrectly treat this as a premature end to the enclosing script tag and thus the payload, corrupting the document.

Applications must support reading documents consisting only of the `PAYLOAD` section, with no surrounding tags or `RUNTIME`, for environments where a web browser is not available/desired or when the overhead of the `RUNTIME` stub is inconvenient. Such a document should be stored with a `.deck` extension.

Applications must tolerate Windows-style newlines (CR-LF), interpreting them identically to Unix-style newlines (LF only). Emitting Unix newlines when creating documents is strongly recommended.


Data Blocks
-----------
The payload may contain fields this guide calls _data blocks_. Data blocks provide a representation of arbitrary binary data which fits in a [JSON](https://www.json.org) string, and has less overhead than a JSON array of numbers. Since APIs for copying and pasting non-textual data are highly heterogenous between operating systems and application frameworks, and many browsers provide limited-to-nonexistent support for this functionality, Decker uses the same data block formats to represent non-textual data in the system clipboard.

Data blocks have a structure like `%%TTTFP...`:

- `%%`: a short text pattern which is very rare at the beginning of prose text or Lil expressions, to avoid false-positives.
- `TTT`: a three-character code indicating the data block's type.
- `F`: a single-character _format_, specifying the precise representation used for the block type.
- `P...`: any number of characters representing a [Base64](https://en.wikipedia.org/wiki/Base64) encoded payload of bytes, with the conventional choice of `A-Z`,`a-z`,`0-9`,`+`, and `/` as the character set and `=` as the padding symbol, as necessary.

In the future, the official Decker file format may support additional formats for some block types. If independent forks wish to introduce their own formats it is strongly recommended that they begin with format `Z` and work backwards alphabetically, to reduce the chance of clashes.

The data block types used by Decker are as follows:

- `IMG`: an _image record_. Image records are used for card backgrounds, canvas backgrounds, patterns, and image selections copied to the clipboard.  All image formats always begin with 4 bytes giving a pair of 16-bit unsigned big-endian integers indicating the _width_ and _height_ of the image data, respectively, followed by the image data. The supported formats are:

	- `0`: a packed 1-bit image. Each byte represents 8 horizontally adjacent pixels. Bytes are laid out in rows, left-to-right. Images with a width that is not evenly divisible by 8 will be padded with 0 bits. This format is used for most imported images.

	- `1`: an 8-bit image, in which each byte represents the pattern which should be used to draw one pixel. (See: _Pattern Record_.) Bytes are laid out in rows, left-to-right. This format is suitable for images containing animated patterns.

	- `2`: a run-length encoded (RLE) 8-bit image. In each pair of bytes, the first byte indicates a pattern number (See: _Pattern Record_), and the second byte indicates the number of pixels to assign with this pattern, scanning in rows, left-to-right. RLE provides a substantially more compact lossless representation of low-complexity images without requiring a complicated encoder or decoder.

- `FNT` a _font record_. The format is always `0`. Decker fonts define glyphs corresponding to printable ASCII including the space character (32-126; 95 glyphs total), an extra glyph representing an ellipsis (for truncating text on display), as well as some metadata to permit different font sizes and variable-width glyphs. The payload has the following structure:

	- 1 unsigned byte: the maximum width of each glyph in the font, in pixels.
	- 1 unsigned byte: the height of glyphs, in pixels, including all vertical padding.
	- 1 unsigned byte: the number of horizontal pixels to advance between characters.
	- 96 glyph records, each consisting of:
		- 1 unsigned byte giving the _true_ width of the glyph: how many pixels to advance horizontally after drawing the glyph.
		- `(width/8)*height` bytes of packed image data, in which each byte represents 8 horizontally adjacent pixels. Glyphs with a width that is not evenly divisible by 8 will be padded with 0 bits. Note the similarity to `IMG0`.

- `SND`: a _sound record_. Sound records are used for audio clips in decks, as well as sound data copied to the clipboard. The format is always `0`. The payload consists of a series of 8-bit signed 8khz PCM monophonic samples.

Payload
-------
The `PAYLOAD` consists of a series of lines. Except in `{script:ID}` chunks, empty lines and lines beginning with a `#` are ignored as comments, giving users flexibility in annotating documents as they wish. Applications are not required to preserve comments.

A _Chunk_ begins with a line that begins and ends with curly braces `{}`, indicating the type of the chunk, and spans until the beginning of the next chunk. Some chunk types have an _ID_ which is separated from the chunk type with a colon (`:`). The following chunk types exist:

- `{deck}`: contains properties pertaining to the entire document.
- `{sounds}`: a dictionary of _Sound Records_. Every property line has an ID and an `SND0` data block.
- `{fonts}`: a dictionary of _Font Records_. Every property line has an ID and an `FNT0` data block. The built-in fonts `menu`, `body`, and `mono` are provided by Decker itself, but _could_ be overridden by definitions in this chunk.
- `{card:ID}`: contains properties pertaining to a card, and is followed by a `{widgets}` chunk (optional).
- `{widgets}`: a collection of a card or prototype's _Widget Records_.
- `{script:ID}`: contains a Lil script.
- `{module:ID}`: contains a Lil module and its metadata.
- `{contraption:ID}`: contains a contraption definition (Prototype).
- `{data}`: contains a Lil module's key-value store.
- `{end}`: terminates a `{script}` chunk.

Except in `{script:ID}` chunks, non-comment lines within a chunk are _Property Lines_ consisting of an _ID_ followed by a colon (`:`). The remainder of the line is interpreted as JSON data. JSON data _must_ escape all forward-slash characters (`/` as `\/`). For example:
```
{deck}
version:1
name:"AC\/DC Fan Zine"
size:[320,240]
card:4
```

`{script:ID}` chunks contain verbatim Lil code and are always terminated by an `{end}` on its own line. For example:
```
{script:3}
# this is a Lil comment
on click do
  alert["You clicked a thing!"]
end
{end}
```

Chunks permit flexible ordering within a document, but often their relative ordering is significant:

- The order in which `{card:ID}` chunks appear is their order in the deck.
- The order of property lines within a `{widget}` chunk is likewise the drawing order of those widgets.
- The `{widget}` chunk describes the contents of its preceding `{card:ID}` chunk.

IDs and the contents of a `{script:ID}` chunk can contain nearly any printable ASCII characters. To avoid ambiguity, the following _plain_ characters can be replaced with equivalent _escaped_ sequences of characters:

| Plain | Escaped | Notes                                                       |
| :---- | :------ | :---------------------------------------------------------- |
| `{`   | `{l}`   | Always escape this character.                               |
| `}`   | `{r}`   | Always escape this character.                               |
| `:`   | `{c}`   | Always escape this character in IDs.                        |
| `/`   | `{s}`   | Escape this character if it is immediately preceded by `<`. |

The `{deck}` Chunk
------------------
The Deck chunk contains a number of optional properties with metadata pertaining to the entire document.

- `version`: an integer indicating the revision of the file format. This guide describes version number `1`. If the format is ever revised in a _non-backwards-compatible manner_, this number will be incremented.
- `name`: a human-readable string giving the title of the deck.
- `author`: a human-readable string describing the creator of the deck. Intended to aid in the organization of galleries or collections of decks.
- `locked`: an integer. If nonzero, the document viewer should display the deck with a minimal UI, disabling any editing features.
- `script`: a number or string corresponding to a `{script:ID}` chunk, representing top-level event handlers applying to the entire deck.
- `size`: an array of 2 integers providing the width and height of the deck, respectively, in pixels. This applies to every card. The minimum valid size is `[320,240]`, and the default is `[512,342]`. Note that the background _Image Records_ of cards, if present, contain redundant information about their dimensions- this permits hand-altering the size of a deck without mangling its contents.
- `card`: an integer giving the index of the card which will be shown when the deck is opened. If this index is invalid, reset it to `0`.
- `patterns`: an 8x224 _Image Record_ representing a vertical strip of 28 8x8 patterns. Pattern 0 will always be overruled as "all white" and pattern 1 will always be overruled as "all black". If any of these patterns are modified, they will alter the appearance of Decker's UI pervasively. If this property is not provided, Decker will supply a default palette.
- `animations`: an array of four arrays, each of which may contain up to 8 integers. Each integer is an index to a pattern. These four sequences represent the animated patterns 28, 29, 30, and 31. Animation sequences may not themselves reference indices 28, 29, 30, or 31. If this property is not provided, Decker will supply default animation sequences: `[[13,9,5,1,5,9],[4,4,8,14,14,8],[18,18,20,19,19,20],[0,0,0,0,1,1,1,1]]`.

The `{card:ID}` Chunk
---------------------
Card chunks always have an ID, which serves as the `name` of the card. They also may have additional optional properties:

- `image`: an _Image Record_ used as the background of this card.
- `script`: a number or string corresponding to a `{script:ID}` chunk, representing event handlers applying to this card.

An example of a `{card:ID}` chunk complete with `{widget}` section:
```
{card:home}

{widgets}
one:{"type":"button",text:"Click Me!"}
two:{"type":"button",text:"Check Me Out","style":"check","value":1}
```

The `{widgets}` Chunk
---------------------
The `{widgets}` chunk is a dictionary of widgets belonging to the preceding `{card:ID}` or `{contraption:ID}` chunk, in their drawing order, back-to-front. Each widget is a JSON object with one or more properties.

Some properties are common to all widgets:

- `type` a widget type; one of { `"button"`, `"field"`, `"slider"`, `"canvas"`, `"grid"`, `"contraption"` }.
- `pos` an array of 2 integers providing the x and y position of the widget, respectively, in pixels.
- `size` an array of 2 integers providing the width and height of the widget, respectively, in pixels.
- `show` control how the widget is drawn. One of:
	- `"solid"`: Draw the widget with a white (pattern 0) background. (default).
	- `"transparent"`: Draw the widget without a background, or, in the case of `canvas`, allow graphics beneath the widget to show through pattern 0.
	- `"invert"`: Draw the widget with a white-on-black colorscheme, or, in the case of `canvas`, every pixel in the image will toggle (XOR) pixels in the graphics beneath it.
	- `"none"`: Do not draw this widget at all. Such a widget will not produce any events.
- `locked` an integer; if nonzero, this widget is non-editable. For `button`, this means it cannot be clicked. A locked `field` may not be edited, and a locked `canvas` may not be drawn upon by user mouse events. The default is `0`.
- `animated` an integer; if nonzero, this widget will be sent a `view[]` event on each frame (60hz). The default is `0`.
- `font` a string corresponding to the ID of a _Font Record_, used for drawing any text on this widget.
- `script` a number or string corresponding to a `{script:ID}` chunk, representing event handlers applying to this widget.


Each `type` of widget has its own additional optional fields:

- `"button"`:
	Buttons are the simplest type of interactive widget, responding to cursor-clicks.
	- `text`: a string; the button label.
	- `style`: one of { `"round"` (default), `"rect"`, `"check"`, `"invisible"` }. The `"round"` and `"rect"` styles are normal buttons. The `"check"` style is displayed as a checkbox with a label and an underlying boolean value. An `"invisible"` button cannot be selected by pressing tab and has no appearance except its `"text"` (if any) and inverting its boundary while depressed.
	- `value`: one of {`0` or `1`}; used for checkbox state.

- `"field"`:
	Fields are the basic widget for storing information. They can contain "formatted" text or plain user-entered text, and may have a vertical scrollbar. Locked fields may also be useful for labels, headings, or displaying information computed by a script.
	- `border`: an integer; draw an outline for the field? Default is `1`.
	- `scrollbar`: an integer; does this field have a scrollbar? Default is `0`. Note that without a visible scrollbar, it is still possible to scroll a field programmatically, or by dragging a selection.
	- `style`: one of { `"rich"` (default), `"plain"`, `"code"` }. A `"rich"` field permits entry and editing of formatted text. A `"plain"` field is text-only, with a single font. A `"code"` field is likewise text-only, with some changes in behavior making the field more suitable as a code editor: tabbing does not leave the field, shift+`/` toggles Lil block comments, etc.
	- `align`: one of { `"left"` (default), `"center"`, `"right"` }.
	- `value`: either a string (unformatted text) or a rectangular dictionary with the following columns (rich text):
		- `text`: an array of strings.
		- `font`: an array of strings referencing _font records_. An invalid or empty name indicates using the field's `font` property.
		- `arg`:  an array of strings. Each is either an _image record_ (an inline image), an ordinary string (making this span a hyperlink), or an empty string (ordinary text).
	- `scroll`: an integer; the scroll offset in rendered pixels.

- `"slider"`:
	Sliders represent a single number constrained within a specified numeric interval.
	- `style`: one of { `"horiz"` (default), `"vert"`, `"bar"`, `"compact"` }. A `"horiz"` or `"vert"` slider resembles a horizontal or vertical scrollbar, respectively. A `"bar"` has a much simpler appearance, and resembles a horizontal progress bar. A `"compact"` slider moves only in discrete ticks in response to left/right button presses, more like what some UI toolkits call a "spinner".
	- `interval`: an array of two numbers giving a minimum and maximum value for the slider (inclusive). Default is `[0,100]`.
	- `step`: the granularity of individual steps on this slider. Default is `1`. The step value must be greater than zero.
	- `format`: a string which controls how the value will be displayed in the `"bar"` or `"compact"` mode, as by the Lil `format` primitive. Default is `"%f"`.
	- `value`: a floating-point number. Default is `0`.

- `"canvas"`:
	Canvases are repositionable, stackable rectangles which can be drawn on- either by a user or through scripting. Canvases are useful for creating cards with animation, procedural drawing, or cards where parts of the background can be shown or hidden selectively.
	- `border`: an integer; draw an outline for the canvas? Default is `1`.
	- `draggable`: an integer; allow this canvas to be repositioned in interact mode? Default in `0`.
	- `image`: an _Image Record_.
	- `brush`: an integer brush-shape index used for drawing. Default is `0`.
	- `pattern`: an integer pattern index used for drawing. Default is `1`.
	- `clip`: an array of four numbers giving the x, y, width, and height of the clipping rectangle, respectively. Default is the entire canvas.
	- `scale`: a float scale-up factor used when drawing the canvas. Default is `1.0`. The logical size of a given canvas is the ceiling of its `size` divided by its `scale`.

- `"grid"`:
	Grids display tabular information. The grid may scroll vertically and the user can select a row or cell by clicking. Grids are less flexible than a set of fields, but make it easier to see a lot of information at once. A single-column grid is also a reasonable stand-in for a dropdown or list selector.
	- `headers`: an integer; show column headers? Default is 1. Among other things, suppressing headers is useful if you want to use a secondary grid with "summary" columns beneath a grid containing rows of data.
	- `scrollbar`: an integer; does this grid have a scrollbar? Default is `1`. Note that without a visible scrollbar, it is still possible to scroll a grid programmatically.
	- `lines`: an integer; does this grid draw grid lines between cells? Default is `1`.
	- `widths`: an array of up to 255 integers. How many pixels wide should each column be shown? If more columns exist than there are numbers in this array the remaining columns are automatically sized.
	- `format`: a string consisting of a sequence of format type characters (as in the left argument to the Lil `format` operator) controlling how columns are displayed. If this string is not provided or not long enough for the table's columns, remaining columns are treated as strings (`s`). This format will also be applied to CSV data exported from the grid.
	- `value`: a dictionary keyed by column names. Columns must be arrays, for which each item is recursively composed of _only_ arrays, dictionaries, strings, and numbers. Columns need not necessarily be of uniform types.
	- `scroll`: an integer; the scroll offset in rows.
	- `row`: an integer; the selected row index, or -1.

- `"contraption"`:
	- `def`: a string; the ID of a contraption definition (Prototype).
	- `widgets`: a dictionary of JSON objects representing the widgets contained in this contraption instance. The properties given here override any properties from the corresponding `widgets` of the contraption's definition. Note that the `pos` of any "inner" widget is relative to the `pos` of the contraption which contains it.

The `{module:ID}` Chunk
-----------------------
Module chunks always have an ID, which serves as the `name` of the module. They also may have additional optional properties:

- `description`: a string giving a human-readable description of the purpose of the module.

Modules may contain a `{data}` chunk containing a dictionary of names associated with JSON values, in the same structure as `{widgets}` or `{fonts}`.

Module properties and/or data are always followed immediately by a `{script}` chunk (with no ID) representing the module body.

An example of a complete `{module:ID}` chunk:
```
{module:logger}
description:"a utility module for logging"
{data}
log:{time:[],message:[]}
{script}
log:table data.log

mod.put:on _ x do
	log:insert
		time:sys.now
		message:x
	into log
	data.log:cols log
	log
end

mod.get:on _ do
	log
end
{end}
```

The `{contraption:ID}` Chunk
----------------------------
Contraption chunks always have an ID, which serves as the `name` of the Prototype. They may have additional optional properties:

- `size`: an array of 2 integers providing the width and height of instances of the Prototype, respectively, in pixels.
- `description`: a string giving a human-readable description of the purpose of the Prototype.
- `image`: an _Image Record_ used as the background of instances of the Prototype.
- `script`: a number or string corresponding to a `{script:ID}` chunk, analogous to the script of a card from the perspective of contained widgets.
- `template`: a string containing a Lil script that will be used as the default script for newly-created instances of this Prototype.
- `resizable`: an integer; allow instances of this prototype to be resized? Default in `0`.
- `margin`: an array of four integers giving the margin inset from the left, top, right, and bottom edges, respectively, for controlling widget reflow and dividing the background image into four static corners and five tileable regions. Default is `[0,0,0,0]`.
- `attributes`: a rectangular dictionary with the following columns:
	- `name`: the attribute name of an editable property for instances of this Prototype.
	- `label`: the display label of an editable property for instances of this Prototype. If missing, the `name` will be used.
	- `type`: the type of this editable property, for the purpose of supplying a sensible user interface for editing. An invalid `type` will cause the current row to be ignored.

Attribute `type`s may be one of the following:

- `bool`: A boolean 0/1 value, editable with a checkbox.
- `number`: A numeric value, editable with a field.
- `string`: A string value, editable with a field.
- `code`: A string value, editable with a larger field in "code" editing mode.
- `rich`: A string value, editable with a larger field in "rich" editing mode.

An example of a complete `{contraption:ID}` chunk and its associated `{widgets}` and `{script:ID}` chunks:
```
{contraption:spinner}
size:[150,50]
description:"unbounded numeric field that can be incremented or decremented by a configurable step"
attributes:{"name":["value","step"],"label":["Value","Step"],"type":["number","number"]}
script:"spinner.root"

{widgets}
down:{"type":"button","text":"<","script":"spinner.0"}
up:{"type":"button","text":">","script":"spinner.1"}
value:{"type":"field","value":"0","locked":1}
step:{"type":"field","value":"1","show":"none"}

{script:spinner.root}
on set_value x do
  value.text:0+x
end
on get_value do
  0+value.text
end
on set_step x do
  step.text:0+x
end
on get_step do
  0+step.text
end
{end}

{script:spinner.0}
on click do
  value.text:value.text-step.text
end
{end}

{script:spinner.1}
on click do
  value.text:value.text+step.text
end
{end}
```

An Example Deck
---------------
The following is a simple structurally valid deck, with `...` elisions for clarity:
```
<body><script language="decker">
{deck}
version:1
size:[320,240]
card:0

{card:homeCard}
{widgets}
pusher:{"type":"button","text":"Click Me!","script":0}

{script:0}
on click do
  alert["you clicked the button.\nhooray for you."]
end
{end}

{fonts}
body:"%%FNT0CAoBAQAAAAA...AAAAAqAAA"
bold:"%%FNT0EA0BAgAAAAA...AAAAAAAAA"

</script>
...
```

Changelog
---------
1.0:
- Initial release.

1.2:
- Introduced the `canvas.draggable` field.

1.10:
- Deprecated the `card.parent` field, a mechanism for inheriting the widgets and properties of a "template" card.

1.12:
- Introduced the `{contraption:ID}` section and the `contraption` widget type.
- Introduced the `animated` property for all widgets.
