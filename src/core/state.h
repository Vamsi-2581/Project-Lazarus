#pragma once
// Crash recovery + build fingerprinting. Both live under Lazarus/state/.
//
// Crash recovery is dumb on purpose: before we call into a mod's init we
// write its id to boot.lock, and delete the file once init returns. If the
// game dies inside that init, the file survives, and next launch we see it
// and quarantine that mod. No crash handler, no SEH magic needed for the
// "hard crash / process killed" case.

#include <filesystem>
#include <set>
#include <string>

namespace lz {

class BootGuard {
public:
    explicit BootGuard(std::filesystem::path state_dir);

    // what was in progress when the last session died, "" if it shut down fine.
    // consumes the lock file.
    std::string take_stale_lock();

    void begin(const std::string& what);
    void end();

    std::set<std::string> load_quarantine() const;
    void quarantine(const std::string& id, const std::string& reason) const;

    std::filesystem::path lock_path() const { return dir_ / "boot.lock"; }
    std::filesystem::path quarantine_path() const { return dir_ / "quarantine.txt"; }

private:
    std::filesystem::path dir_;
};

// First 16 hex chars of SHA-256(GameAssembly.dll). That file changes with
// every code patch, so it's the most honest "which build is this" signal we
// have that doesn't depend on Steam. Cached by size+mtime because hashing
// 59MB on every launch costs ~100ms for no reason.
// Returns "" when the file can't be read.
std::string build_fingerprint(const std::filesystem::path& game_assembly, const std::filesystem::path& cache_file);
std::string sha256_hex(const std::filesystem::path& file);

struct KnownBuild {
    const char* fingerprint;
    const char* description;
};
// nullptr if unknown
const KnownBuild* find_known_build(const std::string& fingerprint);

}  // namespace lz
