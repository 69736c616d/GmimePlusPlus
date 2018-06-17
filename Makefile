CXX=g++
CXXFLAGS=  $(CENVFLAGS) -g  -std=c++17 -I/usr/include -I/usr/local/include -I/usr/local/include/gmime-2.6 -Iinclude -I.  -I/usr/include/glib-2.0/ -I/usr/lib64/glib-2.0/include/
LIBS=-L/usr/lib64/ -L/usr/local/lib/ -L/usr/lib  -L../lib  -L/lib64/

SHLIB= -lgmime-2.6 -lglib-2.0 -lgobject-2.0 -lresolv -lcrypto


all: gtest

gtest: gmimepp.o main.o
	$(CXX)  -o gtest main.o gmimepp.o  $(LIBS) $(SHLIB)

clean:
	/bin/rm *.o
	if [ -a gtest ]; then rm gtest; fi;
