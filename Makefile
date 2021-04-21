
# Executable
BINS = uicdemod

# Compilation flags
CFLAGS = -Wall -pedantic -O2
LDLIBS = -lm -lpulse -lpulse-simple

# Commands
INSTALL = /usr/bin/install -D
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

# Directories
prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin

HEADERS := $(wildcard *.h)
OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))
LIBSOBJ := $(filter-out $(BINS:%=%.o),$(OBJECTS))

# Disable built-in wildcard rules
.SUFFIXES:

# Keep objects to speed up recompilation
.PRECIOUS: %.o

# Default target: compile all programs
all: $(BINS)

%: %.o $(LIBSOBJ)
	$(CC) $(CFLAGS) $(LIBSOBJ) $< -o $@ $(LDLIBS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean targets
clean:
	$(RM) $(OBJECTS) $(BINS)

# Install
install: $(BINS:%=install_%)

install_%: %
	$(INSTALL_PROGRAM) $< $(DESTDIR)$(bindir)/$<

# Uninstall
uninstall: $(BINS:%=uninstall_%)

uninstall_%: %
	$(RM) $(DESTDIR)$(bindir)/$<
