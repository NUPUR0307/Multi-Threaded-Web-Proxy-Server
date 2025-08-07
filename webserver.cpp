#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h> 
#include <filesystem>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <chrono> 
#include <ctime>

using namespace std;

const int PORT = 37000; //CS-528 Group 37
const bool DEBUG = false;
const bool TRANSMISSION_DEBUG = false;
const bool PROXY_AVL = true;

int thread_id =0;

int SERVER_SOCKET = 0;

void termination_handler(int signum) {
    cout << "\nTERMINATION TRIGGERED"<<endl;
   cout<<"Closing WEB Server Socket"<<endl;
   close(37000);
   cout << "Ending WEB Server" <<endl;
   exit(signum);
}

struct ClientInfo {
    // Creating a struct for new thread for each client 
    // and pass the pointer to the thread
    int client_socket;
    std::string client_ip;
};

bool file_exists(const std::string& filename) {
    //This function is to validate if the requested file exists or not
    //input : string filename
    //output: boolean true if available, false if not available.

    std::ifstream file(filename);
    
    if (file) {file.close();return true;}
    
    file.close();
    return false;
}

string requested_file_in(string request){
    /* Extracts "GET" line from the request
    *  Extracts the requested object/file from "GET" line
    *  input: client request
    *  output: string file_name / file_path in GET
    */

    string request_temp = request;
    string file_path = "";
    string get = "";

    
    //size_t is used instead of int
    //since the sizes and indices of request are stored we use size_t 
    size_t get_pos = request.find("GET");
    size_t get_EOL = request.find("\n");
    get +=  request_temp.substr(get_pos,get_EOL);

    size_t start = get.find(" ")+1;
    size_t end = get.rfind(" ");
    file_path = get.substr(start, end - start);

    if(file_path[0] == '/'){file_path.erase(0,1);}

    // std::cout<<"GET from Request   : "<<get<<endl;
    // std::cout<<"file_path from GET : "<<file_path<<endl;

    //std::cout<<"Client Request   : "<<get<<endl;
        

    if(DEBUG == true){
        std::cout<<"\033[1;34m";
        std::cout<<"DEBUG LOGS IN requested_file_in(request)"<<endl;
        std::cout<<"DEBUG LOGS >>  Extracting GET from Request   : "<<get<<endl;
        std::cout<<"DEBUG LOGS >>  Extracting file_path from GET : "<<file_path<<endl;
        std::cout<<endl;
        std::cout<<"\033[1;36m";
    }

    return file_path;
}

void send_response(int client_socket, string content_type, string file_path) {
    /* Create the response and transmit it along with the data to the client
    *  Validate again if the file is available for transmission.
    *  Create the "response" header and transmit the packets.
    *  response is 200 OK if file is available and ready for transmission
    *  response is 404 Not Found if not available.
    */

    std::ifstream file(file_path, std::ios::binary);

    if (!file) {
        // If the File is not found, create and send a 404 response
        std::string not_found_response = "HTTP/1.1 404 Not Found\r\n\r\n";
        send(client_socket, not_found_response.c_str(), not_found_response.length(), 0);

        if(DEBUG == true){
            std::cout<<"\033[1;31m";
            std::cout<<"DEBUG LOGS IN send_response(client_socket, content_type, file_path)"<<endl;
            std::cout<<"DEBUG LOGS >> Created Response  : "<<not_found_response<<endl;
            std::cout<<endl;
            std::cout<<"\033[1;36m";
        }

        return; // Exit the function
    }

    //File is available for transmission
    //Create a 200 OK response header

    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: " + content_type + "\r\n\r\n";
    send(client_socket, response.c_str(), response.length(), 0);

    //console logs
   //std::cout<<"- - -Server Response - - -"<<endl<<response<<"- - -End of Response - - -"<<endl;

    if(DEBUG == true){
        std::cout<<"\033[1;34m";
        std::cout<<"DEBUG LOGS IN send_response(client_socket, content_type, file_path)"<<endl;
        std::cout<<"DEBUG LOGS >> Created Response  : "<<response<<endl;
        std::cout<<endl;
        std::cout<<"\033[1;36m";
    }

    // Read and send the file content in chunks
    char file_buffer[1024];
    int total_number_of_packets = 1;
    //std::cout<<"Transmitting file : "<<file_path<<endl<<endl;

    //Continues tranmission till the end of file
    while (!file.eof()) {
        file.read(file_buffer, sizeof(file_buffer));
        int bytes_read = file.gcount(); //will always be less than 128 bytes
        
        if (bytes_read > 0) {
            int bytes_sent = send(client_socket, file_buffer, bytes_read, 0);

            if(DEBUG == true && TRANSMISSION_DEBUG == true){
                std::cout<<"DEBUG LOGS IN send_response(client_socket, content_type, file_path)"<<endl;
                std::cout<<"DEBUG LOGS >> Sending Packet Number : "<<total_number_of_packets++<<endl;
                std::cout<<endl;
            }

            if (bytes_sent < 0) {
                // Handle send error
                std::cerr << "\033[1;31m Error transmitting packets \033[1;36m " << std::endl;
                break; // Exit the loop on error
            }
        }
    }
    file.close();
}

int create_response(int client_socket, string request, string file_path){
    /* Create the response and transmit it along with the data to the client
    *  Provide only HTML and PDF files 
    *  Validate if the requested file is available
    *  Create the "response" header and transmit the packets.
    *  if file avaialblle invoke send_response to create and
    *                     send response 200 OK along with file 
    *  if not available send response 404 Not Found to client.
    */
    
     
    
    string response;

    if (request.find("GET") != std::string::npos) {
        if(!file_exists(file_path)){
            //std::cout<<"404 NOT FOUND"<<endl;
            response = "HTTP/1.0 404 Not Found\r\n\r\n404 Not Found";

            auto timestamp = chrono::duration_cast<chrono::nanoseconds>(chrono::steady_clock::now().time_since_epoch()).count();
            std::cout<<"\033[1;31m";
            std::cout << "server-response, "<<404<<", "<<thread_id<<", "<<timestamp << std::endl;
            std::cout<<"\033[1;36m";
    

            if(DEBUG == true){
                std::cout<<"\033[1;31m";
                std::cout<<"DEBUG LOGS IN create_response(int client_socket, string request, string file_path)"<<endl;
                std::cout<<"DEBUG LOGS >> Created Response  : "<<response<<endl;
                std::cout<<endl;
                std::cout<<"\033[1;36m";
            }

            send(client_socket, response.c_str(), response.size(), 0);

            return 404;

        }
        else if(file_path.find("html") != string::npos){
            auto timestamp = chrono::duration_cast<chrono::nanoseconds>(chrono::steady_clock::now().time_since_epoch()).count();
            std::cout << "server-response, "<<200<<", "<<thread_id<<", "<<timestamp << std::endl;    
            send_response(client_socket, "text/html", file_path);
        }
        else if(file_path.find("pdf") != string::npos){
            auto timestamp = chrono::duration_cast<chrono::nanoseconds>(chrono::steady_clock::now().time_since_epoch()).count();
            std::cout << "server-response, "<<200<<", "<<thread_id<<", "<<timestamp << std::endl;    
            send_response(client_socket, "application/pdf", file_path);
        }

        return 200;
    } 
    else {
        // Respond with a 400 Bad Request for unsupported methods
        response = "HTTP/1.0 400 Bad Request\r\n\r\n";

        return 400;
    }

    return 111;
}

int handle_request(int client_socket,string client_ip){
     /* 
    *  Handle accepted client requests
    *  Read/Parse client request 
    *  invoke requested_file_in and create_response
    *  close socket after transmission is completed
    */
    
    char buffer[1024];
    string response;
    int status_code = 404;

    // Read the HTTP request from the client
    int bytesRead = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytesRead > 0) {
        // Parse the HTTP request
        string request(buffer, bytesRead);
        string temp_request = request;

        string requested_file;
        string file_path;

        file_path = requested_file_in(request);     
        
        // Simple check for a GET request
        status_code = create_response(client_socket,request,file_path);
        
    }
    
    if(DEBUG == true){
        std::cout<<"\033[1;34m";
        std::cout<<"DEBUG LOGS IN handle_request(int client_socket)"<<endl;
        std::cout<<"DEBUG LOGS >> Closing connection with client: " <<client_ip<<endl;
        std::cout<<endl;
        std::cout<<"\033[1;36m";
    }

    // Close the client socket
   
    //std::cout<<"Closing connection with client  : " <<client_ip<<endl<<endl;
    close(client_socket);

    return status_code;
}

void* handle_client(void* arg) {
     /* 
    *  Handle accepted client requests
    *  create a new thread/socket and handle requests
    *  invoke handle_request
    */
    ClientInfo* client_info = static_cast<ClientInfo*>(arg);

    int client_socket = client_info->client_socket;
    std::string client_ip = client_info->client_ip;

    thread_id++;

    if(DEBUG){
        std::cout<<"\033[1;34m";
        std::cout<<"DEBUG LOGS IN handle_client(void* arg)"<<endl;
        std::cout<<"DEBUG LOGS >> Creating new socket for :" <<client_ip<<endl;
        std::cout<<"DEBUG LOGS >> Handling Client Request" <<client_ip<<endl;
        std::cout<<endl;   
        std::cout<<"\033[1;36m";
    }
    
    int status_code = handle_request(client_socket, client_ip);

    close(client_socket);
    //HERE
    
    delete client_info; //Free the memory
    pthread_exit(NULL);
}

int main(){
    // CREATE a server socket 
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    SERVER_SOCKET = server_socket;

    sockaddr_in server_address;
    socklen_t server_size = sizeof(server_address);
    
    signal(SIGINT, termination_handler);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;
    
    // CONVERT network integer to string
    getpeername(server_socket, (struct sockaddr*)&server_address, &server_size);
    std::string server_ip = inet_ntoa(server_address.sin_addr);

    // BIND the socket to IP address 192.168.86.34 and port 52837
    bind(server_socket,(struct sockaddr*)&server_address, server_size);

    if(DEBUG == true){
        std::cout<<"\033[1;34m";
        std::cout<<"DEBUG LOGS FOR: main()"<<endl;
        std::cout<<"DEBUG LOGS >> Server IP :"<<server_ip<<endl;
        std::cout<<"DEBUG LOGS >> Server Port :"<<ntohs(server_address.sin_port)<<endl;
        std::cout<<"DEBUG LOGS >> The Server Socket has been bound"<<endl<<endl;
        std::cout<<"\033[1;36m";
    }

    listen(server_socket, 10);
    std::cout <<"\033[1;36m"<<"Server listening on port " << PORT << "...\n"<<endl;

    while (true) {
        // Accept incoming connection
        sockaddr_in client_address;
        socklen_t client_size = sizeof(client_address);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_size);

        // CONVERT network integer to string
        getpeername(client_socket, (struct sockaddr*)&client_address, &client_size);
        std::string client_ip = inet_ntoa(client_address.sin_addr);

        if(DEBUG == true){
        std::cout<<"\033[1;34m";
        std::cout<<"DEBUG LOGS >> INCOMING CONNECTION"<<endl;
        std::cout<<"DEBUG LOGS >> Client IP:"<<client_ip<<endl;
        std::cout<<"DEBUG LOGS >> Client Port :"<<htons(client_address.sin_port)<<endl<<endl;
        std::cout<<"\033[1;36m";
        }

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

    close(server_socket);

    //server-response,STATUS-CODE,THREAD-ID,TIMESTAMP

    return 0;   
}