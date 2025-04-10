VERSION=$(shell cat VERSION)
UNAME=$(shell uname)
EXTRA_FLAGS?=

ifneq ("$(wildcard /usr/bin/olpc-hwinfo)","")
	# building on an OLPC; use SDL 1.2
	SDL=$(shell sdl-config --cflags --libs)
	SDL:=$(SDL) -lSDL_image
else
	SDL=$(shell sdl2-config --cflags --libs)
	SDL:=$(SDL) -lSDL2_image
endif
ifeq ($(UNAME),Darwin)
	OPEN=open
	COMPILER=clang
	FLAGS=-Wall -Werror -Wextra -Wpedantic -Os -Wstrict-prototypes
	# -Wno-misleading-indentation silences warnings which are entirely spurious.
	FLAGS:=$(FLAGS) -Wno-misleading-indentation -Wno-unknown-warning-option
	# -Wno-overlength-strings works around a standards limitation which in practice is still portable.
	FLAGS:=$(FLAGS) -Wno-overlength-strings
	# FLAGS:=$(FLAGS) -fsanitize=undefined
	# FLAGS:=$(FLAGS) -fsanitize=address
endif
ifeq ($(UNAME),Linux)
	OPEN=xdg-open
	COMPILER=gcc
	# _BSD_SOURCE is required by older versions of GCC to find various posix extensions like realpath().
	# _DEFAULT_SOURCE is the same deal, except newer versions of GCC need it
	# _POSIX_C_SOURCE is also needed by bestline on older versions of GCC
	# -lm is required for math.h
	FLAGS=-std=c99 -D _BSD_SOURCE -D _DEFAULT_SOURCE -D _POSIX_C_SOURCE -lm -Wall -Wextra -O2
	# -Wno-misleading-indentation silences warnings which are entirely spurious.
	# -Wno-format-truncation likewise silences spurious warnings regarding snprintf() truncation.
	FLAGS:=$(FLAGS) -Wno-misleading-indentation -Wno-format-truncation
endif
ifeq ($(UNAME),OpenBSD)
	OPEN=xdg-open
	COMPILER=clang
	FLAGS=-Wall -Werror -Wextra -Wpedantic -O2
	# -Wno-misleading-indentation silences warnings which are entirely spurious.
	FLAGS:=$(FLAGS) -Wno-misleading-indentation -Wno-unknown-warning-option
	FLAGS:=$(FLAGS) -lm
endif
ifeq ($(UNAME),NetBSD)
	# Required packages: bash, gmake, SDL2, SDL2_image, xdg-tools
	OPEN=xdg-open
	COMPILER=gcc
	FLAGS=-std=c99 -lm -Wall -Wextra -O2
	FLAGS:=$(FLAGS) -Wno-misleading-indentation -Wno-format-truncation
endif
ifneq ("$(EXTRA_FLAGS)","")
	FLAGS:=$(FLAGS) $(EXTRA_FLAGS)
endif
# give the user the option for a verbose build by doing make V=1
ifneq ($(V),1)
	Q=@
endif

# include potentially unsafe/nonportable scripting APIs
# FLAGS:=$(FLAGS) -DDANGER_ZONE

resources:
	$(Q)chmod +x ./scripts/resources.sh
	$(Q)./scripts/resources.sh examples/decks/tour.deck

lilt: resources
	$(Q)mkdir -p c/build
	$(Q)$(COMPILER) ./c/lilt.c -o ./c/build/lilt $(FLAGS) -DVERSION="\"$(VERSION)\""

decker: resources
	$(Q)mkdir -p c/build
	$(Q)$(COMPILER) ./c/decker.c -o ./c/build/decker $(SDL) $(FLAGS) -DVERSION="\"$(VERSION)\""

clean:
	$(Q)rm -rf ./c/build/
	$(Q)rm -rf ./js/build/
	$(Q)rm -f docs/*.html

install:
	$(Q)chmod +x ./scripts/install.sh
	$(Q)./scripts/install.sh

uninstall:
	$(Q)chmod +x ./scripts/uninstall.sh
	$(Q)./scripts/uninstall.sh

test: lilt
	$(Q)chmod +x ./scripts/test_interpreter.sh
	$(Q)./scripts/test_interpreter.sh "./c/build/lilt "
	$(Q)./c/build/lilt tests/dom/arrays.lil
	$(Q)./c/build/lilt tests/dom/images.lil
	$(Q)./c/build/lilt tests/dom/domtests.lil
	$(Q)./c/build/lilt tests/dom/test_roundtrip.lil
	$(Q)./c/build/lilt tests/puzzles/weeklychallenge.lil

run: lilt
	$(Q)./c/build/lilt

rundecker: decker
	./c/build/decker

.PHONY: jsres
js: jsres
	$(Q)mkdir -p js/build/
	$(Q)echo "VERSION=\"${VERSION}\"" > js/build/lilt.js
	$(Q)cat js/lil.js js/repl.js >> js/build/lilt.js

testjs: js
	$(Q)chmod +x ./scripts/test_interpreter.sh
	$(Q)./scripts/test_interpreter.sh "node js/build/lilt.js"
	$(Q)node js/build/lilt.js tests/dom/arrays.lil
	$(Q)node js/build/lilt.js tests/dom/images.lil
	$(Q)node js/build/lilt.js tests/dom/domtests.lil
	$(Q)node js/build/lilt.js tests/dom/test_roundtrip.lil
	$(Q)node js/build/lilt.js tests/puzzles/weeklychallenge.lil

testawk:
	$(Q)chmod +x ./scripts/test_interpreter.sh
	$(Q)./scripts/test_interpreter.sh "awk -f tools/awk/lila.awk"
	$(Q)awk -f tools/awk/lila.awk tests/dom/arrays.lil
	$(Q)awk -f tools/awk/lila.awk tests/dom/images.lil
	$(Q)awk -f tools/awk/lila.awk tests/puzzles/weeklychallenge.lil

web-decker: js
	$(Q)chmod +x ./scripts/web_decker.sh
	$(Q)./scripts/web_decker.sh examples/decks/tour.deck js/build/decker.html $(VERSION)

runweb: web-decker
	$(OPEN) js/build/decker.html

.PHONY: docs
docs:
	$(Q)./c/build/lilt scripts/lildoc.lil docs/lil.md         docs/lil.html
	$(Q)./c/build/lilt scripts/lildoc.lil docs/lilt.md        docs/lilt.html
	$(Q)./c/build/lilt scripts/lildoc.lil docs/decker.md      docs/decker.html
	$(Q)./c/build/lilt scripts/lildoc.lil docs/format.md      docs/format.html
	$(Q)./c/build/lilt scripts/lildoc.lil docs/lilquickref.md docs/lilquickref.html
	$(Q)./c/build/lilt scripts/lildoc.lil docs/learn.md       docs/learn.html
