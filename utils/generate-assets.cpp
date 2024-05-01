#include <filesystem>

using std::filesystem;

/** Generates single asset. Takes the src file and creates from it an inc file
 * 
 */
void generateAsset(path src, path dst) {

}

void generateAssetsIn(path src, path dst) {
    create_directories(dst);
    for (auto const & entry : directory_iterator{src}) {
        path dstPath = dst / entry.path().filename();
        if (entry.is_directory()) 
            generateAssetsIn(entry.path(), dstPath);
        else if (entry.is_regular_file()) 
            generateAsset(entry.path(), dstPath);
    }
}



/** Generates includable data from binary assets. 

    Usage:

        generate-assets SRC_DIR DEST_DIR 
 */
int main(int argc, char * argv[]) {

}