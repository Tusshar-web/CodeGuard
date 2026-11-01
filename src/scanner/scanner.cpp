#include "scanner.h"
#include <filesystem>

using namespace std;

namespace fs = filesystem;

vector<string> scanDirectory(const string& path) {
    vector<string> files;

    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (entry.is_regular_file()) {
            string extension = entry.path().extension().string();

            if (extension == ".cpp" || extension == ".c" ||
                extension == ".h" || extension == ".hpp") {
                files.push_back(entry.path().string());
            }
        }
    }

    return files;
}