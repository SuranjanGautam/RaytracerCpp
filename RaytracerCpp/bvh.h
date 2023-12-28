#pragma once

#include "general.h"

#include "hittable.h"
#include "hittablelist.h"
#include <algorithm>

class bvh_node : public hittable {
public:
	bvh_node(const hittable_list& list) : bvh_node(list.objects,0,list.objects.size()) {};

	bvh_node(const std::vector<shared_ptr<hittable>>& src_objects, size_t start, size_t end) {
		auto objs = src_objects;

		int axis = random_int(0, 2);

		auto comparer = (axis == 0) ? box_x_compare : (axis == 1) ? box_y_compare : box_z_compare;

		size_t object_length = end - start;

		if (object_length == 1)
		{
			left = right = objs[start];
		}
		else if (object_length == 2)
		{
			if (comparer(objs[start], objs[start + 1])) {
				left = objs[start];
				right = objs[start + 1];
			}
			else
			{
				left = objs[start + 1];
				right = objs[start];
			}
		}
		else
		{
			std::sort(objs.begin() + start, objs.begin() + end, comparer);

			auto mid = start + (object_length / 2);

			left = (make_shared<bvh_node>(objs, start, mid));
			right = (make_shared<bvh_node>(objs, mid, end));
		}

		bbox = aabb(left->bounding_box(), right->bounding_box());
		
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		if (!bbox.hit(r, ray_t))
			return false;

		bool hit_left = left->hit(r, ray_t, rec);
		bool hit_right = right->hit(r, interval(ray_t.min, hit_left ? rec.t : ray_t.max), rec);

		return hit_left || hit_right;
	}

	aabb bounding_box() const override {
		return bbox;
	}

private:
	shared_ptr<hittable> left,right;
	aabb bbox;

	static bool box_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b, int axis_index) {
		return a->bounding_box().axis(axis_index).min < b->bounding_box().axis(axis_index).min;
	}

	static bool box_x_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
		return box_compare(a, b, 0);
	}

	static bool box_y_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
		return box_compare(a, b, 1);
	}

	static bool box_z_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
		return box_compare(a, b, 2);
	}
};
