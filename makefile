CXX = g++
CXXFLAGS =  -O2
LDLIBS = -lresolv

client: client.cpp
	$(CXX) $(CXXFLAGS) client.cpp -o client $(LDLIBS)

oracle: oracle.java
	javac oracle.java
