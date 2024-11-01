#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

int main(int argc, char **argv) {
  if (argc != 4) {
    std::cerr << "Usage: ./sender [server_address] [port] [username]\n";
    return 1;
  }

  std::string server_hostname;
  int server_port;
  std::string username;

  server_hostname = argv[1];
  server_port = std::stoi(argv[2]);
  username = argv[3];

  // connect to server
  Connection connection;
  connection.connect(server_hostname, server_port);

  if (!connection.is_open()) {
    std::cerr << "Error: cannot connect to server\n";
    return 1;
  }
  
  // send slogin message
  Message senderLogin(TAG_SLOGIN, username);
  if (!connection.send(senderLogin)) {
    std::cerr << "Error: cannot send message\n";
    connection.close();
    return 1;
  }

  Message response;
  if (!connection.receive(response)) {
    std::cerr << "Error: cannot receive response from server for login\n";
    connection.close();
    return 1;
  }
  else {
    if (response.tag.compare("err") == 0) {
      std::cerr << response.data;
      connection.close();
      return 1;
    }
  }
  
  // loop reading commands from user, sending messages to server as appropriate
  std::string line;
  while (true) {
    std::cout << "> ";
    std::getline(std::cin, line);
    if (line.compare(0, 1, "/") == 0) {
      if (line.compare(0, 5, "/join") == 0) {
	std::string room = line.substr(6, line.length() - 6);
	Message sendJoin(TAG_JOIN, room);
	if (!connection.send(sendJoin)) {
	  std::cerr << "Error: cannot send message\n";
	}

	Message response;
	if (!connection.receive(response)) {
	  std::cerr << "Error: cannot receive response from server\n";
	}
	else {
	  if (response.tag.compare("err") == 0) {
	    std::cerr << response.data;
	  }
	  else {
	    if (response.tag.compare("ok") != 0) {
	      std::cerr << "Error: invalid response from server\n";
	    }
	  }
	}
      }
      else if (line.compare("/leave") == 0) {
	Message leaveMsg(TAG_LEAVE);
	if (!connection.send(leaveMsg)) {
	  std::cerr << "Error: cannot send message\n";
	}

	Message response;
	if (!connection.receive(response)) {
          std::cerr << "Error: cannot receive response from server\n";
        }
        else {
          if (response.tag.compare("err") == 0) {
            std::cerr << response.data;
          }
          else {
            if (response.tag.compare("ok") != 0) {
              std::cerr << "Error: invalid response from server\n";
            }
          }
        }
      }
      else if (line.compare("/quit") == 0) {
	Message quitMsg(TAG_QUIT);
	if(!connection.send(quitMsg)) {
	  std::cerr << "Error: cannot send message\n";
	}

	Message response;
	if (!connection.receive(response)) {
	  std::cerr << "Error: cannot receive response from server\n";
	}
	else {
	  if (response.tag.compare("ok") == 0) {
	    return 0;
	  }
	  else if (response.tag.compare("err") == 0) {
	    std::cerr << response.data;
	    return 0;
	  }
	  else {
	    std::cerr << "Error: invalid response from server\n";
	  }
	}
      }
      else {
	std::cerr << "Error: invalid command\n";
      }
    }
    else {
      Message sendAll(TAG_SENDALL, line);
      if(!connection.send(sendAll)) {
	std::cerr << "Error: cannot send message\n";
      }

      Message response;
      if (!connection.receive(response)) {
	std::cerr << "Error: cannot receive response from server\n";
      }
      else {
        if (response.tag.compare("err") == 0) {
          std::cerr << response.data;
        }
        else {
          if (response.tag.compare("ok") != 0) {
            std::cerr << "Error: invalid response from server\n";
          }
        }
      }
    }
  }
}
