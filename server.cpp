#include <pthread.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <set>
#include <vector>
#include <cctype>
#include <cassert>
#include "message.h"
#include "connection.h"
#include "user.h"
#include "room.h"
#include "guard.h"
#include "server.h"

////////////////////////////////////////////////////////////////////////
// Server implementation data types
////////////////////////////////////////////////////////////////////////

// add any additional data types that might be helpful
// for implementing the Server member functions

struct ConnInfo {
  int clientfd;
  const char* webroot;
  Connection* conn;
  Server* serverInstance;
  std::string room_name;
};
  
////////////////////////////////////////////////////////////////////////
// Client thread functions
////////////////////////////////////////////////////////////////////////

namespace {  
  void chat_with_sender(ConnInfo& info, std::string username) {
    User *sender = new User(username);
    Connection* conn = info.conn;
    
    while(true) {
    Message msg;
    if (!conn->receive(msg)) {
      std::cerr << "Error: cannot read message from sender\n";
      pthread_exit(NULL);
      return;
    }
    else {
      // initial join to room
      if (msg.tag.compare(TAG_JOIN) == 0) {
	const std::string room_name = msg.data;
	Room *room = info.serverInstance->find_or_create_room(room_name);
	room->add_member(sender);
	if (room->user_in_room(sender)) {
	  info.room_name = room_name;
	  std::string ok_msg = "joined room";
	  Message ok_response(TAG_OK, ok_msg);
	  if(!conn->send(ok_response)) {
	    std::cerr << "Error: cannot send message\n";
	    pthread_exit(NULL);
	    return;
	  }
	}
	else {
	  std::string err_msg = "cannot join room";
	  Message err_response(TAG_ERR, err_msg);
	  if(!conn->send(err_response)) {
	    std::cerr << "Error: cannot send message\n";
	    pthread_exit(NULL);
	    return;
	  }
	}
      }
      // leave a room
      else if (msg.tag.compare(TAG_LEAVE) == 0) {
	const std::string room_name = info.room_name;
	Room *room = info.serverInstance->find_or_create_room(room_name);
	if (!(room->user_in_room(sender))) {
	  std::string err_msg = "user not in room";
          Message err_response(TAG_ERR, err_msg);
          if(!conn->send(err_response)) {
	    std::cerr << "Error: cannot send message\n";
	    pthread_exit(NULL);
            return;
	  }
	}
	else {
	  room->remove_member(sender);
	  info.room_name = "";
	  std::string ok_msg = "leaving room";
          Message ok_response(TAG_OK, ok_msg);
          if(!conn->send(ok_response)) {
	    std::cerr << "Error: cannot send message\n";
            pthread_exit(NULL);
            return;
	  }
	}
      }
      // send messages to all receivers in the room
      else if (msg.tag.compare(TAG_SENDALL) == 0) {
	Room *room = info.serverInstance->find_or_create_room(info.room_name);
	if (!(room->user_in_room(sender))) {
	  std::string err_msg = "user not in a room";
          Message err_response(TAG_ERR, err_msg);
          if(!conn->send(err_response)) {
	    std::cerr << "Error: cannot send message\n";
            pthread_exit(NULL);
            return;
	  }
	}
	else {
	  room->broadcast_message(username, msg.data);
	  std::string ok_msg = "message sent";
	  Message ok_response(TAG_OK, ok_msg);
	  if(!conn->send(ok_response)) {
	    std::cerr << "Error: cannot send message\n";
            pthread_exit(NULL);
            return;
	  }
	}
      }
      // quit sender
      else if (msg.tag.compare(TAG_QUIT) == 0) {
	std::string ok_msg = "bye!";
	Message ok_response(TAG_OK, ok_msg);
	if(!conn->send(ok_response)) {
	  std::cerr << "Error: cannot send message\n";
          pthread_exit(NULL);
          return;
	}
	pthread_exit(NULL);
	return;
      }
      else {
	std::cerr << "Error: invalid command from sender\n";
      }
    }
    }
    pthread_exit(NULL);
    delete sender;
  }
  
  void chat_with_receiver(ConnInfo& info, std::string username) {
    User *receiver = new User(username);
    Connection* conn = info.conn;

    Message msg;
    if (!conn->receive(msg)) {
      std::cerr << "Error: cannot read message from sender\n";
      pthread_exit(NULL);
      return;
    }
    else {
      // join a room
      if (msg.tag.compare(TAG_JOIN) == 0) {
        const std::string room_name = msg.data;
        Room *room = info.serverInstance->find_or_create_room(room_name);
	room->add_member(receiver);
	if (room->user_in_room(receiver)) {
	  info.room_name = room_name;
	  std::string ok_msg = "welcome";
          Message ok_response(TAG_OK, ok_msg);
          if(!conn->send(ok_response)) {
	    std::cerr << "Error: cannot send a message";
	    pthread_exit(NULL);
	    return;
	  }
        }
        else {
          std::string err_msg = "cannot join room";
          Message err_response(TAG_ERR, err_msg);
          if(!conn->send(err_response)) {
	    std::cerr << "Error: cannot send message\n";
            pthread_exit(NULL);
            return;
	  }
        }
      }
      // quit the receiver before joining room
      else if (msg.tag.compare(TAG_QUIT) == 0) {
	pthread_exit(NULL);
	return;
      }
    }
    // loop to process are delivered messages from sender
    while (true) {
      Message *delivery_msg = receiver->mqueue.dequeue();
      if (delivery_msg) {
	if(!conn->send(*delivery_msg)) {
	  std::cerr << "Error: cannot send message\n";
          pthread_exit(NULL);
	  delete delivery_msg;
	  return;
	}
        delete delivery_msg;
      }                                     
    }
    pthread_exit(NULL);
    delete receiver;
  }

  void *worker(void *arg) {
  pthread_detach(pthread_self());

  // use a static cast to convert arg from a void* to
  // whatever pointer type describes the object(s) needed
  // to communicate with a client (sender or receiver)

  ConnInfo* info = static_cast<ConnInfo*>(arg);
  
  // read login message (should be tagged either with
  // TAG_SLOGIN or TAG_RLOGIN), send response

  // add testing if first command is quit  
  info->conn = new Connection(info->clientfd);
  
  Message login;
  if (!info->conn->receive(login)) {
    std::cerr << "Error: cannot read login message\n";
    pthread_exit(NULL);
  }
  else {
    if (login.tag.compare(TAG_SLOGIN) || login.tag.compare(TAG_RLOGIN)) {
      std::string okLoginData = "logged in";
      std::string errorLoginData = "cannot login";
      Message okResponse(TAG_OK, okLoginData);
      Message errResponse(TAG_ERR, errorLoginData);
      if (!info->conn->send(okResponse)) {
	info->conn->send(errResponse);
	pthread_exit(NULL);
      }
    }
    else {
      std::cerr << "Error: did not receive login message\n";
    }
  }
  
  
  // depending on whether the client logged in as a sender or
  // receiver, communicate with the client (implementing
  // separate helper functions for each of these possibilities
  // is a good idea)

  if (login.tag.compare(TAG_SLOGIN) == 0) {
    chat_with_sender(*info, login.data);
  }
  else if (login.tag.compare(TAG_RLOGIN) == 0) {
    chat_with_receiver(*info, login.data);
  }
  else {
    std::cerr << "Error: can only chat with sender or receiver\n";
  }

  close(info->clientfd);
  free(info);
  
  return 0;
  }

}

////////////////////////////////////////////////////////////////////////
// Server member function implementation
////////////////////////////////////////////////////////////////////////

Server::Server(int port)
  : m_port(port)
  , m_ssock(-1) {
  // initialize mutex
  pthread_mutex_init(&m_lock, NULL);
  
}

Server::~Server() {
  // destroy mutex
  pthread_mutex_destroy(&m_lock);
}

bool Server::listen() {
  // use open_listenfd to create the server socket, return true if successful, false if not

  std::string m_port_string = std::to_string(m_port);
  
  m_ssock = open_listenfd(m_port_string.c_str());
  
  if (m_ssock < 0) {
    return false;
  }
  else {
    return true;
  }
}

void Server::handle_client_requests() {
  // infinite loop calling accept or Accept, starting a new pthread for each connected client

  while (true) {
    int clientfd = accept(m_ssock, NULL, NULL);
    if (clientfd < 0) {
      std::cerr << "Error: server cannot accept client connection\n";
    }

    ConnInfo *info = new ConnInfo;
    info->clientfd = clientfd;
    info->serverInstance = this;
    info->room_name = "";

    {
      Guard lock(m_lock);
      
      pthread_t thr_id;
      if (pthread_create(&thr_id, NULL, worker, info) != 0) {
	std::cerr << "Error with creating thread\n";
      }
    }
     
  }
  
}

Room *Server::find_or_create_room(const std::string &room_name) {
  // return a pointer to the unique Room object representing the named chat room, creating a new one if necessary

  std::map<std::string, Room *>::iterator room;

  {
    Guard lock(m_lock);
    room = m_rooms.find(room_name);

    if(room == m_rooms.end()) {
      Room *create_room = new Room(room_name);
      m_rooms[room_name] = create_room;
      return create_room;
    }
    else {
      return room->second;
    }
  }
  
}
