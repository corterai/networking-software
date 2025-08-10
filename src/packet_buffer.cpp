#include "packet_buffer.h"
#include <cstring>
#include <algorithm>
#include <stdexcept>

PacketBuffer::PacketBuffer(size_t capacity, Type type)
    : buffer_(capacity, 0)
    , data_size_(0)
    , type_(type)
    , flags_(Flags::NONE)
    , timestamp_(0)
    , interface_index_(0)
    , reference_count_(1)
    , next_(nullptr) {
}

PacketBuffer::PacketBuffer(const PacketBuffer& other)
    : buffer_(other.buffer_)
    , data_size_(other.data_size_)
    , type_(other.type_)
    , flags_(other.flags_)
    , timestamp_(other.timestamp_)
    , interface_index_(other.interface_index_)
    , reference_count_(1)
    , next_(nullptr) {
    if (other.next_) {
        next_ = std::make_unique<PacketBuffer>(*other.next_);
    }
}

PacketBuffer::PacketBuffer(PacketBuffer&& other) noexcept
    : buffer_(std::move(other.buffer_))
    , data_size_(other.data_size_)
    , type_(other.type_)
    , flags_(other.flags_)
    , timestamp_(other.timestamp_)
    , interface_index_(other.interface_index_)
    , reference_count_(other.reference_count_)
    , next_(std::move(other.next_)) {
    other.data_size_ = 0;
    other.reference_count_ = 0;
}

PacketBuffer::~PacketBuffer() = default;

PacketBuffer& PacketBuffer::operator=(const PacketBuffer& other) {
    if (this != &other) {
        buffer_ = other.buffer_;
        data_size_ = other.data_size_;
        type_ = other.type_;
        flags_ = other.flags_;
        timestamp_ = other.timestamp_;
        interface_index_ = other.interface_index_;
        reference_count_ = 1;
        
        if (other.next_) {
            next_ = std::make_unique<PacketBuffer>(*other.next_);
        } else {
            next_.reset();
        }
    }
    return *this;
}

PacketBuffer& PacketBuffer::operator=(PacketBuffer&& other) noexcept {
    if (this != &other) {
        buffer_ = std::move(other.buffer_);
        data_size_ = other.data_size_;
        type_ = other.type_;
        flags_ = other.flags_;
        timestamp_ = other.timestamp_;
        interface_index_ = other.interface_index_;
        reference_count_ = other.reference_count_;
        next_ = std::move(other.next_);
        
        other.data_size_ = 0;
        other.reference_count_ = 0;
    }
    return *this;
}

void PacketBuffer::clear() {
    data_size_ = 0;
    std::fill(buffer_.begin(), buffer_.end(), 0);
}

void PacketBuffer::reset() {
    clear();
    flags_ = Flags::NONE;
    timestamp_ = 0;
    interface_index_ = 0;
    next_.reset();
}

bool PacketBuffer::resize(size_t new_size) {
    if (new_size > buffer_.capacity()) {
        buffer_.resize(new_size);
    }
    if (new_size <= buffer_.capacity()) {
        data_size_ = new_size;
        return true;
    }
    return false;
}

void PacketBuffer::reserve(size_t capacity) {
    buffer_.reserve(capacity);
}

uint8_t* PacketBuffer::data() {
    return buffer_.data();
}

const uint8_t* PacketBuffer::data() const {
    return buffer_.data();
}

uint8_t* PacketBuffer::dataAt(size_t offset) {
    if (!isValidOffset(offset)) {
        return nullptr;
    }
    return buffer_.data() + offset;
}

const uint8_t* PacketBuffer::dataAt(size_t offset) const {
    if (!isValidOffset(offset)) {
        return nullptr;
    }
    return buffer_.data() + offset;
}

size_t PacketBuffer::size() const {
    return data_size_;
}

size_t PacketBuffer::capacity() const {
    return buffer_.capacity();
}

bool PacketBuffer::empty() const {
    return data_size_ == 0;
}

void PacketBuffer::setSize(size_t size) {
    if (size <= buffer_.capacity()) {
        data_size_ = size;
    }
}

bool PacketBuffer::append(const uint8_t* data, size_t length) {
    if (!data || length == 0) {
        return false;
    }
    
    if (data_size_ + length > buffer_.capacity()) {
        buffer_.resize(data_size_ + length);
    }
    
    std::memcpy(buffer_.data() + data_size_, data, length);
    data_size_ += length;
    return true;
}

bool PacketBuffer::prepend(const uint8_t* data, size_t length) {
    if (!data || length == 0) {
        return false;
    }
    
    if (data_size_ + length > buffer_.capacity()) {
        buffer_.resize(data_size_ + length);
    }
    
    moveData(0, length, data_size_);
    std::memcpy(buffer_.data(), data, length);
    data_size_ += length;
    return true;
}

bool PacketBuffer::insert(size_t offset, const uint8_t* data, size_t length) {
    if (!data || length == 0 || !isValidOffset(offset)) {
        return false;
    }
    
    if (data_size_ + length > buffer_.capacity()) {
        buffer_.resize(data_size_ + length);
    }
    
    moveData(offset, offset + length, data_size_ - offset);
    std::memcpy(buffer_.data() + offset, data, length);
    data_size_ += length;
    return true;
}

bool PacketBuffer::remove(size_t offset, size_t length) {
    if (!isValidRange(offset, length)) {
        return false;
    }
    
    if (offset + length < data_size_) {
        moveData(offset + length, offset, data_size_ - (offset + length));
    }
    data_size_ -= length;
    return true;
}

void PacketBuffer::setNext(std::unique_ptr<PacketBuffer> next) {
    next_ = std::move(next);
}

PacketBuffer* PacketBuffer::getNext() const {
    return next_.get();
}

std::unique_ptr<PacketBuffer> PacketBuffer::takeNext() {
    return std::move(next_);
}

PacketBuffer::Type PacketBuffer::getType() const {
    return type_;
}

void PacketBuffer::setType(Type type) {
    type_ = type;
}

PacketBuffer::Flags PacketBuffer::getFlags() const {
    return flags_;
}

void PacketBuffer::setFlags(Flags flags) {
    flags_ = flags;
}

void PacketBuffer::addFlags(Flags flags) {
    flags_ |= flags;
}

void PacketBuffer::removeFlags(Flags flags) {
    flags_ &= ~flags;
}

bool PacketBuffer::hasFlags(Flags flags) const {
    return (flags_ & flags) == flags;
}

uint64_t PacketBuffer::getTimestamp() const {
    return timestamp_;
}

void PacketBuffer::setTimestamp(uint64_t timestamp) {
    timestamp_ = timestamp;
}

uint32_t PacketBuffer::getInterfaceIndex() const {
    return interface_index_;
}

void PacketBuffer::setInterfaceIndex(uint32_t index) {
    interface_index_ = index;
}

uint32_t PacketBuffer::getReferenceCount() const {
    return reference_count_;
}

void PacketBuffer::incrementReferenceCount() {
    ++reference_count_;
}

void PacketBuffer::decrementReferenceCount() {
    if (reference_count_ > 0) {
        --reference_count_;
    }
}

bool PacketBuffer::isValidOffset(size_t offset) const {
    return offset <= data_size_;
}

bool PacketBuffer::isValidRange(size_t offset, size_t length) const {
    return offset <= data_size_ && length <= data_size_ && offset + length <= data_size_;
}

void PacketBuffer::moveData(size_t from_offset, size_t to_offset, size_t length) {
    if (from_offset != to_offset && length > 0) {
        std::memmove(buffer_.data() + to_offset, buffer_.data() + from_offset, length);
    }
}