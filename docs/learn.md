Learn Lil in 10 Minutes
=======================
Lil is a multi-paradigm scripting language primarily used by [Decker](http://beyondloom.com/decker/).

This guide is designed to help users with prior programming experience get up to speed with Lil quickly, emphasizing example code over lengthy explanations. Lil's documentation also includes a much more detailed [language reference manual](lil.html), a [quick-reference card](lilquickref.html), and a [reference manual for Decker, including its programming APIs](decker.html).

Feel free to try examples as you work through this guide in Lil's [online sandbox](http://beyondloom.com/tools/trylil.html), the Decker [Listener](decker.html#thelistener), or from the comfort of your command line with [Lilt](lilt.html).

```lil
# an octothorpe begins a line comment.
# all whitespace is equivalent; indentation is stylistic.

###################################################
#
#  1. Simple Datatypes and Variables
#
###################################################

"one"                          # strings are enclosed in double-quotes,
"two\nthree\"four"             # and may include the escapes \n \" \\.

-23.84                         # numbers are floating point values.

2*3+5          # -> 16         # expressions evaluate right-to-left,
(2*3)+5        # -> 11         # ...unless overridden by parentheses.

()             # -> ()         # the empty list
11,22          # -> (11,22)    # "," joins values to form lists.
list 42        # -> (42)       # "list" makes a list of count 1.

a:42           # -> 42         # the symbol ":" ("becomes") performs assignment.
z              # -> nil        # referencing unbound variables returns nil.

d:("a","b") dict 11,22         # "dict" makes a dictionary from lists of keys and values.
keys  d        # -> ("a","b")  # "keys" extracts the keys of a dict.
range d        # -> (11,22)    # "range" extracts the elements of a dict.
range 4        # -> (0,1,2,3)  # ...or generates a sequence of integers [0,n).

(3,5,7)[1]     # -> 5          # lists can be indexed with [], and count from 0.
d.a            # -> 11         # dicts can be indexed with a dot and a name,
d["b"]         # -> 22         # ...or equivalently, with []; required for non-string keys.

nil+2          # -> 2          # nil will coerce to identity values- 0, "", ()- as needed.
5 unless nil   # -> 5          # 'unless' will replace nil with another value.
5 unless 0     # -> 0          # ...but leave falsey values intact.
5 fill 2,nil,3 # -> (2,5,3)    # replace nils in bulk with 'fill'.

# collection operators:
count    11,22,33    # -> 3
first    11,22,33    # -> 11
last     11,22,33    # -> 33
2 take   11,22,33    # -> (11,22)
2 drop   11,22,33    # -> (33)
typeof   11,22,33    # -> "list"
2 window 11,22,33,44 # -> ((11,22),(33,44))

###################################################
#
#  2. Control Flow
#
###################################################

# the "each" loop iterates over the elements of a list, string, or dict:
each v k i in ("a","b","c") dict 11,22,33
 # up to three variable names: value, key, and index.
 show[2*v k i] # show[] prettyprints values to stdout.
end
# prints:
# 22 "a" 0
# 44 "b" 1
# 66 "c" 2
# ...and returns the dictionary {"a":22,"b":44,"c":66}.

# the "while" loop:
v:2
while v<256
 show[v:v*4]
end
# prints:
# 8
# 32
# 128
# 512
# ...and returns the number 512.

# conditionals:
if age<21
 "can't rent cars"
elseif age>65
 "senior discounts"
else
 "just a regular slob"
end

# "if" and "while" treat 0, nil, or the empty string, list, or dict as "falsey".
# any other value is considered "truthy".
if 3  "yes" end  # -> "yes"
if () "no"  end  # -> nil

###################################################
#
#  3. Functions and Scope
#
###################################################

on myfunc x y do     # functions are declared with "on".
 show[x y]           # function body can contain any number of expressions.
 x + 10 * y          # function body returns the last expression.
end

# call functions with []
# parameters are NOT separated by commas!
myfunc[2 5] # -> 52

# functions are values, and can be passed around:
on twice f x do
 f[f[x]]
end
twice[on double x do x*2 end  10] # -> 40

# functions can be given a variadic argument with "..."
on vary ...x do
 show[x]
end
f[11 22 33]          # -> (11,22,33)

# variables have lexical scope:
on outer x do        # function argument defines local x.
 on inner x do       # function argument shadows outer x.
  x:x*2              # redefine the local x, don't mutate the outer x.
  show[x]            # -> 10
  local z:99         # "local" explicitly declares a local variable.
  show[z]            # -> 99
 end
 inner[x]
 show[x]             # -> 5
 show[z]             # -> nil # z is not defined in this scope.
end
outer[5]

###################################################
#
#  4. Tables and Queries
#
###################################################

# make tables with "insert":
people:insert name age job with
 "Alice"  25 "Development" 
 "Sam"    28 "Sales"
 "Thomas" 40 "Development"
 "Sara"   34 "Development"
 "Walter" 43 "Accounting"
end

# tables are like string-keyed dictionaries of uniform-count columns.
# tables are like a list of dictionaries with the same string keys.
people.name[2]  # -> "Thomas"
people[2].name  # -> "Thomas"

# query tables using a SQL-like syntax:
select from people
# +----------+-----+---------------+
# | name     | age | job           |
# +----------+-----+---------------+
# | "Alice"  | 25  | "Development" |
# | "Sam"    | 28  | "Sales"       |
# | "Thomas" | 40  | "Development" |
# | "Sara"   | 34  | "Development" |
# | "Walter" | 43  | "Accounting"  |
# +----------+-----+---------------+

# naming and computing columns, filtering with "where":
select firstName:name dogYears:7*age where job="Development" from people
# +-----------+----------+
# | firstName | dogYears |
# +-----------+----------+
# | "Alice"   | 175      |
# | "Thomas"  | 280      |
# | "Sara"    | 238      |
# +-----------+----------+

# summarizing with "by" (groupby):
select job:first job employees:count name by job from people
# +---------------+-----------+
# | job           | employees |
# +---------------+-----------+
# | "Development" | 3         |
# | "Sales"       | 1         |
# | "Accounting"  | 1         |
# +---------------+-----------+

# "update" works like "select" but patches selected cells of the table.
# note that tables, like lists and dicts, are immutable!
# "update" returns a new, amended table value and does not reassign the variable "people":
update job:"Software Engineering" where job="Development" from people
# +----------+-----+------------------------+
# | name     | age | job                    |
# +----------+-----+------------------------+
# | "Alice"  | 25  | "Software Engineering" |
# | "Sam"    | 28  | "Sales"                |
# | "Thomas" | 40  | "Software Engineering" |
# | "Sara"   | 34  | "Software Engineering" |
# | "Walter" | 43  | "Accounting"           |
# +----------+-----+------------------------+

# queries apply to strings, lists, dicts, with columns "key", "value", "index".
# sorting with "orderby" (asc for ascending, desc for descending):
select index value orderby value asc from "BACB"
# +-------+-------+
# | index | value |
# +-------+-------+
# | 1     | "A"   |
# | 0     | "B"   |
# | 3     | "B"   |
# | 2     | "C"   |
# +-------+-------+

# "extract" works like "select", but returns a list instead of a table:
extract name orderby name asc from people
# -> ("Alice","Sam","Sara","Thomas","Walter")

# if you don't specify a column expression, extracts the first column:
extract orderby name asc from people
# (same as above)

# more table and collection operators:
a join b    # natural join (by column name).
a cross b   # cross join / cartesian product.
a , b       # concatenate rows of two tables.
3 limit t   # at most 3 rows of a table.
rows t      # list of rows as dictionaries.
cols t      # dictionary of columns as lists.
table x     # make a table from dict-of-list / list-of-dict.
raze t      # form a dictionary from the first two columns of a table.
flip x      # transpose a table's rows and columns.

###################################################
#
#  5. String Manipulation
#
###################################################

",|" split "one,|two,|three"    # break a string on a delimiter.
# -> ("one","two","three")

"::" fuse ("one","two","three") # concatenate strings with a delimiter.
# -> "one::two::three"

# the "format" operator uses a printf()-like format string
# to control converting one or more values into a string:
"%i : %s : %f" format (42,"Apple",-23.7)
# -> "42 : Apple : -23.7"

"%08.4f" format pi           # 0-padding, field width, precision.
# -> "003.1416"

"%l : %u" format "One","Two" # lowercase or uppercase.
# -> "one : TWO"

d:("a","b") dict 11,22
"%j" format list d           # JSON.
# -> "{\"a\":11,\"b\":22}"

# Lil Object-Value Encoding (LOVE) is a JSON superset
# which supports non-string dict keys, tables,
# and serializing image, array, and sound interfaces:
"%J" format (11,22) dict 33,44
"{11:33,22:44}"

# the "parse" operator tokenizes a string into values.
# "parse" and "format" use the same pattern syntax:
"%v[%i]" parse "foo[23]"
# -> ("foo",23)

"%j" parse "{'foo':[1,2,],'bar':34.5}"  # tolerant JSON parsing.
# -> {"foo":(1,2),"bar":34.5}

"< %[b]s %[a]s >" format d              # named elements.
# -> "< 22 11 >"

# the "like" operator performs glob-matching:
"foo"       like "f*"  # -> 1      (prefix match; * matches 0 or more chars)
"foo"       like "*oo" # -> 1      (suffix match)
"fxo"       like "f.o" # -> 1      (. matches any single char)
"a4"        like "a#"  # -> 1      (# matches any single digit)
"c#"        like ".`#" # -> 1      (` escapes special chars)
("a2","b2") like "a#"  # -> (1,0)  (left argument can be a column)

###################################################
#
#  6. Implicit Iteration
#
###################################################

# arithmetic operators "conform" over lists;
# this is how column expressions in queries work, too:
1+2                 # -> 3              (number plus number)
1+10,20,30          # -> (11,21,31)     (number plus each of list)
(10,20,30)+1        # -> (11,22,31)     (each of list plus number)
(10,100,1000)+1,2,3 # -> (11,102,1003)  (each of list plus each of list)

# the "=" equality operator conforms; use in query expressions:
3=3                 # 1
3=2,3,5             # (0,1,0)

# the "~" equality operator compares entire values; use with "if":
3~3                 # -> 1
3~2,3,5             # -> 0

# the "@" operator applies the left argument to each item on the right:
(11,22,33) @ 2,1,2       # -> (33,22,33)    (indexing a list)
double @ 10,20           # -> (20,40)       (applying a function)
first @ ("Alpha","Beta") # -> ("A","B")     (applying a unary operator)

# ".." is shorthand for "at every index":
t:"%j" parse "[{'a':22,'b':33},{'a':55}]"
t..a                     # -> 22,55
each v in t v.a end      # -> 22,55

# unary operators for reducing lists to a residue:
sum  1,2,3                    # -> 6
prod 2,3,4                    # -> 24       (product)
min  9,5,7                    # -> 5        (minimum; all)
max  9,5,7                    # -> 9        (maximum; some)
raze ((list 1,2),(list 3,4))  # -> 1,2,3,4  (flatten nesting)

###################################################
#
#  7. Arithmetic and Logic
#
###################################################

2 ^ 3           # -> 8                (exponentiation)
5 % 3,4,5,6,7   # -> (3,4,0,1,2)      (y modulo x)
3 | 5           # -> 5                (maximum; logical OR  for 0/1)
3 & 5           # -> 3                (minimum; logical AND for 0/1)
3 < 2,3,5       # -> (0,0,1)          (less)
3 > 2,3,5       # -> (1,0,0)          (more)
! 0,1,3,5       # -> (1,0,0,0)        (not)

# no x<=y or x>=y operators; use !x>y or !x<y instead

# named unary operators, which all conform:
# floor cos sin tan exp ln sqrt

# the constants "pi" and "e" are predefined as globals

mag 3,4           # -> 5              (vector magnitude)
u:unit pi/3       # -> (0.5,0.866025) (unit vector at angle)
(pi/3)~heading u  # -> 1              (angle of vector)

###################################################
#
#  8. Interfaces and Built-in Functions
#
###################################################

# interfaces are dictionary-like values representing system resources, mutable data.
# singleton utility interfaces:
sys.now                                       # sys: workspace info, rng, time
bits.xor[3 42]                                # bits: bit-wise arithmetic
rtext.replace["the orange!" "orange" "apple"] # rtext: rich text tables

# built-in functions for constructing instances of certain interfaces:
arr:array[16 "i16b"] # array: a 1D mutable array for manipulating binary data
img:image[(10,20)]   # image: a 2D mutable byte array for graphics operations

sys~sys              # -> 1          # interfaces compare with reference-equality.
sys=(sys,sys,123)    # -> (1,1,0)
typeof sys           # -> "system"   # two ways to identify interface type.
sys.type             # -> "system"
keys sys             # -> ()         # interfaces are non-enumerable!

# more built-in functions:
eval[str vars] # evaluate a string of Lil with a dict of local variable bindings
random[x]      # choose a random element from a string, list, dict, or table
readcsv[x]     # parse a string with CSV data into a table
writecsv[x]    # format a table into a CSV string
readxml[x]     # parse an XML/HTML document into a tree of dictionaries
writexml[x]    # format a tree of dictionaries into well-formed XML

###################################################
#
#  9. Decker
#
###################################################

# Decker is a GUI-builder like HyperCard or VisualBASIC:
go["http://beyondloom.com/decker/"]

# the parts of a "Deck" have corresponding interfaces, and can be given scripts.
# if a Widget, Card, or Deck contains function definitions with specific names,
# they will be called in response to events:
on click do                  # (imagine this is in a Button's script)
 me                          # the current Widget's interface.
 card                        # the Card interface containing the Widget.
 deck                        # the Deck interface containing the Card.
 myField.text:1+myField.text # we can refer to other widgets on the same card by name.

 # we can also refer to Cards in the Deck by name.
 # for a Widget on another Card, index through "card.widgets":
 f:otherCard.widgets.otherField
 f.text:f.text+1

 # we can send "synthetic" events to other parts of the Deck:
 otherButton.event["click"]
end

# each event handler is its own fresh universe; variables do not persist.
# the only state preserved between events is state stored in Widgets!
field.text          # string.
button.value        # 1/0 boolean.
slider.value        # number.
grid.value          # table.
canvas.copy[]       # Image interface.
# ...and so on. Contraptions are custom widgets, with a user-defined API.

# we can also treat Cards like records, with consistently-named Widgets as their fields.
# consider a checkbox on each Card in a Deck tracking whether it had been visited:
on view do          # (in the script for each Card) 
 visited.value:1    # "visited" is a checkbox.
end

# we could then find the Cards that have been visited so far with a query:
extract value where value..widgets.visited.value from deck.cards

# ...or reset all our visited flags:
deck.cards..widgets.visited.value:0

# we can override builtins and insert side-effects:
on go x trans delay do
 if trans show["start transition: " trans] end
 # "send" here calls the definition of "go" in the next-highest lexical scope:
 send go[x trans delay]
end

# if unhandled, events "bubble up": Widget -> Card -> Deck.
# to prevent bubbling, define an empty handler:
on navigate x do end

# some important attributes that apply to all Widgets:
x.pos        # (x,y) position on the Card, in pixels.
x.size       # (width,height) dimensions, in pixels.
x.name       # string uniquely identifying this Widget on a Card.
x.index      # drawing order on the Card, back-to-front.
x.show       # one of "solid", "invert", "transparent", or "none"
```
