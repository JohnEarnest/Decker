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
	FLAGS:=$(FLAGS) -lm -Wno-implicit-const-int-float-conversion
	# -Wno-overlength-strings works around a standards limitation which in practice is still portable.
	FLAGS:=$(FLAGS) -Wno-overlength-strings

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
ifneq ($(V),1)
	Q:=@
endif

# include potentially unsafe/nonportable scripting APIs
# FLAGS:=$(FLAGS) -DDANGER_ZONE

decker: c/build/decker
lilt: c/build/lilt

c/build/decker: c/resources.h c/decker.c c/dom.h c/lil.h
	@mkdir -p c/build
	$(Q)$(COMPILER) ./c/decker.c -o ./c/build/decker $(SDL) $(FLAGS) -DVERSION="\"$(VERSION)\""

c/resources.h: examples/decks/tour.deck js/lil.js js/danger.js js/decker.html js/decker.html
	@chmod +x ./scripts/resources.sh
	$(Q)./scripts/resources.sh examples/decks/tour.deck

c/build/lilt: c/resources.h c/lilt.c c/dom.h c/lil.h
	@mkdir -p c/build
	$(Q)$(COMPILER) ./c/lilt.c -o ./c/build/lilt $(FLAGS) -DVERSION="\"$(VERSION)\""

clean:
	@rm -rf ./c/resources.h
	@rm -rf ./c/build/
	@rm -rf ./js/build/
	@rm -f docs/*.html

install:
	@chmod +x ./scripts/install.sh
	@./scripts/install.sh

uninstall:
	@chmod +x ./scripts/uninstall.sh
	@./scripts/uninstall.sh

test: lilt
	@chmod +x ./scripts/test_interpreter.sh
	@./scripts/test_interpreter.sh "./c/build/lilt "
	@./c/build/lilt tests/dom/arrays.lil
	@./c/build/lilt tests/dom/images.lil
	@./c/build/lilt tests/dom/domtests.lil
	@./c/build/lilt tests/dom/test_roundtrip.lil
	@./c/build/lilt tests/puzzles/weeklychallenge.lil

run: lilt
	@./c/build/lilt

rundecker: decker
	./c/build/decker

.PHONY: jsres
js: jsres
	@mkdir -p js/build/
	@echo "VERSION=\"${VERSION}\"" > js/build/lilt.js
	@cat js/lil.js js/repl.js >> js/build/lilt.js

testjs: js
	@chmod +x ./scripts/test_interpreter.sh
	@./scripts/test_interpreter.sh "node js/build/lilt.js"
	@node js/build/lilt.js tests/dom/arrays.lil
	@node js/build/lilt.js tests/dom/images.lil
	@node js/build/lilt.js tests/dom/domtests.lil
	@node js/build/lilt.js tests/dom/test_roundtrip.lil
	@node js/build/lilt.js tests/puzzles/weeklychallenge.lil

testawk:
	@chmod +x ./scripts/test_interpreter.sh
	@./scripts/test_interpreter.sh "awk -f tools/awk/lila.awk"
	@awk -f tools/awk/lila.awk tests/dom/arrays.lil
	@awk -f tools/awk/lila.awk tests/dom/images.lil
	@awk -f tools/awk/lila.awk tests/puzzles/weeklychallenge.lil

web-decker: js
	@chmod +x ./scripts/web_decker.sh
	@./scripts/web_decker.sh examples/decks/tour.deck js/build/decker.html $(VERSION)

runweb: web-decker
	$(OPEN) js/build/decker.html

.PHONY: docs
docs: lilt
	@./c/build/lilt scripts/lildoc.lil docs/lil.md         docs/lil.html
	@./c/build/lilt scripts/lildoc.lil docs/lilt.md        docs/lilt.html
	@./c/build/lilt scripts/lildoc.lil docs/decker.md      docs/decker.html
	@./c/build/lilt scripts/lildoc.lil docs/format.md      docs/format.html
	@./c/build/lilt scripts/lildoc.lil docs/lilquickref.md docs/lilquickref.html
	@./c/build/lilt scripts/lildoc.lil docs/learn.md       docs/learn.html
