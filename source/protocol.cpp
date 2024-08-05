#include "protocol.hpp"

namespace Protocol {

    
template<typename T>
T* getFromBuffer(void *buff) {
    return reinterpret_cast<T *>(alignUp(buff, alignof(T)));
}

template<typename T>
inline T* consumeFromBuffer(void *&buff) {
    T *ret = getFromBuffer<T>(buff);
    buff = (uint8_t *)ret + sizeof *ret;
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
    while(readEnd != sendHead && *consumeFromBuffer<ControlSeq::Code>(tmpHead) == ControlSeq::SKIP) {
        tmpHead += consumeFromBuffer<ControlSeq::Skip>(tmpHead)->offsetToNextReadEnd;
        readEnd = tmpHead;
    }
}

NetReturn PacketHolder::rollbackSendHead(uint8_t &*packetBuffer, uint32_t size) {
    collapseSends();

    uint8_t *tmpHead = readEnd;
    uint8_t *priorSendHead = sendHead, *priorReadEnd = readEnd;

    tmpHead -= size;
    tmpHead = alignDown(tmpHead, PACKET_ALIGNMENT);
    tmpHead -= sizeof Packets::Tag;

    tmpHead -= sizeof ControlSeq::Packet;
    tmpHead = alignDown(tmpHead, alignof(ControlSeq::Packet));

    tmpHead -= sizeof ControlSeq::Code;
    tmpHead = alignDown(tmpHead, alignof(ControlSeq::Code));

    if(size > bufferLen) return {0, NetReturn::NOT_ENOUGH_SPACE};
    if(tmpHead < buffer) {
        tmpHead = buffer + bufferLen - size;
    }
    if(!isLocationValid(sendHead, readHead, tmpHead)) {
        return {0, NetReturn::NOT_ENOUGH_SPACE};
    }


    readEnd = sendHead = tmpHead;

    *consumeBuffer<ControlSeq::Code>(tmpHead) = ControlSeq::Packet;
    
    auto *packetControl = consumeBuffer<ControlSeq::Packet>(tmpHead);
    const auto *packetControlLoc = reinterpret_cast<const uint8_t *>(packetControl); 
    packetControl->offsetToNextSend = priorSendHead - packetControl;
    packetControl->offsetToNextReadEnd = priorReadEnd - packetControl;
    packetControl->size = size;
    packetControl->senderId = ~0;

    packetBuffer = alignUp(tmpHead + sizeof Packets::Tag, PACKET_ALIGNMENT);
    return {0, NetReturn::OK};
}

NetReturn PacketHolder::extractPacketInfo(PacketConstructionArgs &output) const {
    const uint8_t *tmpHead = processHead;
    ControlSeq::Code code = *consumeBuffer<ControlSeq::Code>(tmpHead);
    
    switch(code) {
        ControlSeq::PACKET:
            const auto *packetControl = consumeBuffer<ControlSeq::Packet>(tmpHead);
            
            const uint8_t *packetBuffer 
                = alignUp(tmpHead + sizeof ControlSeq::Packet, PACKET_ALIGNMENT);
            
            const auto *tag = reinterpret_cast<const Packets::Tag *>
                (packetBuffer - sizeof Packets::Tag);
            
            output.tag = *tag;
            output.buffer = packetBuffer;
            output.len = packetControl->size;
            return {0, NetReturn::OK};
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
    const auto code = *consumeBuffer<ControlSeq::Code>(processHead);
    const uint8_t *tmpHead = processHead;
    switch(code) {
        ControlSeq::PACKET:
            processHead += consumeBuffer<ControlSeq::Packet>(tmpHead)->offsetToNextSend;
        ControlSeq::SKIP:
            processHead += consumeBuffer<ControlSeq::Skip>(tmpHead)->offsetToNextSend;
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
            const auto *packetControl = consumeBuffer<ControlSeq::Packet>(tmpHead);
            const uint8_t *packet = alignUp(tmpHead + sizeof Packets::Tag, PACKET_ALIGNMENT);
            packet -= sizeof Packets::Tag;
            
            NetReturn res = writer.write(packet, packetControl->size + sizeof Packets::Tag);
            if(res.errorCode != NetReturn::OK) return res;

            *code = ControlSeq::SKIP;
            return res;
        case ControlSeq::SKIP:
            const auto *skip = consumeBuffer<ControlSeq::Skip>(tmpHead);
            sendHead = reinterpret_cast<const uint8_t *>(skip) + skip->offsetToNextSend;
            return {1, NetReturn::OK};
    }
}

NetReturn PacketHolder::readPacket(Transmission::Reader &reader) {
    resizeRead();
    
    uint8_t *tmpHead = readHead;
    uint8_t oldHead = readHead;

    tmpHead = alignUp(tmpHead, alignof(ControlSeq::Code));
    tmpHead += sizeof ControlSeq::Code;

    tmpHead = alignUp(tmpHead, alignof(ControlSeq::Packet));
    tmpHead += sizeof ControlSeq::Packet;

    tmpHead += sizeof Packets::Tag;
    tmpHead = alignUp(tmpHead, PACKET_ALIGNMENT);
    tmpHead += MAX_PACKET_SIZE;

    // Buffer must always be large enough to sustain at least one transmission
    if(tmpHead - readHead > bufferLen) return netHandleInvalidState();

    if(tmpHead - buffer > bufferLen) tmpHead -= bufferLen;
    if(!isLocationValid(readHead, sendHead, tmpHead)) {
        return {0, NetReturn::NOT_ENOUGH_SPACE};
    }

    std::swap(readHead, tmpHead);
    
    *consumeBuffer<ControlSeq::Code>(tmpHead) = ControlSeq::PACKET;
    auto *packetControl = consumeBuffer<ControlSeq::Packet>(tmpHead);
    
    tmpHead += sizeof Packets::Tag;
    tmpHead = alignUp(tmpHead, PACKET_ALIGNMENT);
    tmpHead -= sizeof Packets::Tag;

    NetReturn res = reader.read(tmpHead, MAX_PACKET_SIZE + sizeof Packets::Tag, &packetControl->senderId);

    if(
        (res.errorCode != NetReturn::OK && res.errorCode != NetReturn::CANDIDATE)
        || res.bytes < sizeof Packets::Tag
    ) {
        readHead = oldHead;
        return res.errorCode == NetReturn::OK ? {0, NetReturn::INVALID_DATA} : res;
    }

    readHead = tmpHead + res.bytes;

    packetControl->size = res.bytes - sizeof Packets::Tag;
    packetControl->offsetToNextSend = readHead - reinterpret_cast<const uint8_t *>(packetControl);
    packetControl->offsetToNextReadEnd = packetControl->offsetToNextSend;

    processEnd = readHead;

    return res;
}

}
