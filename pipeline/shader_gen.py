import math
import os
from os import path
import re

import yaml

class Shader:
  texture_count = 0
  def __init__(self):
    self.context = {}
    self.counts = {}

  def get_count(self, key):
    if key not in self.counts:
      self.counts[key] = 0
    return self.counts[key]

  def set_val(self, name, key):
    if key.startswith('$'):
      self.context[name] = self.get_count(key)
    else:
      self.context[name] = bool(key)

  def handle(self, x):
    ret = self.get_count(x)
    self.counts[x] += 1
    return str(ret)

def get_section(name):
  filename = name.split('.')[0]
  f = open('engine_resources/shader_models/' + filename + '.gl').readlines()
  sections = [(i, filename + '.' + l.split(' ')[1].strip()) 
                for i, l in enumerate(f) if l.startswith('$section')]
  
  for i, s in enumerate(sections[:-1]):
    if s[1] == name: return f[s[0] + 1:sections[i+1][0]]

  if sections[-1][1] == name: return f[sections[-1][0] + 1:]
  raise ValueError('failed to get section: ' + name)

def parse(lines, shader, parsed = {}):
  ret = ''
  for l in lines:
    if l.startswith('$include'): 
      section_name = l.split(' ')[1].strip()
      if section_name in parsed:
        ret += parsed[section_name]
      else:
        section = get_section(section_name)
        parsed_section = parse(section, shader, parsed)
        parsed[section_name] = parsed_section
        ret += parsed_section
    elif l.startswith('$set'):
      name = l.split(' ')[1].strip()
      key = l.split(' ')[2].strip()
      shader.set_val(name, key)
    else: ret += l

  ret = re.sub('\$[a-z_]*', lambda x: shader.handle(x.group()), ret)

  return ret

shader_path = 'resources/sponza/shaders/'
generated_shader_path = 'generated/resources/sponza/shaders/'

shader_files = [f for root,dirs,files in os.walk(shader_path) for f in files if f.endswith(".gl")]
for filename in shader_files:
  f = open(shader_path + filename).readlines()
  vert_i = f.index('$vert_shader\n')
  frag_i = f.index('$frag_shader\n')

  shader = Shader() 
  includes = {}
  vertex = parse(f[vert_i + 1:frag_i], shader, includes)
  fragment = parse(f[frag_i + 1:], shader, includes)

  shader_name = filename.split('.')[0]
  os.makedirs(os.path.dirname(generated_shader_path + shader_name + '/'), exist_ok=True)
  f = open(generated_shader_path + shader_name + '/vert.gl', 'w+')
  f.write(vertex)
  f.close()
  f = open(generated_shader_path + shader_name + '/frag.gl', 'w+')
  f.write(fragment)
  f.close()


  f = open(generated_shader_path + shader_name + '/config.yaml', 'w+')
  f.write(yaml.dump(shader.context))
  f.close()