#pragma once

#include "hittable.h"
#include "vec3.h"
#include "vertex.h"


class triangle : public hittable {
public:
	triangle(shared_ptr<vertex> _v1, shared_ptr<vertex> _v2, shared_ptr<vertex> _v3,shared_ptr<material> m) :vertices{_v1,_v2,_v3},mat(m) {
		auto min = vec3(
			fmin(vertices[0]->position[0], fmin(vertices[1]->position[0], vertices[2]->position[0])),
			fmin(vertices[0]->position[1], fmin(vertices[1]->position[1], vertices[2]->position[1])),
			fmin(vertices[0]->position[2], fmin(vertices[1]->position[2], vertices[2]->position[2]))
		);

		auto max = vec3(
			fmax(vertices[0]->position[0], fmax(vertices[1]->position[0], vertices[2]->position[0])),
			fmax(vertices[0]->position[1], fmax(vertices[1]->position[1], vertices[2]->position[1])),
			fmax(vertices[0]->position[2], fmax(vertices[1]->position[2], vertices[2]->position[2]))
		);

		v0 = vertices[0]->position;
		v1 = vertices[1]->position;
		v2 = vertices[2]->position;

		bbox = aabb(min, max).pad();
		normal = cross(vertices[1]->position - vertices[0]->position, vertices[2]->position - vertices[0]->position);
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {	
		
		auto ndotd = dot(normal, r.direction());

		if (fabs(ndotd) < 1e-8)
			return false;		

		auto u = v1 - v0;
		auto v = v2 - v0;
		auto rov0 = r.origin() - v0;
				
		auto d = 1 / ndotd;
		auto t = d * dot(-normal, rov0);

		if (!ray_t.contains(t))
			return false;

		auto q = cross(rov0, r.direction());
		auto alpha = d * dot(-q, v);
		auto beta = d * dot(q, u);
		
		if (alpha < 0.0 || beta < 0.0 || (alpha + beta)>1.0) return false;

		auto intersection = r.at(t);

		rec.t = t;
		rec.p = intersection;
		rec.mat = mat;

		auto w = 1 - alpha - beta;

		rec.u = w * vertices[0]->u + alpha * vertices[1]->u + beta * vertices[2]->u;
		rec.v = w * vertices[0]->v + alpha * vertices[1]->v + beta * vertices[2]->v;

		if (smooth)
		{			
			rec.set_face_normal(r, w * vertices[0]->normal + alpha* vertices[1]->normal + beta * vertices[2]->normal);
		}
		else
			rec.set_face_normal(r, normal);
		
		return true;
	}

	aabb bounding_box() const override {
		return bbox;
	}
	bool smooth = true;
private:	
	vec3 v0;
	vec3 v1;
	vec3 v2;
	vec3 normal;
	shared_ptr<vertex> vertices[3];	
	shared_ptr<material> mat;
	aabb bbox;
};