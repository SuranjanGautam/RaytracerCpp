#pragma once


#include "hittable.h"
#include "vec3.h"

class triangle : public hittable {
public:
	triangle() {}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {}

	aabb bounding_box() const override {
		return bbox;
	}
private:
	

	shared_ptr<material> mat;
	aabb bbox;
};