#include <netinet/in.h>
#include <stdint.h>

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

 
/**
 * We assume peer id's are unqiue. The protocol doesn't
 * support rejecting a peer for not choosing a unique id,
 * but I think it's a safe assumption.
 */
class Peer {
public:
    uint32_t id;
    int socket_fd;
    std::vector<std::string> files;
    struct sockaddr_in address;

    bool operator==(const Peer& other) {
        return id == other.id;
    }
};

class PeerList {
    std::unordered_map<std::string, Peer> filemap = {};
    // I'd love to use peer->id as the index into this, but std::vector isn't sparse,
    // and peer->id can be arbitrarily large.
    // Could use another hash map, but ¯\_(ツ)_/¯
    std::vector<Peer> peers = {};

    std::optional<Peer> find(std::string filename) {
        if (filemap.count(filename) < 1) {
            return {};
        }

        return filemap[filename];
    }

    void add(Peer peer) {
        for (const auto& file : peer.files) {
            filemap[file] = peer;
        }

        peers.push_back(peer);
    }

    // I generally dislike the std's API design of not returning elements that it removes.
    // I may never use this return value, but I'll be glad to have it if I *do* end up needing it.
    std::optional<Peer> remove(Peer peer) {
        for (const auto& file : peer.files) {
            filemap.erase(file);
        }

        std::optional<Peer> out = {};
        for (auto it = peers.begin(); it != peers.end();) {
            if (*it == peer) {
                out = *it;
                peers.erase(it);
            } else {
                ++it;
            }
        }

        return out;
    }
};