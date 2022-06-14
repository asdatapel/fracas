#pragma once

#include "common.hpp"

template <typename T, i64 size>
struct StaticArray {
  const static i64 SIZE = size;
  T elements[SIZE];
  i64 count = 0;

  StaticArray() = default;
  StaticArray(std::initializer_list<T> il)
  {
    assert(il.size() <= size);
    count = il.size();

    i64 i = 0;
    for (auto t : il) {
      elements[i] = t;
      i++;
    }
  }

  T& operator[](i64 i)
  {
    assert(i < count);
    return elements[i];
  }

  int push_back(T val)
  {
    if (count >= SIZE) {
      DEBUG_PRINT("static array overfull\n");
      return -1;
    }

    elements[count] = val;
    return count++;
  }

  int insert(i64 i, T val)
  {
    if (count >= SIZE) {
      DEBUG_PRINT("static array overfull\n");
      return -1;
    }

    memcpy(&elements[i + 1], &elements[i], sizeof(T) * (count - i));
    elements[i] = val;
    count++;
    return i;
  }

  void swap_delete(int i)
  {
    elements[i] = elements[count - 1];
    count--;
  }

  void shift_delete(int i)
  {
    while (i + 1 < count) {
      elements[i] = elements[i + 1];
      i++;
    }
    count--;
  }

  void clear() { count = 0; }
};