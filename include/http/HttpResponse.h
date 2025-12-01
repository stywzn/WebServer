#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>    // open
#include <unistd.h>   // close
#include <sys/stat.h> // stat
#include <sys/mman.h> // mmap, munmap
#include <string>
#include "Buffer.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);
    void MakeResponse(Buffer& buff);
    char* File();
    size_t FileLen() const;
    void UnmapFile();

private:
    void AddStateLine_(Buffer& buff);
    void AddHeader_(Buffer& buff);
    void AddContent_(Buffer& buff);

    void ErrorHtml_();
    std::string GetFileType_();

    int code_;
    bool isKeepAlive_;
    std::string path_;
    std::string srcDir_; // 资源的根目录
    
    char* mmFile_;       // mmap 映射的内存指针
    struct stat mmFileStat_; // 文件状态信息
    
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE; // 后缀名 -> Content-Type
    static const std::unordered_map<int, std::string> CODE_STATUS; // 状态码 -> 描述
};

#endif // HTTP_RESPONSE_H