 #include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class SerialSender {
private:
    int sockfd;
    
    bool connectToQEMU(const std::string& host, int port) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            std::cerr << "Errore creazione socket" << std::endl;
            return false;
        }
        
        sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
            std::cerr << "Indirizzo non valido" << std::endl;
            return false;
        }
        
        if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            std::cerr << "Connessione fallita" << std::endl;
            return false;
        }
        
        return true;
    }
    
    bool sendAll(const void* data, size_t size) {
        const char* ptr = static_cast<const char*>(data);
        while (size > 0) {
            ssize_t sent = write(sockfd, ptr, size);
            if (sent <= 0) {
                return false;
            }
            ptr += sent;
            size -= sent;
        }
        return true;
    }

public:
    bool sendFile(const std::string& filename, const std::string& host = "127.0.0.1", int port = 1234) {
        // Leggi il file
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Errore apertura file: " << filename << std::endl;
            return false;
        }
        
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<char> fileData(fileSize);
        file.read(fileData.data(), fileSize);
        file.close();
        
        std::cout << "File: " << filename << " (" << fileSize << " bytes)" << std::endl;
        
        // Connetti a QEMU
        if (!connectToQEMU(host, port)) {
            return false;
        }
        
        // Header magic
        uint32_t headerMagic = 0xD72A90B1;
        if (!sendAll(&headerMagic, sizeof(headerMagic))) {
            std::cerr << "Errore invio header" << std::endl;
            close(sockfd);
            return false;
        }
        
        // Dimensione file (little-endian)
         uint32_t sizeLE = fileSize;
        if (!sendAll(&sizeLE, sizeof(sizeLE))) {
            std::cerr << "Errore invio dimensione" << std::endl;
            close(sockfd);
            return false;
        } 
        
        // Dati file
        if (!sendAll(fileData.data(), fileSize)) {
            std::cerr << "Errore invio dati" << std::endl;
            close(sockfd);
            return false;
        }
        
        
        close(sockfd);
        std::cout << "File inviato con successo!" << std::endl;
        return true;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <file.bin> [host] [port]" << std::endl;
        std::cout << "Esempio: " << argv[0] << " kernel.bin 127.0.0.1 1234" << std::endl;
        return 1;
    }
    
    std::string filename = argv[1];
    std::string host = (argc > 2) ? argv[2] : "127.0.0.1";
    int port = (argc > 3) ? std::stoi(argv[3]) : 4321;
    
    SerialSender sender;
    if (sender.sendFile(filename, host, port)) {
        return 0;
    } else {
        return 1;
    }
}