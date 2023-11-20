#pragma once

#include "vec3.h"
#include "hittable.h"

class material {
public:
	~material() = default;
	virtual bool scatter(const ray& r_in, const hit_record& rec, color& attunation, ray& scattered) const = 0;
};

class lambertian : public material {
public:
	lambertian(const color& _albedo):albedo(_albedo) {};

	bool scatter(const ray& r_in, const hit_record& rec, color& attunation, ray& scattered) const override
	{
		auto scatter_direction = rec.normal + random_unit_vector();
		if (scatter_direction.near_zero())
		{
			scatter_direction = rec.normal;
		}
		scattered = ray(rec.p, scatter_direction);
		attunation = albedo;
		return true;
	}
private:
	color albedo;
};

class metal : public material {
public:
	metal(const color& _albedo,double _fuzz) :albedo(_albedo),fuzz(_fuzz <1? _fuzz :1) {};

	bool scatter(const ray& r_in, const hit_record& rec, color& attunation, ray& scattered) const override
	{
		auto reflected = reflect(unit_vector(r_in.direction()), rec.normal);
		scattered = ray(rec.p, reflected + fuzz * random_in_unit_sphere());
		attunation = albedo;
		return (dot(scattered.direction(),rec.normal)>0);
	}
private:
	color albedo;
	double fuzz;
};

class dielectric : public material {
public:
	dielectric(double rf_index) :ref_index(rf_index) {};

	bool scatter(const ray& r_in, const hit_record& rec, color& attunation, ray& scattered) const override
	{
		attunation = color(1, 1, 1);
		double refraction_ratio = rec.front_face ? (1.0 / ref_index) : ref_index;		
		auto unit_direction = unit_vector(r_in.direction());

		double cos_theta = fmin(dot(-unit_direction, rec.normal), 1);
		double sin_theta = sqrt(1.0 - cos_theta * cos_theta);

		vec3 direction;
		bool can_not_refract = refraction_ratio * sin_theta > 1;

		if (can_not_refract || reflectance(cos_theta,refraction_ratio)>random_double())
		{
			direction = reflect(unit_direction, rec.normal);
		}
		else
		{
			direction = refract(unit_direction, rec.normal, refraction_ratio);
		}

		scattered = ray(rec.p, direction);
		
		return true;
	}
private:
	double ref_index;

	static double reflectance(double cosine,double  _ref_index) {

		auto r0 = (1.0 - _ref_index) / (1.0 + _ref_index);
		r0 *= r0;
		return r0 + (1 - r0) * pow((1 - cosine), 5);

	}
};