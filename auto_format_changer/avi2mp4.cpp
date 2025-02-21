#include <iostream>
#include <filesystem>
#include <string>
#include <cstdlib>
#include <unordered_set>
#include <chrono>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

class FolderWatcher {
private:
    fs::path directory;
    std::unordered_set<std::string> processed_files;
    const std::vector<std::string> target_exts = { ".avi", ".mkv" };
    const int check_interval = 60; // 检测间隔(秒)
    const fs::path output_dir = "2mp4 output"; // 固定输出目录名

public:
    FolderWatcher(const std::string& path) : directory(path) {}

    void start() {
        std::cout << "开始监控文件夹: " << directory << "\n"
            << "输出目录: " << (directory / output_dir) << std::endl;
        while (true) {
            check_new_files();
            std::this_thread::sleep_for(std::chrono::seconds(check_interval));
        }
    }

private:
    void check_new_files() {
        try {
            for (const auto& entry : fs::directory_iterator(directory)) {
                if (is_target_file(entry)) {
                    convert_video(entry.path());
                    processed_files.insert(entry.path().filename().string());
                }
            }
        }
        catch (const fs::filesystem_error& e) {
            std::cerr << "监控错误: " << e.what() << std::endl;
        }
    }

    bool is_target_file(const fs::directory_entry& entry) {
        return entry.is_regular_file() &&
            std::find(target_exts.begin(), target_exts.end(),
                entry.path().extension()) != target_exts.end() &&
            processed_files.find(entry.path().filename().string()) == processed_files.end();
    }

    void convert_video(const fs::path& input_path) {
        // 构造输出路径
        fs::path relative_path = input_path.lexically_relative(directory);
        fs::path output_path = directory / output_dir / relative_path;
        output_path.replace_extension(".mp4");

        // 创建输出目录
        fs::create_directories(output_path.parent_path());

        // 转换命令
        std::string command =
            "ffmpeg -y -i \"" + input_path.string() + "\" "
            "-c:v libx264 -preset fast -crf 23 "
            "-c:a aac -b:a 128k \"" + output_path.string() + "\"";

        std::cout << "\n开始转换: " << input_path.filename() << std::endl;
        std::cout << "输出路径: " << output_path << std::endl;

        int result = std::system(command.c_str());
        if (result == 0) {
            std::cout << "✓ 转换成功\n" << std::endl;
        }
        else {
            std::cerr << "✗ 转换失败! 错误码: " << result << std::endl;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "使用方法: " << argv[0] << " <监控目录>" << std::endl;
        std::cerr << "2mp4 .         监控当前目录 " << std::endl;
        std::cerr << "2mp4 folder    监控指定目录 " << std::endl;
        return 1;
    }

    FolderWatcher watcher(argv[1]);
    watcher.start();

    return 0;
}