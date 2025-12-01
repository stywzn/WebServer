#include "http/HttpRequest.h"
#include <algorithm> // for std::search

// 初始化/重置请求对象
void HttpRequest::Init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE; // 初始状态
    header_.clear();
    post_.clear();
}

// 主状态机：解析 Buffer 中的数据
bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";
    if(buff.readableBytes() <= 0) return false;

    // 只要还有数据且没解析完，就一直循环
    while(buff.readableBytes() && state_ != FINISH) {
        // --- 1. 获取当前行的数据范围 ---
        // 起点：Buffer 的读指针 (Public API)
        const char* lineStart = buff.peek();
        // 终点：Buffer 的有效数据末尾
        const char* lineEndPtr = lineStart + buff.readableBytes();
        
        // --- 2. 搜索行结束符 \r\n ---
        const char* lineEnd = std::search(lineStart, lineEndPtr, CRLF, CRLF + 2);

        std::string line;

        // 如果没找到 \r\n
        if(lineEnd == lineEndPtr) {
            // 如果是在读 Body，可能不需要 CRLF (这里简化处理)
            // 如果是在读 Header 或 RequestLine，必须凑齐一行才处理
            if(state_ != BODY) return false; 
        }

        // --- 3. 提取这一行 ---
        line = std::string(lineStart, lineEnd);
        
        // --- 4. 移动 Buffer 读指针 ---
        // 跳过当前行数据 + 跳过 \r\n (2字节)
        buff.retrieveUntil(lineEnd + 2);

        // --- 5. 状态机流转 ---
        switch(state_) {
            case REQUEST_LINE:
                if(!ParseRequestLine_(line)) return false;
                ParsePath_(); // 解析完请求行后，处理一下路径
                break;
                
            case HEADERS:
                ParseHeader_(line);
                if(buff.readableBytes() <= 0) {
                     // 如果读完了还没遇到空行，可能是 POST 请求体还在后面
                     // 这里做一个简单的边界判断，防止死循环
                     if(method_ == "POST") state_ = BODY;
                     else state_ = FINISH;
                }
                break;
                
            case BODY:
                ParseBody_(line);
                break;
                
            default:
                break;
        }
    }
    return true;
}

// 解析请求行：GET /index.html HTTP/1.1
bool HttpRequest::ParseRequestLine_(const std::string& line) {
    // 找第一个空格：方法结束
    size_t methodEnd = line.find(' ');
    if(methodEnd == std::string::npos) return false;
    method_ = line.substr(0, methodEnd);

    // 找第二个空格：路径结束
    size_t pathEnd = line.find(' ', methodEnd + 1);
    if(pathEnd == std::string::npos) return false;
    path_ = line.substr(methodEnd + 1, pathEnd - methodEnd - 1);

    // 剩下的是版本号
    version_ = line.substr(pathEnd + 1);
    
    state_ = HEADERS; // 状态变为解析头部
    return true;
}

// 解析头部：Host: localhost
void HttpRequest::ParseHeader_(const std::string& line) {
    if(line.empty()) {
        // 遇到空行，Header 结束
        if(method_ == "POST") {
            state_ = BODY;
        } else {
            state_ = FINISH;
        }
        return;
    }

    // 找冒号
    size_t colon = line.find(':');
    if(colon == std::string::npos) return;

    std::string key = line.substr(0, colon);
    std::string value = line.substr(colon + 1);
    
    // 去掉 value 前面的空格
    while(!value.empty() && value[0] == ' ') value.erase(0, 1);
    
    header_[key] = value;
}

// 解析 Body
void HttpRequest::ParseBody_(const std::string& line) {
    body_ = line;
    ParsePost_();
    state_ = FINISH;
}

// 处理路径缺省值
void HttpRequest::ParsePath_() {
    if(path_ == "/") {
        path_ = "/index.html"; 
    }
}

void HttpRequest::ParsePost_() {
    // 暂时不深入解析 Post 数据
}

std::string HttpRequest::path() const { return path_; }
std::string HttpRequest::method() const { return method_; }
std::string HttpRequest::version() const { return version_; }
std::string HttpRequest::GetPost(const std::string& key) const {
    if(post_.count(key)) return post_.at(key);
    return "";
}
std::string HttpRequest::GetPost(const char* key) const {
    if(post_.count(key)) return post_.at(key);
    return "";
}