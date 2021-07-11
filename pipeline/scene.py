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

    def from_dict(dict):
        return Vec3(dict['x'], dict['y'], dict['z'])


class LightDef:
    color = Vec3(0, 0, 0)
    outer_angle = 0.0
    inner_angle = 0.0


class EntityDef:
    ref = ""
    pos = Vec3(0, 0, 0)
    rot = Vec3(0, 0, 0)
    scale = Vec3(0, 0, 0)


def to_enum(v):
    return v.upper().replace('/', '_').replace('.', '_')


file = open('resources/models/set.yaml', 'r')
entities = {}
lights = {}
for v in yaml.load(file)['entities']:
    e = EntityDef()
    name = v['name']
    e.ref = v['ref']
    e.pos = Vec3.from_dict(v['position'])
    e.rot = Vec3.from_dict(v['rotation'])
    e.scale = Vec3.from_dict(v['scale'])
    entities[name] = e

    if 'light' in v:
        l = LightDef()
        l.color = Vec3.from_dict(v['light']['color'])
        l.outer_angle = v['light']['outer_angle']
        l.inner_angle = v['light']['inner_angle']
        lights[name] = l

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
const static int ENTITY_DEF_COUNT = $entity_count;
constexpr static EntityDef EntityDefs[ENTITY_DEF_COUNT] = {
$entity_list
};

struct LightDef
{
    EntityDefId parent;
    Vec3f color;
    float outer_angle;
    float inner_angle;
};
enum struct LightDefId
{
$light_enums
};
const static int LIGHT_DEF_COUNT = $light_count;
constexpr static LightDef LightDefs[LIGHT_DEF_COUNT] = {
$light_list
};
""")
entity_def_template = Template(
    """\t{$mesh_id, $material_id, $pos, $rot, $scale},""")
light_def_template = Template(
    """\t{EntityDefId::$parent, $color, $outer_angle, $inner_angle},""")
vec3_template = Template("""{$x, $y, $z}""")


def vec3_str(vec):
    return vec3_template.substitute({
        'x': vec.x,
        'y': vec.y,
        'z': vec.z,
    })


entity_enums = ['\t' + to_enum(name) for name in entities]
entity_list = [entity_def_template.substitute({
    'mesh_id': 'MeshId::' + entities[name].ref,
    'material_id': 'MaterialId::' + entities[name].ref,
    'pos': vec3_str(entities[name].pos),
    'rot': vec3_str(entities[name].rot),
    'scale': vec3_str(entities[name].scale)})
    for name in entities]
light_enums = ['\t' + to_enum(name) for name in lights]
light_list = [light_def_template.substitute({
    'parent': to_enum(name),
    'color': vec3_str(lights[name].color),
    'outer_angle': lights[name].outer_angle,
    'inner_angle': lights[name].inner_angle})
    for name in lights]

out = file_template.substitute({
    'entity_count': len(entities),
    'entity_enums': ',\n'.join(entity_enums),
    'entity_list': '\n'.join(entity_list),
    'light_count': len(lights),
    'light_enums': ',\n'.join(light_enums),
    'light_list': '\n'.join(light_list),
})

outfile = open("generated/scene_def.hpp", 'w+')
outfile.write(out)
