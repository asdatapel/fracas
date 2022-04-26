parameters:
  - name: model
    type: FLOAT
    is_matrix: true
    size: 16
  - name: view
    type: FLOAT
    is_matrix: true
    size: 16
  - name: projection
    is_matrix: true
    type: FLOAT
    size: 16
  - name: camera_position
    type: FLOAT
    size: 3
vertex_inputs:
  - name: pos
    index: 0
    type: FLOAT
    size: 3
  - name: uv
    index: 1
    type: FLOAT
    size: 2
  - name: normal
    index: 2
    type: FLOAT
    size: 2
fragment_inputs:
  - name: pos
    index: 0
    type: FLOAT
    size: 3
  - name: uv
    index: 1
    type: FLOAT
    size: 2
  - name: normal
    index: 2
    type: FLOAT
    size: 2
textures:
  - name: irradiance
    type: CUBE
  - name: env_map
    type: CUBE
  - name: brdf
    type: 2D
gl_code:
  vert: pbr_lit_3d.vert
  frag: pbr_lit_3d.frag