#include "protocol.hpp"
#include "transmission.hpp"

#include <cassert>

namespace Protocol {

    
template<typename T>
T* getFromBuffer(uint8_t *buff) {
    return reinterpret_cast<T *>(alignUp(buff, alignof(T)));
}

template<typename T>
inline T* consumeBuffer(uint8_t *&buff) {
    T *ret = getFromBuffer<T>(buff);
    buff = reinterpret_cast<uint8_t *>(ret + 1);
    return ret;
}

template<typename T>
inline const T* consumeBuffer(const uint8_t *&buff) {
    const T *ret = reinterpret_cast<const T *>(alignUp(buff, alignof(T)));
    buff = reinterpret_cast<const uint8_t *>(ret + 1);
    return ret;
}
/*
 *
 * Protocol:
 *
 * Tag (4 bytes)
 * Packet (responsible for including length if necessary)
 *
 * Buffer structure:
 *
 * The buffer is composed of a series of control codes, which may
 * optionally include data associated with each code.
 *
 * Each code begins with a byte indicating what data follows:
 *
 */

namespace ControlSeq {

    enum Code : uint8_t {
        PACKET,
        SKIP
    };

    struct Packet {
        ssize_t offsetToNextReadEnd; // Relative to the start of this struct
        ssize_t offsetToNextSend; // Relative to the start of this struct
        uint32_t size;
        uint8_t senderId;
    };

    struct Skip {
        ssize_t offsetToNextReadEnd; // Relative to the start of this struct
        ssize_t offsetToNextSend;
    };

}

void PacketHolder::resizeRead() {
    uint8_t *tmpHead = readEnd;
   
    while(readEnd != sendHead 
        && *consumeBuffer<ControlSeq::Code>(tmpHead) == ControlSeq::SKIP) 
    {
        auto *skip = consumeBuffer<ControlSeq::Skip>(tmpHead);
        tmpHead = reinterpret_cast<uint8_t *>(skip);
        tmpHead += skip->offsetToNextReadEnd;
        readEnd = tmpHead;
    }
}


static uint8_t* calculateNewSendHead(uint8_t *tmpHead, uint32_t size) {


    tmpHead -= size;
    tmpHead = alignDown(tmpHead, Packets::PACKET_ALIGNMENT);
    tmpHead -= sizeof(Packets::Tag);

    tmpHead -= sizeof(ControlSeq::Packet);
    tmpHead = alignDown(tmpHead, alignof(ControlSeq::Packet));

    tmpHead -= sizeof(ControlSeq::Code);
    tmpHead = alignDown(tmpHead, alignof(ControlSeq::Code));

    return tmpHead;
}
NetReturn PacketHolder::rollbackSendHead(uint8_t *&packetBuffer, uint32_t size, 
    uint8_t destination) 
{
    resizeRead();

    uint8_t *tmpHead = readEnd;
    uint8_t *priorSendHead = sendHead, *priorReadEnd = readEnd;

    tmpHead = calculateNewSendHead(tmpHead, size);

    if(tmpHead < buffer) {
        tmpHead = calculateNewSendHead(buffer + bufferLen, size);
        if(tmpHead < buffer) return netHandleInvalidState();
    }
    if(!isLocationValid(readEnd, readHead, tmpHead)) {
        return {0, NetReturn::NOT_ENOUGH_SPACE};
    }


    readEnd = sendHead = tmpHead;

    *consumeBuffer<ControlSeq::Code>(tmpHead) = ControlSeq::PACKET;
    
    auto *packetControl = consumeBuffer<ControlSeq::Packet>(tmpHead);
    const auto *packetControlLoc = reinterpret_cast<const uint8_t *>(packetControl); 
    packetControl->offsetToNextSend = priorSendHead - packetControlLoc;
    packetControl->offsetToNextReadEnd = priorReadEnd - packetControlLoc;
    packetControl->size = size;
    packetControl->senderId = destination;

    packetBuffer = alignUp(tmpHead + sizeof(Packets::Tag), Packets::PACKET_ALIGNMENT);
    return {0, NetReturn::OK};
}

NetReturn PacketHolder::extractPacketInfo(PacketConstructionArgs &output) const {
    const uint8_t *tmpHead = processHead;
    ControlSeq::Code code = *consumeBuffer<ControlSeq::Code>(tmpHead);
    
    switch(code) {
        case ControlSeq::PACKET: 
        {
            const auto *packetControl = consumeBuffer<ControlSeq::Packet>(tmpHead);
            
            const uint8_t *packetBuffer 
                = alignUp(tmpHead + sizeof(Packets::Tag), Packets::PACKET_ALIGNMENT);
            
            const auto *tag = reinterpret_cast<const uint32_t *>
                (packetBuffer - sizeof(Packets::Tag));
            
            output.tag = (Packets::Tag)ntohl(*tag);
            output.buffer = packetBuffer;
            output.len = packetControl->size;
            return {0, NetReturn::OK};
        }
        default:
            return netHandleInvalidState();
    }
}

void PacketHolder::dropPacket() {
    uint8_t *tmpHead = processHead;
    *consumeBuffer<ControlSeq::Code>(tmpHead) = ControlSeq::SKIP;
}

NetReturn PacketHolder::getSenderId() const {
    const uint8_t *tmpHead = processHead;
    if(*consumeBuffer<ControlSeq::Code>(tmpHead) == ControlSeq::PACKET) {
        return {consumeBuffer<ControlSeq::Packet>(tmpHead)->senderId, NetReturn::OK};
    }
    else return {0, NetReturn::INVALID_DATA};
}

void PacketHolder::finishProcessing() {
    uint8_t *tmpHead = processHead;
    const auto code = *consumeBuffer<ControlSeq::Code>(tmpHead);
    switch(code) {
        case ControlSeq::PACKET:
        {
            auto *packetControl = 
                consumeBuffer<ControlSeq::Packet>(tmpHead);
            
            processHead = reinterpret_cast<uint8_t *>(packetControl)
                + packetControl->offsetToNextSend;
            break;
        }
        case ControlSeq::SKIP:
        { 
           auto *skip = 
                consumeBuffer<ControlSeq::Skip>(tmpHead);
            
            processHead = reinterpret_cast<uint8_t *>(skip)
                + skip->offsetToNextSend;
            break;
        }
    }
}

NetReturn PacketHolder::sendPacket(Transmission::Writer &writer) {
    if(sendHead == processHead) {
        return {0, NetReturn::OK};
    }
    uint8_t *tmpHead = sendHead;
    auto *code = consumeBuffer<ControlSeq::Code>(tmpHead);
    switch(*code) {
        case ControlSeq::PACKET:
        {
            const auto *packetControl = consumeBuffer<ControlSeq::Packet>(tmpHead);
            
            const uint8_t *packet = alignUp(tmpHead + sizeof(Packets::Tag), 
                Packets::PACKET_ALIGNMENT);

            packet -= sizeof(Packets::Tag);
            
            NetReturn res = writer.write(packet, 
                packetControl->size + sizeof(Packets::Tag), packetControl->senderId);

            if(res.errorCode != NetReturn::OK) return res;

            *code = ControlSeq::SKIP;
            return res;
        }

        case ControlSeq::SKIP:
        {
            auto *skip = consumeBuffer<ControlSeq::Skip>(tmpHead);
            sendHead = reinterpret_cast<uint8_t *>(skip) + skip->offsetToNextSend;
            return {1, NetReturn::OK};
        }
    }
    return {}; // unreachable
}

static uint8_t* calculateEnd(uint8_t *tmpHead) {

    tmpHead = alignUp(tmpHead, alignof(ControlSeq::Code));
    tmpHead += sizeof(ControlSeq::Code);

    tmpHead = alignUp(tmpHead, alignof(ControlSeq::Packet));
    tmpHead += sizeof(ControlSeq::Packet);

    tmpHead += sizeof(Packets::Tag);
    tmpHead = alignUp(tmpHead, Packets::PACKET_ALIGNMENT);
    tmpHead += Packets::MAX_PACKET_SIZE;
    return tmpHead;
}

void PacketHolder::initCachedReadHead() {
    cachedReadHead = calculateEnd(buffer);
    if(static_cast<size_t>(cachedReadHead - buffer) > bufferLen) {
        netHandleInvalidState();
    }
}

uint8_t* PacketHolder::makeValid(uint8_t *start) {

    uint8_t *tmp = calculateEnd(start);
    if(static_cast<size_t>(tmp - buffer) > bufferLen) {
        start = buffer;
        assert(static_cast<size_t>(calculateEnd(buffer) - buffer) < bufferLen);
    }
    return start;

}

NetReturn PacketHolder::readPacket(Transmission::Reader &reader) {
    resizeRead();
    
    uint8_t *tmpHead = readHead;
    uint8_t *oldHead = readHead;

    if(!isLocationValid(readEnd, readHead, cachedReadHead)) {
        return {0, NetReturn::NOT_ENOUGH_SPACE};
    }
    readHead = cachedReadHead;
    cachedReadHead = makeValid(calculateEnd(readHead));

    *consumeBuffer<ControlSeq::Code>(tmpHead) = ControlSeq::PACKET;
    auto *packetControl = consumeBuffer<ControlSeq::Packet>(tmpHead);
    
    tmpHead += sizeof(Packets::Tag);
    tmpHead = alignUp(tmpHead, Packets::PACKET_ALIGNMENT);
    tmpHead -= sizeof(Packets::Tag);

    NetReturn res = reader.read(tmpHead, Packets::MAX_PACKET_SIZE + sizeof(Packets::Tag), 
        &packetControl->senderId);

    if(res.errorCode != NetReturn::OK && res.errorCode != NetReturn::CANDIDATE) {
        readHead = oldHead;
        return res;
    }
    else if(res.bytes < sizeof(Packets::Tag)) {
        readHead = oldHead;
        return {0, NetReturn::INVALID_DATA};
    }

    readHead = makeValid(tmpHead + res.bytes);
    cachedReadHead = makeValid(calculateEnd(readHead));

    packetControl->size = res.bytes - sizeof(Packets::Tag);
    packetControl->offsetToNextSend = readHead - reinterpret_cast<const uint8_t *>(packetControl);
    packetControl->offsetToNextReadEnd = packetControl->offsetToNextSend;

    processEnd = readHead;

    return res;
}

}
