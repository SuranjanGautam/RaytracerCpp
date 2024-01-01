#pragma once

#include "general.h"

#include "hittable.h"
#include "hittablelist.h"
#include <algorithm>
#include <future>
#include <thread>

class bvh_node : public hittable {
public:
	bvh_node(const hittable_list& list) {
		int _count = 0;
		int* countptr = &_count;
		*this = bvh_node(list.objects, 0, list.objects.size(), countptr);
	};

	bvh_node(const std::vector<shared_ptr<hittable>>& src_objects, size_t start, size_t end,int* count) {
		
		vector<shared_ptr<hittable>> objs(src_objects.begin() + start, src_objects.begin() + end);
		
		start = 0;
		end = objs.size();

		if (objs.size() == 0) return;

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

			std::sort(objs.begin() , objs.end(), comparer);
			
			

			auto mid = start + (object_length / 2);
			
			
			if (*count < 6)
			{
				*count++;
				shared_ptr<hittable>* leftptr = &left;
				shared_ptr<hittable>* rightptr = &right;
				std::vector<shared_ptr<hittable>>* objsptr = &objs;

				auto lthread = std::thread([&, start, mid] {
					*leftptr = make_shared<bvh_node>(*objsptr, start, mid, count);
					});
				auto rthread = std::thread([&, end, mid] {
					*rightptr = make_shared<bvh_node>(*objsptr, mid, end, count);
					});

				lthread.join();
				rthread.join();
			}
			else
			{

				left = (make_shared<bvh_node>(objs, start, mid,count));
				right = (make_shared<bvh_node>(objs, mid, end,count));
			}
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



