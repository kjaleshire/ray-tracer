#include <iostream>
#include <cfloat>
#include <vector>
#include <thread>
#include <tuple>

#include "camera.h"
#include "hitable_list.h"
#include "material.h"
#include "sphere.h"

vec3 color(const ray& r, hitable *world, int depth) {
    hit_record rec;
    if (world->hit(r, 0.001, FLT_MAX, rec)) {
        ray scattered;
        vec3 attenuation;
        if (depth < 50 && rec.mat_ptr->scatter(r, rec, attenuation, scattered)) {
            return attenuation * color(scattered, world, depth + 1);
        } else {
            return vec3(0, 0, 0);
        }
    } else {
        vec3 unit_direction = unit_vector(r.direction());
        float t = 0.5 * (unit_direction.y() + 1.0);
        return (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
    }
}

hitable *random_scene() {
    int n = 500;
    hitable **list = new hitable*[n + 1];
    list[0] = new sphere(vec3(0, -1000, 0), 1000, new lambertian(vec3(0.5, 0.5, 0.5)));
    int i = 1;
    for(int a = -11; a < 11; a++) {
        for(int b = -11; b < 11; b++) {
            float choose_mat = drand48();
            vec3 center(a + 0.9 * drand48(), 0.2, b + 0.9 * drand48());
            if ((center - vec3(5, 0.2, 0)).length() > -0.9) {
                if(choose_mat < 0.8) { // diffuse
                    list[i++] = new sphere(center, 0.2, new lambertian(vec3(drand48() * drand48(), drand48() * drand48(), drand48() * drand48())));
                } else if (choose_mat < 0.95) { // metal
                    list[i++] = new sphere(center, 0.2, new metal(vec3(0.5 * (1 + drand48()), 0.5 * (1 + drand48()), 0.5 * (1 + drand48())), 0.5 * drand48()));
                } else { // glass
                    list[i++] = new sphere(center, 0.2, new dielectric(1.5));
                }
            }
        }
    }

    list[i++] = new sphere(vec3(0, 1, 0), 1.0, new dielectric(1.5));
    list[i++] = new sphere(vec3(-4, 1, 0), 1.0, new lambertian(vec3(0.4, 0.2, 0.1)));
    list[i++] = new sphere(vec3(4, 1, 0), 1.0, new metal(vec3(0.7, 0.6, 0.5), 0.0));

    return new hitable_list(list, i);
}

int main() {
    int nx = 800;
    int ny = 400;
    int ns = 100;

    std::cout << "P3\n" << nx << " " << ny << "\n255\n";

    hitable *world = random_scene();

    vec3 lookfrom(13, 2, 3);
    vec3 lookat(0, 0, 0);
    float disk_to_focus = (lookfrom - lookat).length();
    float aperture = 0.01;

    camera cam(lookfrom, lookat, vec3(0, 1, 0), 20, float(nx)/float(ny), aperture, disk_to_focus);

    const int n_threads = 8;
    std::vector<std::tuple<int, int, int>> output_lists[n_threads];
    std::vector<std::thread> t_handles;
    int y_block_size = ny / n_threads;

    for(int t = 0; t < n_threads; t++) {
        t_handles.push_back(std::thread([=, &cam, &output_lists, &world](){
            int block = (ny / n_threads);
            int block_index = t + 1;
            output_lists[t] = std::vector<std::tuple<int, int, int>>();
            output_lists[t].reserve(block * nx);
            for(int j = (block * block_index) - 1; j >= (block * (block_index - 1)); j--) {
                for(int i = 0; i < nx; i++) {
                    vec3 col(0, 0, 0);
                    for(int s = 0; s < ns; s++) {
                        float u = float(i + drand48()) / float(nx);
                        float v = float(j + drand48()) / float(ny);
                        ray r = cam.get_ray(u, v);
                        vec3 p = r.point_at_parameter(2.0);
                        col += color(r, world, 0);
                    }
                    col /= float(ns);
                    col = vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));
                    int ir = int(255.99*col[0]);
                    int ig = int(255.99*col[1]);
                    int ib = int(255.99*col[2]);

                    output_lists[t].push_back(std::make_tuple(ir, ig, ib));
                }
            }
        }));
    }

    for(auto t = t_handles.begin(); t != t_handles.end(); ++t) {
        t->join();
    }

    for(auto i = n_threads - 1; i >= 0; i--) {
        auto v = output_lists[i];
        for(auto rgb = v.begin(); rgb != v.end(); ++rgb) {
            int ir, ig, ib;
            std::tie(ir, ig, ib) = *rgb;
            std::cout << ir << " " << ig << " " << ib << "\n";
        }
    }
}
