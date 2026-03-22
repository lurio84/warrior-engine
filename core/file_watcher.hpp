#pragma once
#include <efsw/efsw.hpp>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

// Thread-safe file watcher built on efsw.
// efsw fires callbacks from a background thread → we queue events and
// drain them on the main thread each frame via poll().

class FileWatcher : public efsw::FileWatchListener {
public:
    void watch(const std::string& dir);

    // Call once per frame from the main thread.
    // cb receives the full path of every file that changed.
    void poll(const std::function<void(const std::string&)>& cb);

private:
    void handleFileAction(efsw::WatchID,
                          const std::string& dir,
                          const std::string& filename,
                          efsw::Action       action,
                          std::string        old_filename) override;

    efsw::FileWatcher        watcher_;
    std::mutex               mutex_;
    std::vector<std::string> pending_;
};
