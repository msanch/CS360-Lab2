server: server.cpp
	g++ -std=c++11 -o server server.cpp

sigint: sigint.cpp
	g++ -o sigint sigint.cpp

stat: stat.cpp
	g++ -o stat stat.cpp
