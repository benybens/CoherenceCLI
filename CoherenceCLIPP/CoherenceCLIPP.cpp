#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h> // Required for inet_pton
#include "tinyxml2.h"
#include <chrono>
#include <iomanip>

#pragma comment(lib, "Ws2_32.lib") // Link Winsock library

using namespace tinyxml2;

// Server connection details
const std::string SERVER_IP = "127.0.0.1"; // Localhost
const int SERVER_PORT = 20480;            // Default emWave port

// Function to initialize Winsock
bool initializeWinsock() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed with error: " << result << std::endl;
        return false;
    }
    return true;
}
void logWithTimestamp(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);

    // Use localtime_s for thread safety
    struct tm timeInfo;
    localtime_s(&timeInfo, &now_time);

    std::cout << "[" << std::put_time(&timeInfo, "%H:%M:%S") << "] " << message << std::endl;
}


// Function to connect to the emWave server
SOCKET connectToEmWave() {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        return INVALID_SOCKET;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);

    // Use inet_pton instead of inet_addr
    if (inet_pton(AF_INET, SERVER_IP.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address or address not supported" << std::endl;
        closesocket(sock);
        return INVALID_SOCKET;
    }

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Error connecting to server: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        return INVALID_SOCKET;
    }

    std::cout << "Connected to emWave sensor!" << std::endl;
    return sock;
}

// Function to parse and display coherence score from XML
void parseAndDisplayCoherence(const std::string& xmlData) {
    tinyxml2::XMLDocument doc;
    if (doc.Parse(xmlData.c_str()) != XML_SUCCESS) {
        return; // Ignore XML parsing errors silently
    }

    XMLElement* root = doc.FirstChildElement("D01");
    if (!root) {
        return; // Ignore entries without the <D01> tag
    }

    const char* coherenceScore = root->Attribute("S");
    const char* heartRate = root->Attribute("HR");

    if (coherenceScore && heartRate) {
        std::cout << "Coherence Score: " << coherenceScore
            << ", Heart Rate: " << heartRate << std::endl;
    }
}


// Main function
int main() {
    if (!initializeWinsock()) {
        return 1;
    }

    SOCKET sock = connectToEmWave();
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    // Send command to start session
    const std::string startCommand = "<CMD ID=\"2\" />\n";
    send(sock, startCommand.c_str(), static_cast<int>(startCommand.size()), 0);

    char buffer[1024];
    while (true) {
        int bytesRead = recv(sock, buffer, static_cast<int>(sizeof(buffer) - 1), 0);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0'; // Null-terminate the received data
            std::string xmlData(buffer);
            parseAndDisplayCoherence(xmlData);
        }
        else if (bytesRead == 0) {
            std::cout << "Connection closed by server." << std::endl;
            break;
        }
        else {
            std::cerr << "Error receiving data: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}

