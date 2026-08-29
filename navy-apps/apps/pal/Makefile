REPO_PATH = repo
ifeq ($(wildcard repo/include/palcommon.h),)
  $(shell mkdir -p $(NAVY_HOME)/fsimg/share/games)
  $(shell git clone --depth=1 https://github.com/NJU-ProjectN/pal-navy.git $(REPO_PATH))
endif

PAL_DATA_DIR ?= $(REPO_PATH)/data
PAL_REQUIRED_FILES = abc.mkf ball.mkf data.mkf f.mkf fbp.mkf fire.mkf \
                     gop.mkf map.mkf mgo.mkf pat.mkf rgm.mkf rng.mkf \
                     sss.mkf sdlpal.cfg

NAME = pal
SRCS = $(shell find $(REPO_PATH)/src/ -name "*.c" -or -name "*.cpp")
INC_PATH = $(REPO_PATH)/include
LIBS += libminiSDL libfixedptc
include $(NAVY_HOME)/Makefile

install: install-file
init:;

check-data:
	@if [ ! -d "$(PAL_DATA_DIR)" ]; then \
		echo "PAL game data not found: $(abspath $(PAL_DATA_DIR))" >&2; \
		echo "Set PAL_DATA_DIR to a licensed PAL data directory." >&2; \
		exit 1; \
	fi
	@missing=""; \
	for file in $(PAL_REQUIRED_FILES); do \
		if [ ! -f "$(PAL_DATA_DIR)/$$file" ]; then missing="$$missing $$file"; fi; \
	done; \
	if [ -n "$$missing" ]; then \
		echo "PAL game data is incomplete or not lower-case." >&2; \
		echo "Missing from $(abspath $(PAL_DATA_DIR)):$$missing" >&2; \
		exit 1; \
	fi
	@config="$(PAL_DATA_DIR)/sdlpal.cfg"; \
	get_config() { \
		awk -v wanted="$$1" ' \
			function trim(value) { sub(/^[ \t]+/, "", value); sub(/[ \t\r]+$$/, "", value); return value } \
			{ line = $$0; separator = index(line, "="); \
			  if (separator == 0) next; \
			  key = tolower(trim(substr(line, 1, separator - 1))); \
			  value = trim(substr(line, separator + 1)); \
			  if (key == wanted && value != "") result = value; } \
			END { print result }' "$$config"; \
	}; \
	width="$$(get_config windowwidth)"; \
	height="$$(get_config windowheight)"; \
	if [ "$$width" != 320 ] || [ "$$height" != 200 ]; then \
		echo "PAL sdlpal.cfg must set WindowWidth=320 and WindowHeight=200." >&2; \
		exit 1; \
	fi; \
	game_path="$$(get_config gamepath)"; \
	case "$$game_path" in \
		""|/share/games/pal|/share/games/pal/) ;; \
		*) echo "PAL GamePath must be /share/games/pal/ or omitted." >&2; exit 1 ;; \
	esac; \
	message_file="$$(get_config messagefilename)"; \
	if [ -n "$$message_file" ]; then \
		case "$$message_file" in \
			/share/games/pal/*) message_rel=$${message_file#/share/games/pal/} ;; \
			/*) echo "PAL MessageFileName must be relative to /share/games/pal/." >&2; exit 1 ;; \
			*) message_rel="$$message_file" ;; \
		esac; \
		case "$$message_rel" in \
			""|../*|*/../*|*/..) echo "PAL MessageFileName escapes the game data directory." >&2; exit 1 ;; \
			*) ;; \
		esac; \
		if [ ! -f "$(PAL_DATA_DIR)/$$message_rel" ]; then \
			echo "PAL MessageFileName is missing: $(abspath $(PAL_DATA_DIR))/$$message_rel" >&2; \
			exit 1; \
		fi; \
	elif [ ! -f "$(PAL_DATA_DIR)/m.msg" ] || [ ! -f "$(PAL_DATA_DIR)/word.dat" ]; then \
		echo "PAL text data requires both m.msg and word.dat," >&2; \
		echo "or an existing MessageFileName entry in sdlpal.cfg." >&2; \
		exit 1; \
	fi

install-file: check-data
	@mkdir -p $(NAVY_HOME)/fsimg/share/games
	ln -sf -T $(abspath $(PAL_DATA_DIR)) $(NAVY_HOME)/fsimg/share/games/pal

.PHONY: install-file init
