#include "blob.hpp"

#include "openmesh/crypto/aead.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace openmesh::storage::detail {
namespace {

constexpr std::size_t kHeaderSize = 4 + 1 + kSaltBytes; // magic + version + salt

std::optional<Bytes> read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) {
        return std::nullopt;
    }
    const auto size = in.tellg();
    if (size < 0) {
        return std::nullopt;
    }
    Bytes data(static_cast<std::size_t>(size));
    in.seekg(0);
    if (!data.empty() && !in.read(reinterpret_cast<char*>(data.data()), size)) {
        return std::nullopt;
    }
    return data;
}

bool valid_header(const Bytes& file, const std::array<char, 4>& magic) {
    return file.size() >= kHeaderSize && std::equal(magic.begin(), magic.end(), file.begin()) &&
           file[4] == kFileVersion;
}

} // namespace

std::optional<Bytes> read_salt(const std::string& path, const std::array<char, 4>& magic) {
    auto file = read_file(path);
    if (!file || !valid_header(*file, magic)) {
        return std::nullopt;
    }
    return Bytes(file->begin() + 5, file->begin() + 5 + static_cast<std::ptrdiff_t>(kSaltBytes));
}

bool write_blob(const std::string& path, const std::array<char, 4>& magic, const Bytes& salt,
                const Bytes& key, const Bytes& plaintext) {
    if (salt.size() != kSaltBytes || key.size() != crypto::kSessionKeyBytes) {
        return false;
    }
    Bytes out;
    out.insert(out.end(), magic.begin(), magic.end());
    out.push_back(kFileVersion);
    out.insert(out.end(), salt.begin(), salt.end());

    // Bind the header as associated data, then append the sealed payload.
    const Bytes sealed = crypto::seal(key, plaintext, out);
    if (sealed.empty()) {
        return false;
    }
    out.insert(out.end(), sealed.begin(), sealed.end());

    // Atomic write: write a temp file, then rename over the target.
    const std::string tmp = path + ".tmp";
    {
        std::ofstream os(tmp, std::ios::binary | std::ios::trunc);
        if (!os || !os.write(reinterpret_cast<const char*>(out.data()),
                             static_cast<std::streamsize>(out.size()))) {
            return false;
        }
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(tmp, ec);
        return false;
    }
    return true;
}

std::optional<Bytes> read_blob(const std::string& path, const std::array<char, 4>& magic,
                               const Bytes& key) {
    auto file = read_file(path);
    if (!file || !valid_header(*file, magic)) {
        return std::nullopt;
    }
    const Bytes header(file->begin(), file->begin() + static_cast<std::ptrdiff_t>(kHeaderSize));
    const Bytes sealed(file->begin() + static_cast<std::ptrdiff_t>(kHeaderSize), file->end());
    return crypto::open(key, sealed, header);
}

} // namespace openmesh::storage::detail
