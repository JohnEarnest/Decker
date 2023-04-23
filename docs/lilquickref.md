title:Lil Quick Reference

<style>
body{margin:1em 5em 5em 3em;font-size:15px;}
h1,h2,figcaption{font-family:Helvetica Neue,Arial,sans-serif;}
h1{color:#21183c;font-weight:700;}
h2{color:#21183c;font-weight:300;}
pre,code{background-color:Gainsboro;tab-size:2;font-size:large;font-size:15px;}
pre{margin:0 .5em;padding:.5em;border:1px solid #aaa;border-radius:5px;}
li{margin:1em 0;}
table{margin:0 .5em;border-collapse:collapse;border:1px solid #aaa;}
td{padding:5px;}th{padding:10px;border-bottom:1px solid #aaa;background-color:Gainsboro;}
td,th:not(:first-child){border-left:1px solid #aaa;}
</style>

Lil Quick Reference
===================
Types
-----
| `typeof`     | False      | Examples                            |
| :----------- | :--------- | :---------------------------------- |
| `"number"`   | `0`        | `42` `37.5` `-29999`                |
| `"string"`   | `""`       | `"foo\nbar"`                        |
| `"list"`     | `()`       | `11,22,33` `list 3`                 |
| `"dict"`     | `()dict()` | `("a","b") dict 11,22`              |
| `"table"`    | n/a        | `table range 2` `insert a:1 into 0` |
| `"function"` | n/a        | `on x y do x+y end`                 |

Primitives
----------
| Valence | Purpose    |                                                                             |
| :------ | :--------- | :-------------------------------------------------------------------------- |
| monad   | arithmetic | `-` `!` `floor` `cos` `sin` `tan` `exp` `ln` `sqrt` `mag` `unit` `heading`  |
| monad   | reducers   | `count` `first` `last` `sum` `min` `max` `raze`                             |
| monad   | data       | `range` [0...n) `list` (enlist) `rows` `cols` `table` `typeof` `flip`       |
| dyad    | arithmetic | `+` `-` `*` `/` `%` (y mod x) `^` (pow) `&` (min) <code>\|</code> (max)     |
| dyad    | logical    | `<` `>` `=` (conforming equal) `~` (match) `unless` (x if y is `0`)         |
| dyad    | string     | `fuse` `split` `parse` `format`                                             |
| dyad    | data       | `,` (concat) `@` (index each right) `dict` `take` `drop` `limit` `in`       |
| dyad    | joins      | `join` (natural join/zip) `cross` (cross join/cartesian product)            |

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
- `select  n:x where a by b orderby c asc from d` reorder, compute, or filter a table
- `update  n:x where a by b orderby c asc from d` modify rows/columns of a table in place
- `extract n:x where a by b orderby c asc from d` like select, but yields non-tabular values
- `insert  n:x into d` append to a table, or `into 0` to make a new table
- `index`: original row number
- `gindex`: original index within row's group
- `group`: index of row's group (by appearance)
- `column`: the entire current subtable, for referencing denormal columns

Formatting
----------
A format is `%*-0N.DX`: `*` skip, `0` pad, `N` width.<br/>
`-` is invert char class (`ro`) or left justify.<br/>
`.D` is decimal places (`fc`), size of char class (`ro`), or truncate to `D` characters.

| `X`       | Purpose  |                                                                                                |
| :-------- | :------- | :--------------------------------------------------------------------------------------------- |
| `%nmz`    | Parsing  | literal `%`, number of chars read, matches? matches to end?                                    |
| `ro`      | Matching | repeat (0 or more in char class), optional (0 or 1 in char class)                              |
| `sula`    | String   | string, uppercase string, lowercase string, ASCII chars                                        |
| `bficChH` | Number   | bool, float, int, currency (`-$1.23`), plain currency (`-1.23`), hex lowercase, hex uppercase  |
| `jep`     | Misc.    | JSON, unix epoch, time-parts {`year`, `month`, `day`, `hour`, `minute`, `second`}              |
