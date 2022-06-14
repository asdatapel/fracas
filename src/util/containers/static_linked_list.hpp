#pragma once

#include "common.hpp"

template <typename T, typename Container>
struct StaticLinkedList {
  struct Element {
    T value;
    Element *next = nullptr;
  };

  Element *head = nullptr;
  Element *tail = nullptr;
  i64 count = 0;

  T& push_back(T val, Container *c) {
    if (!head) {
      head = c->push_back({val});
      tail = head;
    } else {
      tail->next = c->push_back({val});
      tail = tail->next;
    }

    return tail->next;
  }

  T* operator[](u64 i) {
    Element *next = head;
    while(next) {
      if (i == 0) {
        return &next->value;
      }
      i--;
      next = next->next;
    }

    return nullptr;
  }
};