#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>

using namespace std;

#define BUF_SIZE 1024

// FFmpeg 转换命令
string getFFmpegCommand(const string& inputPath, const string& outputPath) {
    return "ffmpeg -i \"" + inputPath + "\" -c:v libx264 -crf 18 -c:a aac -b:a 128k \"" + outputPath + "\"";
}

// 执行外部命令
bool executeCommand(const string& command) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    string cmd = "cmd.exe /c " + command;

    if (!CreateProcess(NULL, (wchar_t*)cmd.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        cout << "CreateProcess failed!" << endl;
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}

// 监控文件夹中的 AVI 文件
void monitorFolder(const string& folderPath) {
    DWORD dwNotifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE;

    HANDLE hDirectory = CreateFile(
        (WCHAR*)folderPath.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (hDirectory == INVALID_HANDLE_VALUE) {
        cout << "CreateFile failed!" << endl;
        return;
    }

    BYTE buffer[1024];
    DWORD bytesReturned;
    int i;

    while (true) {
        if (ReadDirectoryChangesW(
            hDirectory,
            &buffer,
            BUF_SIZE,
            TRUE,
            dwNotifyFilter,
            &bytesReturned
        )) {
            do {
                PFILE_NOTIFY_INFORMATION pNotify = (PFILE_NOTIFY_INFORMATION)buffer;
                do {
                    if (pNotify->Action == FILE_ACTION_ADDED &&
                        string(pNotify->FileName).substr(pNotify->FileNameLength - 4) == ".avi") {
                        string inputFilePath = folderPath + "\\" + string(pNotify->FileName);
                        string outputFilePath = folderPath + "\\" + string(pNotify->FileName).substr(0, pNotify->FileNameLength - 4) + ".mp4";

                        string command = getFFmpegCommand(inputFilePath, outputFilePath);
                        cout << "Converting: " << inputFilePath << " to " << outputFilePath << endl;

                        if (executeCommand(command)) {
                            cout << "Conversion completed successfully!" << endl;
                        }
                        else {
                            cout << "Conversion failed!" << endl;
                        }
                    }

                    pNotify = (PFILE_NOTIFY_INFORMATION)((BYTE*)pNotify + pNotify->NextEntryOffset);
                } while (pNotify->NextEntryOffset != 0);
            } while (bytesReturned > 0);
        }
    }
}

int main() {
    string folderPath = "."; // 当前文件夹
    cout << "Monitoring folder: " << folderPath << endl;

    monitorFolder(folderPath);
    return 0;
}
