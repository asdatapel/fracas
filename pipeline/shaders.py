import os
import sys

if len(sys.argv) != 2:
    print("Wrong number of args")
    exit()
dir = sys.argv[1]

# traverse root directory, and list directories as dirs and files as files


# for _, _, files in os.walk(dir):
#     for file in files:
#         print(file)
all_uniforms = {}
shader_files = ["\\".join([root, f]) for root,dirs,files in os.walk(dir) for f in files if f.endswith(".gl")]
for filename in shader_files:
    file = open(filename, 'r')
    split_lines = [line.split() for line in file if line.startswith("uniform")]
    uniform_names = [l[-1][:-1] for l in split_lines if not l[1].startswith("sampler")]
    sampler_names = [l[-1][:-1] for l in split_lines if l[1].startswith("sampler")]

    for u in uniform_names:
        all_uniforms[u] = False
    for u in sampler_names:
        all_uniforms[u] = True


for k in all_uniforms:
    print(k.upper(),",")
for k in all_uniforms:
    print("{", '"' + k + '"', ",", str(all_uniforms[k]).lower(), "},")
    
# for k in textures:
#     print(k)
# for k in textures:
#     print(k.upper())