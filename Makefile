CC := gcc
override CFLAGS += -O3 -Wall

SOURCE := MemManager.c
BINARY := MemManager
DATA_STRUCTURES := tlb_list.c frame_list.c

GIT_HOOKS := .git/hooks/applied
all: $(GIT_HOOKS) $(BINARY)

debug: CFLAGS += -DDEBUG -g
debug: $(GIT_HOOKS) $(BINARY)

$(GIT_HOOKS):
	scripts/install-git-hooks

$(BINARY): $(BINARY).o $(patsubst %.c, %.o, $(DATA_STRUCTURES))
	$(CC) $(CFLAGS) $^ -o $@

$(BINARY).o: $(SOURCE) $(patsubst %.c, %.h, $(SOURCE))
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f *.o $(BINARY)