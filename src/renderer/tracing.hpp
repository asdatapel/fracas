#pragma once

#include <atomic>
#include <vector>

#include "../math.hpp"
#include "../scene/scene.hpp"
#include "../util.hpp"
#include "../util/timer.hpp"

struct Triangle {
  Vec3f verts[3];
  Vec3f center;
};

struct BvhNode {
  AABB bounds;

  i32 child_0 = -1;
  i32 child_1 = -1;

  i32 triangle_i_start;
  i32 triangle_count;
};

std::vector<Triangle> triangles;
Array<BvhNode, 2000000> nodes;

void set_aabb(BvhNode *node)
{
  node->bounds.min = {1e30, 1e30, 1e30};
  node->bounds.max = {-1e30, -1e30, -1e30};

  for (i32 i = node->triangle_i_start; i < node->triangle_i_start + node->triangle_count; i++) {
    Triangle tri     = triangles[i];
    node->bounds.min = min(node->bounds.min, tri.verts[0]);
    node->bounds.max = max(node->bounds.max, tri.verts[0]);
    node->bounds.min = min(node->bounds.min, tri.verts[1]);
    node->bounds.max = max(node->bounds.max, tri.verts[1]);
    node->bounds.max = max(node->bounds.max, tri.verts[2]);
    node->bounds.min = min(node->bounds.min, tri.verts[2]);
  }
}

float surface_area_heuristic(BvhNode *node, i32 axis, float partition)
{
  AABB bounds_0;
  AABB bounds_1;
  bounds_0.min = {1e30, 1e30, 1e30};
  bounds_0.max = {-1e30, -1e30, -1e30};
  bounds_1.min = {1e30, 1e30, 1e30};
  bounds_1.max = {-1e30, -1e30, -1e30};

  i32 count_0 = 0;
  i32 count_1 = 0;

  for (i32 i = node->triangle_i_start; i < node->triangle_i_start + node->triangle_count; i++) {
    Triangle tri = triangles[i];
    if (tri.center[axis] < partition) {
      bounds_0.min = min(bounds_0.min, tri.verts[0]);
      bounds_0.max = max(bounds_0.max, tri.verts[0]);
      bounds_0.min = min(bounds_0.min, tri.verts[1]);
      bounds_0.max = max(bounds_0.max, tri.verts[1]);
      bounds_0.min = min(bounds_0.min, tri.verts[2]);
      bounds_0.max = max(bounds_0.max, tri.verts[2]);
      count_0++;
    } else {
      bounds_1.min = min(bounds_1.min, tri.verts[0]);
      bounds_1.max = max(bounds_1.max, tri.verts[0]);
      bounds_1.min = min(bounds_1.min, tri.verts[1]);
      bounds_1.max = max(bounds_1.max, tri.verts[1]);
      bounds_1.min = min(bounds_1.min, tri.verts[2]);
      bounds_1.max = max(bounds_1.max, tri.verts[2]);
      count_1++;
    }
  }

  // unneccesary to multiply by 2
  Vec3f size_0      = bounds_0.max - bounds_0.min;
  float area_0_half = size_0.x * size_0.y + size_0.x * size_0.z + size_0.y * size_0.z;
  Vec3f size_1      = bounds_1.max - bounds_1.min;
  float area_1_half = size_1.x * size_1.y + size_1.x * size_1.z + size_1.y * size_1.z;

  float cost = area_0_half * count_0 + area_1_half * count_1;
  if (cost > 0) return cost;
  return 1e30;
}

void split(BvhNode *node)
{
  assert(node->triangle_count > 0);
  if (node->triangle_count < 50) return;

  Vec3f size = node->bounds.max - node->bounds.min;

  float debug_last_cost;
  float min_cost = 1e30;
  i32 min_axis   = -1;
  float min_partition;
  for (i32 axis = 0; axis < 3; axis++) {
    const i32 N_SAMPLES = 16;
    for (i32 sample = 0; sample < N_SAMPLES; sample++) {
      float partition = node->bounds.min[axis] + (sample + 1) * (size[axis] / (N_SAMPLES + 2));
      float cost      = surface_area_heuristic(node, axis, partition);
      debug_last_cost = cost;
      if (cost < min_cost) {
        min_cost      = cost;
        min_axis      = axis;
        min_partition = partition;
      }
    }
  }

  if (min_axis > -1 && nodes.len < (nodes.MAX_LEN - 2)) {
    i32 front = node->triangle_i_start;
    i32 back  = node->triangle_i_start + node->triangle_count - 1;
    while (front <= back) {
      Triangle tri = triangles[front];
      if (tri.center[min_axis] < min_partition) {
        front++;
      } else {
        triangles[front] = triangles[back];
        triangles[back]  = tri;
        back--;
      }
    }

    i32 child_0_i             = nodes.append({});
    i32 child_1_i             = nodes.append({});
    BvhNode *child_0          = &nodes[child_0_i];
    BvhNode *child_1          = &nodes[child_1_i];
    node->child_0             = child_0_i;
    node->child_1             = child_1_i;
    child_0->triangle_i_start = node->triangle_i_start;
    child_0->triangle_count   = front - node->triangle_i_start;
    set_aabb(child_0);
    child_1->triangle_i_start = node->triangle_i_start + child_0->triangle_count;
    child_1->triangle_count   = node->triangle_count - child_0->triangle_count;
    set_aabb(child_1);

    split(child_0);
    split(child_1);
  }
}

BvhNode create_bvh(Scene *scene)
{
  for (i32 i = 0; i < scene->entities.size; i++) {
    if (scene->entities.data[i].assigned &&
        scene->entities.data[i].value.type == EntityType::MESH) {
      Entity *e  = &scene->entities.data[i].value;
      Mesh *mesh = e->mesh;

      i32 tri_count = mesh->verts / 3;
      i32 vert_size = mesh->buf_size / mesh->verts / sizeof(float);
      for (i32 tri_i = 0; tri_i < tri_count; tri_i++) {
        i32 offset = vert_size * tri_i * 3;

        Triangle tri;
        tri.verts[0] = *(Vec3f *)(mesh->data + offset);
        tri.verts[1] = *(Vec3f *)(mesh->data + offset + vert_size);
        tri.verts[2] = *(Vec3f *)(mesh->data + offset + vert_size + vert_size);

        tri.verts[0] = e->transform * tri.verts[0];
        tri.verts[1] = e->transform * tri.verts[1];
        tri.verts[2] = e->transform * tri.verts[2];

        tri.center = (tri.verts[0] + tri.verts[1] + tri.verts[2]) / 3;

        triangles.push_back(tri);
      }
    }
  }

  Timer timer2;

  i32 root_i             = nodes.append({});
  BvhNode *root          = &nodes[root_i];
  root->triangle_i_start = 0;
  root->triangle_count   = triangles.size();
  set_aabb(root);

  split(root);

  printf("triangles: %llu\n", triangles.size());
  printf("nodels: %zu\n", nodes.len);

  printf("Built BVH in:");
  timer2.print_ms();

  return {};
}

float ray_tri_intersect(Vec3f ray_origin, Vec3f ray_dir, Triangle &tri)
{
  Vec3f edge_0 = tri.verts[1] - tri.verts[0];
  Vec3f edge_1 = tri.verts[2] - tri.verts[0];
  Vec3f h      = cross(ray_dir, edge_1);
  float a      = dot(edge_0, h);

  // parralel
  if (a > -0.0001f && a < 0.0001f) return -1;

  float f = 1 / a;
  Vec3f s = ray_origin - tri.verts[0];
  float u = f * dot(s, h);

  // missed triangle
  if (u < 0 || u > 1) return -1;

  Vec3f q = cross(s, edge_0);
  float v = f * dot(ray_dir, q);

  // missed triangle
  if (v < 0 || u + v > 1) return -1;

  float t = f * dot(edge_1, q);
  return t;
}

// http://psgraphics.blogspot.com/2016/02/new-simple-ray-box-test-from-andrew.html
bool ray_aabb_intersect(Vec3f ray_origin, Vec3f ray_dir, AABB &aabb)
{
  // for (i32 axis = 0; axis < 3; axis++) {
  //   float inv_d = 1.f / ray_dir[axis];
  //   float t_0 =  (aabb.min[axis] - ray_origin[axis]) * inv_d;
  //   float t_1 =  (aabb.max[axis] - ray_origin[axis]) * inv_d;

  //   if (inv_d < 0.f) {
  //     float tmp = t_0;
  //     t_0 = t_1;
  //     t_1 = tmp;
  //   }

  //   tmin
  // }

  auto max_component = [](Vec3f in) { return fmaxf(in.x, fmaxf(in.y, in.z)); };
  auto min_component = [](Vec3f in) { return fminf(in.x, fminf(in.y, in.z)); };

  Vec3f inv_dir = 1.f / ray_dir;
  Vec3f t0      = (aabb.min - ray_origin) * inv_dir;
  Vec3f t1      = (aabb.max - ray_origin) * inv_dir;
  Vec3f tmin = min(t0, t1), tmax = max(t0, t1);

  return max_component(tmin) <= min_component(tmax);
}

float traverse_bvh(Vec3f ray_origin, Vec3f ray_dir, i32 node_i = 0)
{
  BvhNode *node = &nodes[node_i];

  if (!ray_aabb_intersect(ray_origin, ray_dir, node->bounds)) return 1e30;

  float min_t = 1e30;
  if (node->child_0 == -1 && node->child_1 == -1) {
    for (i32 i = node->triangle_i_start; i < node->triangle_i_start + node->triangle_count; i++) {
      float t = ray_tri_intersect(ray_origin, ray_dir, triangles[i]);
      if (t > 0) {
        min_t = fminf(min_t, t);
      }
    }
  } else {
    min_t = fminf(min_t, traverse_bvh(ray_origin, ray_dir, node->child_0));
    min_t = fminf(min_t, traverse_bvh(ray_origin, ray_dir, node->child_1));
  }

  return min_t;
}

template <typename T, size_t SIZE>
struct ThreadSafeArray {
  T elements[SIZE];
  std::atomic<i32> count = 0;

  const static size_t MAX_COUNT = SIZE;

  i32 push_back(i32 n)
  {
    i32 p = count.fetch_add(n);
    assert(p <= MAX_COUNT - n);
    return p;
  }
  T &operator[](i32 i)
  {
    return elements[i];
  }
};
ThreadSafeArray<BvhNode, 20000000> threaded_nodes;

template <typename T, size_t SIZE>
struct ThreadSafeWorkQueue {
  std::mutex lock;
  T elements[SIZE];
  i32 head  = 0;
  i32 count = 0;

  const static size_t MAX_COUNT = SIZE;

  i32 push(T elem)
  {
    lock.lock();
    i32 pos       = (head + count) % SIZE;
    elements[pos] = elem;
    assert(count < SIZE);
    count++;
    lock.unlock();

    return pos;
  }

  b8 pop(T *elem)
  {
    b8 exists = false;

    lock.lock();
    if (count > 0) {
      i32 pos = head % SIZE;
      *elem   = elements[pos];

      head++;
      count--;
      exists = true;
    }
    lock.unlock();

    return exists;
  }
};
struct BvhConstructJob {
  i32 node_i;
};

ThreadSafeWorkQueue<BvhConstructJob, 1000000> queue;
std::atomic<i32> remaining_jobs = 0;

void push_job(i32 node_i) { queue.push({node_i}); }

void threaded_split(BvhNode *node)
{
  assert(node->triangle_count > 0);
  if (node->triangle_count < 5) {
    remaining_jobs--;
    return;
  }

  Vec3f size = node->bounds.max - node->bounds.min;

  float debug_last_cost;
  float min_cost = 1e30;
  i32 min_axis   = -1;
  float min_partition;
  for (i32 axis = 0; axis < 3; axis++) {
    const i32 N_SAMPLES = 4;
    for (i32 sample = 0; sample < N_SAMPLES; sample++) {
      float partition = node->bounds.min[axis] + (sample + 1) * (size[axis] / (N_SAMPLES + 2));
      float cost      = surface_area_heuristic(node, axis, partition);
      debug_last_cost = cost;
      if (cost < min_cost) {
        min_cost      = cost;
        min_axis      = axis;
        min_partition = partition;
      }
    }
  }

  if (min_axis > -1) {
    i32 front = node->triangle_i_start;
    i32 back  = node->triangle_i_start + node->triangle_count - 1;
    while (front <= back) {
      Triangle tri = triangles[front];
      if (tri.center[min_axis] < min_partition) {
        front++;
      } else {
        triangles[front] = triangles[back];
        triangles[back]  = tri;
        back--;
      }
    }

    i32 child_0_i             = threaded_nodes.push_back(2);
    i32 child_1_i             = child_0_i + 1;
    node->child_0             = child_0_i;
    node->child_1             = child_1_i;
    BvhNode *child_0          = &threaded_nodes[child_0_i];
    BvhNode *child_1          = &threaded_nodes[child_1_i];
    child_0->triangle_i_start = node->triangle_i_start;
    child_0->triangle_count   = front - node->triangle_i_start;
    set_aabb(child_0);
    child_1->triangle_i_start = node->triangle_i_start + child_0->triangle_count;
    child_1->triangle_count   = node->triangle_count - child_0->triangle_count;
    set_aabb(child_1);

    remaining_jobs += 2;
    push_job(child_0_i);
    push_job(child_1_i);
  }

  remaining_jobs--;
}

void bvh_thread()
{
  while (true) {
    BvhConstructJob job;
    if (queue.pop(&job)) {
      threaded_split(&threaded_nodes[job.node_i]);
    }

    if (remaining_jobs == 0) {
      return;
    }
  }
}

BvhNode create_bvh_threaded(Scene *scene)
{
  Timer timer;

  for (i32 i = 0; i < 100; i++) {
    if (scene->entities.data[i].assigned &&
        scene->entities.data[i].value.type == EntityType::MESH) {
      Entity *e  = &scene->entities.data[i].value;
      Mesh *mesh = e->mesh;

      i32 tri_count = mesh->verts / 3;
      i32 vert_size = mesh->buf_size / mesh->verts / sizeof(float);
      for (i32 tri_i = 0; tri_i < tri_count; tri_i++) {
        i32 offset = vert_size * tri_i * 3;

        Triangle tri;
        tri.verts[0] = *(Vec3f *)(mesh->data + offset);
        tri.verts[1] = *(Vec3f *)(mesh->data + offset + vert_size);
        tri.verts[2] = *(Vec3f *)(mesh->data + offset + vert_size + vert_size);

        tri.verts[0] = e->transform * tri.verts[0];
        tri.verts[1] = e->transform * tri.verts[1];
        tri.verts[2] = e->transform * tri.verts[2];

        tri.center = (tri.verts[0] + tri.verts[1] + tri.verts[2]) / 3;

        triangles.push_back(tri);
      }
    }
  }
    
  printf("Processed triangles in:");
  timer.print_ms();
  printf("triangles: %llu\n", triangles.size());

  //////////////////////////////////////
  Timer timer2;


  i32 root_i             = threaded_nodes.push_back(1);
  BvhNode *root          = &threaded_nodes[root_i];
  root->triangle_i_start = 0;
  root->triangle_count   = triangles.size();
  set_aabb(root);

  remaining_jobs++;
  push_job(root_i);
  
  std::thread t0(bvh_thread);
  std::thread t1(bvh_thread);
  std::thread t2(bvh_thread);
  std::thread t3(bvh_thread);
  std::thread t4(bvh_thread);
  std::thread t5(bvh_thread);
  std::thread t6(bvh_thread);
  std::thread t7(bvh_thread);
  std::thread t8(bvh_thread);
  std::thread t9(bvh_thread);
  std::thread t10(bvh_thread);
  std::thread t11(bvh_thread);
  std::thread t12(bvh_thread);
  std::thread t13(bvh_thread);
  std::thread t14(bvh_thread);
  std::thread t15(bvh_thread);
  std::thread t16(bvh_thread);
  std::thread t17(bvh_thread);
  std::thread t18(bvh_thread);
  std::thread t19(bvh_thread);

  t0.join();
  t1.join();
  t2.join();
  t3.join();
  t4.join();
  t5.join();
  t6.join();
  t7.join();
  t8.join();
  t9.join();
  t10.join();
  t11.join();
  t12.join();
  t13.join();
  t14.join();
  t15.join();
  t16.join();
  t17.join();
  t18.join();
  t19.join();

  printf("nodels: %d\n", threaded_nodes.count.load());

  printf("Built BVH in:");
  timer2.print_ms();

  return {};
}


float traverse_bvh_threaded(Vec3f ray_origin, Vec3f ray_dir, i32 node_i = 0)
{
  BvhNode *node = &threaded_nodes[node_i];

  if (!ray_aabb_intersect(ray_origin, ray_dir, node->bounds)) return 1e30;

  float min_t = 1e30;
  if (node->child_0 == -1 && node->child_1 == -1) {
    for (i32 i = node->triangle_i_start; i < node->triangle_i_start + node->triangle_count; i++) {
      float t = ray_tri_intersect(ray_origin, ray_dir, triangles[i]);
      if (t > 0) {
        min_t = fminf(min_t, t);
      }
    }
  } else {
    min_t = fminf(min_t, traverse_bvh_threaded(ray_origin, ray_dir, node->child_0));
    min_t = fminf(min_t, traverse_bvh_threaded(ray_origin, ray_dir, node->child_1));
  }

  return min_t;
}