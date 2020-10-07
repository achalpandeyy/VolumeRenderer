#version 330 core

layout (location = 0) out vec4 color;

in vec3 view_ray_direction;

uniform vec3 cam_pos;
uniform sampler3D volume;
uniform sampler1D transfer_function;
uniform ivec3 volume_dims;

vec2 IntersectBox(vec3 origin, vec3 direction);

void main()
{
    color = vec4(0.8f, 0.3f, 0.2f, 1.f);

#if 0
    color = vec4(0.f);

    vec3 ray_dir = normalize(view_ray_direction);

    vec2 t_hit = IntersectBox(cam_pos, ray_dir);
    if (t_hit.x > t_hit.y)
        discard;

    t_hit.x = max(t_hit.x, 0.f);

    vec3 entry_point = cam_pos + t_hit.x * ray_dir;
    vec3 exit_point = cam_pos + t_hit.y * ray_dir;

    // float t_end = length(exit_point - entry_point);
    float t_end = length(view_ray_direction);
    float dt = min(t_end, t_end / (2.f * length(view_ray_direction * volume_dims)));

    // vec3 dt_vec = 1.f / (vec3(volume_dims) * abs(ray_dir));
    // float dt = min(dt_vec.x, min(dt_vec.y, dt_vec.z));

    // float dt = min(t_hit.y, t_hit.y / 2.f * (length(view_ray_direction * volume_dims)));

    vec3 p = cam_pos + t_hit.x * ray_dir;
    for (float t = t_hit.x; t < t_hit.y; t += dt)
    {
        float val = texture(volume, p).r;

        vec4 val_color = texture(transfer_function, val);

        color.rgb += (1.f - color.a) * val_color.a * val_color.rgb;
        color.a += (1.f - color.a) * val_color.a;

        if (color.a >= 0.99f)
            break;

        p += dt * ray_dir;
    }
#endif
}

vec2 IntersectBox(vec3 origin, vec3 direction)
{
    const vec3 box_min = vec3(0.0);
    const vec3 box_max = vec3(1.0);

    vec3 inv_dir = 1.0 / direction;

    vec3 tmin_tmp = (box_min - origin) * inv_dir;
    vec3 tmax_tmp = (box_max - origin) * inv_dir; 

    vec3 tmin = min(tmin_tmp, tmax_tmp);
    vec3 tmax = max(tmin_tmp, tmax_tmp);

    float t0 = max(tmin.x, max(tmin.y, tmin.z));
    float t1 = min(tmax.x, min(tmax.y, tmax.z));

    return vec2(t0, t1);
}