CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O3 -flto
LDFLAGS = -pthread -flto

ifdef DEBUG
CXXFLAGS = -std=c++17 -Wall -Wextra -g -fsanitize=address,leak,undefined
LDFLAGS = -pthread -fsanitize=address,leak,undefined
endif

ifdef HTTPS
CXXFLAGS += -DENABLE_HTTPS
LDFLAGS += -lssl -lcrypto
SRCS_SSL = utils/ssl.cpp
endif

SRCS = main.cpp \
       config/config.cpp \
       http/parser.cpp \
       core/koneksi.cpp \
       server/server.cpp \
       utils/logger.cpp \
       $(SRCS_SSL)

OBJS = $(patsubst %.cpp, build/%.o, $(SRCS))
TARGET = build/synx

$(shell mkdir -p build/config build/http build/core build/server build/utils)

VERSION = 0.1.0
ARCH = $(shell uname -m)
RELEASE_NAME = $(TARGET)-$(VERSION)-linux-$(ARCH)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)
	@rm -f $(OBJS)

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build/

https:
	$(MAKE) HTTPS=1 clean all

debug:
	$(MAKE) DEBUG=1 clean all

run: $(TARGET)
	./$(TARGET)

install:
	@if [ -f synx ]; then \
		cp synx /usr/local/bin/synx; \
	elif [ -f $(TARGET) ]; then \
		cp $(TARGET) /usr/local/bin/synx; \
	else \
		echo "Harap jalankan 'make' terlebih dahulu, atau download file binary release."; \
		exit 1; \
	fi
	@if [ ! -f /etc/synx.conf ]; then cp synx.conf /etc/synx.conf 2>/dev/null || true; fi
	@echo "synx offline installer sukses dipasang di /usr/local/bin/synx"

uninstall:
	rm -f /usr/local/bin/synx
	rm -f /etc/synx.conf
	@echo "synx uninstalled"

release: https
	strip $(TARGET)
	tar -czvf $(RELEASE_NAME).tar.gz -C build synx ../synx.conf ../README.md ../LICENSE
	sha256sum $(RELEASE_NAME).tar.gz > $(RELEASE_NAME).tar.gz.sha256
	@echo "Release build: $(RELEASE_NAME).tar.gz"

release-binary: https
	strip $(TARGET)
	@echo "Binary ready: $(TARGET)"

test: $(TARGET)
	./$(TARGET) --benchmark & \
	PID=$$!; \
	sleep 2; \
	curl -s http://localhost:8080/ > /dev/null; \
	kill $$PID || true; \
	wait $$PID || true; \
	echo "Tests passed"

valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

.PHONY: all clean https debug run install uninstall test valgrind release release-binary
