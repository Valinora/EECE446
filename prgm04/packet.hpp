#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

enum Action {
  JOIN = 0,
  PUBLISH,
  SEARCH,
  FETCH,
};

class Packet {
 protected:
  std::vector<std::uint8_t> buf;

 public:
  ssize_t send_all(int s) {
    ssize_t total = 0;
    ssize_t bytesleft = buf.size();
    ssize_t n;

    while (total < buf.size()) {
      n = send(s, buf.data() + total, bytesleft, 0);
      if (n == -1) {
        // Return the error code
        return n;
      } else if (n == 0) {
        // Connection closed
        break;
      }
      total += n;
      bytesleft -= n;
    }
    return total;
  }

  int recv_all(int s) {
    ssize_t total = 0;
    ssize_t bytesleft = 0;
    ssize_t n;

    while (true) {
      buf.resize(total + 1024);
      bytesleft = 1024;
      n = recv(s, buf.data() + total, bytesleft, 0);
      if (n < 0) {
        return n;
      } else if (n == 0) {
        // Peer has closed the connection
        break;
      }
      total += n;
    }

    buf.resize(total);
    return 0;
  }

  virtual void to_netbuf() = 0;
};

class Join : public Packet {
 public:
  uint32_t peer_id;
  static const Action tag = JOIN;

  Join(uint32_t peer_id) : peer_id(peer_id) {}

  void to_netbuf() override {
    buf.resize(sizeof(Action) + sizeof(peer_id));

    uint8_t* offset = buf.data();
    memcpy(offset, &tag, sizeof(Action));

    offset += sizeof(Action);
    uint32_t peer_id_n = htonl(peer_id);
    memcpy(offset, &peer_id_n, sizeof(peer_id_n));
  }
};

class Publish : public Packet {
 public:
  uint32_t count;
  std::vector<std::string> filenames;
  static const Action tag = PUBLISH;

  Publish(uint32_t count, const std::vector<std::string>& filenames) : count(count), filenames(filenames) {}

  void to_netbuf() override {
    size_t size = sizeof(Action) + sizeof(count);
    for (const auto& filename : filenames) {
      size += filename.size();
    }
    buf.resize(size);

    uint8_t* offset = buf.data();
    memcpy(offset, &tag, sizeof(Action));

    offset += sizeof(Action);
    uint32_t count_n = htonl(count);
    memcpy(offset, &count_n, sizeof(count_n));

    offset += sizeof(count_n);
    for (const auto& filename : filenames) {
      memcpy(offset, filename.data(), filename.size());
      offset += filename.size();
    }
  }
};

class Search : public Packet {
 public:
  std::string search_term;
  Action tag = SEARCH;

  Search(const std::string& search_term) : search_term(search_term) {}

  void to_netbuf() override {
    size_t size = sizeof(Action) + search_term.size();
    buf.resize(size);

    uint8_t* offset = buf.data();
    memcpy(offset, &tag, sizeof(Action));

    offset += sizeof(Action);
    memcpy(offset, search_term.data(), search_term.size());
  }
};

class Fetch : public Packet {
 public:
  std::string filename;
  Action tag = FETCH;

  Fetch(const std::string& filename) : filename(filename) {}

  void to_netbuf() override {
    size_t size = sizeof(Action) + filename.size();
    buf.resize(size);

    uint8_t* offset = buf.data();
    memcpy(offset, &tag, sizeof(Action));

    offset += sizeof(Action);
    memcpy(offset, filename.data(), filename.size());
  }
};