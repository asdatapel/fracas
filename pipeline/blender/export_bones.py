from string import Template

import bpy

bones = bpy.data.objects['Armature'].data.edit_bones
pose_bones = bpy.data.objects['Armature'].pose.bones

vec3_template = Template("""\t{$x, $y, $z}""")

rest_pos_array_template = Template("""
std::array<Vec3f, $count > bone_rest_positions = {{
    $body
}};
""")
keyframe_template = Template("""
{
std::array<Vec3f, $count > {{
    $pos_body
}},
std::array<Vec3f, $count > {{
    $rot_body
}},
}
""")
bones_array_template = Template("""
std::array<Bone, $count > bones = {{
    $body
}};
""")

rest_pos_list = [vec3_template.substitute({
    'x': b.tail.x,
    'y': b.tail.z,
    'z': -b.tail.y,
}) for b in bones]
rest_pos = rest_pos_array_template.substitute({
    'count': len(bones),
    'body': ',\n'.join(rest_pos_list),
})

keyframes_list = []
for b in pose_bones:
    pos_body_list = []
    rot_body_list = []
    
    for f in range(bpy.context.scene.frame_start, bpy.context.scene.frame_end+1, 7):
        bpy.context.scene.frame_set(f)
        pos_body_list.append(
            vec3_template.substitute({
                'x': b.tail.x,
                'y': b.tail.z,
                'z': -b.tail.y,
            }
        ))
    for f in range(bpy.context.scene.frame_start, bpy.context.scene.frame_end+1, 7):
        bpy.context.scene.frame_set(f)
        rot = b.rotation_quaternion.to_euler()
        
        rot_body_list.append(
            vec3_template.substitute({
                'x': rot[0],
                'y': rot[2],
                'z': -rot[1],
            }
        ))
        
    keyframes_list.append(
        keyframe_template.substitute({
            'count': len(pos_body_list),
            'pos_body': ',\n'.join(pos_body_list),
            'rot_body': ',\n'.join(rot_body_list),
        })
    )
    
bones_array = bones_array_template.substitute({
    'count': len(bones),
    'body': ',\n'.join(keyframes_list),
})

print(rest_pos)
print(bones_array)