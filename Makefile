CC=gcc

ifeq ($(HOME),/root)
DB_DIR=/home/$(SUDO_USER)/Documents/Book-Manager
else
DB_DIR=$(HOME)/Documents/Book-Manager
endif

ifeq ($(HOME),/root)
fig_DIR=/home/$(SUDO_USER)/.Book-Manager
else
fig_DIR=$(HOME)/.Book-Manager
endif

CFLAGS= -Wall `pkg-config --cflags gtk+-3.0` -DDB_DIR=\"$(DB_DIR)\" -Dfig_DIR=\"$(fig_DIR)\"
LDFLAGS= -lm -lsqlite3 `pkg-config --libs gtk+-3.0`


.PHONY: all install clean show

all: book-manager

book-manager: main.o
	$(CC) -o $@ $^ $(LDFLAGS)

main.o: main.c
	$(CC) $(CFLAGS) -c $<
	rm -r $(DB_DIR)
	mkdir -m 777 $(DB_DIR)

install:
	install -m 755 book-manager /usr/local/bin/
	install -m 644 Data/book-manager.desktop /usr/share/applications/
	install -m 644 Figures/book-manager_icon.png /usr/share/icons/hicolor/48x48/apps/
	install -m 777 Data/books.db $(DB_DIR)
	install -d $(fig_DIR)
	install -m 644 Figures/books.png $(fig_DIR)

clean:
	rm -f *.o book-manager
	
show:
	printf "Book Manager Successfully Instaled"!!!
