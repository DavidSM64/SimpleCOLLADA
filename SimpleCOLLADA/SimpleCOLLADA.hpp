#pragma once
#ifndef RAPIDXML_LOCATION
#define RAPIDXML_LOCATION "rapidxml.hpp"
#endif
#include RAPIDXML_LOCATION
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>
using namespace std;
using namespace rapidxml;

namespace SimpleCOLLADA {
	/* Macros */
	#define XML_ATTRIBUTE_FOR_LOOP(node) xml_attribute<>* attribute = node->first_attribute(); \
		 attribute; attribute = attribute->next_attribute()
	#define XML_NODE_CHILD_FOR_LOOP(node) xml_node<> *child = node->first_node(); \
		 child; child = child->next_sibling()
	#define EXISTS(node) node != NULL
	#define NOT_FOUND -1
	#define FOUND(i) i != NOT_FOUND
	#define FOUND_XYZ(x,y,z) (FOUND(x) && FOUND(y) && FOUND(z))
	#define FOUND_ST(s, t) (FOUND(s) && FOUND(t))
	#define MAX_ABC(a,b,c) (a > b ? (a > c ? a : c) : (b > c ? b : c))
	#define BYTES_TO_UINT(a,b,c,d) ((a << 24) | (b << 16) | (c << 8) | d)
	#define ID_SUBSTR(str) string(str).substr(1,string(str).length()-1)
	#define ERROR_MSG(str) cerr << str << endl;
	#define ERROR_MSG_NO_FIND(str) cerr << "Could not find: \"" << str << "\"" << endl;
	#define FLAG(str) cout << "FLAG " << str << endl;

	typedef unsigned char u8;
	typedef unsigned short u16;
	typedef unsigned int u32;
	typedef signed char s8;
	typedef signed short s16;
	typedef signed int s32;

	enum UP_AXIS { 
		X_UP, Y_UP, Z_UP, INVALID, NO_FIND 
	};

	typedef struct _Triangle {
		long position[3] = { -1, -1, -1 },
			normal[3] = { -1, -1, -1 },
			uv[3] = { -1, -1, -1 },
			color[3] = { -1, -1, -1 };
	} Triangle;

	typedef struct _Vertex {
		float x, y, z;
	} Vertex;

	typedef struct _TextureCoord {
		float u, v;
	} TextureCoord;

	typedef struct _VertexColor {
		float r, g, b, a = 1.0;
	} VertexColor;

	typedef struct _Normal {
		float nx, ny, nz;
	} Normal;

	class Material {
	private:
		string name;
		string filename;
		float transparency = 1.0;
		u32 color = 0xFFFFFFFF;
	public:
		inline string getName() { return name; };
		inline string getFileName() { return filename; };
		inline float getTransparency() { return transparency; };
		inline u32 getColor() { return color; };
		inline void setName(string n) { name = n; };
		inline void setFileName(string fn) { filename = fn; };
		inline void setTransparency(float transp) { transparency = transp; };
		inline void setColor(u32 col) { color = col; };
	};

	class ModelNode {
	private:
		vector<Triangle*> triangles;
		vector<Vertex*> vertices;
		vector<TextureCoord*> texuvs;
		vector<Normal*> normals;
		unordered_map<string, vector<VertexColor*>> vertexColorMap;
		Material* material;
	public:
		inline void addTriangle(Triangle* tri) { triangles.push_back(tri); }
		inline void addVertex(Vertex* vert) { vertices.push_back(vert); }
		inline void addTextureCoord(TextureCoord* tc) { texuvs.push_back(tc); }
		inline void addVertexColor(VertexColor* vc, string group) { vertexColorMap[group].push_back(vc); }
		inline void addNormal(Normal* nrm) { normals.push_back(nrm); }
		inline void resizeNormals(size_t newSize) { normals.resize(newSize); }
		inline void setMaterial(Material* mat) { material = mat; }
		inline Triangle* getTriangle(int index) { return triangles[index]; }
		inline Vertex* getVertex(int index) { return vertices[index]; }
		inline TextureCoord* getTextureCoord(int index) { return texuvs[index]; }
		inline Normal* getNormal(int index) { return normals[index]; }
		inline VertexColor* getVertexColor(string group, int index) { 
			if(vertexColorMap.find(group) != vertexColorMap.end())
				if(index < vertexColorMap[group].size())
					return vertexColorMap[group][index];
			return NULL;
		}
		inline Material* getMaterial() { return material; }
		inline size_t getNumOfTriangles() { return triangles.size(); };
		inline size_t getNumOfVertices() { return vertices.size(); };
		inline size_t getNumOfTexCoords() { return texuvs.size(); };
		inline size_t getNumOfNormals() { return normals.size(); };
		inline size_t getNumOfVertexColorGroups() { return vertexColorMap.size(); };
		inline size_t getNumOfVertexColors(string group) { return vertexColorMap[group].size(); };
		inline string getFirstVertexColorGroupName() {
			if(vertexColorMap.size() > 0) return vertexColorMap.begin()->first;
			else return "";
		}
		inline vector<string> getVertexColorGroupNames() { 
			vector<string> keys;
			for (auto it = vertexColorMap.begin(); it != vertexColorMap.end(); ++it)
				keys.push_back(it->first);
			return keys;
		};
		inline ~ModelNode() { // destructor
			for (size_t i = 0; i < triangles.size(); i++) delete triangles[i];
			for (size_t i = 0; i < vertices.size(); i++) delete vertices[i];
			for (size_t i = 0; i < texuvs.size(); i++) delete texuvs[i];
			for (size_t i = 0; i < normals.size(); i++) delete normals[i];
			for (auto it = vertexColorMap.begin(); it != vertexColorMap.end(); ++it)
				for (size_t i = 0; i < it->second.size(); i++) 
					delete it->second[i];
		}
	};

	class Model {
	private:
		unordered_map<string, xml_node<>*> lib_visuals;
		unordered_map<string, xml_node<>*> lib_geometries;
		unordered_map<string, xml_node<>*> lib_materials;
		unordered_map<string, xml_node<>*> lib_effects;
		unordered_map<string, xml_node<>*> lib_images;
		unordered_map<string, xml_node<>*> materialSymbolTargetMap;


		unordered_map<string, Material*> materialIdMap;

		vector<float> parse_float_vector(string str) {
			vector<float> result;
			istringstream iss(str);
			char* pEnd;
			for (string float_str; iss >> float_str;) {
				result.push_back(strtof(float_str.c_str(), &pEnd));
			}
			return result;
		}

		vector<u32> parse_u32_vector(string str) {
			vector<u32> result;
			istringstream iss(str);
			char* pEnd;
			for (string u32_str; iss >> u32_str;) {
				result.push_back(strtol(u32_str.c_str(), &pEnd, 10));
			}
			return result;
		}

		xml_attribute<>* findAttribute(xml_node<>* node, string name) {
			if (node->first_attribute() == NULL)
				return NULL;
			for (XML_ATTRIBUTE_FOR_LOOP(node))
				if (string(attribute->name()) == name)
					return attribute;
			return NULL;
		}

		void buildLocalIdMap(unordered_map<string, xml_node<>*> &map, xml_node<> *root, string id_pattern) {
			xml_attribute<>* id_attr = findAttribute(root, id_pattern);
			if (EXISTS(id_attr)) {
				map[string(id_attr->value())] = root;
				//cout << "Added " << string(id_attr->value()) << " to local ID Map" << endl;
			}
			if (root->first_node() != 0) {
				for (XML_NODE_CHILD_FOR_LOOP(root)) {
					buildLocalIdMap(map, child, id_pattern);
				}
			}
		}

		typedef struct _accessor_param {
			string name;
			string type;
		} Accessor_param;

		typedef struct _mesh_source {
			string name;
			size_t count, stride, error = 0;
			vector<float> float_array;
			vector<Accessor_param> params;
		} Mesh_source;

		vector<Accessor_param> parse_accessor_params(xml_node<> *accessor) {
			vector<Accessor_param> params;
			for (XML_NODE_CHILD_FOR_LOOP(accessor)) {
				if (string(child->name()) == "param") {
					xml_attribute<>* nameAttr = findAttribute(child, "name");
					xml_attribute<>* typeAttr = findAttribute(child, "type");
					if (EXISTS(nameAttr) && EXISTS(typeAttr)) {
						Accessor_param newParam;
						newParam.name = nameAttr->value();
						newParam.type = typeAttr->value();
						params.push_back(newParam);
					}
				}
			}
			return params;
		}

		int getParamOffset(string paramName, vector<Accessor_param> params) {
			for (size_t i = 0; i < params.size(); i++)
				if (params[i].name == paramName) 
					return i;
			return NOT_FOUND;
		}

		Mesh_source parse_source(xml_node<> *source) {
			Mesh_source mesh_source;
			xml_attribute<>* srcNameAttr = findAttribute(source, "name");
			if (EXISTS(srcNameAttr))
				mesh_source.name = string(srcNameAttr->value());
			xml_node<> *common = source->first_node("technique_common");
			if (EXISTS(common)) {
				xml_node<> *accessor = common->first_node("accessor");
				if (EXISTS(accessor)) {
					xml_attribute<>* cntAttr = findAttribute(accessor, "count");
					xml_attribute<>* strAttr = findAttribute(accessor, "stride");
					xml_attribute<>* srcAttr = findAttribute(accessor, "source");
					if (EXISTS(cntAttr) && EXISTS(strAttr) && EXISTS(srcAttr)) {
						xml_node<> *float_arr = lib_geometries[ID_SUBSTR(srcAttr->value())];
						if (EXISTS(float_arr)) {
							mesh_source.params = parse_accessor_params(accessor);
							mesh_source.float_array = parse_float_vector(float_arr->value());
							mesh_source.stride = stoi(string(strAttr->value()));
							mesh_source.count = stoi(string(cntAttr->value()));
						} else {
							mesh_source.error = 3;
							ERROR_MSG_NO_FIND("float_array");
						}
					}
				} else {
					mesh_source.error = 2;
					ERROR_MSG_NO_FIND("accessor");
				}
			} else {
				mesh_source.error = 1;
				ERROR_MSG_NO_FIND("technique_common");
			}
			return mesh_source;
		}

		void parse_geo_normals(xml_node<> *source, ModelNode* model) {
			Mesh_source src = parse_source(source);
			if (!src.error) { // If no errors had occured...
				int xOffset = getParamOffset("X", src.params);
				int yOffset = getParamOffset("Y", src.params);
				int zOffset = getParamOffset("Z", src.params);
				for (size_t i = 0; i < src.count; i++) {
					Normal* n = new Normal();
					n->nx = src.float_array[i * 3 + xOffset];
					n->ny = src.float_array[i * 3 + yOffset];
					n->nz = src.float_array[i * 3 + zOffset];
					model->addNormal(n);
				}
			}
		}

		void parse_geo_positions(xml_node<> *source, ModelNode* model) {
			Mesh_source src = parse_source(source);
			if (!src.error) { // If no errors had occured...
				int xOffset = getParamOffset("X", src.params);
				int yOffset = getParamOffset("Y", src.params);
				int zOffset = getParamOffset("Z", src.params);
				if (FOUND_XYZ(xOffset, yOffset, zOffset)) {
					for (size_t i = 0; i < src.count; i++) {
						Vertex* v = new Vertex;
						v->x = src.float_array[i * src.stride + xOffset];
						v->y = src.float_array[i * src.stride + yOffset];
						v->z = src.float_array[i * src.stride + zOffset];
						model->addVertex(v);
					}
				}
			}
		}

		void parse_geo_texCoords(xml_node<> *source, ModelNode* model) {
			Mesh_source src = parse_source(source);
			if (!src.error) { // If no errors had occured...
				int sOffset = getParamOffset("S", src.params);
				int tOffset = getParamOffset("T", src.params);
				if (FOUND_ST(sOffset, tOffset)) {
					for (size_t i = 0; i < src.count; i++) {
						TextureCoord* tc = new TextureCoord;
						tc->u = src.float_array[i * src.stride + sOffset];
						tc->v = src.float_array[i * src.stride + tOffset];
						model->addTextureCoord(tc);
					}
				}
			}
		}

		void parse_geo_rgb(xml_node<> *source, ModelNode* model) {
			Mesh_source src = parse_source(source);
			if (src.stride < 3) return;
			if (!src.error) { // If no errors had occured...
				int rOffset = getParamOffset("R", src.params);
				int gOffset = getParamOffset("G", src.params);
				int bOffset = getParamOffset("B", src.params);
				int aOffset = getParamOffset("A", src.params);
				if (FOUND_XYZ(rOffset, gOffset, bOffset)) {
					for (size_t i = 0; i < src.count; i++) {
						VertexColor* vc = new VertexColor;
						vc->r = src.float_array[i * src.stride + rOffset];
						vc->g = src.float_array[i * src.stride + gOffset];
						vc->b = src.float_array[i * src.stride + bOffset];
						if (FOUND(aOffset) && src.stride > 3) 
							vc->a = src.float_array[i * src.stride + aOffset];
						model->addVertexColor(vc, src.name);
					}
				}
			}
		}

		void parse_geo_vertices(xml_node<> *vertices, ModelNode* model) {
			for (XML_NODE_CHILD_FOR_LOOP(vertices)) {
				if (string(child->name()) == "input") {
					xml_attribute<>* semAttr = findAttribute(child, "semantic");
					xml_attribute<>* srcAttr = findAttribute(child, "source");
					if (EXISTS(semAttr) && EXISTS(srcAttr)) {
						//cout << "semantic = " << semAttr->value() << endl;
						//cout << "source = " << ID_SUBSTR(srcAttr->value()) << endl;
						if (string(semAttr->value()) == "POSITION")
							parse_geo_positions(lib_geometries[ID_SUBSTR(srcAttr->value())], model);
						else if (string(semAttr->value()) == "NORMAL")
							parse_geo_normals(lib_geometries[ID_SUBSTR(srcAttr->value())], model);
					}
				}
			}
		}

		typedef struct _semantics_offsets {
			int vertexOffset=-1, normalOffset=-1, texCoordOffset=-1, colorOffset=-1;
		} Semantics_offsets;

		void parse_triangles_inputs(xml_node<> *triangles, ModelNode* model, Semantics_offsets& offsets) {
			for (XML_NODE_CHILD_FOR_LOOP(triangles)) {
				if (string(child->name()) == "input") {
					xml_attribute<>* semAttr = findAttribute(child, "semantic");
					xml_attribute<>* srcAttr = findAttribute(child, "source");
					xml_attribute<>* offAttr = findAttribute(child, "offset");
					xml_attribute<>* setAttr = findAttribute(child, "set");
					if (EXISTS(semAttr) && EXISTS(srcAttr)) {
						if (string(semAttr->value()) == "VERTEX") {
							parse_geo_vertices(lib_geometries[ID_SUBSTR(srcAttr->value())], model);
							offsets.vertexOffset = stoi(string(offAttr->value()));
						} else if (string(semAttr->value()) == "NORMAL") {
							parse_geo_normals(lib_geometries[ID_SUBSTR(srcAttr->value())], model);
							offsets.normalOffset = stoi(string(offAttr->value()));
						} else if (string(semAttr->value()) == "TEXCOORD") {
							offsets.texCoordOffset = stoi(string(offAttr->value()));
							xml_attribute<>* setAttr = findAttribute(child, "set");
							if (EXISTS(setAttr)) {
								if (string(setAttr->value()) == "0")
									parse_geo_texCoords(lib_geometries[ID_SUBSTR(srcAttr->value())], model);
							} else {
								parse_geo_texCoords(lib_geometries[ID_SUBSTR(srcAttr->value())], model);
							}
						}
						else if (string(semAttr->value()) == "COLOR") {
							if (string(setAttr->value()) == "0")
								offsets.colorOffset = stoi(string(offAttr->value()));
							parse_geo_rgb(lib_geometries[ID_SUBSTR(srcAttr->value())], model);
						}
					}
				}
			}
		}

		void parse_triangles(xml_node<> *triangles, ModelNode* model) {
			Semantics_offsets offset_pos;
			xml_node<>* indices = triangles->first_node("p");
			xml_attribute<>* countAttr = findAttribute(triangles, "count");
			if (EXISTS(indices) && EXISTS(countAttr)) {
				bool normalsAreInVertices = false;
				u32 tri_count = stol(string(countAttr->value()));
				parse_triangles_inputs(triangles, model, offset_pos);
				vector<u32> index_list = parse_u32_vector(string(indices->value()));
				u32 stride = index_list.size() / (tri_count * 3);
				if (offset_pos.normalOffset < 1 && model->getNumOfNormals() > 0)
					normalsAreInVertices = true;
				bool hasVerts =	offset_pos.vertexOffset > -1;
				bool hasTexCoords = offset_pos.texCoordOffset > -1;
				bool hasNormals = offset_pos.normalOffset > -1;
				bool hasColors = offset_pos.colorOffset > -1;
				for (size_t i = 0; i < tri_count; i++) {
					Triangle* tri = new Triangle();
					for (size_t j = 0; j < 3; j++) {
						if (hasVerts)
							tri->position[j] = index_list[(i*stride*3) + (j*stride) + offset_pos.vertexOffset];
						if (hasTexCoords)
							tri->uv[j] = index_list[(i*stride*3) + (j*stride) + offset_pos.texCoordOffset];
						if (hasColors)
							tri->color[j] = index_list[(i*stride*3) + (j*stride) + offset_pos.colorOffset];
						if (normalsAreInVertices)
							tri->normal[j] = index_list[(i*stride*3) + (j*stride) + offset_pos.vertexOffset];
						else if (hasNormals) 
							tri->normal[j] = index_list[(i*stride*3) + (j*stride) + offset_pos.normalOffset];
					}
					model->addTriangle(tri);
				}
			}
			else {
				ERROR_MSG_NO_FIND("p");
			}
		}

		bool checkPolylistIsTriangulated(xml_node<> *polylist) {
			xml_node<> *vcount = polylist->first_node("vcount");
			if (EXISTS(vcount)) {
				vector<u32> vcount_arr = parse_u32_vector(string(vcount->value()));
				for (size_t i = 0; i < vcount_arr.size(); i++)
					if (vcount_arr[i] != 3)
						return false;
				return true;
			}
			return false;
		}

		void recursivelyFindAllInstanceGeometryNodes(vector<xml_node<>*>& nodes, xml_node<> *current) {
			if (string(current->name()) == "instance_geometry")
				nodes.push_back(current);
			else if (string(current->name()) == "instance_node") {
				xml_attribute<>* urlAttr = findAttribute(current, "url");
				if(EXISTS(urlAttr))
					recursivelyFindAllInstanceGeometryNodes(nodes, lib_visuals[ID_SUBSTR(urlAttr->value())]);
			}
			else if (current->first_node() != 0)
				for (XML_NODE_CHILD_FOR_LOOP(current))
					recursivelyFindAllInstanceGeometryNodes(nodes, child);
		}

		int doesMaterialNameAlreadyExist(string name) {
			for (size_t i = 0; i < materials.size(); i++) {
				if (materials[i]->getName() == name)
					return (int)i;
			}
			return NOT_FOUND;
		}

		Material* parse_new_material(xml_node<> *mat_node) {
			if (!EXISTS(mat_node)) {
				ERROR_MSG("Material node is null!")
				return NULL;
			}
			Material* mat = new Material;
			xml_attribute<>* nameAttr = findAttribute(mat_node, "name");
			if (EXISTS(nameAttr))
				mat->setName(string(nameAttr->value()));
			int findMaterial = doesMaterialNameAlreadyExist(mat->getName());
			if (FOUND(findMaterial)) {
				delete mat;
				return materials[findMaterial];
			}
			xml_node<> *instance_effect = mat_node->first_node("instance_effect");
			if (EXISTS(instance_effect)) {
				xml_attribute<>* urlAttr = findAttribute(instance_effect, "url");
				if (EXISTS(urlAttr)) {
					xml_node<>* effect = lib_effects[ID_SUBSTR(urlAttr->value())];
					if (EXISTS(effect)) {
						xml_node<>* profile_COMMON = effect->first_node("profile_COMMON");
						unordered_map<string, xml_node<>*> localSidMap;
						buildLocalIdMap(localSidMap, profile_COMMON, "sid");
						xml_node<>* technique = profile_COMMON->first_node("technique");
						if (EXISTS(technique) && EXISTS(technique->first_node())) {
							xml_node<>* diffuse = technique->first_node()->first_node("diffuse");
							xml_node<>* transparent = technique->first_node()->first_node("transparent");
							//cout << diffuse->first_node()->name() << endl;
							if (EXISTS(diffuse)) {
								xml_node<>* texture = diffuse->first_node("texture");
								xml_node<>* color = diffuse->first_node("color");
								if (EXISTS(texture)) {
									xml_attribute<>* urlTexAttr = findAttribute(texture, "texture");
									if (EXISTS(urlTexAttr)) {
										xml_node<>* sampler2D = localSidMap[string(urlTexAttr->value())]->first_node("sampler2D");
										if (EXISTS(sampler2D) && EXISTS(sampler2D->first_node("source"))) {
											xml_node<>* surface = localSidMap[string(sampler2D->first_node("source")->value())]->first_node("surface");
											if (EXISTS(surface)) {
												xml_node<>* init_from = surface->first_node("init_from");
												if (EXISTS(init_from)) {
													xml_node<>* image = lib_images[string(init_from->value())];
													if (EXISTS(image)) {
														// All of this is here to get the texture's filename. This is the beauty of COLLADA.
														mat->setFileName(string(image->first_node("init_from")->value()));
													} else ERROR_MSG_NO_FIND("image");
												} else ERROR_MSG_NO_FIND("init_from");
											} else ERROR_MSG_NO_FIND("surface");
										} else ERROR_MSG_NO_FIND("sampler2D");
									} else ERROR_MSG_NO_FIND("urlTexAttr");
								} else if (EXISTS(color)) {
									vector<float> col_data = parse_float_vector(string(color->value()));
									u8 r = (u8)(col_data[0] * 255.0), g = (u8)(col_data[1] * 255.0), b = (u8)(col_data[2] * 255.0);
									mat->setColor(BYTES_TO_UINT(r,g,b,0xFF));
								}
							}
							if (EXISTS(transparent)) { // Get transparency data
								xml_attribute<>* opaqueAttr = findAttribute(transparent, "opaque");
								if (string(opaqueAttr->value()) == "A_ONE") {
									xml_node<> *color = transparent->first_node("color");
									if (EXISTS(color)) {
										vector<float> trans_data = parse_float_vector(string(color->value()));
										mat->setTransparency(trans_data[3]);
									}
								} else if (string(opaqueAttr->value()) == "RGB_ZERO") {
									xml_node<> *color = transparent->first_node("color");
									if (EXISTS(color)) {
										vector<float> trans_data = parse_float_vector(string(color->value()));
										mat->setTransparency(MAX_ABC(trans_data[0], trans_data[1], trans_data[2]));
									}
								}
							}
						} else ERROR_MSG_NO_FIND("technique");
					} else ERROR_MSG_NO_FIND("effect");
				} else ERROR_MSG_NO_FIND("urlAttr");
			} else ERROR_MSG_NO_FIND("instance_effect");
			materials.push_back(mat);
			return mat;
		}

		void parse_geometry(xml_node<> *geometry) {
			xml_node<> *mesh = geometry->first_node("mesh");
			if (EXISTS(mesh)) {
				for (XML_NODE_CHILD_FOR_LOOP(mesh)) {
					if (string(child->name()) == "triangles") { // Sketchup's approach
						ModelNode* model = new ModelNode();
						xml_attribute<>* matAttr = findAttribute(child, "material");
						if (EXISTS(matAttr)) {
							model->setMaterial(parse_new_material(materialSymbolTargetMap[string(matAttr->value())]));
						}
						parse_triangles(child, model);
						modelNodes.push_back(model);
					}
					else if (string(child->name()) == "polylist") { // Blender's approach
						if (checkPolylistIsTriangulated(child)) {
							ModelNode* model = new ModelNode();
							xml_attribute<>* matAttr = findAttribute(child, "material");
							if (EXISTS(matAttr)) {
								model->setMaterial(parse_new_material(materialSymbolTargetMap[string(matAttr->value())]));
							}
							parse_triangles(child, model);
							modelNodes.push_back(model);
						}
						else {
							ERROR_MSG("Error: Mesh is not triangulated!");
						}
					}
				}
			}
			else {
				ERROR_MSG_NO_FIND("mesh");
			}
		}
		
		void parse_geo_material(xml_node<> *geonode) {
			xml_node<> *bind_material = geonode->first_node("bind_material");
			if (EXISTS(bind_material)) {
				xml_node<> *technique_common = bind_material->first_node("technique_common");
				if (EXISTS(technique_common)) {
					for (XML_NODE_CHILD_FOR_LOOP(technique_common)) {
						if (string(child->name()) == "instance_material") {
							xml_attribute<>* mat_tarAttr = findAttribute(child, "target");
							xml_attribute<>* mat_symAttr = findAttribute(child, "symbol");
							if (EXISTS(mat_tarAttr) && EXISTS(mat_symAttr)) {
								materialSymbolTargetMap[string(mat_symAttr->value())] = lib_materials[ID_SUBSTR(mat_tarAttr->value())];
							}
						}
					}
				} else ERROR_MSG_NO_FIND("technique_common");
			} else ERROR_MSG_NO_FIND("bind_material");
		}

		void parse_scene(xml_node<> *scene) {
			if (EXISTS(scene)) {
				xml_node<> *ins = scene->first_node("instance_visual_scene");
				if (EXISTS(ins)) {
					xml_attribute<>* urlAttr = findAttribute(ins, "url");
					if (EXISTS(urlAttr)) {
						vector<xml_node<>*> geonodes;
						recursivelyFindAllInstanceGeometryNodes(geonodes, lib_visuals[ID_SUBSTR(urlAttr->value())]);
						if (geonodes.size() > 0) {
							for (size_t i = 0; i < geonodes.size(); i++) {
								parse_geo_material(geonodes[i]);
								xml_attribute<>* geo_urlAttr = findAttribute(geonodes[i], "url");
								if (EXISTS(geo_urlAttr)) {
									parse_geometry(lib_geometries[ID_SUBSTR(geo_urlAttr->value())]);
								}
							}
						}
					}
				} else {
					ERROR_MSG_NO_FIND("instance_visual_scene");
				}
			}
			else {
				ERROR_MSG("Error: Scene node is NULL!");
			}
		}

		void buildLibraryMaps(xml_node<>* COLLADA) {
			xml_node<>* library_visual_scenes = COLLADA->first_node("library_visual_scenes");
			xml_node<>* library_geometries = COLLADA->first_node("library_geometries");
			xml_node<>* library_materials = COLLADA->first_node("library_materials");
			xml_node<>* library_effects = COLLADA->first_node("library_effects");
			xml_node<>* library_images = COLLADA->first_node("library_images");
			if (EXISTS(library_visual_scenes)) buildLocalIdMap(lib_visuals, library_visual_scenes, "id");
			if (EXISTS(library_geometries)) buildLocalIdMap(lib_geometries, library_geometries, "id");
			if (EXISTS(library_materials)) buildLocalIdMap(lib_materials, library_materials, "id");
			if (EXISTS(library_effects)) buildLocalIdMap(lib_effects, library_effects, "id");
			if (EXISTS(library_images)) buildLocalIdMap(lib_images, library_images, "id");
		}

		int getUpAxis(xml_node<>* COLLADA) {
			xml_node<> *asset = COLLADA->first_node("asset");
			if (EXISTS(asset)) {
				xml_node<> *up_axis = asset->first_node("up_axis");
				if (EXISTS(up_axis)) {
					string val = string(up_axis->value());
					if (val == "X_UP")		return 0; // X_UP
					else if (val == "Y_UP") return 1; // Y_UP
					else if (val == "Z_UP") return 2; // Z_UP
					else					return 3; // INVALID
				}
			}
			return 4; // NO_FIND
		}

	public:
		vector<ModelNode*> modelNodes;
		vector<Material*> materials;
		UP_AXIS upAxis;
		Model(string filename) {
			ifstream infile;
			infile.open(filename);
			stringstream strStream;
			strStream << infile.rdbuf();
			xml_document<> doc;
			string name = strStream.str();
			doc.parse<0>((char*)name.c_str());
			if (string(doc.first_node()->name()) == "COLLADA") {
				buildLibraryMaps(doc.first_node());
				parse_scene(doc.first_node()->first_node("scene"));
				upAxis = (UP_AXIS)getUpAxis(doc.first_node());
			}
			infile.close();
		}
		~Model() { // destructor
			for (size_t i = 0; i < modelNodes.size(); i++) 
				delete modelNodes[i];
			for (size_t i = 0; i < materials.size(); i++)
				delete materials[i];
		}
	};

}
