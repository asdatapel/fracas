import os
import sys

import pathlib

dir = "shaders"

sections = {}
lib_filenames = ["\\".join([root, f]) for root,dirs,files in os.walk(dir) for f in files if f.endswith(".lib")]
for filename in lib_filenames:
    file = open(filename, 'r').readlines()
    section_starts = [(i, line.split()[1]) for i, line in enumerate(file) if line.startswith("#section")] + [(len(file), "invalid")]
    for i, (start_line, name) in enumerate(section_starts[:-1]): 
        if not sections.get(name):
            sections[name] = ""

        end_line = section_starts[i + 1][0]
        lines = [line for line in file[start_line + 1:end_line]]
        sections[name] += ''.join(lines)

shader_filenames = [(root, f) for root,dirs,files in os.walk(dir) for f in files if f.endswith(".gl")]

for filename in shader_filenames:
    file = open(filename[0] + '/' + filename[1], 'r').readlines()
    
    pathlib.Path("resources2/" + filename[0]).mkdir(parents=True, exist_ok=True) 
    output_file = open("resources2/" + filename[0] + '/' + filename[1], 'w+')
    output = ""

    for line in file: 
        if line.startswith("#include"):
            section = line.split()[1]
            output += sections[section]
        else:
            output += line

    output_file.write(output)