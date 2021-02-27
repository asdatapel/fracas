#pragma once

#include <stdio.h>

#include "platform.hpp"
#include "util.hpp"

struct Mesh
{
    float *data;
    int verts;
    uint64_t buf_size;
};

bool is_whitespace(char c) { return c == ' ' || c == '\n' || c == '\r' || c == '\0' || c == '/'; };

String parse_token(char **buf, char *end)
{
    while (*buf < end && is_whitespace(**buf))
        (*buf)++;

    String ret;
    ret.data = *buf;
    ret.len = 0;

    while (*buf < end && !is_whitespace(**buf))
    {
        ret.len++;
        (*buf)++;
    }

    return ret;
}

void skip_to_next_line(char **buf, char *end)
{
    while (*buf < end && !(**buf == '\n' || **buf == '\r'))
        (*buf)++;
    while (*buf < end && is_whitespace(**buf))
        (*buf)++;
}

Mesh load_obj()
{
    const char *filename = "resources/models/curved_x.obj";
    FileData file = read_entire_file(filename);

    int v_count = 0;
    int vt_count = 0;
    int vn_count = 0;
    int f_count = 0;

    char *buf = file.data;
    char *buf_end = file.data + file.length;
    String token;
    while ((token = parse_token(&buf, buf_end)).len)
    {
        // replace whitespace seperator will null terminator, makes it much easier to user atof() later
        *buf = '\0'; 

        if (strcmp(token, String::from("v")))
        {
            v_count++;
            skip_to_next_line(&buf, buf_end);
        }
        else if (strcmp(token, String::from("vt")))
        {
            vt_count++;
            skip_to_next_line(&buf, buf_end);
        }
        else if (strcmp(token, String::from("vn")))
        {
            vn_count++;
            skip_to_next_line(&buf, buf_end);
        }
        else if (strcmp(token, String::from("f")))
        {
            f_count++;
            skip_to_next_line(&buf, buf_end);
        }
        else
        {
            skip_to_next_line(&buf, buf_end);
        }
    }

    float *v = (float *)malloc(sizeof(float) * 3 * v_count);
    float *vt = (float *)malloc(sizeof(float) * 2 * vt_count);
    float *vn = (float *)malloc(sizeof(float) * 3 * vn_count);
    float *f = (float *)malloc(sizeof(float) * 8 * 3 * f_count);
    int v_i = 0, vt_i = 0, vn_i = 0, f_i = 0;

    buf = file.data;
    buf_end = file.data + file.length;
    while ((token = parse_token(&buf, buf_end)).len)
    {
        if (strcmp(token, String::from("v")))
        {
            String x = parse_token(&buf, buf_end);
            String y = parse_token(&buf, buf_end);
            String z = parse_token(&buf, buf_end);
            v[v_i++] = strtof(x.data, nullptr);
            v[v_i++] = strtof(y.data, nullptr);
            v[v_i++] = strtof(z.data, nullptr);
            skip_to_next_line(&buf, buf_end);
        }
        else if (strcmp(token, String::from("vt")))
        {
            String u = parse_token(&buf, buf_end);
            String v = parse_token(&buf, buf_end);
            vt[vt_i++] = strtof(u.data, nullptr);
            vt[vt_i++] = strtof(v.data, nullptr);
            skip_to_next_line(&buf, buf_end);
        }
        else if (strcmp(token, String::from("vn")))
        {
            String x = parse_token(&buf, buf_end);
            String y = parse_token(&buf, buf_end);
            String z = parse_token(&buf, buf_end);
            vn[vn_i++] = strtof(x.data, nullptr);
            vn[vn_i++] = strtof(y.data, nullptr);
            vn[vn_i++] = strtof(z.data, nullptr);
            skip_to_next_line(&buf, buf_end);
        }
        else if (strcmp(token, String::from("f")))
        {
            for (int i =0; i < 3; i++){
                int pos = (atoi(parse_token(&buf, buf_end).data) - 1) * 3;
                int uv = (atoi(parse_token(&buf, buf_end).data) - 1) * 2;
                int normal = atoi(parse_token(&buf, buf_end).data);
                f[f_i++] = v[pos];
                f[f_i++] = v[pos + 1];
                f[f_i++] = v[pos + 2];
                f[f_i++] = vt[uv];
                f[f_i++] = -vt[uv + 1];
                f[f_i++] = vn[normal];
                f[f_i++] = vn[normal + 1];
                f[f_i++] = vn[normal + 2];
            }
            skip_to_next_line(&buf, buf_end);
        }
        else
        {
            skip_to_next_line(&buf, buf_end);
        }
    }

    free(v);
    free(vn);
    free(vt);

    return {f, 3 * f_count, sizeof(float) * 8 * 3 * f_count};
}