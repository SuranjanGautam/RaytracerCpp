#pragma once
#include "hittable.h"
#include "vec3.h"

class sphere : public hittable {
public:
	sphere(point3 _center, double _radius, shared_ptr<material> _material) :center(_center), radius(_radius),mat(_material) {};

	
	bool hit(const ray& r, interval ray_t, hit_record& rec) const override{
		auto A_minus_C = r.origin() - center;
		auto a = r.direction().length_squared();
		auto b = dot(r.direction(), A_minus_C);
		auto c = A_minus_C.length_squared() - (radius * radius);

		auto determinant = (b * b) - (a * c);

		if (determinant < 0)
			return false;
		auto sqrtd = sqrt(determinant);
		auto root = (-b - sqrtd) / a;
		if (!ray_t.sorrounds(root))
		{
			root = (-b + sqrtd) / a;
			if (!ray_t.sorrounds(root))
			{
				return false;
			}
		}
		rec.t = root;
		rec.p = r.at(root);
		vec3 out_normal = (rec.p - center) * (1/radius);
		rec.set_face_normal(r, out_normal);
		rec.mat = mat;

		return true;
	}
private:
	point3 center;
	double radius;
	shared_ptr<material> mat;
};