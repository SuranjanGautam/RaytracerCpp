#pragma once

#include "hittable.h"
#include <vector>
#include <memory>

using std::vector;
using std::shared_ptr;


class hittable_list : public hittable {
public:
	vector<shared_ptr<hittable>> objects;

	hittable_list() {};
	hittable_list(shared_ptr<hittable> object) { add(object); };

	void clear() { objects.clear(); }
	void add(shared_ptr<hittable> object)
	{
		objects.push_back(object);
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		hit_record temprec;
		double closest_so_far=ray_t.max;
		bool hit_anything = false;
		for(const auto& object : objects){		
			if (object->hit(r, interval(ray_t.min,closest_so_far), temprec))
			{
				hit_anything = true;
				closest_so_far = temprec.t;
				rec = temprec;
			}
		}
		return hit_anything;
	}
};