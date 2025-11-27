#include "lob/market_data.hpp"
#include <cstring>
#include <arpa/inet.h>

namespace lob {

// Binary serialization format (simplified implementation)
// In a full implementation, this would include proper endianness handling and CRC32

namespace {

// htonll/ntohll may already be defined on some platforms (e.g., macOS)
#ifndef htonll
inline uint64_t htonll(uint64_t hostval) {
    return ((uint64_t)htonl(hostval & 0xFFFFFFFF) << 32) | htonl(hostval >> 32);
}
#endif

#ifndef ntohll
inline uint64_t ntohll(uint64_t netval) {
    return ((uint64_t)ntohl(netval & 0xFFFFFFFF) << 32) | ntohl(netval >> 32);
}
#endif

struct BinaryHeader {
    uint32_t magic;          // 'LOB1'
    uint16_t version;        // 1
    uint8_t symbol_len;
    uint8_t reserved;
    uint32_t num_bids;
    uint32_t num_asks;
    uint64_t timestamp;
    uint64_t sequence_number;
} __attribute__((packed));

struct BinaryPriceLevel {
    int64_t price;
    uint64_t quantity;
} __attribute__((packed));

} // anonymous namespace

std::vector<uint8_t> DepthSnapshot::to_binary() const {
    std::vector<uint8_t> buffer;
    
    // Calculate total size
    size_t total_size = sizeof(BinaryHeader) + symbol.size() +
                       (bids.size() + asks.size()) * sizeof(BinaryPriceLevel) + 4;  // +4 for CRC
    
    buffer.reserve(total_size);
    
    // Write header
    BinaryHeader header;
    header.magic = htonl(0x4C4F4231);  // 'LOB1'
    header.version = htons(1);
    header.symbol_len = static_cast<uint8_t>(symbol.size());
    header.reserved = 0;
    header.num_bids = htonl(static_cast<uint32_t>(bids.size()));
    header.num_asks = htonl(static_cast<uint32_t>(asks.size()));
    header.timestamp = htonll(timestamp);
    header.sequence_number = htonll(sequence_number);
    
    const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&header);
    buffer.insert(buffer.end(), header_bytes, header_bytes + sizeof(header));
    
    // Write symbol
    buffer.insert(buffer.end(), symbol.begin(), symbol.end());
    
    // Write bid levels
    for (const auto& level : bids) {
        BinaryPriceLevel bin_level;
        bin_level.price = htonll(level.price);
        bin_level.quantity = htonll(level.quantity);
        
        const uint8_t* level_bytes = reinterpret_cast<const uint8_t*>(&bin_level);
        buffer.insert(buffer.end(), level_bytes, level_bytes + sizeof(bin_level));
    }
    
    // Write ask levels
    for (const auto& level : asks) {
        BinaryPriceLevel bin_level;
        bin_level.price = htonll(level.price);
        bin_level.quantity = htonll(level.quantity);
        
        const uint8_t* level_bytes = reinterpret_cast<const uint8_t*>(&bin_level);
        buffer.insert(buffer.end(), level_bytes, level_bytes + sizeof(bin_level));
    }
    
    // Placeholder CRC (in production, would calculate actual CRC32)
    uint32_t crc = 0;
    buffer.push_back((crc >> 24) & 0xFF);
    buffer.push_back((crc >> 16) & 0xFF);
    buffer.push_back((crc >> 8) & 0xFF);
    buffer.push_back(crc & 0xFF);
    
    return buffer;
}

DepthSnapshot DepthSnapshot::from_binary(const std::vector<uint8_t>& data) {
    DepthSnapshot snapshot;
    
    if (data.size() < sizeof(BinaryHeader)) {
        return snapshot;  // Invalid data
    }
    
    // Read header
    const BinaryHeader* header = reinterpret_cast<const BinaryHeader*>(data.data());
    
    if (ntohl(header->magic) != 0x4C4F4231) {
        return snapshot;  // Invalid magic
    }
    
    snapshot.timestamp = ntohll(header->timestamp);
    snapshot.sequence_number = ntohll(header->sequence_number);
    
    uint32_t num_bids = ntohl(header->num_bids);
    uint32_t num_asks = ntohl(header->num_asks);
    
    // Read symbol
    size_t offset = sizeof(BinaryHeader);
    snapshot.symbol = std::string(reinterpret_cast<const char*>(data.data() + offset),
                                  header->symbol_len);
    offset += header->symbol_len;
    
    // Read bid levels
    for (uint32_t i = 0; i < num_bids; ++i) {
        if (offset + sizeof(BinaryPriceLevel) > data.size()) break;
        
        const BinaryPriceLevel* bin_level =
            reinterpret_cast<const BinaryPriceLevel*>(data.data() + offset);
        
        PriceLevel level;
        level.price = ntohll(bin_level->price);
        level.quantity = ntohll(bin_level->quantity);
        level.order_count = 0;  // Not stored in binary format
        
        snapshot.bids.push_back(level);
        offset += sizeof(BinaryPriceLevel);
    }
    
    // Read ask levels
    for (uint32_t i = 0; i < num_asks; ++i) {
        if (offset + sizeof(BinaryPriceLevel) > data.size()) break;
        
        const BinaryPriceLevel* bin_level =
            reinterpret_cast<const BinaryPriceLevel*>(data.data() + offset);
        
        PriceLevel level;
        level.price = ntohll(bin_level->price);
        level.quantity = ntohll(bin_level->quantity);
        level.order_count = 0;  // Not stored in binary format
        
        snapshot.asks.push_back(level);
        offset += sizeof(BinaryPriceLevel);
    }
    
    return snapshot;
}

} // namespace lob

