#include "guard.h"
#include "message.h"
#include "message_queue.h"
#include "user.h"
#include "room.h"


Room::Room(const std::string &room_name)
  : room_name(room_name) {
  // initialize the mutex
  pthread_mutex_init(&lock, NULL);
}

Room::~Room() {
  // destroy the mutex
  pthread_mutex_destroy(&lock);
}

void Room::add_member(User *user) {
  // add User to the room
  Guard roomlock(lock);
  members.insert(user);
}

void Room::remove_member(User *user) {
  // remove User from the room
  Guard roomlock(lock);
  members.erase(user);
}

void Room::broadcast_message(const std::string &sender_username, const std::string &message_text) {
  // send a message to every (receiver) User in the room

  Guard roomlock(lock);
  
  for (User *i : members) {
    std::string sendMsg = room_name + ":" + i->username + ":" + message_text;
    Message sendAllMessage(TAG_DELIVERY, sendMsg);
    i->mqueue.enqueue(&sendAllMessage);
  }

}

bool Room::user_in_room(User *user) {
  // check if user is in the room
  
  Guard roomlock(lock);

  if (members.find(user) != members.end()) {
    return true;
  }
  else {
    return false;
  }
}
