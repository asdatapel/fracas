import os
import yaml
from string import Template


class Vec3:
    x = 0.0
    y = 0.0
    z = 0.0

    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z


class EntityDef:
    ref = ""
    pos = Vec3(0, 0, 0)
    rot = Vec3(0, 0, 0)
    scale = Vec3(0, 0, 0)


def to_enum(v):
    return v.upper().replace('/', '_').replace('.', '_')


file = open('models/wip/set.txt', 'r')
entities = {}
for v in yaml.load(file)['entities']:
    e = EntityDef()
    name = v['name']
    e.ref = v['ref']
    e.pos = Vec3(v['position']['x'], v['position']['y'], v['position']['z'])
    e.rot = Vec3(v['rotation']['x'], v['rotation']['y'], v['rotation']['z'])
    e.scale = Vec3(v['scale']['x'], v['scale']['y'], v['scale']['z'])

    entities[name] = e

file_template = Template("""
#pragma once

#include "../src/scene/entity.hpp"

struct EntityDef
{
    MeshId mesh_id;
    MaterialId material_id;

    Vec3f position;
    Vec3f rotation;
    Vec3f scale;
};

enum struct EntityDefId
{
$entity_enums
};
const static int ENTITY_DEF_COUNT = $count;
constexpr static EntityDef EntityDefs[ENTITY_DEF_COUNT] = {
$entity_list
};
""")
def_template = Template("""\t{$mesh_id, $material_id, $pos, $rot, $scale},""")
vec3_template = Template("""{$x, $y, $z}""")


def vec3_str(vec):
    return vec3_template.substitute({
        'x': vec.x,
        'y': vec.y,
        'z': vec.z,
    })


enums = ['\t' + to_enum(name) for i, name in enumerate(entities)]
entity_list = [def_template.substitute({
    'mesh_id': 'MeshId::' + entities[name].ref,
    'material_id': 'MaterialId::' + entities[name].ref,
    'pos': vec3_str(entities[name].pos),
    'rot': vec3_str(entities[name].rot),
    'scale': vec3_str(entities[name].scale)})
    for name in entities]

out = file_template.substitute({
    'count': len(entities),
    'entity_enums': ',\n'.join(enums),
    'entity_list': '\n'.join(entity_list),
})

outfile = open("generated/scene_def.hpp", 'w+')
outfile.write(out)
