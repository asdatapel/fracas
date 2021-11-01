#pragma once

#include <cassert>
#include <cmath>
#include <algorithm>
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
            next = (char *)loc;
        }
    }
};

struct Memory
{
    StackAllocator *allocator;
    StackAllocator *temp;
};

template <typename T>
struct RefArray
{
    RefArray() = default;

    T &operator[](int i)
    {
        assert(i < len);
        return data[i];
    }

    T *data;
    size_t len = 0;
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
    AllocatedString() {}

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

    String() {}

    String(char *data, uint16_t len)
    {
        this->data = data;
        this->len = len;
    }

    template <size_t N>
    String(AllocatedString<N> &str2)
    {
        data = str2.data;
        len = str2.len;
    }

    template <size_t N>
    void operator=(AllocatedString<N> &str2)
    {
        data = str2.data;
        len = str2.len;
    }

    template <size_t N>
    String(const char (&str)[N])
    {
        data = (char *)&str;
        len = N - 1; // remove '\0'
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

    bool operator<(const String &o) const
    {
        return len < o.len || strncmp(data, o.data, len) < 0;
    }
    
    static String from(int i, StackAllocator *allocator)
    {
        String ret;
        ret.data = allocator->alloc(15); // should never be more than 15 digits, right 
        _itoa_s(i, ret.data, 15, 10);
        ret.len = strlen(ret.data);
        return ret;
    }
    
    static String from(float val, StackAllocator *allocator)
    {
        String ret;
        ret.data = allocator->alloc(40);
        ret.len = snprintf(ret.data, 40, "%f", val);
        allocator->free(ret.data + ret.len);
        return ret;
    }

};

bool strcmp(String str1, String str2)
{
    return str1.len == str2.len && !strncmp(str1.data, str2.data, str1.len);
}

template <uint16_t N>
AllocatedString<N> string_to_allocated_string(String str)
{
    AllocatedString<N> ret;
    ret.len = std::min(str.len, N);
    memcpy(ret.data, str.data, ret.len);
    return ret;
}