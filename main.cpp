#include <iostream>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include "tinyxml2.h"

using namespace boost::asio;
using namespace tinyxml2;

const std::string HOST = "127.0.0.1";
const int PORT = 20480;

void parseAndDisplayXML(const std::string& xmlData) {
    XMLDocument doc;
    if (doc.Parse(xmlData.c_str()) == XML_SUCCESS) {
        XMLElement* root = doc.FirstChildElement("D01");
        if (root) {
            const char* coherenceScore = root->Attribute("S");
            const char* heartRate = root->Attribute("HR");
            std::cout << "Coherence Score: " << (coherenceScore ? coherenceScore : "N/A")
                      << ", Heart Rate: " << (heartRate ? heartRate : "N/A") << std::endl;
        }
    } else {
        std::cerr << "Error parsing XML data." << std::endl;
    }
}

void connectAndMonitor() {
    try {
        io_service io;
        ip::tcp::socket socket(io);
        socket.connect(ip::tcp::endpoint(ip::address::from_string(HOST), PORT));
        std::cout << "Connected to emWave sensor." << std::endl;

        // Start a session
        std::string startCommand = "<CMD ID=\"2\" />\n";
        socket.send(buffer(startCommand));

        char data[1024];
        while (true) {
            size_t length = socket.read_some(buffer(data));
            std::string xmlData(data, length);
            parseAndDisplayXML(xmlData);
        }
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "Starting CoherenceCLI..." << std::endl;
    connectAndMonitor();
    return 0;
}
