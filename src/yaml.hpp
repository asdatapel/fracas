#pragma once

#include "platform.hpp"
#include "util.hpp"

namespace YAML
{
    struct Literal;
    struct KeyPair;
    struct List;
    struct Dict;

    struct Value
    {
        enum struct Type
        {
            LITERAL,
            KEYPAIR,
            LIST,
            DICT,
        };
        Type type;

        String as_literal();
        KeyPair *as_keypair();
        List *as_list();
        Dict *as_dict();
    };
    struct Literal : Value
    {
        String value;
        Literal() { type = Type::LITERAL; }
        Literal(String value)
        {
            type = Type::LITERAL;
            this->value = value;
        }
    };
    struct KeyPair : Value
    {
        String key;
        Value *value = nullptr;
        KeyPair() { type = Type::KEYPAIR; }
    };
    struct List : Value
    {
        struct Elem
        {
            Value *value = nullptr;
            Elem *next = nullptr;
        };

        Elem *head = nullptr;
        Elem *tail = nullptr;
        int len = 0;

        Value *get(int i)
        {
            int elem_i = 0;
            Elem *elem = head;
            while (elem_i < i && elem != nullptr)
            {
                elem = elem->next;
                elem_i++;
            }

            assert(elem_i == i);
            return elem->value;
        }

        void push_front(Value *value, StackAllocator *alloc)
        {
            Elem *new_elem = (Elem *)alloc->alloc(sizeof(Elem));
            new (new_elem) Elem;
            new_elem->value = value;
            new_elem->next = head;
            head = new_elem;

            if (!tail)
            {
                tail = head;
            }

            len++;
        }

        void push_back(Value *value, StackAllocator *alloc)
        {
            Elem *new_elem = (Elem *)alloc->alloc(sizeof(Elem));
            new (new_elem) Elem;
            new_elem->value = value;

            if (tail)
            {
                tail->next = new_elem;
            }
            tail = new_elem;

            if (!head)
            {
                head = tail;
            }

            len++;
        }

        List() { type = Type::LIST; }
    };
    struct Dict : List
    {
        Value *get(String key)
        {
            Elem *elem = head;
            while (elem != nullptr)
            {
                KeyPair *kp = elem->value->as_keypair();
                if (strcmp(kp->key, key))
                {
                    return kp->value;
                }
                elem = elem->next;
            }

            return nullptr;
        }

        void push_front(String key, Value *value, StackAllocator *alloc)
        {
            KeyPair *new_kp = (KeyPair *)alloc->alloc(sizeof(KeyPair));
            new (new_kp) KeyPair();
            new_kp->key = key;
            new_kp->value = value;
            Elem *new_elem = (Elem *)alloc->alloc(sizeof(Elem));
            new (new_elem) Elem;
            new_elem->value = new_kp;
            new_elem->next = head;
            head = new_elem;

            if (!tail)
            {
                tail = head;
            }

            len++;
        }

        void push_back(String key, Value *value, StackAllocator *alloc)
        {
            KeyPair *new_kp = (KeyPair *)alloc->alloc(sizeof(KeyPair));
            new (new_kp) KeyPair();
            new_kp->key = key;
            new_kp->value = value;
            Elem *new_elem = (Elem *)alloc->alloc(sizeof(Elem));
            new (new_elem) Elem;
            new_elem->value = new_kp;

            if (tail)
            {
                tail->next = new_elem;
            }
            tail = new_elem;

            if (!head)
            {
                head = tail;
            }

            len++;
        }
        Dict() { type = Type::DICT; }
    };

    String Value::as_literal()
    {
        assert(type == Type::LITERAL);
        return ((Literal *)this)->value;
    }
    KeyPair *Value::as_keypair()
    {
        assert(type == Type::KEYPAIR);
        return (KeyPair *)this;
    }
    List *Value::as_list()
    {
        assert(type == Type::LIST);
        return (List *)this;
    }
    Dict *Value::as_dict()
    {
        assert(type == Type::DICT);
        return (Dict *)this;
    }

    void serialize(Value *root, StackAllocator *alloc, int indents = 0, bool should_newline = true)
    {
        auto append = [&](String str, bool newline = false)
        {
            if (newline)
            {
                char *next = alloc->alloc(1 + indents * 2);
                *next++ = '\n';
                for (int i = 0; i < indents; i++)
                {
                    *next++ = ' ';
                    *next++ = ' ';
                }
            }

            char *next = alloc->alloc(str.len);
            memcpy(next, str.data, str.len);
        };

        switch (root->type)
        {
        case Value::Type::LIST:
        {
            List *l = root->as_list();

            List::Elem *elem = l->head;
            while (elem)
            {
                append("- ", elem != l->head || should_newline);
                serialize(elem->value, alloc, indents + 1, false);

                elem = elem->next;
            }
        }
        break;
        case Value::Type::DICT:
        {
            Dict *d = root->as_dict();

            List::Elem *elem = d->head;
            while (elem)
            {
                KeyPair *kp = elem->value->as_keypair();

                append(kp->key, elem != d->head || should_newline);
                append(": ");
                serialize(kp->value, alloc, indents + 1, true);

                elem = elem->next;
            }
        }
        break;
        case Value::Type::LITERAL:
        {
            append(root->as_literal(), false);
        }
        break;
        default:
            assert(false);
        }
    }

    Value *deserialize(char **buf, char *buf_end, StackAllocator *alloc, int indents = 0, bool newline = true)
    {
        auto eat_spaces = [&]()
        {
            int i = 0;
            while (**buf == ' ' && *buf < buf_end)
            {
                i++;
                (*buf)++;
            }
            return i;
        };
        auto num_indents = [&]()
        {
            int i = 0;
            while (*(*buf + i) == ' ' && *buf < buf_end)
            {
                i++;
            }
            return i / 2;
        };
        auto eat_newline = [&]()
        {
            int i = 0;
            while ((**buf == '\n' || **buf == '\r') && *buf < buf_end)
            {
                (*buf)++;
                i++;
            }
            return i;
        };
        auto eat_token = [&]()
        {
            String token;
            token.data = *buf;
            while (**buf != ':' && **buf != '\n' && **buf != '\r' && *buf < buf_end)
            {
                token.len++;
                (*buf)++;
            }
            return token;
        };
        auto clean_token = [](String token) {
        };

        if (newline && num_indents() < indents)
        {
            Literal *lit = (Literal *)alloc->alloc(sizeof(Literal));
            new (lit) Literal("");
            return lit;
        }

        eat_spaces();
        if (**buf == '-' && !std::isdigit(*(*buf + 1)))
        {
            (*buf)++;
            eat_spaces();

            List *list = (List *)alloc->alloc(sizeof(List));
            new (list) List;

            Value *first_val = deserialize(buf, buf_end, alloc, indents + 1, false);
            list->push_back(first_val, alloc);

            while (num_indents() == indents && *buf < buf_end)
            {
                eat_spaces();
                assert(**buf == '-');
                (*buf)++;
                eat_spaces();

                list->push_back(deserialize(buf, buf_end, alloc, indents + 1, false), alloc);
            }
            return list;
        }
        else
        {
            String token = eat_token();
            clean_token(token);

            if (**buf == ':')
            {
                (*buf)++;
                eat_spaces();

                Dict *dict = (Dict *)alloc->alloc(sizeof(Dict));
                new (dict) Dict;

                String first_key = token;
                Value *first_val = deserialize(buf, buf_end, alloc, indents + 1, eat_newline());
                dict->push_back(first_key, first_val, alloc);

                while (num_indents() == indents && *buf < buf_end)
                {
                    eat_spaces();

                    String key = eat_token();
                    assert(**buf == ':');
                    (*buf)++;
                    eat_spaces();

                    Value *val = deserialize(buf, buf_end, alloc, indents + 1, eat_newline());
                    dict->push_back(key, val, alloc);
                }
                return dict;
            }
            else
            {
                eat_newline();
                Literal *lit = (Literal *)alloc->alloc(sizeof(Literal));
                new (lit) Literal(token);
                return lit;
            }
        }
    }

    Value *deserialize(String buf, StackAllocator *alloc)
    {
        return deserialize(&buf.data, buf.data + buf.len, alloc, 0);
    }

        // some utility functions
    Dict *new_dict(StackAllocator *alloc)
    {
        YAML::Dict *dict = (YAML::Dict *)alloc->alloc(sizeof(YAML::Dict));
        new (dict) YAML::Dict;
        return dict;
    };
    List *new_list(StackAllocator *alloc)
    {
        YAML::List *list = (YAML::List *)alloc->alloc(sizeof(YAML::List));
        new (list) YAML::List;
        return list;
    };
    Literal *new_literal(String val, StackAllocator *alloc)
    {
        YAML::Literal *lit = (YAML::Literal *)alloc->alloc(sizeof(YAML::Literal));
        new (lit) YAML::Literal(val);
        return lit;
    };
}
