#include <algorithm> // for std::copy_n
#include <array>
#include <bit> // for std::bit_width
#include <span>

namespace calco {

struct bit_writer {
    std::span<unsigned char> buffer;
    std::size_t bit_offset{0};

    constexpr void skip(std::size_t amount) {
        bit_offset += amount;
        buffer = buffer.subspan(bit_offset / 8);
        bit_offset %= 8;
    }

    constexpr void write(bool b) {
        if (b) { buffer[0] |= 1 << bit_offset; }
        skip(1);
    }

    constexpr void write(unsigned char b) {
        buffer[0] |= b << bit_offset;
        buffer[1] = b >> (8 - bit_offset);
        buffer = buffer.subspan(1);
    }

    constexpr void write(unsigned char b, std::size_t bits) { // Beccati sto trapezio
        const std::size_t available_bits = 8 - bit_offset;
        if (bits <= available_bits) { // ENTRA???
            buffer[0] |= b << bit_offset;
            skip(bits);
        } else { // NON PUÒ ENTRARE!!!!
            write(b, available_bits);
            write(b >> available_bits, bits - available_bits);
        }
    }
};

struct bit_reader {
    std::span<const unsigned char> buffer;
    unsigned int current{0}, current_bits{0};

    constexpr int rent(unsigned int &out, int bits = 1) {
        while (current_bits < bits && buffer.size()) {
            current |= buffer[0] << current_bits;
            current_bits += 8;
            buffer = buffer.subspan(1);
        }

        out = current;
        return current_bits;
    }

    constexpr void advance(int bits) noexcept {
        current >>= bits;
        current_bits -= bits;
    }

    constexpr bool operator==(const bit_reader& rhs) const noexcept {
        return buffer.data() == rhs.buffer.data()
            && buffer.size() == rhs.buffer.size()
            && current == rhs.current
            && current_bits == rhs.current_bits;
    }
};

/**
 * translate is a symmetric transformation that moves
 * frequently-used symbols close to the letter codes.
 * 
 * This helps the algorithm to compress better. Consider
 * that this transformation follows the idea of Huffman 
 * Trees where frequent symbols are mapped close together
 * (toward 0 in that case). Since storing a translation 
 * table is too much as this algorithm must be fast, simple
 * and must not take too much space itself (in instruction
 * or constants), a small static translation is enough to 
 * improve the situation.
 */
constexpr inline char translate(char c) {
    switch (c) {
    case 32: return 127;
    case 127: return 32;

    case 34: return 126;
    case 126: return 34;
    }
    return c;
}

/**
 * measure returns the size of the compressed version of
 * the parameter `in`, in bytes.
 *
 * At runtime, it is not really useful to rely on this 
 * function, but it's better to overallocate a bit since
 * compress returns the same size after compression.
 *
 * At compile-time, on the other side, it's better to spend
 * a little more time measuring before committing to a size
 * since that's going to be the size that will be stored in
 * the executable.
 */
constexpr std::size_t measure(const std::span<const char> in) {
    std::size_t bits = 0;
    char prev = 'A';
    for (const auto c : in) {
        const unsigned char translated = translate(c);
        const unsigned char diff = translated ^ prev;
        if (std::bit_width(diff) > 5) {
            bits += 9;
        } else {
            bits += 6;
        }
        prev = translated;
    }
    return (bits + 7) / 8;
}
/**
 * compress stores the compressed version of `in` into `out`.
 * It returns the number of bytes written. It is expected for
 * out to be big enough as the function will not check by default.
 * Moreover, `out` must be zeroed before calling this function or
 * the result may be corrupted.
 */
constexpr std::size_t compress(const std::span<const char> in, const std::span<unsigned char> out) {
    bit_writer writer{out};

    char prev = 'A';
    std::size_t bits = 0;
    for (const auto c : in) {
        const unsigned char translated = translate(c);
        const unsigned char diff = translated ^ prev;
        if (std::bit_width(diff) > 5) {
            writer.write(true);
            writer.write(diff);
            bits += 9;
        } else {
            writer.write(diff << 1, 6);
            bits += 6;
        }
        prev = translated;
    }

    return (bits + 7) / 8;
}

/**
 * iter_end represent the last element in an iteration.
 */
constexpr struct iter_end_t {
    constexpr auto operator=(const auto& end) { return end.operator=(*this); }
} iter_end;

class decompress_iterator {
    bit_reader reader;
    char current;
    std::size_t count;

    constexpr void next() {
        if (count > 0) {
            unsigned int bits;
            int size = reader.rent(bits, 9);
            // This logic assumes that the input is well formatted.
            char diff;
            if (size >= 9 && bits & 1) {
                reader.advance(9);
                diff = bits >> 1;
            } else {
                reader.advance(6);
                diff = (bits >> 1) & 0x1f;
            }
            current = diff ^ current;
            count--;
        }
    }
public:
    using difference_type   = std::ptrdiff_t;
    using value_type        = char;

    constexpr inline decompress_iterator() noexcept {}
    constexpr inline decompress_iterator(std::span<const unsigned char> data, std::size_t count) 
        : reader{data}, current{'A'}, count{count}
    {
        next();
    }

    constexpr const value_type operator*() const noexcept {
        return translate(current);
    }

    constexpr decompress_iterator& operator++() {
        next();
        return *this;
    }
    constexpr decompress_iterator operator++(int) const noexcept {
        auto copy = *this;
        return copy++;
    }

    constexpr bool operator==(const decompress_iterator& rhs) const noexcept {
        return reader == rhs.reader 
            && current == rhs.current
            && count == rhs.count;
    }

    constexpr bool operator==(const iter_end_t&) const noexcept {
        return count == 0;
    }
};

struct compressed_string {
    std::span<const unsigned char> data;
    std::size_t decompressed_size;

    using iterator = decompress_iterator;

    constexpr inline std::size_t size() const noexcept { return decompressed_size; }
    constexpr inline iterator begin() const noexcept { return iterator{data, decompressed_size}; }
    constexpr inline auto end() const noexcept { return iter_end; }
};

template<std::size_t N>
struct constexpr_string {
    char data[N]{};
 
    constexpr inline constexpr_string(char const(&pp)[N]) noexcept {
        std::copy_n(pp, N, data);
    }

    constexpr inline std::size_t size() const noexcept { return N; }
    constexpr inline auto begin() const noexcept { return std::begin(data); }
    constexpr inline auto end() const noexcept { return std::end(data); }
};

template<std::size_t N>
struct constexpr_compressed_string {
    std::size_t decompressed_size;
    std::array<unsigned char, N> data;

    using iterator = decompress_iterator;

    constexpr inline std::size_t size() const noexcept { return decompressed_size; }
    constexpr inline iterator begin() const noexcept { return iterator{data, decompressed_size}; }
    constexpr inline auto end() const noexcept { return iter_end; }
};

template <constexpr_string str>
constexpr auto make_compressed_string() {
    constexpr std::size_t compressed_size = measure(str.data);
    if constexpr (compressed_size >= str.size()) {
        return str;
    } else {
        constexpr_compressed_string<compressed_size> result{
            .decompressed_size = str.size(),
        };
        compress(str.data, result.data);
        return result;
    }
}

template <constexpr_string str>
constexpr inline auto operator ""_compressed() {
    return make_compressed_string<str>();
}

} // namespace calco
