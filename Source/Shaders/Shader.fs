#version 330 core

layout (location = 0) out vec4 out_frag_color;

in vec3 color;

uniform sampler2D entry_points_sampler;
uniform sampler2D exit_points_sampler;

uniform sampler3D volume;
uniform sampler1D transfer_function;
uniform ivec3 volume_dims;

uniform float sampling_rate;

#define REF_SAMPLING_INTERVAL 150.0

vec4 RayTraversal(vec3 entry_point, vec3 exit_point)
{
    vec4 result = vec4(0.0);
    vec3 ray_direction = exit_point - entry_point;
    float t_end = length(ray_direction);
    float dt = min(t_end, t_end / (sampling_rate * length(ray_direction * volume_dims)));
    float sample_count = ceil(t_end / dt);

    ray_direction = normalize(ray_direction);
    float t = 0.5f * dt;
    vec3 sample_pos;

    while (t < t_end)
    {
        sample_pos = entry_point + t * ray_direction;

        // val ranges from 0 to 1
        float val = texture(volume, sample_pos).r;
        vec4 val_color = texture(transfer_function, val);
        // vec4 val_color = vec4(val);

        // Opacity correction
        val_color.a = 1.0 - pow(1.0 - val_color.a, dt * REF_SAMPLING_INTERVAL);

        result.rgb += (1.f - result.a) * val_color.a * val_color.rgb;
        result.a += (1.f - result.a) * val_color.a;

        if (result.a >= 0.99f)
            break;

        t += dt;
    }

    return result;
}

void main()
{
    // TODO: This shouldn't be harcoded
    vec2 tex_coords = vec2(gl_FragCoord.x / 1280.f, gl_FragCoord.y / 720.f);
    vec3 entry_point = texture(entry_points_sampler, tex_coords).rgb;
    vec3 exit_point = texture(exit_points_sampler, tex_coords).rgb;

    #if 1
    vec4 out_color = vec4(0.0);

    if (entry_point != exit_point)
    {
        out_color = RayTraversal(entry_point, exit_point);
    }
    #endif

    vec4 bg_color = vec4(0.5f, 0.5f, 0.5f, 1.f);

    out_frag_color.a = out_color.a + bg_color.a - out_color.a * bg_color.a;
    out_frag_color.rgb = bg_color.a * bg_color.rgb * (1.0  - out_color.a) + out_color.rgb * out_color.a;
}