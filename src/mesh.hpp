#pragma once

#include <stdio.h>

#include "platform.hpp"
#include "util.hpp"

struct Component
{
    int offset, size, stride;
};
struct Mesh
{
    float *data;
    int verts;
    uint64_t buf_size;

    int components_count;
    Component *components;
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

Mesh load_obj(FileData file, StackAllocator *allocator, StackAllocator *temp_allocator)
{
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

    float *v = (float *)temp_allocator->alloc(sizeof(float) * 3 * v_count);
    float *vt = (float *)temp_allocator->alloc(sizeof(float) * 2 * vt_count);
    float *vn = (float *)temp_allocator->alloc(sizeof(float) * 3 * vn_count);
    int v_i = 0, vt_i = 0, vn_i = 0;

    float *f = (float *)allocator->alloc(sizeof(float) * 8 * 3 * f_count);
    int f_i = 0;
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
            for (int i = 0; i < 3; i++)
            {
                int pos = (atoi(parse_token(&buf, buf_end).data) - 1) * 3;
                int uv = (atoi(parse_token(&buf, buf_end).data) - 1) * 2;
                int normal = (atoi(parse_token(&buf, buf_end).data) - 1) * 3;
                f[f_i++] = v[pos];
                f[f_i++] = v[pos + 1];
                f[f_i++] = v[pos + 2];
                f[f_i++] = vt[uv];
                f[f_i++] = vt[uv + 1];
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

    temp_allocator->free(v);
    temp_allocator->free(vn);
    temp_allocator->free(vt);

    Mesh mesh = {f, 3 * f_count, sizeof(float) * 8 * 3 * f_count};
    mesh.components_count = 3;
    mesh.components = (Component*)allocator->alloc(mesh.components_count * sizeof(Component));
    mesh.components[0] = {0, 3, 8};
    mesh.components[1] = {3, 2, 8};
    mesh.components[2] = {5, 3, 8};

    return mesh;
}

void free_mesh(Mesh mesh)
{
    free(mesh.data);
    free(mesh.components);
}

//TODO modify this to take in two meshes and output new mesh
Mesh load_obj_extra_uvs(FileData file1, FileData file2, StackAllocator *allocator, StackAllocator *temp_allocator)
{
    Mesh mesh1 = load_obj(file1, allocator, temp_allocator);
    Mesh mesh2 = load_obj(file2, allocator, temp_allocator);

    float *f = (float *)allocator->alloc(sizeof(float) * 10 * mesh1.verts);
    int f_i = 0;
    for (int i = 0; i < mesh1.verts; i++)
    {
        int v_i = i * 8;
        f[f_i++] = mesh1.data[v_i + 0];
        f[f_i++] = mesh1.data[v_i + 1];
        f[f_i++] = mesh1.data[v_i + 2];
        f[f_i++] = mesh1.data[v_i + 3];
        f[f_i++] = mesh1.data[v_i + 4];
        f[f_i++] = mesh1.data[v_i + 5];
        f[f_i++] = mesh1.data[v_i + 6];
        f[f_i++] = mesh1.data[v_i + 7];
        f[f_i++] = mesh2.data[v_i + 3];
        f[f_i++] = mesh2.data[v_i + 4];
    }

    allocator->free(mesh1.data);
    allocator->free(mesh2.data);
    
    Mesh mesh = {f, mesh1.verts, sizeof(float) * 10 * mesh1.verts};
    mesh.components_count = 4;
    mesh.components = (Component*)allocator->alloc(mesh.components_count * sizeof(Component));
    mesh.components[0] = {0, 3, 10};
    mesh.components[1] = {3, 2, 10};
    mesh.components[2] = {5, 3, 10};
    mesh.components[3] = {8, 2, 10};

    return mesh;
}