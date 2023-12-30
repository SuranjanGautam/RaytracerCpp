#pragma once
#include "general.h"
#include "material.h"
#include "hittable.h"
#include <thread>

class camera {
public:
	double aspect_ratio = 1;
	int image_width = 800;
	int image_height;
	int samples_per_pixel = 100;
	int max_depth = 20; //bounches
	int threadsize = 50;
	color* pixelarray;
	shared_ptr<texture> background = make_shared<solid_color>(color(0.5, 0.7, 1.0));

	int vertical_fov = 90;

	point3 lookfrom = point3(0, 0, -1);
	point3 lookat = point3(0, 0, 0);
	vec3 vup = vec3(0, 1, 0);

	double defocus_angle = 0;
	double focus_dist = 10;

	int seedMultiplier = 999;

	bool multithreading = true;

	void render(const hittable& world) {	
		initialize();
		const hittable* worldptr = &world;
		if (multithreading)
		{
			vector<std::thread> threads;
			int _threadsize = threadsize;
			for (int z = 0; z < _threadsize;z++)
			{
				threads.push_back(std::thread(&camera::blockOperation, *this, worldptr, z, _threadsize));
			}
			for (auto& th : threads)
			{
				th.join();
			}
		}
		else
		{
			for (int j = 0; j < image_height; j++) {
				std::cerr << "\rLines Remaining " << j << std::flush;				
				for (int i = 0; i < image_width; i++)
				{
					pixelOperation(worldptr, i, j);
				}
			}
		}
	}

	void blockOperation(const hittable* world, int z, int threadsize)
	{
		int blocksize = (int)ceilf( (float)image_height / threadsize);
		int startpoint = z * blocksize;
		int end = fmin(startpoint + blocksize,image_height);
		for (int j = startpoint; j < end; j++) {
			for (int i = 0; i < image_width; i++)
			{
				pixelOperation(world, i, j);
			}
		}
	}

	void pixelOperation(const hittable* world, int i, int j)
	{
		srand(i * j * seedMultiplier);
		color pixel_color = color(0, 0, 0);
		for (int samplecount = 0; samplecount < samples_per_pixel; samplecount++)
		{
			ray r = get_ray(i, j);
			pixel_color += ray_color(r, max_depth, *world);
		}
		int index = (j * image_width) + i;
		//pixelarray.push_back(write_color(pixel_color, samples_per_pixel));
		pixelarray[index] = write_color(pixel_color, samples_per_pixel);
	}
	
	void initialize() {
		image_height = static_cast<int>(image_width / aspect_ratio);
		image_height = (image_height < 1) ? 1 : image_height;

		const int arraysize = image_height * image_width;
		
		if (!pixelarray)
		{
			pixelarray = (color*)malloc(arraysize * sizeof(color));
			initsize = arraysize;
		}if (initsize != arraysize)
		{
			free(pixelarray);
			pixelarray = (color*)malloc(arraysize * sizeof(color));
			initsize = arraysize;
		}

		center = lookfrom;

		//auto focal_length = (lookfrom-lookat).length();
		auto theta = degree_to_radian(vertical_fov);
		auto h = tan(theta / 2);
		auto viewport_height = 2 * h * focus_dist;
		auto viewport_width = viewport_height * (static_cast<double>(image_width) / image_height);
		
		w = unit_vector(lookfrom - lookat);
		u = unit_vector(cross(vup, w));
		v = cross(w, u);

		auto viewport_u = viewport_width * u;
		auto viewport_v = -viewport_height * v;

		pixel_delta_u = viewport_u / image_width;
		pixel_delta_v = viewport_v / image_height;

		auto viewport_upper_left = center - (focus_dist * w) - viewport_u / 2 - viewport_v / 2;
		pixel00_loc = viewport_upper_left + (0.5 * (pixel_delta_u + pixel_delta_v));

		// Calculate the camera defocus disk basis vectors.
		auto defocus_radius = focus_dist * tan(degree_to_radian(defocus_angle / 2));
		defocus_disk_u = defocus_radius * u;
		defocus_disk_v = defocus_radius * v;
	}

private:	
	point3 center;
	point3 pixel00_loc;
	vec3 pixel_delta_u,pixel_delta_v;
	vec3 u, v, w;
	vec3 defocus_disk_u, defocus_disk_v;
	int initsize;

	

	color ray_color(const ray& r, int depth ,const hittable& world) const
	{
		hit_record rec;

		if (depth <= 0)
		{
			return color(0, 0, 0);
		}

		if (!world.hit(r, interval(0.001, infinity), rec))		
			return background_color(r);		

		ray scattered;
		color attenuation;
		color color_from_emission = rec.mat->emitted(rec.u,rec.v,rec.p);
		if (!rec.mat->scatter(r, rec, attenuation, scattered))
		{
			return color_from_emission;
		}
		color color_from_scatter = attenuation * ray_color(scattered, depth - 1, world);
		return color_from_scatter + color_from_emission;
		
		/*vec3 unit_dir = r.direction();
		auto a = 0.5 * (unit_dir.y() + 1.0);
		return ((1.0 - a) * color(1.0, 1.0, 1.0)) + (a * color(0.5, 0.7, 1.0));*/
	}

	const color& background_color(const ray& r) const
	{
		auto dir = r.direction();
		auto theta = acos(-dir.y());
		auto phi = atan2(-dir.z(), dir.x()) + pi;
		auto u = phi / (2 * pi);
		auto v = theta / pi;

		return background->value(u, v, r.origin());
	}

	ray get_ray(int i, int j) const {
		auto pixel_loc = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
		auto pixel_sample = pixel_loc + pixel_sample_square();		
		auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
		auto raydirection = unit_vector(pixel_sample - ray_origin);
		return ray(ray_origin, raydirection);
	}

	point3 defocus_disk_sample() const {
		// Returns a random point in the camera defocus disk.
		auto p = random_in_unit_disk();
		return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
	}

	vec3 pixel_sample_square() const {
		auto px = -0.5 + random_double();
		auto py = -0.5 + random_double();
		return (px * pixel_delta_u) + (py * pixel_delta_v);
	}
};