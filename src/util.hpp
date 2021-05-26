#pragma once

#include <cassert>
#include <cmath>
#include <stdint.h>

#define DEBUG_PRINT printf

template <typename T, size_t N>
struct Array
{
    Array() = default;
    Array(std::initializer_list<T> il)
    {
        assert(il.size() <= N);
        len = il.size();

        int i = 0;
        for (auto t: il)
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

struct String
{
    char *data = nullptr;
    uint16_t len = 0;

    template <size_t N>
    static String from(const char (&str)[N])
    {
        String ret;
        ret.data = (char *)&str;
        ret.len = N - 1; // remove '\0'
        return ret;
    }
};

template <size_t N>
struct AllocatedString : String
{
    AllocatedString()
    {
        data = arr;
    }

    void operator=(const AllocatedString &str2)
    {
        len = fmin(str2.len, MAX_LEN);
        memcpy(data, str2.data, len);
    }

    void operator=(const String &str2)
    {
        len = fmin(str2.len, MAX_LEN);
        memcpy(data, str2.data, len);
    }

    int append(char c)
    {
        if (len >= MAX_LEN)
            return -1;
            
        arr[len] = c;
        return len++;
    }

    int append(char *s)
    {
        int i = 0;
        while (s[i] != '\0' && len < MAX_LEN)
        {
            arr[len++] = s[i];
        }

        return len;
    }

    int append(String s)
    {
        if (len + s.len >= MAX_LEN)
            return -1;
            
        memcpy(arr + len, s.data, s.len);
        len += s.len;
        return len;
    }

    char arr[N];
    static const uint16_t MAX_LEN = N;
};

bool strcmp(String str1, String str2)
{
    return str1.len == str2.len && !strncmp(str1.data, str2.data, str1.len);
}
