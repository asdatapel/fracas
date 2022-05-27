#include "scene.hpp"

#include <string>

#include "../assets.hpp"
#include "../camera.hpp"
#include "../graphics/framebuffer.hpp"
#include "../graphics/graphics.hpp"
#include "../material.hpp"
#include "../yaml.hpp"

Entity *Scene::get(int id)
{
  return entities.data[id].assigned ? &entities.data[id].value : nullptr;
}

void Scene::init(Memory mem)
{
  entities.init(mem.allocator, 1024);
  new (&saved_transforms) DynamicArray<EntityTransform, 128>(&scene_allocator);
}

void Scene::serialize(const char *filename, Assets *assets, StackAllocator *alloc)
{
  auto new_dict = [&]() {
    YAML::Dict *dict = (YAML::Dict *)alloc->alloc(sizeof(YAML::Dict));
    new (dict) YAML::Dict;
    return dict;
  };
  auto new_list = [&]() {
    YAML::List *list = (YAML::List *)alloc->alloc(sizeof(YAML::List));
    new (list) YAML::List;
    return list;
  };
  auto new_literal = [&](String val) {
    YAML::Literal *lit = (YAML::Literal *)alloc->alloc(sizeof(YAML::Literal));
    new (lit) YAML::Literal(val);
    return lit;
  };

  YAML::Dict scene_yaml;
  YAML::List entities_yaml;
  scene_yaml.push_back("entities", &entities_yaml, alloc);

  for (int i = 0; i < entities.size; i++) {
    if (entities.data[i].assigned) {
      Entity *e               = &entities.data[i].value;
      YAML::Dict *entity_yaml = new_dict();
      entity_yaml->push_back("id", new_literal(String::from(i, alloc)), alloc);
      entity_yaml->push_back("name", new_literal(e->debug_tag.name), alloc);
      entity_yaml->push_back("type", new_literal(to_string(e->type)), alloc);

      YAML::Dict *transform_yaml = new_dict();
      YAML::Dict *position_yaml  = new_dict();
      position_yaml->push_back("x", new_literal(String::from(e->transform.position.x, alloc)),
                               alloc);
      position_yaml->push_back("y", new_literal(String::from(e->transform.position.y, alloc)),
                               alloc);
      position_yaml->push_back("z", new_literal(String::from(e->transform.position.z, alloc)),
                               alloc);
      YAML::Dict *rotation_yaml = new_dict();
      rotation_yaml->push_back("x", new_literal(String::from(e->transform.rotation.x, alloc)),
                               alloc);
      rotation_yaml->push_back("y", new_literal(String::from(e->transform.rotation.y, alloc)),
                               alloc);
      rotation_yaml->push_back("z", new_literal(String::from(e->transform.rotation.z, alloc)),
                               alloc);
      YAML::Dict *scale_yaml = new_dict();
      scale_yaml->push_back("x", new_literal(String::from(e->transform.scale.x, alloc)), alloc);
      scale_yaml->push_back("y", new_literal(String::from(e->transform.scale.y, alloc)), alloc);
      scale_yaml->push_back("z", new_literal(String::from(e->transform.scale.z, alloc)), alloc);
      transform_yaml->push_back("position", position_yaml, alloc);
      transform_yaml->push_back("rotation", rotation_yaml, alloc);
      transform_yaml->push_back("scale", scale_yaml, alloc);
      entity_yaml->push_back("transform", transform_yaml, alloc);

      if (e->type == EntityType::MESH) {
        YAML::Dict *mesh_yaml = new_dict();
        mesh_yaml->push_back("mesh", new_literal(String::from(e->mesh->asset_id, alloc)),
                             alloc);
        mesh_yaml->push_back("material", new_literal(String::from(e->material->asset_id, alloc)),
                             alloc);
        mesh_yaml->push_back("shader", new_literal(String::from(e->shader->asset_id, alloc)),
                             alloc);
        entity_yaml->push_back("mesh", mesh_yaml, alloc);
      } else if (e->type == EntityType::LIGHT) {
        YAML::Dict *light_yaml = new_dict();

        YAML::Dict *color_yaml = new_dict();
        color_yaml->push_back("x", new_literal(String::from(e->spot_light.color.x, alloc)), alloc);
        color_yaml->push_back("y", new_literal(String::from(e->spot_light.color.y, alloc)), alloc);
        color_yaml->push_back("z", new_literal(String::from(e->spot_light.color.z, alloc)), alloc);
        light_yaml->push_back("color", color_yaml, alloc);

        light_yaml->push_back("inner_angle",
                              new_literal(String::from(e->spot_light.inner_angle, alloc)), alloc);
        light_yaml->push_back("outer_angle",
                              new_literal(String::from(e->spot_light.outer_angle, alloc)), alloc);

        entity_yaml->push_back("spotlight", light_yaml, alloc);
      } else if (e->type == EntityType::SPLINE) {
        YAML::List *spline_yaml = new_list();
        for (int p = 0; p < e->spline.points.len; p++) {
          YAML::Dict *point_yaml = new_dict();
          point_yaml->push_back("x", new_literal(String::from(e->spline.points[p].x, alloc)),
                                alloc);
          point_yaml->push_back("y", new_literal(String::from(e->spline.points[p].y, alloc)),
                                alloc);
          point_yaml->push_back("z", new_literal(String::from(e->spline.points[p].z, alloc)),
                                alloc);
          spline_yaml->push_back(point_yaml, alloc);
        }
        entity_yaml->push_back("spline", spline_yaml, alloc);
      }
      entities_yaml.push_back(entity_yaml, alloc);
    }
  }

  String out;
  out.data = alloc->next;
  YAML::serialize(&scene_yaml, alloc, 0, false);
  out.len = alloc->next - out.data;

  write_file(filename, out);
}

void Scene::update(float timestep)
{
  if (playing_sequence) {
    assert(current_sequence);
    apply_keyed_animation(current_sequence, sequence_t);
    sequence_t += timestep;
  }
}

void Scene::set_sequence(KeyedAnimation *seq)
{
  if (current_sequence) {
    for (int i = 0; i < saved_transforms.count; i++) {
      EntityId id        = saved_transforms[i].id;
      get(id)->transform = saved_transforms[i].transform;
    }
    saved_transforms.count = 0;
  }

  if (seq) {
    for (int i = 0; i < seq->tracks.count; i++) {
      EntityId id = seq->tracks[i].entity_id;
      saved_transforms.push_back({id, get(id)->transform});
    }
  }

  current_sequence = seq;
}
void Scene::play_sequence() { playing_sequence = true; }
void Scene::stop_sequence() { playing_sequence = false; }
bool Scene::is_sequence_finished() { return get_frame() > current_sequence->end_frame; }
void Scene::set_t(f32 t) { sequence_t = t; }
void Scene::set_frame(u32 frame)
{
  assert(current_sequence);
  sequence_t = (f32)frame / current_sequence->fps;
}
u32 Scene::get_frame()
{
  if (!current_sequence) return 0;

  return sequence_t * current_sequence->fps;
}
void Scene::apply_keyed_animation(KeyedAnimation *keyed_anim, f32 t)
{
  for (u32 i = 0; i < keyed_anim->tracks.count; i++) {
    KeyedAnimationTrack &track = keyed_anim->tracks[i];
    Entity *entity             = get(track.entity_id);

    entity->transform = track.eval(t, keyed_anim->fps);
  }
}
void Scene::apply_keyed_animation(KeyedAnimation *keyed_anim, i32 frame)
{
  f32 t = (f32)frame / keyed_anim->fps;
  apply_keyed_animation(keyed_anim, t);
}