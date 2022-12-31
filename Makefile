VERSION=$(shell cat VERSION)
UNAME=$(shell uname)
SDL=$(shell sdl2-config --cflags --libs)

ifeq ($(UNAME),Darwin)
	OPEN=open
	COMPILER=clang
	FLAGS=-Wall -Werror -Wextra -Wpedantic -Os
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

resources:
	@chmod +x ./scripts/resources.sh
	@./scripts/resources.sh examples/decks/tour.deck

lilt: resources
	@mkdir -p c/build
	@$(COMPILER) ./c/lilt.c -o ./c/build/lilt $(FLAGS) -DVERSION="\"$(VERSION)\""

decker: resources
	@mkdir -p c/build
	@$(COMPILER) ./c/decker.c -o ./c/build/decker $(SDL) -lSDL2_image $(FLAGS) -DVERSION="\"$(VERSION)\""

clean:
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
	@./c/build/lilt tests/dom/domtests.lil
	@./c/build/lilt tests/dom/test_roundtrip.lil

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
	@node js/build/lilt.js tests/dom/domtests.lil
	@node js/build/lilt.js tests/dom/test_roundtrip.lil

web-decker: js
	@chmod +x ./scripts/web_decker.sh
	@./scripts/web_decker.sh examples/decks/tour.deck js/build/decker.html $(VERSION)

runweb: web-decker
	$(OPEN) js/build/decker.html

.PHONY: docs
docs:
	@multimarkdown docs/lil.md    > docs/lil.html
	@multimarkdown docs/lilt.md   > docs/lilt.html
	@multimarkdown docs/decker.md > docs/decker.html
	@multimarkdown docs/format.md > docs/format.html
