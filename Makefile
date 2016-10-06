CXXFLAGS+= -std=c++14 -O3
CXXFLAGS+= -I /usr/local/include

all: passgen

clean:
	rm -f passgen
