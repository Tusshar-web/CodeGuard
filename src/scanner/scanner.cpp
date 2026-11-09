#include "scanner.h"
#include <filesystem>

using namespace std;

namespace fs = filesystem;

vector<string> scanDirectory(const string& path) {
    vector<string> files;

for (auto it = fs::recursive_directory_iterator(
         path, fs::directory_options::skip_permission_denied);
     it != fs::recursive_directory_iterator();
     ++it) {

    if (it->is_directory()) {
        string dirname = it->path().filename().string();

        if (dirname == "build" ||
            dirname == ".git" ||
            dirname == ".vscode") {

            it.disable_recursion_pending();
            continue;
        }
    }

    if (it->is_regular_file()) {
        string extension = it->path().extension().string();

        if (extension == ".cpp" ||
            extension == ".cc" ||
            extension == ".cxx") {

            files.push_back(it->path().string());
        }
    }
}

    return files;
}