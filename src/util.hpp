#pragma once

#include <inttypes.h>
#include <stdint.h>
#include <algorithm>
#include <cassert>
#include <cmath>

#include "common.hpp"

#define DEBUG_PRINT printf

struct StackAllocator {
  char *beg, *end, *next;

  char *last_allocation         = nullptr;
  uint32_t last_allocation_size = 0;

  void init(StackAllocator *from, uint64_t size)
  {
    beg  = (char *)from->alloc(size);
    next = beg;
    end  = beg + size;
  }

  void init(u64 size)
  {
    beg  = (char *)malloc(size);
    next = beg;
    end  = beg + size;
  }

  void reset() { next = beg; }

  char *alloc(uint32_t size)
  {
    assert(next + size < end);

    char *ret = next;
    next += size;

    last_allocation      = ret;
    last_allocation_size = size;

    return ret;
  }

  char *resize(char *ptr, uint32_t size)
  {
    if (ptr != last_allocation) return alloc(size);

    last_allocation_size = size;
    next                 = last_allocation + size;
    return ptr;
  }

  void free(void *loc)
  {
    assert(loc >= beg && loc < end);

    if (loc < next) {
      next = (char *)loc;
    }
  }
};

struct Memory {
  StackAllocator *allocator;
  StackAllocator *temp;
};

struct Temp {
  StackAllocator *allocator;
  char *data = nullptr;

  Temp(Memory mem)
  {
    allocator = mem.temp;
    data      = mem.temp->next;
  }
  Temp(StackAllocator *alloc)
  {
    allocator = alloc;
    data      = alloc->next;
  }
  ~Temp() { allocator->free(data); }

  static Temp start(Memory mem) { return Temp(mem); }
  static Temp start(StackAllocator *alloc) { return Temp(alloc); }

  operator StackAllocator *() { return allocator; }
};

template <typename T>
struct RefArray {
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
struct Array {
  Array() = default;
  Array(std::initializer_list<T> il)
  {
    assert(il.size() <= N);
    len = il.size();

    int i = 0;
    for (auto t : il) {
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
    if (len >= MAX_LEN) {
      DEBUG_PRINT("Array overfull\n");
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
    while (i + 1 < len) {
      arr[i] = arr[i + 1];
      i++;
    }
    len--;
  }

  void clear() { len = 0; }

  T arr[N];
  size_t len                  = 0;
  const static size_t MAX_LEN = N;
};

// TODO this isn't really a freelist, more just a object pool
template <typename T>
struct FreeList {
  struct Element {
    bool assigned;
    union {
      T value;
      Element *next;
    };
  };

  Element *data = nullptr;
  int size      = 0;
  Element *next = nullptr, *last = nullptr;

  void init(StackAllocator *allocator, int size)
  {
    this->size = size;
    data       = (Element *)allocator->alloc(size * sizeof(Element));
    next       = data;
    last       = next;

    for (int i = 0; i < size; i++) {
      free(data + i);
    }
  }

  int push_back(T value)
  {
    Element *current = next;
    if (!current) {
      return -1;
    }

    next = current->next;

    current->value    = value;
    current->assigned = true;

    return current - data;
  }

  T *emplace(T &value, int index)
  {
    if (next == &data[index]) {
      next = data[index].next;
    }
    data[index].value    = value;
    data[index].assigned = true;

    return &data[index].value;
  }

  void free(Element *elem)
  {
    elem->assigned = false;
    elem->next     = nullptr;
    last->next     = elem;
    last           = elem;
  }

  int index_of(T *ptr) { return ((Element *)ptr - data); }
};

template <size_t N>
struct AllocatedString {
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

  int append(char c)
  {
    if (len >= MAX_LEN) return -1;

    data[len] = c;
    return len++;
  }

  int append(const char *s)
  {
    int i = 0;
    while (s[i] != '\0' && len < MAX_LEN) {
      data[len++] = s[i++];
    }

    return len;
  }

  char data[N];
  uint16_t len                  = 0;
  static const uint16_t MAX_LEN = N;
};
// TODO: move this somewhere more appropriate
typedef AllocatedString<64> PlayerName;

struct String {
  char *data = nullptr;
  u32 len    = 0;

  String() {}

  String(char *data, u32 len)
  {
    this->data = data;
    this->len  = len;
  }

  template <size_t N>
  String(AllocatedString<N> &str2)
  {
    data = str2.data;
    len  = str2.len;
  }

  template <size_t N>
  void operator=(AllocatedString<N> &str2)
  {
    data = str2.data;
    len  = str2.len;
  }

  template <size_t N>
  String(const char (&str)[N])
  {
    data = (char *)&str;
    len  = N - 1;  // remove '\0'
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
    s.len  = len + other.len;
    s.data = (char *)allocator->alloc(s.len);
    memcpy(s.data, data, len);
    memcpy(s.data + len, other.data, other.len);
    return s;
  }

  bool operator<(const String &o) const { return len < o.len || strncmp(data, o.data, len) < 0; }

  uint64_t to_uint64()
  {
    uint64_t val = 0;
    for (int i = 0; i < len; i++) {
      if (data[i] < '0' && data[i] > '9') return val;
      val = 10 * val + (data[i] - '0');
    }
    return val;
  }

  static String copy(String other, StackAllocator *allocator)
  {
    String ret;
    ret.len  = other.len;
    ret.data = allocator->alloc(other.len);
    memcpy(ret.data, other.data, ret.len);
    return ret;
  }

  static String from(u32 i, StackAllocator *allocator)
  {
    String ret;
    ret.data = allocator->alloc(15);  // should never be more than 15 digits, right
    _itoa_s(i, ret.data, 15, 10);
    ret.len = strlen(ret.data);
    return ret;
  }

  static String from(i32 i, StackAllocator *allocator)
  {
    String ret;
    ret.data = allocator->alloc(15);  // should never be more than 15 digits, right
    _itoa_s(i, ret.data, 15, 10);
    ret.len = strlen(ret.data);
    return ret;
  }

  static String from(f32 val, StackAllocator *allocator)
  {
    String ret;
    ret.data = allocator->alloc(40);
    ret.len  = snprintf(ret.data, 40, "%f", val);
    allocator->free(ret.data + ret.len);
    return ret;
  }

  static String from(uint64_t i, StackAllocator *allocator)
  {
    String ret;
    ret.data = allocator->alloc(20);  // should never be more than 20 digits, right
    snprintf(ret.data, 20, "%" PRIu64, i);
    ret.len = strlen(ret.data);
    return ret;
  }
};

bool strcmp(String str1, String str2)
{
  return str1.len == str2.len && !strncmp(str1.data, str2.data, str1.len);
}

template <u32 N>
AllocatedString<N> string_to_allocated_string(String str)
{
  AllocatedString<N> ret;
  ret.len = std::min(str.len, N);
  memcpy(ret.data, str.data, ret.len);
  return ret;
}
template <u32 N>
AllocatedString<N> float_to_allocated_string(float val)
{
  AllocatedString<N> ret;
  ret.len = snprintf(ret.data, N, "%g", val);
  return ret;
}
template <u32 N>
AllocatedString<N> i32_to_allocated_string(i32 val)
{
  AllocatedString<N> ret;
  ret.len = snprintf(ret.data, N, "%i", val);
  return ret;
}

String filepath_concat(String str1, String str2, StackAllocator *alloc)
{
  if (str1.data[str1.len - 1] == '/') {
    str1.len--;
  }
  if (str2.data[0] == '/') {
    str2.len--;
    str2.data++;
  }

  String ret;
  ret.len  = str1.len + str2.len + 1;
  ret.data = alloc->alloc(ret.len);

  ret.data[str1.len] = '/';
  memcpy(ret.data, str1.data, str1.len);
  memcpy(ret.data + str1.len + 1, str2.data, str2.len);

  return ret;
}

struct NoVal {
};
template <typename T = b8>
struct Optional {
  b8 exists = false;
  T value;

  Optional(b8 exists, T value)
  {
    this->exists = exists;
    this->value  = value;
  }

  Optional(const NoVal &empty) { exists = false; }

  Optional(const T &value)
  {
    exists      = true;
    this->value = value;
  }

  static Optional of(T value) { return {true, value}; }

  constexpr static NoVal empty = {};
};