#pragma once

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <cctype>

#include "platform.hpp"

struct YamlListElem;
struct YamlLiteral;
struct YamlDictElem;

enum struct YamlType
{
    KEY,
    KEY_VALUE,
};

struct YamlValue
{
    enum struct Type
    {
        LITERAL,
        LIST,
        DICT,
    };
    Type type;

    YamlValue *get(String key);
    YamlValue *at(int i);
};

struct YamlLiteral : public YamlValue
{
    String value;

    YamlLiteral() { type = Type::LITERAL; }
};
struct YamlListElem : public YamlValue
{
    YamlValue *value;
    YamlListElem *next = nullptr;

    YamlListElem() { type = Type::LIST; }
};
struct YamlDictElem : public YamlValue
{
    String key;
    YamlValue *value = nullptr;
    YamlDictElem *next = nullptr;

    YamlDictElem() { type = Type::DICT; }
};

YamlValue *YamlValue::get(String key)
{
    assert(type == Type::DICT);
    YamlDictElem *elem = (YamlDictElem *)this;
    while (elem != nullptr)
    {
        if (strcmp(elem->key, key))
        {
            return elem->value;
        }
        elem = elem->next;
    }

    assert(false);
    return nullptr;
}
YamlValue *YamlValue::at(int i)
{
    assert(type == Type::LIST);
    YamlListElem *elem = (YamlListElem *)this;
    int elem_i = 0;
    while (elem_i < i && elem != nullptr)
    {
        elem = elem->next;
        elem_i++;
    }

    assert(elem_i == i);
    return elem->value;
}

struct YamlParser
{
    struct YamlLine
    {
        int indents = 0;
        bool is_list_element = false;
        String key;
        String value;
    };

    FileData file;
    char *position;
    YamlLine line;

    YamlParser(FileData file)
    {
        this->file = file;
        position = file.data;

        next_line();
    }

    YamlDictElem *parse(StackAllocator *mem)
    {
        return push_dict(mem);
    }

    YamlLiteral *push_literal(StackAllocator *mem)
    {
        YamlLiteral *me = (YamlLiteral *)mem->alloc(sizeof(YamlLiteral));
        *me = YamlLiteral();
        me->value = line.value;
        next_line();

        return me;
    }

    YamlListElem *push_list(StackAllocator *mem)
    {
        YamlListElem *me = (YamlListElem *)mem->alloc(sizeof(YamlListElem));
        *me = YamlListElem();

        int my_indents = line.indents;

        me->value = push_dict(mem);

        if (line.indents == my_indents)
        {
            assert(line.is_list_element);
            me->next = push_list(mem);
        }

        return me;
    }

    YamlDictElem *push_dict(StackAllocator *mem)
    {
        YamlDictElem *me = (YamlDictElem *)mem->alloc(sizeof(YamlDictElem));
        *me = YamlDictElem();
        me->key = line.key;

        int my_indents = line.indents;
        if (line.value.len)
        {
            me->value = push_literal(mem);
        }
        else
        {
            next_line();
            while (line.indents > my_indents)
            {
                if (line.is_list_element)
                {
                    me->value = push_list(mem);
                }
                else
                {
                    me->value = push_dict(mem);
                }
            }
        }
        if (line.indents == my_indents && !line.is_list_element)
        {
            me->next = push_dict(mem);
        }
        return me;
    }

    bool check_pos()
    {
        return position < file.data + file.length;
    }

    int skip_spaces()
    {
        int spaces = 0;
        while (check_pos() && *position == ' ')
        {
            spaces++;
            position++;
        }

        return spaces;
    }

    void skip_newlines()
    {
        while (check_pos() && (*position == '\r' || *position == '\n'))
        {
            position++;
        }
    }

    bool is_identifier(char c)
    {
        return std::isalnum(c) || c == '_';
    }

    void next_line()
    {
        if (!check_pos())
        {
            line.indents = -1;
            return;
        }

        YamlLine next;
        next.indents = skip_spaces() / 2;

        if (check_pos() && *position == '-')
        {
            next.is_list_element = true;
            next.indents++;
            position++;
        }

        skip_spaces();

        assert(check_pos() && is_identifier(*position)); // restriction: every line must have a key
        next.key.data = position;
        while (check_pos() && is_identifier(*position))
        {
            next.key.len++;
            position++;
        }

        skip_spaces();

        assert(check_pos() && *position == ':');
        position++;
        skip_spaces();

        if (check_pos() && !std::isspace(*position))
        {
            next.value.data = position;
            while (check_pos() && !std::isspace(*position))
            {
                next.value.len++;
                position++;
            }
        }

        skip_spaces();
        skip_newlines();

        line = next;
    }
};