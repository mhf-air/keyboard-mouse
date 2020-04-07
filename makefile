
# ================================================================================
CC = gcc

INCLUDE = \
	# -I$(GLFW)/deps -I$(GLFW)/include \

LINK = \
	`pkg-config --libs x11 xtst`

MAIN = .a
EXE = ~/.bin/aa/keyboard-mouse

# ================================================================================
.PHONY: all
all: $(MAIN)

$(MAIN): src/a.c src/a.h
	$(CC) -o $(MAIN) src/a.c $(INCLUDE) $(LINK)
	sudo chown root:root $(MAIN)
	sudo chmod 4775 $(MAIN)
	sudo cp $(MAIN) $(EXE)
	sudo chown root:root $(EXE)
	sudo chmod 4775 $(EXE)

# ================================================================================
.PHONY: clean

clean:
	rm -f $(MAIN)
