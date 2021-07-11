#pragma once

#include <cassert>
#include <cmath>
#include <stdint.h>

#define DEBUG_PRINT printf

struct StackAllocator
{
    char *beg, *end, *next;

    void init(uint32_t size)
    {
        beg = (char *)malloc(size);
        next = beg;
        end = beg + size;
    }

    void reset()
    {
        next = beg;
    }

    char *alloc(uint32_t size)
    {
        assert(next + size < end);

        char *ret = next;
        next += size;
        return ret;
    }

    void free(void *loc)
    {
        assert(loc >= beg && loc < end);

        if (loc < next)
        {
            next = (char*) loc;
        }
    }
};

struct Memory
{
    StackAllocator *allocator;
    StackAllocator *temp;
};


template <typename T, size_t N>
struct Array
{
    Array() = default;
    Array(std::initializer_list<T> il)
    {
        assert(il.size() <= N);
        len = il.size();

        int i = 0;
        for (auto t : il)
        {
            arr[i] = t;
            i++;
        }
    }

    T &operator[](int i)
    {
        assert(i < len);
        return arr[i];
    }

    int append(T val)
    {
        if (len >= MAX_LEN)
        {
            DEBUG_PRINT("reached max client count\n");
            return -1;
        }

        arr[len] = val;
        return len++;
    }

    void swap_delete(int i)
    {
        arr[i] = arr[len - 1];
        len--;
    }

    void shift_delete(int i)
    {
        while (i + 1 < len)
        {
            arr[i] = arr[i + 1];
            i++;
        }
        len--;
    }

    T arr[N];
    size_t len = 0;
    const static size_t MAX_LEN = N;
};


template <size_t N>
struct AllocatedString
{
    AllocatedString(){} 

    template <size_t N2>
    void operator=(const AllocatedString<N2> &str2)
    {
        len = fmin(str2.len, MAX_LEN);
        memcpy(data, str2.data, len);
    }

    template <size_t N2>
    AllocatedString(AllocatedString<N2> &str2)
    {
        len = fmin(str2.len, MAX_LEN);
        memcpy(data, str2.data, len);
    }

    // void operator=(const String &str2)
    // {
    //     data = arr;
    //     len = fmin(str2.len, MAX_LEN);
    //     memcpy(data, str2.data, len);
    //     printf("operator=(const String &str2), %.*s\n", str2.len, str2.data);
    // }

    int append(char c)
    {
        if (len >= MAX_LEN)
            return -1;

        data[len] = c;
        return len++;
    }

    int append(char *s)
    {
        int i = 0;
        while (s[i] != '\0' && len < MAX_LEN)
        {
            data[len++] = s[i];
        }

        return len;
    }

    // int append(String s)
    // {
    //     if (len + s.len >= MAX_LEN)
    //         return -1;

    //     memcpy(arr + len, s.data, s.len);
    //     len += s.len;
    //     return len;
    // }

    char data[N];
    uint16_t len = 0;
    static const uint16_t MAX_LEN = N;
};


struct String
{
    char *data = nullptr;
    uint16_t len = 0;

    String(){}

    template <size_t N>
    String(AllocatedString<N> &str2)
    {
        
        data = str2.data;
        len = str2.len;
    }

    template <size_t N>
    void operator=(const AllocatedString<N> &str2)
    {
        data = str2.data;
        len = str2.len;
    }
    
    template <size_t N>
    static String from(const char (&str)[N])
    {
        String ret;
        ret.data = (char *)&str;
        ret.len = N - 1; // remove '\0'
        return ret;
    }

    char *to_char_array(StackAllocator *allocator)
    {
        char *chars = (char *)allocator->alloc(len + 1);
        memcpy(chars, data, len);
        chars[len] = '\0';
        return chars;
    }

    String concat(String other, StackAllocator *allocator)
    {
        String s;
        s.len = len + other.len;
        s.data = (char *)allocator->alloc(s.len);
        memcpy(s.data, data, len);
        memcpy(s.data + len, other.data, other.len);
        return s;
    }

    // bool operator==(const String &o) const {
    //     return len == o.len && !strncmp(data, o.data, len);
    // }

    bool operator<(const String &o) const {
        return len < o.len || strncmp(data, o.data, len) < 0;
    }
};

bool strcmp(String str1, String str2)
{
    return str1.len == str2.len && !strncmp(str1.data, str2.data, str1.len);
}