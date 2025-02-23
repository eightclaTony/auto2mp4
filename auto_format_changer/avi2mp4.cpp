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

    // 辅助函数：判断路径是否在输出目录下
    bool is_under_output_dir(const fs::path& path) const {
        fs::path output_dir_abs = directory / output_dir;
        auto rel_path = path.lexically_relative(output_dir_abs);
        return !rel_path.empty() && rel_path.native()[0] != '.';
    }

public:
    FolderWatcher(const std::string& path) : directory(fs::absolute(path)) {
        load_processed_files(); // 加载历史记录
        fs::create_directories(directory / output_dir); // 确保输出目录存在
    }

    void start() {
        std::cout << "监控目录: " << directory << "\n"
            << "输出目录: " << (directory / output_dir)
            << "\n硬件加速: NVIDIA NVENC 已启用\n" << std::endl;
        while (true) {
            check_new_files();
            std::this_thread::sleep_for(std::chrono::seconds(check_interval));
        }
    }

private:
    void load_processed_files() {
        std::ifstream file(directory / state_file);
        if (file) {
            std::string line;
            while (std::getline(file, line)) {
                processed_files.insert(line);
            }
        }
    }

    void save_processed_file(const std::string& relative_path) {
        std::ofstream file(directory / state_file, std::ios::app);
        if (file) {
            file << relative_path << "\n";
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
            for (const auto& entry : fs::recursive_directory_iterator(directory)) {
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
        if (!entry.is_regular_file()) {
            return false;
        }

        // 排除输出目录中的文件
        fs::path entry_path = entry.path();
        if (is_under_output_dir(entry_path)) {
            return false;
        }

        // 检查扩展名
        fs::path ext = entry.path().extension();
        if (std::find(target_exts.begin(), target_exts.end(), ext) == target_exts.end()) {
            return false;
        }

        // 检查是否已处理（使用相对路径）
        fs::path relative_path = entry.path().lexically_relative(directory);
        std::string relative_path_str = relative_path.generic_string(); // 统一路径格式
        return !processed_files.contains(relative_path_str);
    }

    void convert_video(const fs::path& input_path) {
        if (!fs::exists(input_path)) {
            std::cerr << "文件不存在: " << input_path << std::endl;
            return;
        }
        auto start_time = std::chrono::system_clock::now();
        fs::path relative_path = input_path.lexically_relative(directory);
        std::string relative_path_str = relative_path.generic_string();
        bool success = false;

        std::cout << "开始转换文件: " << input_path.filename() << std::endl;

        try {
            fs::path output_path = directory / output_dir / relative_path;
            output_path.replace_extension(".mp4");
            fs::create_directories(output_path.parent_path());

            // 智能流映射命令
            std::string command =
                "ffmpeg -hwaccel cuda -y -i \"" + input_path.string() + "\" "
                "-map 0:v? -map 0:a? -map 0:s? "  // 智能流选择（带错误忽略）
                "-map -0:d -map -0:t "            // 排除数据流和附件流
                "-c:v h264_nvenc -preset p6 -cq 23 -pix_fmt yuv420p "
                "-c:a aac -b:a 128k "
                "-c:s mov_text "                  // 仅在有字幕时生效
                "-max_interleave_delta 0 "        // 提升兼容性
                "\"" + output_path.string() + "\" 2> \"" + (directory / output_dir / "ffmpeg_log.txt").string() + "\"";

            int result = std::system(command.c_str());
            success = (result == 0);

            if (success) {
                processed_files.insert(relative_path_str);
                save_processed_file(relative_path_str);
            }
            else {
                std::ifstream log_file(directory / output_dir / "ffmpeg_log.txt");
                if (log_file) {
                    std::string line;
                    while (std::getline(log_file, line)) {
                        if (line.find("Subtitle codec") != std::string::npos) {
                            std::cerr << "字幕编码器不支持: " << line << std::endl;
                        }
                    }
                }
            }
        }
        catch (...) {
            success = false;
        }

        auto end_time = std::chrono::system_clock::now();
        write_log(relative_path_str, success, start_time, end_time);

        std::cout << (success ? "[+]转换成功: " : "[-] 转换失败: ")
            << relative_path_str << std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "使用方法: " << argv[0] << " <监控目录>" << std::endl;
        std::cerr << "n_2mp4 .         监控当前目录 " << std::endl;
        std::cerr << "n_2mp4 folder    监控指定目录 " << std::endl;
        return 1;
    }

    FolderWatcher watcher(argv[1]);
    watcher.start();

    return 0;
}