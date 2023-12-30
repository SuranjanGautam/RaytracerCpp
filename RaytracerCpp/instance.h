#pragma once

#include "general.h"
#include "hittable.h"
#include "mat4.h";

class instance : public hittable {
public:
	instance(shared_ptr<hittable>_obj, vec3 translation, vec3 rotation) :obj(_obj) {
		 
		translationmat = mat4::translation(translation);
		rotationmat = mat4::rotation(rotation);
		scalenmat = mat4::scale(vec3(1,1,1)); //implement scaling later

		transformationmat = scalenmat * rotationmat * translationmat;
		
		invtranslationmat = translationmat.inverse();
		invrotationmat = rotationmat.inverse();
		invscalenmat = scalenmat.inverse();	
		
		invtransformationmat = transformationmat.inverse();

		bbox = obj->bounding_box().transform(transformationmat);
	};

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override
	{
		auto o = toVec3(vec4(r.origin()) * invtransformationmat);
		auto dir = toVec3(vec4(r.direction()) * invrotationmat);
		auto new_ray = ray(o, dir);

		/*auto near = r.at(ray_t.min);
		auto far = r.at(ray_t.max);

		near = toVec3(vec4(near) * invtranslationmat);
		far = toVec3(vec4(far) * invtranslationmat);

		ray_t.min = dot(near - new_ray.origin(), new_ray.direction()) / dot(new_ray.direction(), new_ray.direction());
		ray_t.max = dot(far - new_ray.origin(), new_ray.direction()) / dot(new_ray.direction(), new_ray.direction());*/
		
		if (!obj->hit(new_ray, ray_t, rec))
			return false;

		rec.p = toVec3(transformationmat * vec4(rec.p));
		//rec.t = dot(rec.p - r.origin(), r.direction()) / dot(r.direction(), r.direction());
		rec.set_face_normal(r, toVec3(vec4(rec.normal) * rotationmat));

		return true;
	}

	aabb bounding_box() const override {
		return bbox;
	}

private:
	shared_ptr<hittable> obj;
	aabb bbox;
	mat4 translationmat;
	mat4 rotationmat;
	mat4 scalenmat;
	mat4 invtranslationmat;
	mat4 invrotationmat;
	mat4 invscalenmat;

	mat4 transformationmat;
	mat4 invtransformationmat;

};