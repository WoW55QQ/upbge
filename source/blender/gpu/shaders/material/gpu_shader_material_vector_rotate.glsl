vec3 rotate_around_axis(vec3 p, vec3 axis, float angle)
{
  float costheta = cos(angle);
  float sintheta = sin(angle);
  vec3 r;

  r.x = ((costheta + (1.0 - costheta) * axis.x * axis.x) * p.x) +
        (((1.0 - costheta) * axis.x * axis.y - axis.z * sintheta) * p.y) +
        (((1.0 - costheta) * axis.x * axis.z + axis.y * sintheta) * p.z);

  r.y = (((1.0 - costheta) * axis.x * axis.y + axis.z * sintheta) * p.x) +
        ((costheta + (1.0 - costheta) * axis.y * axis.y) * p.y) +
        (((1.0 - costheta) * axis.y * axis.z - axis.x * sintheta) * p.z);

  r.z = (((1.0 - costheta) * axis.x * axis.z - axis.y * sintheta) * p.x) +
        (((1.0 - costheta) * axis.y * axis.z + axis.x * sintheta) * p.y) +
        ((costheta + (1.0 - costheta) * axis.z * axis.z) * p.z);

  return r;
}

void node_vector_rotate_axis_angle(
    vec3 vector_in, vec3 center, vec3 axis, float angle, vec3 rotation, out vec3 vec)
{
  vec = (length(axis) != 0.0) ?
            rotate_around_axis(vector_in - center, normalize(axis), angle) + center :
            vector_in;
}

void node_vector_rotate_axis_x(
    vec3 vector_in, vec3 center, vec3 axis, float angle, vec3 rotation, out vec3 vec)
{
  vec = rotate_around_axis(vector_in - center, vec3(1.0, 0.0, 0.0), angle) + center;
}

void node_vector_rotate_axis_y(
    vec3 vector_in, vec3 center, vec3 axis, float angle, vec3 rotation, out vec3 vec)
{
  vec = rotate_around_axis(vector_in - center, vec3(0.0, 1.0, 0.0), angle) + center;
}

void node_vector_rotate_axis_z(
    vec3 vector_in, vec3 center, vec3 axis, float angle, vec3 rotation, out vec3 vec)
{
  vec = rotate_around_axis(vector_in - center, vec3(0.0, 0.0, 1.0), angle) + center;
}

void node_vector_rotate_euler_xyz(
    vec3 vector_in, vec3 center, vec3 axis, float angle, vec3 rotation, out vec3 vec)
{
  vec = euler_to_mat3(rotation) * (vector_in - center) + center;
}

void node_vector_rotate_euler_xzy(
    vec3 vector_in, vec3 center, vec3 axis, float angle, vec3 rotation, out vec3 vec)
{
  vec = euler_to_mat3(-rotation.xzy) * (vector_in - center) + center;
}

void node_vector_rotate_euler_yxz(
    vec3 vector_in, vec3 center, vec3 axis, float angle, vec3 rotation, out vec3 vec)
{
  vec = euler_to_mat3(-rotation.yxz) * (vector_in - center) + center;
}

void node_vector_rotate_euler_yzx(
    vec3 vector_in, vec3 center, vec3 axis, float angle, vec3 rotation, out vec3 vec)
{
  vec = euler_to_mat3(rotation.yzx) * (vector_in - center) + center;
}

void node_vector_rotate_euler_zxy(
    vec3 vector_in, vec3 center, vec3 axis, float angle, vec3 rotation, out vec3 vec)
{
  vec = euler_to_mat3(rotation.zxy) * (vector_in - center) + center;
}

void node_vector_rotate_euler_zyx(
    vec3 vector_in, vec3 center, vec3 axis, float angle, vec3 rotation, out vec3 vec)
{
  vec = euler_to_mat3(-rotation.zyx) * (vector_in - center) + center;
}
