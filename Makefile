CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

CPP_FILES = $(wildcard *.cpp)

baka: $(CPP_FILES)
	g++ $(CFLAGS) -o baka $(CPP_FILES) $(LDFLAGS)

.PHONY: t c

t: baka
	./baka

c:
	rm -f baka