CC=~/android-ndk/toolchain/bin/clang
CFLAGS=-Wall -pie
BIN=client

all: $(BIN)

$(BIN): client.c
	$(CC) $(CFLAGS) client.c -o $(BIN)

.PHONY: deploy

deploy:
	adb push $(BIN) /data/local/mbz

.PHONY: clean

clean: 
	rm -f $(BIN)
