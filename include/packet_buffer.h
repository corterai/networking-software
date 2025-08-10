#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <cstring>
#include <algorithm>

/**
 * @brief Network packet buffer class
 * 
 * This class provides efficient memory management for network packets,
 * supporting zero-copy operations and buffer chaining.
 */
class PacketBuffer {
public:
    // Buffer types
    enum class Type {
        RX_PACKET,      // Received packet
        TX_PACKET,      // Packet to transmit
        FRAGMENT,       // Packet fragment
        CONTROL         // Control packet
    };

    // Buffer flags
    enum class Flags : uint32_t {
        NONE = 0x00000000,
        CHECKSUM_OFFLOAD = 0x00000001,
        VLAN_TAGGED = 0x00000002,
        JUMBO_FRAME = 0x00000004,
        BROADCAST = 0x00000008,
        MULTICAST = 0x00000010,
        UNICAST = 0x00000020,
        ERROR = 0x00000040,
        DROPPED = 0x00000080
    };

    // Constructor
    explicit PacketBuffer(size_t capacity = 2048, Type type = Type::RX_PACKET);
    
    // Copy constructor
    PacketBuffer(const PacketBuffer& other);
    
    // Move constructor
    PacketBuffer(PacketBuffer&& other) noexcept;
    
    // Destructor
    ~PacketBuffer();

    // Assignment operators
    PacketBuffer& operator=(const PacketBuffer& other);
    PacketBuffer& operator=(PacketBuffer&& other) noexcept;

    // Buffer management
    void clear();
    void reset();
    bool resize(size_t new_size);
    void reserve(size_t capacity);
    
    // Data access
    uint8_t* data();
    const uint8_t* data() const;
    uint8_t* dataAt(size_t offset);
    const uint8_t* dataAt(size_t offset) const;
    
    // Size operations
    size_t size() const;
    size_t capacity() const;
    bool empty() const;
    void setSize(size_t size);
    
    // Data manipulation
    bool append(const uint8_t* data, size_t length);
    bool prepend(const uint8_t* data, size_t length);
    bool insert(size_t offset, const uint8_t* data, size_t length);
    bool remove(size_t offset, size_t length);
    
    // Buffer chaining
    void setNext(std::unique_ptr<PacketBuffer> next);
    PacketBuffer* getNext() const;
    std::unique_ptr<PacketBuffer> takeNext();
    
    // Metadata
    Type getType() const;
    void setType(Type type);
    
    Flags getFlags() const;
    void setFlags(Flags flags);
    void addFlags(Flags flags);
    void removeFlags(Flags flags);
    bool hasFlags(Flags flags) const;
    
    // Timestamp
    uint64_t getTimestamp() const;
    void setTimestamp(uint64_t timestamp);
    
    // Interface information
    uint32_t getInterfaceIndex() const;
    void setInterfaceIndex(uint32_t index);
    
    // Statistics
    uint32_t getReferenceCount() const;
    void incrementReferenceCount();
    void decrementReferenceCount();

    // Utility methods
    template<typename T>
    T* getHeader() {
        return reinterpret_cast<T*>(data());
    }
    
    template<typename T>
    const T* getHeader() const {
        return reinterpret_cast<const T*>(data());
    }
    
    template<typename T>
    T* getHeaderAt(size_t offset) {
        return reinterpret_cast<T*>(dataAt(offset));
    }
    
    template<typename T>
    const T* getHeaderAt(size_t offset) const {
        return reinterpret_cast<const T*>(dataAt(offset));
    }

private:
    // Data storage
    std::vector<uint8_t> buffer_;
    size_t data_size_;
    
    // Metadata
    Type type_;
    Flags flags_;
    uint64_t timestamp_;
    uint32_t interface_index_;
    uint32_t reference_count_;
    
    // Buffer chaining
    std::unique_ptr<PacketBuffer> next_;
    
    // Helper methods
    bool isValidOffset(size_t offset) const;
    bool isValidRange(size_t offset, size_t length) const;
    void moveData(size_t from_offset, size_t to_offset, size_t length);
};

// Flag operations
inline PacketBuffer::Flags operator|(PacketBuffer::Flags a, PacketBuffer::Flags b) {
    return static_cast<PacketBuffer::Flags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline PacketBuffer::Flags operator&(PacketBuffer::Flags a, PacketBuffer::Flags b) {
    return static_cast<PacketBuffer::Flags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline PacketBuffer::Flags operator~(PacketBuffer::Flags a) {
    return static_cast<PacketBuffer::Flags>(~static_cast<uint32_t>(a));
}

inline PacketBuffer::Flags& operator|=(PacketBuffer::Flags& a, PacketBuffer::Flags b) {
    a = a | b;
    return a;
}

inline PacketBuffer::Flags& operator&=(PacketBuffer::Flags& a, PacketBuffer::Flags b) {
    a = a & b;
    return a;
}