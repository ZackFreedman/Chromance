#ifndef DEBUG_UDP_H_
#define DEBUG_UDP_H_

#include <Arduino.h>
#include <Stream.h>
#include <WiFiUdp.h>

class debugUdp: public Stream {
public:
    using Stream::Stream;
    using Print::write; // Import other write() methods to support things like write(0) properly

    debugUdp(int port){
        conn_.begin(port);
    };
    
    size_t write(uint8_t c) override;
    size_t write(const uint8_t *buffer, size_t size) override;
    size_t write(const char *buffer, size_t size);
    size_t write(const char *str);
    int read(void) override;
    int read(char* buffer, size_t size);
    int read(uint8_t* buffer, size_t size) override;
    size_t readBytes(uint8_t *buffer, size_t length) override;
    int available(void) override;
    int peek(void) override;
    size_t readBytesUntil(char terminator, char *buffer, size_t length); // as readBytes with terminator character
    String readStringUntil(char terminator);
    virtual void flush() { conn_.flush(); }
    virtual bool outputCanTimeout () { return true; }

private:
    WiFiUDP conn_;
    // int available(void) override;
};
#endif