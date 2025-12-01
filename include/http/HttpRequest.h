#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <unordered_map>
#include <regex>
#include "Buffer.h"  // 因为 Buffer.h 就在 include 下，所以直接引用

class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH
    };

    HttpRequest() { Init(); }
    ~HttpRequest() = default;

    void Init();
    bool parse(Buffer& buff);

    std::string path() const;
    std::string method() const;
    std::string version() const;
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;

private:
    bool ParseRequestLine_(const std::string& line);
    void ParseHeader_(const std::string& line);
    void ParseBody_(const std::string& line);

    void ParsePath_();
    void ParsePost_();

    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;
};

#endif // HTTP_REQUEST_H