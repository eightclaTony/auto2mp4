#include <iostream>
#include <filesystem>
#include <string>
#include <cstdlib>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    // 设置默认目录为当前目录
    std::string directory = ".";

    // 通过命令行参数获取目录路径
    if (argc > 1) {
        directory = argv[1];
    }

    try {
        // 遍历目录中的所有文件（不包含子目录）
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".avi") {
                std::string input_path = entry.path().string();
                std::string output_path = entry.path().stem().string() + ".mp4";

                // 构造FFmpeg命令（处理路径中的空格）
                std::string command = "ffmpeg -i \"" + input_path + "\" "
                    "-c:v libx264 -c:a aac -y \"" + output_path + "\"";

                // 执行转换命令
                int result = std::system(command.c_str());

                if (result == 0) {
                    std::cout << "[成功] 转换完成: " << input_path << " -> " << output_path << std::endl;
                }
                else {
                    std::cerr << "[错误] 转换失败: " << input_path << " (FFmpeg错误码: " << result << ")" << std::endl;
                }
            }
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "文件系统错误: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}