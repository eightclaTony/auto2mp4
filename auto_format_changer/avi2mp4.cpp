#include <iostream>
#include <filesystem>
#include <string>
#include <cstdlib>
#include <unordered_set>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;

class FolderWatcher {
private:
    fs::path directory;
    std::unordered_set<std::string> processed_files;
    const std::string target_ext = ".avi";
    const int check_interval = 5; // 检测间隔(秒)

public:
    FolderWatcher(const std::string& path) : directory(path) {}

    void start() {
        std::cout << "开始监控文件夹: " << directory << std::endl;
        while (true) {
            check_new_files();
            std::this_thread::sleep_for(std::chrono::seconds(check_interval));
        }
    }

private:
    void check_new_files() {
        try {
            for (const auto& entry : fs::directory_iterator(directory)) {
                if (is_new_avi(entry)) {
                    convert_video(entry.path());
                    processed_files.insert(entry.path().filename().string());
                }
            }
        }
        catch (const fs::filesystem_error& e) {
            std::cerr << "监控错误: " << e.what() << std::endl;
        }
    }

    bool is_new_avi(const fs::directory_entry& entry) {
        return entry.is_regular_file() &&
            entry.path().extension() == target_ext &&
            processed_files.find(entry.path().filename().string()) == processed_files.end();
    }

    void convert_video(const fs::path& input_path) {
        std::string output_path = input_path.stem().string() + ".mp4";

        // 标准CPU编码命令
        std::string command =
            "ffmpeg -y -i \"" + input_path.string() + "\" "
            "-c:v libx264 -preset medium -crf 23 -c:a aac -b:a 128k \"" + output_path + "\"";

        std::cout << "开始转换: " << input_path.filename() << std::endl;

        int result = std::system(command.c_str());
        if (result == 0) {
            std::cout << "✓ 转换成功: " << output_path << "\n" << std::endl;
        }
        else {
            std::cerr << "✗ 转换失败! 错误码: " << result << std::endl;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "使用方法: " << argv[0] << " <监控目录>" << std::endl;
        return 1;
    }

    FolderWatcher watcher(argv[1]);
    watcher.start();

    return 0;
}