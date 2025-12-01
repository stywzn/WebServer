#include "http/HttpResponse.h"
#include <iostream>
#include <cassert> // 【修复 1】必须包含这个头文件才能用 assert
#include <cstring> // 【修复 2】必须包含这个头文件才能用 memset

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    { ".html", "text/html" },
    { ".xml", "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt", "text/plain" },
    { ".rtf", "application/rtf" },
    { ".pdf", "application/pdf" },
    { ".word", "application/nsword" },
    { ".png", "image/png" },
    { ".gif", "image/gif" },
    { ".jpg", "image/jpeg" },
    { ".jpeg", "image/jpeg" },
    { ".au", "audio/basic" },
    { ".mpeg", "video/mpeg" },
    { ".mpg", "video/mpeg" },
    { ".avi", "video/x-msvideo" },
    { ".gz", "application/x-gzip" },
    { ".tar", "application/x-tar" },
    { ".css", "text/css "},
    { ".js", "text/javascript "},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

HttpResponse::HttpResponse() {
    code_ = -1;
    path_ = srcDir_ = "";
    isKeepAlive_ = false;
    mmFile_ = nullptr; 
    // 【修复 3】消除 missing initializer 警告
    // 使用 memset 显式清零，比 {0} 更受严格编译器喜欢
    memset(&mmFileStat_, 0, sizeof(mmFileStat_));
}

HttpResponse::~HttpResponse() {
    UnmapFile();
}

void HttpResponse::Init(const std::string& srcDir, std::string& path, bool isKeepAlive, int code) {
    assert(srcDir != ""); // 现在有了 <cassert>，这行不会报错了
    if(mmFile_) { UnmapFile(); }
    code_ = code;
    isKeepAlive_ = isKeepAlive;
    path_ = path;
    srcDir_ = srcDir;
    mmFile_ = nullptr; 
    memset(&mmFileStat_, 0, sizeof(mmFileStat_)); // 【修复 3】
}

void HttpResponse::MakeResponse(Buffer& buff) {
    if(stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
        code_ = 404;
    }
    else if(!(mmFileStat_.st_mode & S_IROTH)) {
        code_ = 403;
    }
    else if(code_ == -1) { 
        code_ = 200; 
    }

    ErrorHtml_();
    AddStateLine_(buff);
    AddHeader_(buff);
    AddContent_(buff);
}

char* HttpResponse::File() {
    return mmFile_;
}

size_t HttpResponse::FileLen() const {
    return mmFileStat_.st_size;
}

void HttpResponse::ErrorHtml_() {
    if(code_ == 404) {
        path_ = "/404.html";
        stat((srcDir_ + path_).data(), &mmFileStat_);
    }
    if(code_ == 403) {
        path_ = "/403.html";
        stat((srcDir_ + path_).data(), &mmFileStat_);
    }
}

void HttpResponse::AddStateLine_(Buffer& buff) {
    std::string status;
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::AddHeader_(Buffer& buff) {
    buff.append("Connection: ");
    if(isKeepAlive_) {
        buff.append("keep-alive\r\n");
        buff.append("keep-alive: max=6, timeout=120\r\n");
    } else {
        buff.append("close\r\n");
    }
    buff.append("Content-type: " + GetFileType_() + "\r\n");
}

void HttpResponse::AddContent_(Buffer& buff) {
    int srcFd = open((srcDir_ + path_).data(), O_RDONLY);
    if(srcFd < 0) { 
        buff.append("Content-length: 0\r\n\r\n");
        return; 
    }

    // MAP_PRIVATE 建立私有映射
    int* mmRet = (int*)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    
    if(*mmRet == -1) {
        buff.append("Content-length: 0\r\n\r\n"); 
    } else {
        mmFile_ = (char*)mmRet;
        close(srcFd);
        buff.append("Content-length: " + std::to_string(mmFileStat_.st_size) + "\r\n\r\n");
    }
}

void HttpResponse::UnmapFile() {
    if(mmFile_) {
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}

std::string HttpResponse::GetFileType_() {
    std::string::size_type idx = path_.find_last_of('.');
    if(idx == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = path_.substr(idx);
    if(SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}