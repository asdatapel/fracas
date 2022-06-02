#pragma once

#include "../math.hpp"
#include "../scene/scene.hpp"

// this is all stuff to only put probes outside of geometry
// struct GridCell {
//   i32 first_element = -1;
// };
// struct Grid {
//   AABB bounds;
//   Vec3i dimensions;
// };

// static Grid grid;

// void populate_grid(Scene *scene)
// {
  // for (i32 i = 0; i < scene->entities.size; i++) {
  //   if (scene->entities.data[i].assigned &&
  //       scene->entities.data[i].value.type == EntityType::MESH) {
  //     Entity *e = &scene->entities.data[i].value;

  //     AABB box            = e->mesh->bounding_box;
  //     Transform transform = e->transform;
  //     Vec3f p0            = transform * box.min;
  //     Vec3f p1            = transform * Vec3f{box.min.x, box.min.y, box.max.z};
  //     Vec3f p2            = transform * Vec3f{box.min.x, box.max.y, box.min.z};
  //     Vec3f p3            = transform * Vec3f{box.min.x, box.max.y, box.max.z};
  //     Vec3f p4            = transform * Vec3f{box.max.x, box.min.y, box.min.z};
  //     Vec3f p5            = transform * Vec3f{box.max.x, box.min.y, box.max.z};
  //     Vec3f p6            = transform * Vec3f{box.max.x, box.max.y, box.min.z};
  //     Vec3f p7            = transform * box.max;

  //     Vec3f min;
  //     min.x = std::min(std::min(std::min(p0.x, p1.x), std::min(p2.x, p3.x)),
  //                     std::min(std::min(p4.x, p5.x), std::min(p6.x, p7.x)));
  //     min.y = std::min(std::min(std::min(p0.y, p1.y), std::min(p2.y, p3.y)),
  //                     std::min(std::min(p4.y, p5.y), std::min(p6.y, p7.y)));
  //     min.z = std::min(std::min(std::min(p0.z, p1.z), std::min(p2.z, p3.z)),
  //                     std::min(std::min(p4.z, p5.z), std::min(p6.z, p7.z)));
  //     Vec3f max;
  //     max.x = std::max(std::max(std::max(p0.x, p1.x), std::max(p2.x, p3.x)),
  //                     std::max(std::max(p4.x, p5.x), std::max(p6.x, p7.x)));
  //     max.y = std::max(std::max(std::max(p0.y, p1.y), std::max(p2.y, p3.y)),
  //                     std::max(std::max(p4.y, p5.y), std::max(p6.y, p7.y)));
  //     max.z = std::max(std::max(std::max(p0.z, p1.z), std::max(p2.z, p3.z)),
  //                     std::max(std::max(p4.z, p5.z), std::max(p6.z, p7.z)));

  //     Vec3f cell_size = Vec3f{grid.bounds.max - grid.bounds.min} /
  //                             Vec3f{(float)grid.dimensions.x, (float)grid.dimensions.y, (float)grid.dimensions.z};

  //     Vec3i grid_min;
  //     grid_min.x = (min.x - grid.bounds.min.x) / cell_size.x;
  //     grid_min.y = (min.y - grid.bounds.min.y) / cell_size.y;
  //     grid_min.z = (min.z - grid.bounds.min.z) / cell_size.z;
  //     grid_min   = clamp(grid_min, {0, 0, 0}, grid.dimensions);
  //     Vec3i grid_max;
  //     grid_max.x = (max.x - grid.bounds.min.x) / cell_size.x;
  //     grid_max.y = (max.y - grid.bounds.min.y) / cell_size.y;
  //     grid_max.z = (max.z - grid.bounds.min.z) / cell_size.z;
  //     grid_max   = clamp(grid_max, {0, 0, 0}, grid.dimensions);

  //     if (selected_entity_i == i) {
  //       append_cube(min, max);
        
  //       for (i32 x = grid_min.x; x <= grid_max.x; x++) {
  //         for (i32 y = grid_min.y; y <= grid_max.y; y++) {
  //           for (i32 z = grid_min.z; z <= grid_max.z; z++) {
  //             AABB grid_cell_bounds;
              
  //             grid_cell_bounds.min = grid.bounds.min + Vec3f{(float)x, (float)y, (float)z} * cell_size;
  //             grid_cell_bounds.max = grid_cell_bounds.min + cell_size;

  //             append_cube(grid_cell_bounds.min, grid_cell_bounds.max);
  //           }
  //         }
  //       }
  //     }
  //   }
  // }
// }

struct RadianceProbe {
  Vec3f position;
  i32 flat_index = 0;
};

struct IrradianceVolume {
  AABB bounds;
  Vec3i dimensions;

  DynamicArray<RadianceProbe> probes;

  CubemapArray cubemaps;

  i32 get_flat_index(Vec3i i) {
    return (dimensions.z * dimensions.y) * i.x + (dimensions.z) * i.y + i.z;
  }
};

IrradianceVolume create_irradiance_volume(StackAllocator *alloc)
{
  IrradianceVolume iv;
  iv.probes = DynamicArray<RadianceProbe>(alloc);
  iv.dimensions = {12, 4, 6};
  iv.bounds     = {{-20, 0, -10}, {20, 15, 10}};
  iv.cubemaps.init(iv.dimensions.x * iv.dimensions.y * iv.dimensions.z, 
    128, TextureFormat::RGB16F);

  i32 num_probes = iv.dimensions.x * iv.dimensions.y * iv.dimensions.z;
  iv.probes.resize(num_probes);

  Vec3f cell_size = Vec3f{iv.bounds.max - iv.bounds.min} /
                    Vec3f{(float)iv.dimensions.x, (float)iv.dimensions.y, (float)iv.dimensions.z};
  Vec3f min_plus_half = iv.bounds.min + cell_size / 2.f;

  for (i32 x = 0; x < iv.dimensions.x; x++) {
    for (i32 y = 0; y < iv.dimensions.y; y++) {
      for (i32 z = 0; z < iv.dimensions.z; z++) {
        RadianceProbe probe;
        probe.position = min_plus_half + Vec3f{(float)x, (float)y, (float)z} * cell_size;
        probe.flat_index = iv.get_flat_index({x, y, z});

        iv.probes.push_back(probe);
      }
    }
  }

  return iv;
}