#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include "packets.hpp"

namespace Protocol {


class PacketHolder {
    
    uint8_t *buffer;
    size_t bufferLen;
 
    // Where to place the next packet to be read   
    uint8_t *readHead;

    // End of read area (not always up-to-date)
    uint8_t *readEnd;
    
    // Current packet being processed (unless we aren't processing)
    uint8_t *processHead;
    uint8_t *processEnd;

    // Next packet to be sent
    uint8_t *sendHead;



    uint32_t currentPacketLen;

    void resizeRead();
    
    static inline bool isLocationValid(const uint8_t *first, const uint8_t *last, const uint8_t *location) {
        bool isLocationBetween = location < last && location > first;
        if(first < last) return !isLocationBetween;
        else return isLocationBetween;
    }

    NetReturn rollbackSendHead(uint8_t &*packetBuffer, uint32_t packetSize);
protected:

    struct PacketConstructionArgs {
        Packets::Tag tag;
        const uint8_t *buffer;
        uint32_t len;
    };
    NetReturn extractPacketInfo(PacketConstructionArgs &output) const;

public:
   
    // Both the start and end of the buffer must be well-aligned
    inline PacketHolder(void *buffer, uint32_t bufferLen) 
        : buffer(buffer), bufferLen(bufferLen), readHead(buffer), 
        readEnd(buffer), processHead(buffer), 
        processEnd(buffer), sendHead(buffer) {}

    // Does not hold a reference to `packet` once call completes 
    template<typename T>
    NetReturn addPacket(const Packets::Packet<T> &packet) {
        
        uint8_t *packetBuffer;

        NetReturn res = rollbackHead(packetBuffer, packet.getSize());
        if(res.err != NetReturn::OK) return res;

        auto *tag 
            = reinterpret_cast<Packets::Tag *>(packetBuffer - sizeof Packets::Tag);

        *tag = htonl(packet::tag);
        
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
class PacketProcessor : public PacketHolder 
    requires (const T t, Packets::Tag tag, T::PacketUnion *pu, 
        const void *buffer, uint32_t len) 
    {
        {t.constructPacket(tag, pu, buffer, len)} -> NetReturn;
    } 
{
    T packetFactory;
public:
    PacketProcessor(const void *buffer, uint32_t bufferLen, T packetFactory)
        : PacketHolder(buffer, bufferLen), packetFactory(packetFactory) {}
    
    NetReturn processPacket(T::PacketUnion *pu) {
   
        PacketConstructionArgs args;
        NetReturn res = extractPacketInfo(args);
        
        if(res != NetReturn::OK) {
            return res;
        }

        res = packetFactory.constructPacket
            (args.tag, pu, args.packetBuffer, args.len);

        if(res.errorCode == NetReturn::OK) {
            currentPacketLen = res.bytes;
        }
        
        return res;
    }
};

}

#endif
