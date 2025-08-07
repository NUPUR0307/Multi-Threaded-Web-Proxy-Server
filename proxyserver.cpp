#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <chrono> 
#include <ctime>

//CLEAN UP and Output per packet / response

const int BUFFER_SIZE = 4096;
const int BACKLOG = 10;
const int PORT = 37000;
const bool DEBUG = false;

int thread_id = 0;

using namespace std;

void termination_handler(int signum) {
   cout << "\nTERMINATION TRIGGERED"<<endl;
   cout<<"Closing Proxy Server Socket"<<endl;
   close(52837);
   cout << "Ending Proxy Server" << endl;
   exit(signum);
}

void error(const char *msg, const char *func) {
    perror(msg);
    std::cerr << "Error in function: " << func << std::endl;
}

struct ClientInfo {
    // Creating a struct for new thread for each client 
    // and pass the pointer to the thread
    int client_socket;
    std::string client_ip;
};

void* handle_client(void* arg) {
    ClientInfo* client_info = static_cast<ClientInfo*>(arg);

    int client_socket = client_info->client_socket;

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    thread_id++;
    bool SERVER_AVL = true;

    if(DEBUG)std::cout << "HANDLE CLIENT" ;
    cout<< std::endl;

    // Receive the client's request
    ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received < 0) {
        error("Error reading from socket", "recv");
    }

    // Create a new socket to connect to the destination server
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        error("Error opening server socket", "socket");
    }

    // Set up sockaddr_in structure for the destination server
    sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Connect to the destination server
    
    if (connect(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        std::string not_found_response = "HTTP/1.1 502 Bad Gateway\r\n\r\n";
        send(client_socket, not_found_response.c_str(), not_found_response.length(), 0);
        
        cout<<"NOT RESPONDING: Destination server ";
        SERVER_AVL = false;
    }

    // Forward the client's request to the destination server
    
    ssize_t bytes_sent = 0;

    if(SERVER_AVL){
    bytes_sent = send(server_socket, buffer, bytes_received, 0);
    if (bytes_sent < 0) {
        std::string not_found_response = "HTTP/1.1 502 Bad Gateway\r\n\r\n";
        send(client_socket, not_found_response.c_str(), not_found_response.length(), 0);
        cout<<"NOT RESPONDING: Destination server ";
        SERVER_AVL = false;
    }
    
    auto ts_server = chrono::duration_cast<chrono::nanoseconds>(chrono::steady_clock::now().time_since_epoch()).count();

    if(bytes_sent) std::cout<<"proxy-forward, "<<"server, "<<thread_id<<", "<<ts_server<<endl;


    // Receive the response from the destination server
    memset(buffer, 0, BUFFER_SIZE);
    bytes_received = recv(server_socket, buffer, BUFFER_SIZE - 1, 0);

    
    bytes_sent = 0;
    bool first = true;
    while(bytes_received){
        if(DEBUG)std::cout << "RECEIVED FROM WEB SERVER: " << bytes_received << std::endl;
    
        // if (bytes_received < 0) {
        //     //error("Error reading from server", "recv");
        // }

        // Forward the response to the client
        bytes_sent = send(client_socket, buffer, bytes_received, 0);
        // if (bytes_sent < 0) {
        //     //error("Error sending to client", "send");
        // }

        if(first && bytes_sent > 0){
            auto ts_cli = chrono::duration_cast<chrono::nanoseconds>(chrono::steady_clock::now().time_since_epoch()).count();
            if(bytes_sent) std::cout<<"proxy-forward, "<<"client, "<<thread_id<<", "<<ts_cli<<endl;;
            first = false;
        }

        bytes_received = 0;
        bytes_received = recv(server_socket, buffer, BUFFER_SIZE - 1, 0);        

    }
    }
    // Close sockets
    close(client_socket);
    close(server_socket);

    pthread_exit(NULL);
}

int main() {
    int port = 52837;

    signal(SIGINT, termination_handler);

    struct sockaddr_in proxy_addr;
    memset(&proxy_addr, 0, sizeof(proxy_addr));
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_addr.s_addr = INADDR_ANY; // Listen on any available network interface
    proxy_addr.sin_port = htons(port);

    // Create a socket for the proxy server
    int proxy_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (proxy_socket < 0) {
        error("Error opening socket", "socket");
    }

    // Bind the socket to the specified port
    if (bind(proxy_socket, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) < 0) {
        error("Error binding socket", "bind");
    }

    // Listen for incoming connections
    if (listen(proxy_socket, BACKLOG) < 0) {
        error("Error listening for connections", "listen");
    }

    std::cout << "Proxy server listening on port " << port << "..." << std::endl;

    bool run = true;
    while (run == true) {
        // Accept a client connection
        sockaddr_in client_address;
        socklen_t client_size = sizeof(client_address);

        int client_socket = accept(proxy_socket, (struct sockaddr *)&client_address, &client_size);

        // if (client_socket < 0) {
        //     error("Error accepting connection", "accept");
        // }

        // // Handle the client's request in a separate thread or process
        // handle_client(client_socket);

        std::string client_ip = inet_ntoa(client_address.sin_addr);

        // Create a new ClientInfo object for each client
        ClientInfo* client_info = new ClientInfo;
        client_info->client_socket = client_socket;
        client_info->client_ip = client_ip;

        // Create a new thread for each client and pass the ClientInfo pointer
        // Convert the ClientInfo pointer type to void pointer since we are passing a thread
        // The thread will invoke handle_client
        pthread_t client_thread;

        if (pthread_create(&client_thread, NULL, handle_client, static_cast<void*>(client_info)) != 0) {
            
            std::cerr << "Error creating thread" << std::endl;
            
            delete client_info;   //free the memory
            close(client_socket); //close the connection
        }
       
    }
    
    // Close the proxy socket 
    close(proxy_socket);

    return 0;
}
