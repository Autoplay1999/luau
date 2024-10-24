#pragma once
//#include <string.h>
//#include <malloc.h>
#include <string>
#include <memory>
#include <vector>

inline std::string scrypt(const char* s) {
    std::string o;
    for (size_t i = 0; s[i]; ++i) o += (char)(256 - (int)s[i]);
    return o;
}

#if 0
inline char* scrypt(const char* s) {
    size_t l = strlen(s);
    char* b = (char*)malloc(l);
    if (!b) throw 1;
    for (size_t i = 0; i < l; ++i) b[i] = (char)(256 - (int)s[i]);
    return b;
}
#endif

#define scrypt_def(name,s) static std::shared_ptr<std::string> name; if (!name) name = std::make_shared<std::string>(scrypt(s));