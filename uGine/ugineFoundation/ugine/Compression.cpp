#include "Compression.h"

namespace ugine {

// https://zlib.net/zlib_how.html
class CompressorImpl {
public:
    explicit CompressorImpl(bool inflate)
        : m_inflate{ inflate } {
        m_strm.zalloc = Z_NULL;
        m_strm.zfree = Z_NULL;
        m_strm.opaque = Z_NULL;

        int ret{ m_inflate ? inflateInit(&m_strm) : deflateInit(&m_strm, m_level) };
        if (ret == Z_OK) {
            m_initialized = true;
        }
    }

    ~InflatorDeflator() {
        if (m_initialized) {
            if (m_inflate) {
                inflateEnd(&m_strm);
            } else {
                deflateEnd(&m_strm);
            }
            m_initialized = false;
        }
    }

    bool IsBad() const { return !m_initialized; }

    bool Process(const void* data, u32 dataSize, std::vector<u8>& output) {
        if (!m_initialized) {
            return false;
        }

        const unsigned int CHUNK_SIZE{ 512 };
        char chunk[CHUNK_SIZE];

        auto dataIn{ reinterpret_cast<z_const Bytef*>(const_cast<void*>(data)) };

        do {
            m_strm.avail_in = dataSize;
            m_strm.next_in = dataIn;

            m_strm.avail_out = CHUNK_SIZE;
            m_strm.next_out = reinterpret_cast<Bytef*>(chunk);

            int ret{ m_inflate ? inflate(&m_strm, Z_FINISH) : deflate(&m_strm, Z_FINISH) };
            if (ret == Z_STREAM_ERROR) {
                return false;
            }

            const auto inputRead{ dataSize - m_strm.avail_in };
            dataIn += inputRead;
            dataSize -= inputRead;

            const auto outputWritten{ CHUNK_SIZE - m_strm.avail_out };
            output.insert(output.end(), chunk, chunk + outputWritten);
        } while (m_strm.avail_out == 0);

        return true;
    }

private:
    const int m_level{ Z_BEST_COMPRESSION };
    const bool m_inflate{};
    bool m_initialized{};

    z_stream m_strm{};
};

Compressor::Compressor(bool inflate)
    : impl_{ std::make_unique<CompressorImpl>(inflate) } {
}

bool Compressor::IsBad() const {
    return impl_->IsBad();
}

bool Compressor::Process(const void* data, u32 dataSize, std::vector<u8>& output) {
    return impl_->Process(data, dataSize, output);
}

} // namespace ugine