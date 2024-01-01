#pragma once

#include "general.h"
#include "hittablelist.h"
#include "triangle.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <filesystem>
using namespace std;


static vector<string> split(string s, char c) {
	stringstream ss(s);
	vector<string> temp;
	string st;
	while (getline(ss, st, c)) {
		temp.push_back(st);
	}
	return temp;
}
static shared_ptr<hittable_list> LoadMesh(string path, shared_ptr<material> mat)
{
	shared_ptr<hittable_list> mesh = make_shared<hittable_list>();
	vector<shared_ptr<vertex>> vertices;
	vector<int> indices;

	string extension = path.substr(path.length() - 4, 4);
	if (extension != ".obj") {

		cout << "Couldnt load mesh!!";
		return mesh;
	}
	else {

		vector<vec3> pos;
		vector <vec3> tex;
		vector <vec3> norm;


		fstream in;
		in.open(path.c_str());


		string line;
		vector<string> tokens;
		while (in) {
			getline(in, line);
			tokens = split(line, ' ');
			if (line.length() == 0) { break; }
			if (tokens[0] == "#") {
				continue;
			}
			else if (tokens[0] == "v") {				
				auto x = stod(tokens[1]);
				auto y = stod(tokens[2]);
				auto z = stod(tokens[3]);
				pos.push_back(vec3(x, y, z));
			}
			else if (tokens[0] == "vt") {
				
				auto x = stod(tokens[1]);
				auto y = stod(tokens[2]);
				tex.push_back(vec3(x, y,0));
			}
			else if (tokens[0] == "vn") {				
				auto x = stod(tokens[1]);
				auto y = stod(tokens[2]);
				auto z = stod(tokens[3]);
				norm.push_back(vec3(x, y, z));
			}
			else if (tokens[0] == "f") {				
				vector<string> t;
				for (string x : tokens) {
					
					if (x == "f") { continue; }

					
					t = split(x, '/');
					auto vtemp = make_shared<vertex>();
					vtemp->position = pos[stoi(t[0]) - 1];

					if (t.size() > 1) {
						vtemp->u = tex[stoi(t[1]) - 1][0];
						vtemp->v = tex[stoi(t[1]) - 1][1];
					}
					if (t.size() > 2)
						vtemp->normal = norm[stoi(t[2]) - 1];

					vertices.push_back(vtemp);
					indices.push_back(vertices.size() - 1);
					t.clear();
				}
				if (tokens.size() == 5)
				{
					int end = vertices.size() - 1;
					indices.push_back(end - 3);
					indices.push_back(end - 1);
				}				

			}
			else {
				continue;
			}
		}
	}

	for (size_t i = 0; i < indices.size(); i += 3)
	{
		auto tri = make_shared<triangle>(vertices[indices[i]], vertices[indices[i+1]], vertices[indices[i+2]], mat);
		mesh->add(tri);
	}
	
	if (mesh->objects.size() == 0)
	{
		throw std::runtime_error("Failed loading mesh");
	}
	
	return mesh;
}

