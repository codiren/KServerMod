#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <vector>
#include <string>
#include <sstream>
#include <mutex>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024

std::vector<SOCKET> clients;
std::mutex clients_mutex;
std::mutex message_mutex;

static std::string factions[3] = {//string ID in FCS of each Faction, 0th Nameless(default), 1st Player 1, 2nd Player 2, ...
    "204-gamedata.base","10-multiplayr.mod","12-multiplayr.mod"
};// must not be longer than 17 chars

void send_to_all_clients(const std::string& message, bool update_last = true) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    if (update_last) {
        std::lock_guard<std::mutex> msg_lock(message_mutex);
        //last_message = message;
    }

    for (auto it = clients.begin(); it != clients.end();) {
        if (send(*it, message.c_str(), message.size(), 0) == SOCKET_ERROR) {
            closesocket(*it);
            it = clients.erase(it);
        } else {
            ++it;
        }
    }
}
float speed = 1;
void send_floats() {
	return;
    while (true) {
        float num;
        std::cout << "Enter a float: ";
        std::cin >> num;
        
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cerr << "Invalid input. Please enter a valid float." << std::endl;
            continue;
        }
        
        std::string num_str;
		
		/*num_str = "";
		num_str += "1\n";
		num_str += std::to_string(paused);
		num_str += "\n";
		num_str += "2\n";
		num_str += std::to_string(speed);*/
		
		/*num_str = "";
		num_str += "0\n";
		num_str += "B";*/
		
        send_to_all_clients("0\n0\n1\n"+std::to_string(num));
    }
}
struct Vector3 {
    float x;
    float y;
    float z;
    void fromString(const std::string& str) {
        std::stringstream ss(str);
        char comma;
        ss >> x >> comma >> y >> comma >> z;
    }
    std::string toString() const {
        std::ostringstream oss;
        oss << x << "," << y << "," << z;
        return oss.str();
    }
};
Vector3 plr1;
Vector3 plr2;
void handle_client(SOCKET client_socket) {
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.push_back(client_socket);
    }
    std::cout << "Client connected." << std::endl;


std::string num_str;
		
		num_str = "";
		num_str += "1\n";
		num_str += std::to_string(speed);
		num_str += "\n";
		static int client_id_counter = 1;
        int client_id = client_id_counter++;
		if((sizeof(factions) / sizeof(factions[0]))<=client_id_counter)client_id_counter = 1;//reset
        if (send(client_socket, num_str.c_str(), num_str.size(), 0) == SOCKET_ERROR) {
            
        }




    char buffer[BUFFER_SIZE];
    while (true) {
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        //if (bytes_received <= 0) break;		
        
        buffer[bytes_received] = '\0';
        //std::cout << "Client message: " << buffer << std::endl;

        // Respond with last message
        std::string response = buffer;
		
		
		std::string line;
std::string key;
std::istringstream iss(response);


while (std::getline(iss, line)) {
    if (key == "") { key = line;continue; }
    
	if(key == "2"&&client_id == 1&&line!="0,0,0")plr1.fromString(line);
	if(key == "3"&&client_id == 2&&line!="0,0,0")plr2.fromString(line);
	if(key == "1")speed = std::stof(line);
	if(key != "0")std::cout<<"client: "<<key<<" "<<line<<"\n";
	
    key = "";
}
		
		
		
        /*{
            std::lock_guard<std::mutex> lock(message_mutex);
            response = last_message.empty() ? "No messages yet" : last_message;
        }*/
		
		std::string num_str;
		
		num_str += "0\n";
		num_str += factions[client_id];
		num_str += "\n";
		num_str += "1\n";
		num_str += std::to_string(speed);
		num_str += "\n";
		//num_str += "2\n";
		//if(client_id == 1)num_str+=plr1.toString();
		//if(client_id == 2)num_str+=plr2.toString();
		//num_str += ("Player 1="+plr1.toString()+";Player 2="+plr2.toString());
		//num_str += "\n";
		num_str += "2\n";
		num_str += plr1.toString();
		num_str += "\n";
		num_str += "3\n";
		num_str += plr2.toString();
		num_str += "\n";
		
		
		
        
        if (send(client_socket, num_str.c_str(), num_str.size(), 0) == SOCKET_ERROR) {
            break;
        }
    }

    // Cleanup disconnected client
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        auto it = std::find(clients.begin(), clients.end(), client_socket);
        if (it != clients.end()) {
            closesocket(*it);
            clients.erase(it);
        }
    }
    std::cout << "Client disconnected." << std::endl;
}
int main() {
	plr1.fromString("-5139.11,158.019,345.631");
plr2.fromString("-5139.11,158.019,345.631");
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!\n";
        return 1;
    }

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "Socket creation failed!\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed!\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    if (listen(server_fd, 5) == SOCKET_ERROR) {
        std::cerr << "Listen failed!\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;
    std::thread sender_thread(send_floats);

    while (true) {
        sockaddr_in client_addr{};
        int client_len = sizeof(client_addr);
        SOCKET client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed!\n";
            continue;
        }
        
        std::thread(handle_client, client_socket).detach();
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}