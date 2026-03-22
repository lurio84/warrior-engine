#include "file_watcher.hpp"
#include <iostream>

void FileWatcher::watch(const std::string& dir) {
    watcher_.addWatch(dir, this, false /*non-recursive*/);
    watcher_.watch();   // starts background thread
    std::cout << "[FileWatcher] Watching: " << dir << "\n";
}

void FileWatcher::poll(const std::function<void(const std::string&)>& cb) {
    std::vector<std::string> batch;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        batch.swap(pending_);
    }
    for (auto& path : batch)
        cb(path);
}

// Called from efsw's background thread
void FileWatcher::handleFileAction(efsw::WatchID,
                                   const std::string& dir,
                                   const std::string& filename,
                                   efsw::Action       action,
                                   std::string        /*old_filename*/) {
    if (action == efsw::Actions::Delete) return;

    std::lock_guard<std::mutex> lock(mutex_);
    pending_.push_back(dir + filename);
}
