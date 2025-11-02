#include <gfx.h>
#include <immintrin.h>

#define PI 3.14159265358979323846

inline float deg_to_rad(float deg)
{
    return (deg * PI) / 180.0;
}

float reduce(float x)
{
    while (x > PI) x -= 2 * PI;
    while (x < -PI) x += 2 * PI;
    return x;
}

float _cos(float x)
{
    x = reduce(x);
    float x2 = x * x;

    return 1 - x2 / 2.0 + x2 * x2 / 24.0 - x2 * x2 * x2 / 720.0 + x2 * x2 * x2 * x2 / 40320.0;
}

float _sin(float x)
{
    x = reduce(x);
    float x2 = x * x;

    return x * (1 - x2 / 6.0 + x2 * x2 / 120.0 - x2 * x2 * x2 / 5040.0 + x2 * x2 * x2 * x2 / 362880.0);
}

float _tan(float x)
{
    return _sin(x) / _cos(x);
}

inline float _sqrt(float x)
{
    __m128 v = _mm_set_ss(x);
    v = _mm_sqrt_ss(v);
    return _mm_cvtss_f32(v);
}

u32 rng_state = 1;
u32 random_u32()
{
    u32 x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

inline float random_float()
{
    return (random_u32() & 0xFFFFFF) / (float)0x1000000;
}

// typedef struct {
//     float x, y, z;
// } vec3;

typedef __m128 vec3;

inline vec3 vec3_add(vec3 a, vec3 b)
{
    // return (vec3) { a.x + b.x, a.y + b.y, a.z + b.z };
    return _mm_add_ps(a, b);
}

inline vec3 vec3_sub(vec3 a, vec3 b)
{
    // return (vec3) { a.x - b.x, a.y - b.y, a.z - b.z };
    return _mm_sub_ps(a, b);
}

inline vec3 vec3_mul(vec3 a, vec3 b)
{
    // return (vec3) { a.x* b.x, a.y* b.y, a.z* b.z };
    return _mm_mul_ps(a, b);
}

inline vec3 vec3_scale(vec3 a, float s)
{
    // return (vec3) { s* a.x, s* a.y, s* a.z };
    return _mm_mul_ps(a, _mm_set1_ps(s));
}

inline vec3 vec3_sma(float s, vec3 m, vec3 a)
{
    // return (vec3) { s* m.x + a.x, s* m.y + a.y, s* m.z + a.z };
    return _mm_add_ps(_mm_mul_ps(m, _mm_set1_ps(s)), a);
}

inline vec3 vec3_neg(vec3 a)
{
    // return (vec3) { -a.x, -a.y, -a.z };
    __m128 sign = _mm_set1_ps(-0.0f);
    return _mm_xor_ps(a, sign);
}

inline float vec3_dot(vec3 a, vec3 b)
{
    // return a.x * b.x + a.y * b.y + a.z * b.z;
    __m128 mul = _mm_mul_ps(a, b); // (x*x, y*y, z*z, w*w)
    __m128 shuf = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 1, 0, 3)); // shuffle for horizontal add
    __m128 sums = _mm_add_ps(mul, shuf);
    shuf = _mm_shuffle_ps(sums, sums, _MM_SHUFFLE(1, 0, 3, 2));
    sums = _mm_add_ss(sums, shuf);
    return _mm_cvtss_f32(sums);
}

inline vec3 vec3_cross(vec3 a, vec3 b)
{
    // return (vec3) {
    //     a.y* b.z - a.z * b.y,
    //         a.z* b.x - a.x * b.z,
    //         a.x* b.y - a.y * b.x
    // };
    __m128 a_yzx = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1));
    __m128 b_yzx = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1));
    __m128 c = _mm_sub_ps(_mm_mul_ps(a, b_yzx), _mm_mul_ps(a_yzx, b));
    return _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1));
}

inline vec3 vec3_reflect(vec3 v, vec3 n)
{
    // float two_dot = 2 * vec3_dot(v, n);
    // return vec3_sub(v, vec3_scale(n, two_dot));
    float two_dot = 2.0f * vec3_dot(v, n);
    return _mm_sub_ps(v, _mm_mul_ps(n, _mm_set1_ps(two_dot)));
}

inline vec3 vec3_normalize(vec3 v)
{
    // float inv_len = 1.0f / _sqrt(vec3_dot(v, v));
    // return vec3_scale(v, inv_len);

    // SSE2 dot product
    __m128 mul = _mm_mul_ps(v, v);
    __m128 shuf = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 1, 0, 3));
    __m128 sums = _mm_add_ps(mul, shuf);
    shuf = _mm_shuffle_ps(sums, sums, _MM_SHUFFLE(1, 0, 3, 2));
    sums = _mm_add_ss(sums, shuf);
    float len = _mm_cvtss_f32(_mm_sqrt_ss(sums));

    // divide vector by length
    __m128 inv_len = _mm_set1_ps(1.0f / len);
    return _mm_mul_ps(v, inv_len);
}

inline vec3 vec3_lerp(vec3 a, vec3 b, float t)
{
    // return vec3_sma(t, vec3_sub(b, a), a);
    return _mm_add_ps(_mm_mul_ps(_mm_sub_ps(b, a), _mm_set1_ps(t)), a);
}

vec3 mat3_mul_vec3(vec3 u, vec3 v, vec3 n, vec3 local)
{
    // Broadcast local.x, local.y, local.z using shuffles
    __m128 lx = _mm_shuffle_ps(local, local, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 ly = _mm_shuffle_ps(local, local, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 lz = _mm_shuffle_ps(local, local, _MM_SHUFFLE(2, 2, 2, 2));

    // out = u*local.x + v*local.y + n*local.z
    __m128 out = _mm_add_ps(
        _mm_add_ps(_mm_mul_ps(u, lx),
            _mm_mul_ps(v, ly)),
        _mm_mul_ps(n, lz));

    return out;
}

#define SCALE       1
#define WIDTH       (1920 / SCALE)
#define HEIGHT      (1080 / SCALE)

vec3 surf[WIDTH * HEIGHT];
int hits[WIDTH * HEIGHT];

inline u8 lerp_u8(u8 a, u8 b, float t)
{
    float af = a;
    float bf = b;
    return (u8)(af + t * (bf - af));
}

vec3 random_on_sphere()
{
    float z = 1.0f - 2.0f * random_float();
    float r = _sqrt(1.0f - z * z);
    float phi = 2 * PI * random_float();
    float x = r * _cos(phi);
    float y = r * _sin(phi);
    return (vec3) { x, y, z };
}

vec3 random_in_hemisphere(vec3 n)
{
    vec3 v = random_on_sphere();
    return vec3_dot(v, n) < 0 ? vec3_neg(v) : v;
}

inline float _abs(float x)
{
    union { float f; u32 i; } u = { x };
    u.i &= 0x7fffffff;
    return u.f;
}

vec3 random_cosine_direction(vec3 n)
{
    float r1 = random_float();
    float r2 = random_float();

    float phi = 2.0f * PI * r1;
    float x = _cos(phi) * _sqrt(r2);
    float y = _sin(phi) * _sqrt(r2);
    float z = _sqrt(1.0f - r2);
    vec3 local = (vec3){ x, y, z };

    union { __m128 v; float f[4]; } u_n = { n };
    float n_x = u_n.f[0];

    vec3 u = vec3_normalize(_abs(n_x) > 0.9f ? (vec3) { 0, 1, 0 } : vec3_cross((vec3) { 1, 0, 0 }, n));
    vec3 v = vec3_cross(n, u);

    return mat3_mul_vec3(u, v, n, local);
}

typedef struct {
    vec3 origin;
    vec3 dir;
} ray;

struct camera {
    vec3 pos;
    vec3 right;
    vec3 up;
    vec3 forward;
    float near;
    float fov;
};

struct material {
    vec3 color;
    float emissive;
    float roughness;
};

typedef enum {
    MAT_DIF_WHITE,
    MAT_DIF_GREEN,
    MAT_DIF_RED,
    MAT_LIGHT,
    MAT_METAL,
    MAT_YELLOW_LIGHT,
} material_id;

struct material materials[] = {
    [MAT_DIF_WHITE] = {
        .color = {1, 1, 1},
        .emissive = 0,
        .roughness = 1,
    },
    [MAT_DIF_GREEN] = {
        .color = {0, 1, 0},
        .emissive = 0,
        .roughness = 1,
    },
    [MAT_DIF_RED] = {
        .color = {1, 0, 0},
        .emissive = 0,
        .roughness = 1,
    },
    [MAT_LIGHT] = {
        .color = {1, 1, 1},
        .emissive = 10,
        .roughness = 0,
    },
    [MAT_METAL] = {
        .color = {1, 1, 0.25},
        .emissive = 0,
        .roughness = 0.1,
    },
    [MAT_YELLOW_LIGHT] = {
        .color = {1, 1, 0.5},
        .emissive = 20,
        .roughness = 0,
    }
};

struct hit_info {
    float t;
    vec3 p;
    vec3 n;
    material_id mat_id;
};

struct object;

typedef int (*intersect_fn)(struct object*, ray*, struct hit_info*);

struct object {
    void* data;
    intersect_fn intersect;
};

struct sphere {
    vec3 center;
    float radius;
    material_id mat_id;
};

int intersect_sphere(struct object* obj, ray* r, struct hit_info* hi)
{
    struct sphere* s = (struct sphere*)obj->data;

    vec3 oc = vec3_sub(r->origin, s->center);

    float a = vec3_dot(r->dir, r->dir);
    float b = 2.0f * vec3_dot(oc, r->dir);
    float c = vec3_dot(oc, oc) - s->radius * s->radius;

    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0)
        return 0;

    float t = (-b - _sqrt(discriminant)) / (2.0f * a);
    if (t < 0) {
        t = (-b + _sqrt(discriminant)) / (2.0f * a);
        if (t < 0)
            return 0;
    }

    hi->t = t;
    hi->p = vec3_sma(t, r->dir, r->origin);
    hi->n = vec3_scale(vec3_sub(hi->p, s->center), 1.0f / s->radius);
    hi->mat_id = s->mat_id;

    return 1;
}

struct mesh {
    vec3* verts;
    usize num_verts;
    u32* indices;
    usize num_indices;
    material_id* mat_ids;
};

int intersect_triangle(ray* r, vec3 v0, vec3 v1, vec3 v2, float* out_t, vec3* out_n)
{
    const float EPSILON = 1e-7f;
    vec3 edge1, edge2, h, s, q;
    float a, f, u, v;

    // edges
    edge1 = vec3_sub(v1, v0);
    edge2 = vec3_sub(v2, v0);

    // cross product h = dir x edge2
    h = vec3_cross(r->dir, edge2);

    a = vec3_dot(edge1, h);

    // Front-face only: reject if a <= EPSILON
    if (a <= EPSILON)
        return 0;

    f = 1.0f / a;

    // vector from v0 to ray origin
    s = vec3_sub(r->origin, v0);

    u = f * vec3_dot(s, h);
    if (u < 0.0f || u > 1.0f)
        return 0;

    // cross product q = s x edge1
    q = vec3_cross(s, edge1);

    v = f * vec3_dot(r->dir, q);
    if (v < 0.0f || u + v > 1.0f)
        return 0;

    // intersection distance along ray
    *out_t = f * vec3_dot(edge2, q);

    // proper normalized triangle normal
    *out_n = vec3_cross(edge1, edge2);
    *out_n = vec3_normalize(*out_n);

    return 1;
}

int intersect_mesh(struct object* obj, ray* r, struct hit_info* hi)
{
    struct mesh* m = (struct mesh*)obj->data;

    struct hit_info best_hit;
    int set = 0;

    for (usize i = 0; i < m->num_indices; i += 3) {
        vec3 v0 = m->verts[m->indices[i]];
        vec3 v1 = m->verts[m->indices[i + 1]];
        vec3 v2 = m->verts[m->indices[i + 2]];

        float t;
        vec3 n;
        if (intersect_triangle(r, v0, v1, v2, &t, &n)) {
            if (!set || t < best_hit.t) {
                best_hit.t = t;
                best_hit.p = vec3_sma(t, r->dir, r->origin);
                best_hit.n = n;
                best_hit.mat_id = m->mat_ids[i / 3];
                set = 1;
            }
        }
    }

    if (set)
        *hi = best_hit;

    return set;
}

struct object objects[] = {
    {
        .data = &(struct mesh) {
            .verts = (vec3[]) {
                { -4, 0, -4 },
                { 4, 0, -4 },
                { 4, 0, 4 },
                { -4, 0, 4 },

                { -4, 6, -4 },
                { 4, 6, -4 },
                { 4, 6, 4 },
                { -4, 6, 4 }
            },
            .num_verts = 8,
            .indices = (u32[]) {
                0, 2, 1, 0, 3, 2, // bottom
                4, 5, 6, 4, 6, 7, // top
                3, 6, 2, 3, 7, 6, // front
                2, 5, 1, 2, 6, 5, // right
                0, 1, 5, 0, 5, 4, // back
                0, 4, 3, 3, 4, 7  // left
            },
            .num_indices = 6 * 2 * 3,
            .mat_ids = (material_id[]) {
               MAT_DIF_WHITE, MAT_DIF_WHITE,
               MAT_DIF_WHITE, MAT_DIF_WHITE,
               MAT_DIF_WHITE, MAT_DIF_WHITE,
               MAT_DIF_RED, MAT_DIF_RED,
               MAT_DIF_WHITE, MAT_DIF_WHITE,
               MAT_DIF_GREEN, MAT_DIF_GREEN
            },
        },
        .intersect = intersect_mesh
    },
    {
        .data = &(struct sphere) {
            .center = { 0, 1, 0 },
            .radius = 1,
            .mat_id = MAT_METAL
        },
        .intersect = intersect_sphere
    },
    {
        .data = &(struct sphere) {
            .center = { 2, 3, 0 },
            .radius = 1,
            .mat_id = MAT_DIF_WHITE
        },
        .intersect = intersect_sphere
    },
    {
        .data = &(struct mesh) {
            .verts = (vec3[]) {
                { -1, 5.9, -1 },
                { 1, 5.9, -1 },
                { 1, 5.9, 1 },
                { -1, 5.9, 1 }
            },
            .num_verts = 4,
            .indices = (u32[]) {
                0, 1, 2, 0, 2, 3, // top
            },
            .num_indices = 1 * 2 * 3,
            .mat_ids = (material_id[]) {
               MAT_LIGHT, MAT_LIGHT
            },
        },
        .intersect = intersect_mesh
    }
};

#define NUM_OBJECTS (sizeof(objects) / sizeof(objects[0]))

#define INFINITY (1.0f/0.0f)

int intersect_scene(ray* r, struct hit_info* hi)
{
    struct hit_info best_hit;
    int set = 0;

    for (usize i = 0; i < NUM_OBJECTS; i++) {
        struct object* obj = &objects[i];
        struct hit_info hi;
        if (obj->intersect(obj, r, &hi)) {
            if (!set || hi.t < best_hit.t) {
                best_hit = hi;
                set = 1;
            }
        }
    }

    if (set)
        *hi = best_hit;

    return set;
}

// this is the top left corner of the pixel in the world
vec3 pixel_to_point(struct camera* cam, usize x, usize y)
{
    float ar = WIDTH / (float)HEIGHT; // aspect ratio

    float pz = cam->near;              // dist to plane
    float py = pz * _tan(cam->fov / 2); // plane height / 2
    float px = py * ar;                // plane width  / 2

    float pixel_size = 2 * px / WIDTH; // world size of screen pixel
    float sx = (x + random_float()) * pixel_size; // right offset
    float sy = (y + random_float()) * pixel_size; // down offset

    vec3 top_left = vec3_sma(pz, cam->forward, cam->pos);
    top_left = vec3_sma(py, cam->up, top_left);
    top_left = vec3_sma(-px, cam->right, top_left);

    vec3 p = vec3_sma(sx, cam->right, top_left);
    p = vec3_sma(-sy, cam->up, p);

    return p;
}

inline ray make_ray(vec3 a, vec3 b)
{
    vec3 dir = vec3_normalize(vec3_sub(b, a));
    return (ray) { a, dir };
}

inline vec3 vec3_tonemap(vec3 color)
{
    // return (vec3) {
    //     color.x / (1.0f + color.x),
    //         color.y / (1.0f + color.y),
    //         color.z / (1.0f + color.z)
    // };
    __m128 one = _mm_set1_ps(1.0f);
    return _mm_div_ps(color, _mm_add_ps(one, color));
}

u32 color_to_u32(vec3 color)
{
    color = vec3_tonemap(color);
    color = vec3_scale(color, 255.0f);

    union { __m128 v; float f[4]; } u = { color };

    u8 r = u.f[0];
    u8 g = u.f[1];
    u8 b = u.f[2];

    return (r << 16) | (g << 8) | b;
}

bool ray_trace(ray* r, int depth, vec3* out)
{
    // struct hit_info hi;

    // if (depth <= 0)
    //     return (vec3) { 0, 0, 0 };

    // if (!intersect_scene(r, &hi))
    //     return (vec3) { 0, 0, 0 };

    // struct material* mat = &materials[hi.mat_id];

    // if (mat->emissive > 0)
    //     return vec3_scale(mat->color, mat->emissive);

    // hi.p = vec3_sma(0.001f, hi.n, hi.p); // avoid self intersection

    // vec3 reflected = vec3_reflect(r->dir, hi.n);
    // vec3 random_dir = random_cosine_direction(hi.n);
    // vec3 out_dir = vec3_normalize(vec3_lerp(reflected, random_dir, mat->roughness));
    // ray next = { hi.p, out_dir };

    // vec3 in_color = ray_trace(&next, depth - 1);

    // return vec3_mul(mat->color, in_color);

    struct hit_info hi;
    ray next = *r;
    vec3 color = (vec3){ 1, 1, 1 };

    for (int i = 0; i < depth; i++)
    {
        if (!intersect_scene(&next, &hi)) {
            *out = (vec3){ 0, 0, 0 };
            return true;
        }

        struct material* mat = &materials[hi.mat_id];

        if (mat->emissive > 0) {
            *out = vec3_mul(color, vec3_scale(mat->color, mat->emissive));
            return true;
        }

        hi.p = vec3_sma(0.001f, hi.n, hi.p); // avoid self intersection
        vec3 reflected = vec3_reflect(r->dir, hi.n);
        vec3 random_dir = vec3_normalize(hi.n + random_on_sphere());
        vec3 out_dir = vec3_normalize(vec3_lerp(reflected, random_dir, mat->roughness));
        next = (ray){ hi.p, out_dir };

        color = vec3_mul(color, mat->color);
    }

    return false;
}

bool compute(struct camera* cam, usize x, usize y, vec3* out)
{
    vec3 p = pixel_to_point(cam, x, y);
    ray r = make_ray(cam->pos, p);

    return ray_trace(&r, 5, out);
}

inline vec3 accumulate(vec3 old, vec3 new, usize n)
{
    return vec3_scale(vec3_add(vec3_scale(old, n), new), 1.0f / (n + 1));
}

void blit_surf()
{
    for (usize y = 0; y < height; y++) {
        for (usize x = 0; x < width; x++) {
            u32 sx = x * WIDTH / width;
            u32 sy = y * HEIGHT / height;
            vec3 color = surf[sy * WIDTH + sx];
            fb[y * width + x] = color_to_u32(color);
        }
    }

    blit();
}

int min4(int a, int b, int c, int d)
{
    int m = a;
    if (b < m) m = b;
    if (c < m) m = c;
    if (d < m) m = d;
    return m;
}

void filter()
{
    for (usize y = 1; y < HEIGHT - 1; y++) {
        for (usize x = 1; x < WIDTH - 1; x++) {
            int up = hits[(y - 1) * WIDTH + x];
            int down = hits[(y + 1) * WIDTH + x];
            int left = hits[y * WIDTH + (x - 1)];
            int right = hits[y * WIDTH + (x + 1)];
            int this = hits[y * WIDTH + x];

            int min = min4(up, down, left, right);

            if (2 * this < min)
            {
                vec3 sum = (vec3){ 0, 0, 0 };
                sum += surf[(y - 1) * WIDTH + x];
                sum += surf[(y + 1) * WIDTH + x];
                sum += surf[y * WIDTH + (x - 1)];
                sum += surf[y * WIDTH + (x + 1)];
                surf[y * WIDTH + x] = vec3_scale(sum, 0.25f);
                hits[y * WIDTH + x] = min;
            }
        }
    }
}

int main()
{
    usize samples = 0;
    struct camera cam = (struct camera){
        .pos = (vec3){0, 2, -5},
        .right = (vec3){1, 0, 0},
        .up = (vec3){0, 1, 0},
        .forward = (vec3){0, 0, 1},
        .near = 0.01,
        .fov = deg_to_rad(70)
    };

    gfx_init();
    clear(0);

    while (getch() != 'q') {
        for (usize y = 0; y < HEIGHT; y++) {
            for (usize x = 0; x < WIDTH; x++) {
                vec3 new;
                if (compute(&cam, x, y, &new)) {
                    surf[y * WIDTH + x] = accumulate(surf[y * WIDTH + x], new, samples);
                    hits[y * WIDTH + x]++;
                }
            }
        }

        filter();
        blit_surf();
        samples++;
    }

    gfx_exit();
    return 0;
}
