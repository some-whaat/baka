CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

baka: main.cpp
	g++ $(CFLAGS) -o baka main.cpp $(LDFLAGS)

.PHONY: test clean

test: baka
	./baka

clean:
	rm -f baka