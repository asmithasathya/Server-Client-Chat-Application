#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

int main(int argc, char **argv) {
  if (argc != 5) {
    std::cerr << "Usage: ./receiver [server_address] [port] [username] [room]\n";
    return 1;
  }

  std::string server_hostname = argv[1];
  int server_port = std::stoi(argv[2]);
  std::string username = argv[3];
  std::string room_name = argv[4];

  Connection conn;

  // connect to server
   conn.connect(server_hostname, server_port);

  // throw error if cannot connect to server
  if (!conn.is_open()) {
    std::cerr << "Error: cannot connect to server\n";
    return 1;
  }
  // send rlogin and join messages (expect a response from
  // the server for each one)

  Message rLogin(TAG_RLOGIN, username);
  if (!conn.send(rLogin)) {
    std::cerr << "Error: cannot send message\n";
    conn.close();
    return 1;
  }

  Message response;
  if (!conn.receive(response)) {
    std::cerr << "Error: cannot receive response from server for login\n";
    conn.close();
    return 1;
  }
  else {
    if (response.tag.compare("err") == 0) {
      std::cerr << response.data;
      conn.close();
      return 1;
    }
  }

  Message sendJoin(TAG_JOIN, room_name);
  if (!conn.send(sendJoin)) {
    std::cerr << "Error: cannot send message\n";
    conn.close();
    return 1;
  }
  if (!conn.receive(response)) {
    std::cerr << "Error: cannot receive response from server\n";
    conn.close();
    return 1;
  }
  else {
    if (response.tag.compare("err") == 0) {
      std::cerr << response.data;
      conn.close();
      return 1;
    }
    else {
      if (response.tag.compare("ok") != 0) {
	std::cerr << "Error: invalid response from server\n";
      }
    }
  }  
   
  // loop waiting for messages from server
  //       (which should be tagged with TAG_DELIVERY)

  while(true) {
    Message receivedMessage;
    if (!conn.receive(receivedMessage)) {
      std::cerr << "Error: cannot receive response from server\n";
    }
    
    if (receivedMessage.tag == TAG_DELIVERY) {
      std::string data_string = receivedMessage.data;
      std::size_t first_colon = data_string.find(":");
      std::string data_string_cut = data_string.substr(first_colon + 1, data_string.length());
      std::size_t second_colon = data_string_cut.find(":");
      std::string sender = data_string_cut.substr(0, second_colon);
      std::string final_message = data_string_cut.substr(second_colon + 1, data_string_cut.length());
      std::cout << sender << ": " << final_message;
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
  return 0;
}
