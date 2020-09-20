#version 330 core

layout (location = 0) out vec4 color;

in vec3 view_ray_direction;

uniform vec3 cam_pos;
uniform sampler3D volume;
uniform ivec3 volume_dims;

vec2 IntersectBox(vec3 origin, vec3 direction);

void main()
{
    vec3 ray_dir = normalize(view_ray_direction);

    vec2 t_hit = IntersectBox(cam_pos, ray_dir);
    if (t_hit.x > t_hit.y)
        discard;

    t_hit.x = max(t_hit.x, 0.f);

    vec3 dt_vec = 1.f / (vec3(volume_dims) * abs(ray_dir));
    float dt = min(dt_vec.x, min(dt_vec.y, dt_vec.z));

    vec3 p = cam_pos + t_hit.x * ray_dir;
    for (float t = t_hit.x; t < t_hit.y; t += dt)
    {
        float val = texture(volume, p).r;
        vec4 val_color = vec4(val);

        color.rgb += (1.f - color.a) * val_color.a * val_color.rgb;
        color.a += (1.f - color.a) * val_color.a;

        if (color.a >= 0.95f)
            break;

        p += dt * ray_dir;
    }
}

vec2 IntersectBox(vec3 origin, vec3 direction)
{
    const vec3 box_min = vec3(0.f);
    const vec3 box_max = vec3(1.f);

    vec3 inv_dir = 1.f / direction;

    vec3 tmin_tmp = (box_min - origin) * inv_dir;
    vec3 tmax_tmp = (box_max - origin) * inv_dir; 

    vec3 tmin = min(tmin_tmp, tmax_tmp);
    vec3 tmax = max(tmin_tmp, tmax_tmp);

    float t0 = max(tmin.x, max(tmin.y, tmin.z));
    float t1 = min(tmax.x, min(tmax.y, tmax.z));

    return vec2(t0, t1);
}