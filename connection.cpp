#include <sstream>
#include <cctype>
#include <cassert>
#include "csapp.h"
#include "message.h"
#include "connection.h"

Connection::Connection()
  : m_fd(-1)
  , m_last_result(SUCCESS) {
}

Connection::Connection(int fd)
  : m_fd(fd)
  , m_last_result(SUCCESS) {
  // call rio_readinitb to initialize the rio_t object
  rio_readinitb(&m_fdbuf, m_fd);
}

void Connection::connect(const std::string &hostname, int port) {
  // call open_clientfd to connect to the server
  std::string port_string = std::to_string(port);
  m_fd = open_clientfd(hostname.c_str(), port_string.c_str());

  // call rio_readinitb to initialize the rio_t object
  rio_readinitb(&m_fdbuf, m_fd);

}

Connection::~Connection() {
  // close the socket if it is open
  if (is_open()) {
    close();
    m_fd = -1;
  }
}

bool Connection::is_open() const {
  // return true if the connection is open
  if (m_fd < 0) {
    return false;
  }
  else {
    return true;
  }
}

void Connection::close() {
  // close the connection if it is open
  if (is_open()) {
    Close(m_fd);
    m_fd = -1;
  }
}

bool Connection::send(const Message &msg) {
  // send a message
  // return true if successful, false if not
  // make sure that m_last_result is set appropriately

  char buf[Message::MAX_LEN];
  std::string messageString = msg.formatMessage();
  messageString.copy(buf, msg.getLength());
  
  ssize_t n = rio_writen(m_fd, buf, msg.getLength());
  memset(buf, 0, sizeof(buf));

  if (msg.getLength() > Message::MAX_LEN) {
    m_last_result = INVALID_MSG;
    return false;
  }
  else if (n == msg.getLength()) {
    m_last_result = SUCCESS;
    return true;
  }
  else {
    m_last_result = EOF_OR_ERROR;
    return false;
  }
}

bool Connection::receive(Message &msg) {
  // receive a message, storing its tag and data in msg
  // return true if successful, false if not
  // make sure that m_last_result is set appropriately

  char buf[Message::MAX_LEN + 1];
  ssize_t n = rio_readlineb(&m_fdbuf, buf, sizeof(buf));

  if (n > 0) {
    buf[n] = '\0';
    // store tag and data in msg
    std::string strBuf(buf);
    std::size_t pos = strBuf.find(":");
    msg.tag = strBuf.substr(0,pos);
    msg.data = strBuf.substr(pos + 1, strBuf.length());
    
    m_last_result = SUCCESS;
    return true;
  }
  else {
    m_last_result = EOF_OR_ERROR;
    return false;
  }
}
