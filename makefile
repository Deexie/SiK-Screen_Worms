FLAGS=-Wall -Wextra -O2 -std=c++17

all: screen-worms-server

screen-worms-server: server_main.o server.o game_state.o buffer.o err.o
	g++ $(FLAGS) server_main.o server.o game_state.o buffer.o err.o -o screen-worms-server
  
server_main.o: server_main.cpp server.o
	g++ $(FLAGS) -c -o server_main.o server_main.cpp
  
server.o: server.h server.cpp err.o game_state.o buffer.o
	g++ $(FLAGS) -c -o server.o server.cpp
  
game_state.o: game_state.h game_state.cpp
	g++ $(FLAGS) -c -o game_state.o game_state.cpp

buffer.o: buffer.h buffer.cpp
	g++ $(FLAGS) -c -o buffer.o buffer.cpp
  
err.o: err.h err.cpp
	g++ $(FLAGS) -c -o err.o err.cpp
  

clean:
	rm -f screen-worms-server *.o
