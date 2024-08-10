#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include "packets.hpp"

extern "C" {
#include <arpa/inet.h>
}

namespace Transmission {

    class Reader;
    class Writer;

}

namespace Protocol {

constexpr uint32_t MAJOR = 0;
constexpr uint32_t MINOR = 0;


class PacketHolder {
    
    uint8_t *buffer;
    size_t bufferLen;
 
    // Where to place the next packet to be read   
    uint8_t *readHead;
    uint8_t *cachedReadHead; 

    // End of read area (not always up-to-date)
    uint8_t *readEnd;
    
    // Current packet being processed (unless we aren't processing)
    uint8_t *processHead;
    uint8_t *processEnd;

    // Next packet to be sent
    uint8_t *sendHead;


    void resizeRead();
    
    static inline bool isLocationValid(const uint8_t *first, const uint8_t *last, const uint8_t *location) {
        if(first <= last) return location > last || location < first;
        else return location > last && location < first; // first and last are backwards
    }

    void initCachedReadHead();
    uint8_t* makeValid(uint8_t*);

    NetReturn rollbackSendHead(uint8_t *&packetBuffer, uint32_t packetSize, uint8_t destination);
protected:

    struct PacketConstructionArgs {
        Packets::Tag tag;
        const uint8_t *buffer;
        uint32_t len;
    };
    NetReturn extractPacketInfo(PacketConstructionArgs &output) const;

public:
   
    // Both the start and end of the buffer must be well-aligned
    inline PacketHolder(void *_buffer, uint32_t bufferLen) 
        : buffer(reinterpret_cast<uint8_t *>(_buffer)), bufferLen(bufferLen), 
        readHead(buffer),  readEnd(buffer), processHead(buffer), 
        processEnd(buffer), sendHead(buffer) 
    {
        initCachedReadHead();
    }

    template<typename T>
    NetReturn addPacket(const Packets::Packet<T> &packet) {
        return addPacket(packet, 0xFF);
    }
    // Does not hold a reference to `packet` once call completes 
    template<typename T>
    NetReturn addPacket(const Packets::Packet<T> &packet, uint8_t destination) {
        
        uint8_t *packetBuffer;

        uint32_t size = packet.getSize();

        NetReturn res = rollbackSendHead(packetBuffer, size, destination);
        if(res.errorCode != NetReturn::OK) return res;

        auto *tag 
            = reinterpret_cast<uint32_t *>(packetBuffer - sizeof(Packets::Tag));

        *tag = htonl(static_cast<uint32_t>(Packets::Packet<T>::tag));
        
        res = packet.netWriteToBuffer(packetBuffer, size);

        if(res.errorCode == NetReturn::NOT_ENOUGH_SPACE) return netHandleInvalidState();
        return res;
    }

    NetReturn sendPacket(Transmission::Writer &write);
    
    NetReturn readPacket(Transmission::Reader &reader);
 
    NetReturn getSenderId() const;   
    void dropPacket();
    void finishProcessing();

};

template<typename T> 
    requires requires (const T t, Packets::Tag tag, T::PacketUnion *pu, 
        const void *buffer, uint32_t len) 
    {
        {t.constructPacket(tag, pu, buffer, len)} -> std::same_as<NetReturn>;
    } 
class PacketProcessor : public PacketHolder {
    T packetFactory;
public:
    PacketProcessor(void *buffer, uint32_t bufferLen, T packetFactory)
        : PacketHolder(buffer, bufferLen), packetFactory(packetFactory) {}
    
    NetReturn processPacket(T::PacketUnion *pu) {
   
        PacketConstructionArgs args;
        NetReturn res = extractPacketInfo(args);
        
        if(res.errorCode != NetReturn::OK) {
            return res;
        }

        res = packetFactory.constructPacket
            (args.tag, pu, args.buffer, args.len);
        
        return res;
    }
    inline T& getPacketFactory() {return packetFactory;}
};

}

#endif
