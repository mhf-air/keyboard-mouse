
# ================================================================================
CC = gcc

INCLUDE = \
	# -I$(GLFW)/deps -I$(GLFW)/include \

LINK = \
	`pkg-config --libs x11` \
	# -L$(GLFW)/src \
	# `pkg-config --static --libs vulkan glfw3 assimp glib-2.0` \
	# -Wl,-rpath,"\$$ORIGIN/$(ASSIMP)/lib"

MAIN = .a

# ================================================================================
.PHONY: all
all: $(MAIN)

$(MAIN): src/a.c src/a.h
	$(CC) -o $(MAIN) src/a.c $(INCLUDE) $(LINK)

# ================================================================================
.PHONY: clean

clean:
	rm -f $(MAIN)
