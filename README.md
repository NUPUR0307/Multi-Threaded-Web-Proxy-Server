## Multi Threaded Web server with Proxy Server

### SUMMARY

+ The project consists of 2 programs 
    + webserver.cpp is a multi-threaded web server written in C++. 
    + proxyserver.cpp is a multi-threaded proxy server written in C++.
+ The web-server listens on a specified port 37000, accepts incoming request from the proxy server, and serves HTML and PDF files to proxy server. 
+ The proxy-server listens on a specified port 52837, 
    +   Accepts incoming request from the client, and forwards the request to web-server
    +   Receives responses from the web-server, and forwards it to the client.
+ Each client connection is handled in a separate thread, allowing the server to handle multiple client requests concurrently. 

### NOTES, KNOWN BUGS, AND/OR INCOMPLETE PARTS

+ Observed 0 memory leaks. Tested for memory leaks using "leaks". Valgrind not supported for Mac OS Ventura. 
+ Added signal.h header to the multithreaded web-server.
+ No bugs/errors observed when tested on Mac Ventura OS.
+ The proxy-server prints output per response and NOT output per packet. 
+ The proxy-server will send "502 Bad Gateway" if the web-server is not running. The proxy-server "SENDS" a response and is not "FORWARDING" a response in this scenario, thus the output is "NOT RESPONDING: Destination server " and not "proxy-forward,DESTINATION,THREAD-ID,TIMESTAMP".
+ The web browser might request for favicon file which is not available, thus there is a possibility of 2 requests for an HTML file.

### REFERENCES

+ [Computer Networking: A Top-down Approach by  James F. Kurose, Keith W. Ross](https://www.amazon.com/Computer-Networking-James-Kurose-dp-0136681557/dp/0136681557/ref=dp_ob_image_bk) 
+ [geeksforgeeks - Chrono](https://www.geeksforgeeks.org/chrono-in-c/)


### INSTRUCTIONS

+ Execute the below commands:
+ To run the web-server
+        g++ webserver.cpp -o webserver
        ./webserver 
+ To run the proxy-server
+       g++ proxyserver.cpp -o proxyserver
        ./proxyserver
+ Request the following files using the URLs
    + http://127.0.0.1:52837/home.html   (for Proxy Server)
    + http://127.0.0.1:37000/home.html   (for Web Server)




