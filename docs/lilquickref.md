title:Lil Quick Reference

Lil Quick Reference
===================
Types
-----
| `typeof`     | False      | Examples                              |
| :----------- | :--------- | :------------------------------------ |
| `"nil"`      | `nil`      | `nil`                                 |
| `"number"`   | `0`        | `42` `37.5` `-29999`                  |
| `"string"`   | `""`       | `"foo\nbar"`                          |
| `"list"`     | `()`       | `11,22,33` `list 3`                   |
| `"dict"`     | `()dict()` | `("a","b") dict 11,22`                |
| `"table"`    | n/a        | `table range 2` `insert a with 1 end` |
| `"function"` | n/a        | `on x y do x+y end`                   |

Primitives
----------
| Valence | Purpose    |                                                                                       |
| :------ | :--------- | :------------------------------------------------------------------------------------ |
| monad   | arithmetic | `-` `!` `floor` `cos` `sin` `tan` `exp` `ln` `sqrt` `mag` `unit` `heading`            |
| monad   | reducers   | `count` `first` `last` `sum` `prod` `min` `max` `raze`                                |
| monad   | data       | `range` `keys` `list` (enlist) `rows` `cols` `table` `typeof` `flip`                  |
| dyad    | arithmetic | `+` `-` `*` `/` `%` (y mod x) `^` (pow) `&` (min) `|` (max)                           |
| dyad    | logical    | `<` `>` `=` (conforming equal) `~` (match) `unless` (x if y is `0`)                   |
| dyad    | string     | `fuse` `split` `parse` `format` `like`                                                |
| dyad    | data       | `,` (concat) `@` (index each right) `dict` `take` `drop` `limit` `window` `in` `fill` |
| dyad    | joins      | `join` (natural join/zip) `cross` (cross join/cartesian product)                      |

Flow
----
- `if bool ... end`
- `if bool ... else ... end`
- `if bool ... elseif bool ... else ... end` (etc)
- `each val key index in x ... end`
- `while bool ... end`
- `send name[args]`

Queries
-------
- `select  exprs clauses from y` reorder, compute, or filter a table
- `update  exprs clauses from y` modify rows/columns of a table in place
- `extract exprs clauses from y` like select, but yields non-tabular values
- `insert  c1 c2 with "A" 11 "B" 22 end` create a new table
- `insert  c1 c2 with "A" 11 "B" 22 into d` append to a table

- `exprs` can be any number of expressions in the forms:
	- an implicitly named bare expression (`id`, `2*index`)
	- an explicitly named expression (`ident:id`, `dogyears:7*age`)
	- a quoted name, for invalid identifiers (`"not a lil id":foo`)
	- if no expressions are provided, all columns will be returned, like `select *` in SQL

- `clauses` can be any sequence of the following, evaluated right to left:
	- `by a`: group rows by the unique values of column b
	- `where a`: filter rows by a boolean column a
	- `orderby a asc`/`orderby a desc`: sort rows, comparing values of column a as by `<`/`>`

- special columns/values available when computing any column expression:
	- `index`: magic column of original row numbers
	- `gindex`: magic column of current row number within group (or all rows if ungrouped)
	- `group`: magic column of row's group, by appearance (or `0` if ungrouped)
	- `column`: dictionary of the entire group's columns (or all rows if ungrouped)

Formatting
----------
A format is `%[name]*-0N.DX`: `*` skip, `0` pad, `N` width.
`-` is invert char class (`ro`) or left justify.
`.D` is decimal places (`fc`), size of char class (`ro`), or truncate to `D` characters.

| `X`       | Purpose  |                                                                                                |
| :-------- | :------- | :--------------------------------------------------------------------------------------------- |
| `%nmz`    | Parsing  | literal `%`, number of chars read, matches? matches to end?                                    |
| `ro`      | Matching | repeat (0 or more in char class), optional (0 or 1 in char class)                              |
| `sula`    | String   | string, uppercase string, lowercase string, ASCII/DeckRoman chars                              |
| `bficChH` | Number   | bool, float, int, currency (`-$1.23`), plain currency (`-1.23`), hex lowercase, hex uppercase  |
| `jJep`    | Misc.    | JSON, Lil data, unix epoch, time-parts {`year`, `month`, `day`, `hour`, `minute`, `second`}    |
| `qv`      | Lil      | quoted Lil string literal, Lil variable name                                                   |

Glob patterns for `like`:

- `.`: any single character.
- `#`: any single digit 0-9.
- `*`: 0 or more of any character.
- backtick escapes a subsequent special character.
