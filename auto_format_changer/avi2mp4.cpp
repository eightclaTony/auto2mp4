#include <iostream>
#include <filesystem>
#include <string>
#include <cstdlib>
#include <unordered_set>
#include <chrono>
#include <thread>
#include <vector>
#include <fstream>
#include <iomanip>

namespace fs = std::filesystem;

// 跨平台安全获取本地时间
bool safe_localtime(const std::time_t* time, std::tm& tm_out) {
#if defined(_WIN32)
    return localtime_s(&tm_out, time) == 0;
#else
    return localtime_r(time, &tm_out) != nullptr;
#endif
}

class FolderWatcher {
private:
    fs::path directory;
    std::unordered_set<std::string> processed_files;
    const std::vector<std::string> target_exts = { ".avi", ".mkv" };
    const int check_interval = 5;
    const fs::path output_dir = "2mp4 output";
    const fs::path log_file = output_dir / "conversion.log";
    const fs::path state_file = output_dir / "processed_files.txt";

public:
    FolderWatcher(const std::string& path) : directory(path) {
        load_processed_files(); // 加载历史记录
        fs::create_directories(directory / output_dir); // 确保输出目录存在
    }

    void start() {
        std::cout << "监控目录: " << directory << "\n"
            << "输出目录: " << (directory / output_dir) << std::endl;
        while (true) {
            check_new_files();
            std::this_thread::sleep_for(std::chrono::seconds(check_interval));
        }
    }

private:
    void load_processed_files() {
        std::ifstream file(directory / state_file);
        if (file) {
            std::string filename;
            while (std::getline(file, filename)) {
                processed_files.insert(filename);
            }
        }
    }

    void save_processed_file(const std::string& filename) {
        std::ofstream file(directory / state_file, std::ios::app);
        if (file) {
            file << filename << "\n";
        }
    }

    void write_log(const std::string& filename, bool success,
        const std::chrono::system_clock::time_point& start_time,
        const std::chrono::system_clock::time_point& end_time) {
        std::ofstream log(directory / log_file, std::ios::app);
        if (!log) return;

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::time_t end_time_t = std::chrono::system_clock::to_time_t(end_time);

        // 安全获取本地时间
        std::tm local_time{};
        if (!safe_localtime(&end_time_t, local_time)) {
            std::cerr << "时间转换失败" << std::endl;
            return;
        }

        log << "[文件] " << filename << "\n"
            << "[状态] " << (success ? "成功" : "失败") << "\n"
            << "[结束] " << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S") << "\n"
            << "[耗时] " << duration.count() << "ms\n"
            << "--------------------------------\n";
    }

    void check_new_files() {
        try {
            for (const auto& entry : fs::directory_iterator(directory)) {
                if (is_target_file(entry)) {
                    convert_video(entry.path());
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
            !processed_files.contains(entry.path().filename().string());
    }

    void convert_video(const fs::path& input_path) {
        auto start_time = std::chrono::system_clock::now();
        std::string filename = input_path.filename().string();
        bool success = false;

        try {
            fs::path relative_path = input_path.lexically_relative(directory);
            fs::path output_path = directory / output_dir / relative_path;
            output_path.replace_extension(".mp4");
            fs::create_directories(output_path.parent_path());

            std::string command =
                "ffmpeg -y -i \"" + input_path.string() + "\" "
                "-c:v libx264 -preset fast -crf 23 "
                "-c:a aac -b:a 128k \"" + output_path.string() + "\"";

            int result = std::system(command.c_str());
            success = (result == 0);

            if (success) {
                processed_files.insert(filename);
                save_processed_file(filename); // 持久化存储
            }
        }
        catch (...) {
            success = false;
        }

        auto end_time = std::chrono::system_clock::now();
        write_log(filename, success, start_time, end_time);

        std::cout << (success ? "✓ 转换成功: " : "✗ 转换失败: ")
            << filename << std::endl;
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