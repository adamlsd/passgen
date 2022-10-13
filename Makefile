CXXFLAGS+= -std=c++17 -O3
CXXFLAGS+= -I /usr/local/include

all: passgen pingen

clean:
	rm -f passgen
