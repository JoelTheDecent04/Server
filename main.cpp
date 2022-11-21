#include "websocket.h"
#include <cassert>
#include <iostream>
#include <time.h>
#include <string>
#include <unistd.h>

#include "json.hpp"

using namespace websocket;

const char* time_and_date()
{
	static char buf[128];
	time_t t = time(NULL);
    strcpy(buf, ctime(&t));
	buf[strlen(buf) - 1] = 0;
    return buf;
}

struct MargaritaPlayer 
{
    std::string name;
    float health = -1.0f;
    float max_health = -1.0f;
    float energy = -1.0f;
    float max_energy = -1.0f;
    int wave = -1; 
    bool should_receive_snail = false;   
};

class Server
{
    WSServer<Server> wsserver;

    using WSConnection = WSServer<Server>::Connection;

    std::unordered_map<WSConnection*, MargaritaPlayer> players;
    MargaritaPlayer* winner = nullptr;

public:
    void run() {
	    int port = 5252;

        if (!wsserver.init("0.0.0.0", port)) {
            std::cout << "Init failed: " << wsserver.getLastError();
        }

	    std::cout << time_and_date() << " - Server opened on port " << port << std::endl << std::flush;

        while(1) {
            usleep(100);
            wsserver.poll(this);

            

            for (auto& player : players)
            {
                //Add other player info
                nlohmann::json game_state;
                game_state["players"] = nlohmann::json();

                for (auto& other : players)
                {
                    if (other.first != player.first)
                    {
                        nlohmann::json player_json;
                        player_json["name"] = other.second.name;
                        player_json["energy"] = other.second.energy;
                        player_json["max_energy"] = other.second.max_energy;
                        player_json["health"] = other.second.health;
                        player_json["max_health"] = other.second.max_health;
                        player_json["wave"] = other.second.wave;

                        game_state["players"] += player_json;
                        
                    }
                }

                //Add status
                game_state["status"] = "Running";

                //Add winner
                game_state["winner"] = nlohmann::json();
                if (winner)
                    game_state["winner"] = winner->name;

                //Add snail
                if (player.second.should_receive_snail)
                {
                    game_state["snails_received"] = 1;
                    player.second.should_receive_snail = false;
                }

                std::string game_state_text = game_state.dump();


                player.first->send(OPCODE_TEXT, (uint8_t*)game_state_text.c_str(), game_state_text.size());
            }
        }
    }
    
    void stop() {}

    bool onWSConnect(WSConnection& conn, const char* request_uri, const char* host, const char* origin, const char* protocol,
                   const char* extensions, char* resp_protocol, uint32_t resp_protocol_size, char* resp_extensions,
                   uint32_t resp_extensions_size) {
    std::cout << time_and_date() << " - Connection from " << origin << std::endl << std::flush;
    players[&conn] = MargaritaPlayer();
    return true;
    
    }

    void onWSClose(WSConnection& conn, uint16_t status_code, const char* reason) {
        players.erase(&conn);
	    std::cout << time_and_date() << " - Connection closed" << std::endl << std::flush;
    }

    void onWSMsg(WSConnection& conn, uint8_t opcode, const uint8_t* payload, uint32_t pl_len) {
    	std::cout << time_and_date() << " - Message: ";
        for (size_t i = 0; i < pl_len; i++)
            std::cout << payload[i];
        std::cout << std::endl << std::flush;

        nlohmann::json json = nlohmann::json::parse(payload, payload + pl_len);
        MargaritaPlayer& player = players[&conn];
        player.health =     json["health"];
        player.max_health = json["max_health"];
        player.energy =     json["energy"];
        player.max_energy = json["max_energy"];
        player.wave =       json["wave"];
        player.name =       json["name"];

        if ((int)json["snails_sent"] > 0)
        {
            for (auto& player : players)
            {
                if (player.first != &conn)
                    player.second.should_receive_snail = true;
            }
        }

        if (player.wave >= 10 && !winner)
        {
            winner = &player;
        }
    }

    void onWSSegment(WSConnection& conn, uint8_t opcode, const uint8_t* payload, uint32_t pl_len, uint32_t pl_start_idx,
                   bool fin) {
    std::cout << "error: onWSSegment should not be called" << std::endl << std::flush;

  }
};

int main()
{
    std::cout << "Test\n";

    Server server;
    server.run();
	return 0;
}