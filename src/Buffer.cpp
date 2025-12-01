#include "Buffer.h"
#include <cstring>   // perror
#include <iostream>
#include <unistd.h>  // write
#include <sys/uio.h> // readv
#include <assert.h>

Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(0), writePos_(0) {}

size_t Buffer::readableBytes() const { return writePos_ - readPos_; }
size_t Buffer::writableBytes() const { return buffer_.size() - writePos_; }
size_t Buffer::prependableBytes() const { return readPos_; }

const char* Buffer::peek() const { return BeginPtr_() + readPos_; }

void Buffer::retrieve(size_t len) {
    assert(len <= readableBytes());
    readPos_ += len;
}

void Buffer::retrieveUntil(const char* end) {
    assert(peek() <= end);
    retrieve(end - peek());
}

void Buffer::retrieveAll() {
    bzero(&buffer_[0], buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

std::string Buffer::retrieveAllToStr() {
    std::string str(peek(), readableBytes());
    retrieveAll();
    return str;
}

const char* Buffer::BeginPtr_() const { return &*buffer_.begin(); }
char* Buffer::BeginPtr_() { return &*buffer_.begin(); }

void Buffer::MakeSpace_(size_t len) {
    if (writableBytes() + prependableBytes() < len) {
        buffer_.resize(writePos_ + len + 1);
    } else {
        size_t readable = readableBytes();
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
    }
}

void Buffer::append(const char* str, size_t len) {
    assert(str);
    if(writableBytes() < len) {
        MakeSpace_(len);
    }
    std::copy(str, str + len, BeginPtr_() + writePos_);
    writePos_ += len;
}

void Buffer::append(const std::string& str) {
    append(str.data(), str.length());
}

void Buffer::append(const void* data, size_t len) {
    append(static_cast<const char*>(data), len);
}

ssize_t Buffer::readFd(int fd, int* saveErrno) {
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = BeginPtr_() + writePos_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t len = readv(fd, vec, iovcnt);

    if (len < 0) {
        *saveErrno = errno;
    } else if (static_cast<size_t>(len) <= writable) {
        writePos_ += len;
    } else {
        writePos_ = buffer_.size();
        append(extrabuf, len - writable);
    }
    return len;
}

ssize_t Buffer::writeFd(int fd, int* saveErrno) {
    size_t readSize = readableBytes();
    ssize_t len = write(fd, peek(), readSize);
    if (len < 0) {
        *saveErrno = errno;
        return len;
    }
    readPos_ += len;
    return len;
}