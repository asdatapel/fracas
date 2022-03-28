#pragma once

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <cmath>
#include <cstdint>
#include "windows.h"

u32 to_u32(String str)
{
  assert(str.len < 16);
  char buf[20];
  memcpy(buf, str.data, str.len);
  buf[str.len] = '\0';
  return strtoul(buf, nullptr, 10);
}
i32 to_i32(String str)
{
  assert(str.len < 16);
  char buf[20];
  memcpy(buf, str.data, str.len);
  buf[str.len] = '\0';
  return strtol(buf, nullptr, 10);
}

template <typename T>
struct DynamicArray {
  T *data;
  u32 size     = 0;
  u32 capacity = 0;

  DynamicArray()
  {
    capacity = 2;
    data     = (T *)malloc(capacity * sizeof(T));
  }
  void resize(i32 new_capacity)
  {
    capacity = new_capacity;
    data     = (T *)realloc(data, capacity * sizeof(T));
  }
  void reset() { size = 0; }
  void append(T element)
  {
    if (size >= capacity) {
      resize(capacity * 2);
    }

    data[size] = element;
    size++;
  }
  T &operator[](i32 i)
  {
    assert(i < size);
    return data[i];
  }
};

template <typename T>
struct HashMap {
  struct Element {
    bool assigned = false;
    String key;
    T value;
  };
  Element *elements;
  u32 capacity = 0b00001111111111111111111111111111;

  T blank = {};

  HashMap()
  {
    elements = (Element *)calloc(capacity, sizeof(Element));
    assert(elements);
  }

  void emplace(String key, T val)
  {
    u64 h     = hash(key);
    u32 index = h % capacity;
    while (elements[index].assigned) {
      h     = hash(key, h);
      index = h % capacity;
    }

    elements[index].assigned = true;
    elements[index].key      = key;
    elements[index].value    = val;
  }

  T &operator[](String key)
  {
    u64 h     = hash(key);
    u32 index = h % capacity;
    while (elements[index].key != key) {
      if (!elements[index].assigned) return blank;
      h     = hash(key, h);
      index = h % capacity;
    }
    return elements[index].value;
  }

  // http://www.cse.yorku.ca/~oz/hash.html
  u64 hash(String str, u64 start = 5381)
  {
    for (u32 i = 0; i < str.len; i++) {
      u32 c = str.data[i];
      start = ((start << 5) + start) + c;
    }

    return start;
  }
};

String clean(String in)
{
  while (isspace(*in.data)) {
    in.data++;
  }
  while (isspace(in.data[in.len - 1])) {
    in.len--;
  }
  return in;
}

struct Ser {
  u8 *buf;
  u32 pos = 0;

  template <typename T>
  void add(T i)
  {
    T *next = (T *)(buf + pos);
    *next   = i;
    pos += sizeof(T);
  }
  template <>
  void add(String i)
  {
    add(i.len);
    memcpy(buf + pos, i.data, i.len);
    pos += i.len;
  }

  template <typename T>
  T get()
  {
    T *next = (T *)(buf + pos);
    pos += sizeof(T);
    return *next;
  }
  template <>
  String get()
  {
    String ret;
    ret.len  = get<u32>();
    ret.data = (char *)buf + pos;
    pos += ret.len;
    return ret;
  }
};

String read_file(const char *filename)
{
  auto file_handle =
      CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
  assert(file_handle != INVALID_HANDLE_VALUE);

  LARGE_INTEGER filesize;
  GetFileSizeEx(file_handle, &filesize);

  String file;
  file.len            = filesize.QuadPart;
  file.data           = (char *)malloc(file.len + 1);
  file.data[file.len] = '\0';

  DWORD read;
  ReadFile(file_handle, file.data, file.len, &read, NULL);

  CloseHandle(file_handle);

  return file;
}
void write_file(const char *filename, String data)
{
  auto file_handle = CreateFileA(filename, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
  if (file_handle == INVALID_HANDLE_VALUE) {
    printf("ERROR writing file: %s. Dumping to stdout\n", filename);
    printf("%.*s\n", data.len, data.data);
    return;
  }

  DWORD written = 0;
  if (!WriteFile(file_handle, data.data, data.len, &written, nullptr) || written != data.len) {
    printf("ERROR writing file: %s. Dumping to stdout\n", filename);
    printf("%.*s\n", data.len, data.data);
  }

  CloseHandle(file_handle);
}

b8 less_than(String a, String b)
{
  i32 cmp = strncmp(a.data, b.data, min(a.len, b.len));
  return cmp == -1 || (cmp == 0 && a.len < b.len);
}
void mergesort(DynamicArray<String> answers, DynamicArray<i32> src, DynamicArray<i32> dst,
               i32 start, i32 count)
{
  if (count == 1) {
    dst[start] = start;
    return;
  }

  i32 part1_count = count / 2;
  i32 part2_count = count - part1_count;
  mergesort(answers, dst, src, start, part1_count);
  mergesort(answers, dst, src, start + part1_count, part2_count);

  i32 part1_i = start;
  i32 part2_i = start + part1_count;
  for (i32 i = start; i < start + count; i++) {
    if (part1_i < start + part1_count &&
        (part2_i >= start + count || less_than(answers[src[part1_i]], answers[src[part2_i]]))) {
      dst[i] = src[part1_i];
      part1_i++;
    } else {
      dst[i] = src[part2_i];
      part2_i++;
    }
  }
}

DynamicArray<i32> mergesort(DynamicArray<String> answers)
{
  DynamicArray<i32> a;
  DynamicArray<i32> b;
  a.resize(answers.size);
  b.resize(answers.size);
  a.size = answers.size;
  b.size = answers.size;
  mergesort(answers, a, b, 0, answers.size);

  return b;
}

struct Answer {
  i32 index = {};
  i32 score = 0;
};
struct Question {
  String text = {};
  Array<Answer, 8> answers;
};
struct QuestionsAndAnswers {
  DynamicArray<Question> questions;
  DynamicArray<String> answers;
};
QuestionsAndAnswers read_questions()
{
  const i32 FILE_COUNT              = 10;
  const char *filenames[FILE_COUNT] = {
      "./src/server/questions_3_np.csv", "./src/server/questions_3.csv",
      "./src/server/questions_4_np.csv", "./src/server/questions_4.csv",
      "./src/server/questions_5_np.csv", "./src/server/questions_5.csv",
      "./src/server/questions_6_np.csv", "./src/server/questions_6.csv",
      "./src/server/questions_7_np.csv", "./src/server/questions_7.csv",
  };

  String files[FILE_COUNT];

  for (i32 i = 0; i < FILE_COUNT; i++) {
    files[i] = read_file(filenames[i]);
  }

  DynamicArray<Question> questions;
  DynamicArray<String> all_answers;

  for (i32 i = 0; i < FILE_COUNT; i++) {
    String file = files[i];

    b8 has_points     = (i % 2) == 1;
    i32 answers_count = (i / 2) + 3;

    u32 cursor  = 0;
    b8 new_line = false;
    auto token  = [&]() -> String {
      String t = {};
      t.data   = file.data + cursor;
      t.len    = 0;

      new_line = false;

      if (file.data[cursor] == '"') {
        cursor++;
        t.data++;
        while (file.data[cursor] != '"') {
          cursor++;
          t.len++;
        }
        cursor++;
        assert(file.data[cursor] == ',');
      } else {
        while (file.data[cursor] != ',' && file.data[cursor] != '\n' && cursor < file.len) {
          cursor++;
          t.len++;
        }
      }

      if (file.data[cursor] == '\n') {
        new_line = true;
      }
      cursor++;

      return t;
    };

    while (cursor < file.len) {
      Question question;
      question.text = token();

      while (!new_line) {
        Answer answer;

        String text = token();
        for (i32 i = 0; i < text.len; i++) {
          if (text.data[i] >= 'A' && text.data[i] <= 'Z') text.data[i] = 'a' + text.data[i] - 'A';
        }
        all_answers.append(text);
        answer.index = all_answers.size - 1;

        if (has_points)
          answer.score = to_i32(token());
        else
          answer.score = 100 / (answers_count * (answers_count + 1) / 2) *
                         (answers_count - question.answers.len);

        question.answers.append(answer);
      }

      questions.append(question);
    }
  }

  auto sorted = mergesort(all_answers);
  DynamicArray<i32> new_positions;
  new_positions.resize(sorted.size);
  new_positions.size = sorted.size;
  DynamicArray<String> deduped;

  i32 counts[256] = {};

  String prev = "";
  i32 prev_i  = -1;
  for (i32 i = 0; i < sorted.size; i++) {
    String answer = all_answers[sorted[i]];

    for (i32 t = 0; t < answer.len; t++) {
      counts[answer.data[t]]++;
    }

    if (answer != prev) {
      prev = answer;
      prev_i++;

      deduped.append(all_answers[sorted[i]]);
    }
    new_positions[sorted[i]] = prev_i;
  }

  for (i32 i = 0; i < questions.size; i++) {
    for (i32 a = 0; a < questions[i].answers.len; a++) {
      questions[i].answers[a].index = new_positions[questions[i].answers[a].index];
    }
  }

  for (i32 i = 0; i < deduped.size; i++) {
    String answer = deduped[i];
    printf("%.*s\n", answer.len, answer.data);
  }
  _flushall();

  return {questions, deduped};
}

enum struct Part {
  ADJ,
  NOUN,
  VERB,
  ADV,
};
Part to_part(String str)
{
  if (str == "(adj)") return Part::ADJ;
  if (str == "(noun)") return Part::NOUN;
  if (str == "(verb)") return Part::VERB;
  if (str == "(adv)") return Part::ADV;
  assert(false);
  return Part::ADJ;
}

enum struct Type {
  NONE,
  GENERIC,
  RELATED,
  SIMILAR,
  ANTONYM,
};
Type to_type(String str)
{
  if (str == "(generic term)") return Type::GENERIC;
  if (str == "(related term)") return Type::RELATED;
  if (str == "(similar term)") return Type::SIMILAR;
  if (str == "(antonym)") return Type::ANTONYM;
  assert(false);
  return Type::NONE;
}

struct Entry {
  String word;
  DynamicArray<String> synonyms;
};
// HashMap<Entry> parse_thesaurus()
// {
//   String file = read_file("./src/server/th_en_US_v2.dat");

//   u32 cursor  = 0;
//   b8 new_line = false;
//   auto token  = [&]() -> String {
//     String t = {};
//     t.data   = file.data + cursor;
//     t.len    = 0;

//     new_line = false;

//     while (file.data[cursor] != '|' && file.data[cursor] != '\n' && cursor < file.len) {
//       cursor++;
//       t.len++;
//     }

//     if (file.data[cursor] == '\n') {
//       new_line = true;
//     }
//     cursor++;

//     return t;
//   };

//   HashMap<Entry> entries;
//   while (cursor < file.len) {
//     Entry entry;
//     entry.word = token();

//     u32 parts_count = to_u32(token());

//     for (i32 i = 0; i < parts_count; i++) {
//       Part part = to_part(token());

//       while (!new_line) {
//         String synonym = token();
//         Type type      = Type::NONE;

//         if (synonym.data[synonym.len - 1] == ')') {
//           String type_str;
//           type_str.data = synonym.data + synonym.len - 1;
//           type_str.len  = 1;
//           synonym.len--;

//           while (type_str.data[0] != '(') {
//             type_str.data--;
//             type_str.len++;
//             synonym.len--;
//           }
//           synonym.len--;  // space

//           type = to_type(type_str);
//         }

//         if (type == Type::SIMILAR || type == Type::GENERIC || type == Type::NONE) {
//           entry.synonyms.append(synonym);
//         }
//       }
//     }

//     entries.emplace(entry.word, entry);
//   }

//   return entries;
// }