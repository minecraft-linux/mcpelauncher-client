#include "charset.h"

#include <log.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>

namespace {
std::string canonicalName(std::string name) {
    name.erase(std::remove_if(name.begin(), name.end(), [](unsigned char ch) {
                   return ch == '-' || ch == '_' || std::isspace(ch);
               }),
               name.end());
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return name;
}

void appendUtf8(std::string& output, uint32_t codePoint) {
    if(codePoint <= 0x7f) {
        output.push_back(static_cast<char>(codePoint));
    } else if(codePoint <= 0x7ff) {
        output.push_back(static_cast<char>(0xc0 | (codePoint >> 6)));
        output.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
    } else if(codePoint <= 0xffff) {
        output.push_back(static_cast<char>(0xe0 | (codePoint >> 12)));
        output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
    } else {
        output.push_back(static_cast<char>(0xf0 | (codePoint >> 18)));
        output.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
    }
}

std::string decodeLatin1(const uint8_t* bytes, size_t size) {
    std::string output;
    output.reserve(size);
    for(size_t i = 0; i < size; ++i)
        appendUtf8(output, bytes[i]);
    return output;
}

std::string decodeAscii(const uint8_t* bytes, size_t size) {
    std::string output;
    output.reserve(size);
    for(size_t i = 0; i < size; ++i)
        appendUtf8(output, bytes[i] <= 0x7f ? bytes[i] : 0xfffd);
    return output;
}

std::string decodeUtf16(const uint8_t* bytes, size_t size, bool littleEndian) {
    std::string output;
    auto readUnit = [bytes, littleEndian](size_t offset) -> uint16_t {
        if(littleEndian)
            return static_cast<uint16_t>(bytes[offset] | (bytes[offset + 1] << 8));
        return static_cast<uint16_t>((bytes[offset] << 8) | bytes[offset + 1]);
    };

    size_t offset = 0;
    while(offset + 1 < size) {
        const uint16_t first = readUnit(offset);
        offset += 2;
        if(first >= 0xd800 && first <= 0xdbff) {
            if(offset + 1 < size) {
                const uint16_t second = readUnit(offset);
                if(second >= 0xdc00 && second <= 0xdfff) {
                    offset += 2;
                    appendUtf8(output, 0x10000u +
                                           ((static_cast<uint32_t>(first) - 0xd800u) << 10) +
                                           (static_cast<uint32_t>(second) - 0xdc00u));
                    continue;
                }
            }
            appendUtf8(output, 0xfffd);
        } else if(first >= 0xdc00 && first <= 0xdfff) {
            appendUtf8(output, 0xfffd);
        } else {
            appendUtf8(output, first);
        }
    }
    if(offset < size)
        appendUtf8(output, 0xfffd);
    return output;
}
}  // namespace

CharBuffer::CharBuffer(std::string text) : text(std::move(text)) {
}

std::shared_ptr<FakeJni::JString> CharBuffer::toString() {
    return std::make_shared<FakeJni::JString>(text);
}

Charset::Charset(std::string name) : name(canonicalName(std::move(name))) {
}

std::shared_ptr<Charset> Charset::forName(std::shared_ptr<FakeJni::JString> name) {
    const std::string requested = name ? name->asStdString() : "UTF-8";
#ifndef NDEBUG
    Log::debug("Charset", "forName name=%s", requested.c_str());
#endif
    return std::make_shared<Charset>(requested);
}

std::shared_ptr<CharBuffer> Charset::decode(std::shared_ptr<jnivm::ByteBuffer> input) {
    if(!input || !input->buffer || input->capacity <= 0)
        return std::make_shared<CharBuffer>(std::string{});
    if(static_cast<uint64_t>(input->capacity) >
       static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
#ifndef NDEBUG
        Log::warn("Charset", "ByteBuffer capacity is too large: %lld",
                  static_cast<long long>(input->capacity));
#endif
        return std::make_shared<CharBuffer>(std::string{});
    }

    const auto* bytes = static_cast<const uint8_t*>(input->buffer);
    size_t size = static_cast<size_t>(input->capacity);
    std::string decoded;
    if(name == "UTF8") {
        decoded.assign(reinterpret_cast<const char*>(bytes), size);
    } else if(name == "USASCII" || name == "ASCII") {
        decoded = decodeAscii(bytes, size);
    } else if(name == "ISO88591" || name == "LATIN1") {
        decoded = decodeLatin1(bytes, size);
    } else if(name == "UTF16LE") {
        decoded = decodeUtf16(bytes, size, true);
    } else if(name == "UTF16BE") {
        decoded = decodeUtf16(bytes, size, false);
    } else if(name == "UTF16") {
        bool littleEndian = false;
        if(size >= 2 && bytes[0] == 0xff && bytes[1] == 0xfe) {
            littleEndian = true;
            bytes += 2;
            size -= 2;
        } else if(size >= 2 && bytes[0] == 0xfe && bytes[1] == 0xff) {
            bytes += 2;
            size -= 2;
        }
        decoded = decodeUtf16(bytes, size, littleEndian);
    } else {
#ifndef NDEBUG
        Log::warn("Charset", "Unsupported charset %s; preserving input bytes", name.c_str());
#endif
        decoded.assign(reinterpret_cast<const char*>(bytes), size);
    }

#ifndef NDEBUG
    Log::debug("Charset", "decode charset=%s inputBytes=%zu outputBytes=%zu",
               name.c_str(), size, decoded.size());
#endif
    return std::make_shared<CharBuffer>(std::move(decoded));
}
