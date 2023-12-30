#pragma once

#include "general.h";
#include "hittable.h";

class quad : public hittable {
public:
	quad(const point3& _Q,const vec3 & _u, const vec3& _v, shared_ptr<material> _mat):Q(_Q),u(_u),v(_v),mat(_mat) {
		
		auto n = cross(u, v);
		w = n / dot(n, n);

		normal = unit_vector(n);
		D = dot(normal, Q);	

		set_bounding_box();
	};

	virtual void set_bounding_box() {
		bbox = aabb(Q, Q + u + v).pad();
	}

	aabb bounding_box() const override {
		return bbox;
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override 
	{
		auto ndotd = dot(normal, r.direction());

		if (fabs(ndotd) < 1e-8) 
			return false;

		auto t = (D - dot(normal, r.origin()))/ndotd;

		if (!ray_t.contains(t))
			return false;

		auto intersection = r.at(t);

		vec3 p = intersection - Q;

		auto alpha = dot(w, cross(p,v));
		auto beta = dot(w, cross(u, p));

		if (!is_interior(alpha, beta, rec))
			return false;		

		rec.t = t;
		rec.p = intersection;
		rec.mat = mat;
		rec.set_face_normal(r, normal);
		return true;

	}

	virtual bool is_interior(double a, double b, hit_record& rec) const {
		if ((a < 0) || (1 < a) || (b < 0) || (1 < b))
			return false;

		rec.u = a;
		rec.v = b;

		return true;
	}
private:
	point3 Q;
	vec3 u, v;
	shared_ptr<material> mat;
	vec3 normal;
	double D;
	aabb bbox;
	vec3 w;
};
