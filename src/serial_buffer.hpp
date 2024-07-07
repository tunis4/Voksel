#pragma once

#include <vector>
#include <cstring>
#include <iterator>
#include <lz4.h>

#include "util.hpp"

class SerialBuffer {
    std::vector<u8> m_bytes;

public:
    SerialBuffer() {}
    SerialBuffer(std::istreambuf_iterator<char> iter) : m_bytes(iter, {}) {}
    SerialBuffer(const SerialBuffer &) = delete;
    SerialBuffer(const SerialBuffer &&buffer) : m_bytes(std::move(buffer.m_bytes)) {}
    SerialBuffer(const std::vector<u8> &&bytes) : m_bytes(bytes) {}

    u8& operator[](usize index) {
        return m_bytes[index];
    }

    usize size() const {
        return m_bytes.size();
    }

    u8* data() {
        return m_bytes.data();
    }

    const u8* data() const {
        return m_bytes.data();
    }

    usize reserve(usize num_bytes) {
        usize index = m_bytes.size();
        m_bytes.resize(index + num_bytes);
        return index;
    }

    void shrink(usize num_bytes) {
        m_bytes.resize(m_bytes.size() - num_bytes);
    }

    template<TriviallyCopyable T>
    void push(T data) {
        std::memcpy(&m_bytes[reserve(sizeof(T))], &data, sizeof(T));
    }

    template<TriviallyCopyable T>
    void push(const std::vector<T> data) {
        std::memcpy(&m_bytes[reserve(data.size() * sizeof(T))], &data[0], data.size() * sizeof(T));
    }

    template<TriviallyCopyable T>
    void push(T *data, usize size_bytes) {
        std::memcpy(&m_bytes[reserve(size_bytes)], data, size_bytes);
    }

    template<TriviallyCopyable T>
    void pop(T &data) {
        assert(m_bytes.size() >= sizeof(T));
        std::memcpy(&data, &m_bytes[m_bytes.size() - sizeof(T)], sizeof(T));
        shrink(sizeof(T));
    }

    template<TriviallyCopyable T>
    void pop(std::vector<T> &data, usize size) {
        assert(m_bytes.size() >= sizeof(T));
        data.resize(size);
        std::memcpy(&data[0], &m_bytes[m_bytes.size() - size * sizeof(T)], size * sizeof(T));
        shrink(size * sizeof(T));
    }

    template<TriviallyCopyable T>
    void pop(T *data, usize size_bytes) {
        assert(m_bytes.size() >= sizeof(T));
        std::memcpy(data, &m_bytes[m_bytes.size() - size_bytes], size_bytes);
        shrink(size_bytes);
    }

    friend std::ostream& operator<<(std::ostream &stream, const SerialBuffer &buffer) {
        std::ostreambuf_iterator<char> output_iterator(stream);
        std::copy(buffer.m_bytes.begin(), buffer.m_bytes.end(), output_iterator);
        return stream;
    }

    std::vector<u8> compress(int acceleration = 1) {
        int original_size = m_bytes.size();
        std::vector<u8> compressed(LZ4_compressBound(original_size));
        int compressed_size = LZ4_compress_fast((const char*)m_bytes.data(), (char*)compressed.data(), original_size, compressed.size(), acceleration);
        assert(compressed_size > 0);
        compressed.resize(compressed_size);
        return compressed;
    }

    static SerialBuffer decompress(const std::vector<u8> &compressed, usize original_size) {
        SerialBuffer decompressed {};
        decompressed.reserve(original_size);
        int decompressed_size = LZ4_decompress_safe((const char*)compressed.data(), (char*)decompressed.data(), compressed.size(), original_size);
        assert(decompressed_size == (int)original_size);
        return decompressed;
    }
};
