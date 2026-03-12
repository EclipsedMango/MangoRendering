#version 460 core
layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

uniform mat4 u_LightVP[6];
uniform int  u_LightSlot;

in  vec3 v_WorldPos[];
out vec3 g_WorldPos;

void main() {
    for (int face = 0; face < 6; face++) {
        gl_Layer = u_LightSlot * 6 + face;
        for (int v = 0; v < 3; v++) {
            g_WorldPos  = v_WorldPos[v];
            gl_Position = u_LightVP[face] * vec4(v_WorldPos[v], 1.0);
            EmitVertex();
        }
        EndPrimitive();
    }
}