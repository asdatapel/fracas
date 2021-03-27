import os
import sys

all_uniforms = {}
shader_files = ["\\".join([root, f]) for root,dirs,files in os.walk("resources/shaders") for f in files if f.endswith(".gl")]
shader_files += ["\\".join([root, f]) for root,dirs,files in os.walk("./shaders") for f in files if f.endswith(".lib")]
for filename in shader_files:
    file = open(filename, 'r')
    split_lines = [line.split() for line in file if line.startswith("uniform")]
    uniform_names = [l[-1][:-1] for l in split_lines if not l[1].startswith("sampler")]
    sampler_names = [l[-1][:-1] for l in split_lines if l[1].startswith("sampler")]

    for u in uniform_names:
        all_uniforms[u] = False
    for u in sampler_names:
        all_uniforms[u] = True

enum_list = ""
definition_list = ""
for k in all_uniforms:
    enum_list += k.upper() + ",\n"
for k in all_uniforms:
    definition_list += "{" + '"' + k + '"' + "," + str(all_uniforms[k]).lower() + "},\n"

out = f"""
#pragma once

enum struct UniformId
{{
    {enum_list}

    INVALID,
}};
const static int UNIFORM_COUNT = (int)UniformId::INVALID;

struct UniformDefinition
{{
    char name[50];
    bool is_texture;
}};
constexpr static UniformDefinition UNIFORM_DEFINITIONS[UNIFORM_COUNT] = {{
    {definition_list}
}};
"""

outfile = open("generated/uniforms.hpp", 'w+')
outfile.write(out)