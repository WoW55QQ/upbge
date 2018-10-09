#include "KX_MeshBuilder.h"
#include "KX_VertexProxy.h"
#include "KX_BlenderMaterial.h"
#include "KX_Scene.h"
#include "KX_KetsjiEngine.h"
#include "KX_Globals.h"
#include "KX_PyMath.h"

#include "EXP_ListWrapper.h"

#include "BL_Converter.h"

#include "RAS_BucketManager.h"

#include "BLI_math_vector.h"
#include "BLI_math_geom.h"

KX_MeshBuilderSlot::KX_MeshBuilderSlot(KX_BlenderMaterial *material, RAS_DisplayArray::PrimitiveType primitiveType,
		const RAS_DisplayArray::Format& format, unsigned int& origIndexCounter)
	:m_material(material),
	m_array(new RAS_DisplayArray(primitiveType, format)),
	m_origIndexCounter(origIndexCounter)
{
}

KX_MeshBuilderSlot::KX_MeshBuilderSlot(RAS_MeshMaterial *meshmat, const RAS_DisplayArray::Format& format,
		unsigned int& origIndexCounter)
	:m_origIndexCounter(origIndexCounter)
{
	RAS_MaterialBucket *bucket = meshmat->GetBucket();
	m_material = static_cast<KX_BlenderMaterial *>(bucket->GetMaterial());

	RAS_DisplayArray *array = meshmat->GetDisplayArray();
	BLI_assert(array->GetType() == RAS_DisplayArray::NORMAL);
	m_array = new RAS_DisplayArray(*array);

	// Compute the maximum original index from the arrays.
	m_origIndexCounter = std::max(m_origIndexCounter, m_array->GetMaxOrigIndex());
}

KX_MeshBuilderSlot::~KX_MeshBuilderSlot()
{
}

std::string KX_MeshBuilderSlot::GetName()
{
	return m_material->GetName().substr(2);
}

KX_BlenderMaterial *KX_MeshBuilderSlot::GetMaterial() const
{
	return m_material;
}

void KX_MeshBuilderSlot::SetMaterial(KX_BlenderMaterial *material)
{
	m_material = material;
}

bool KX_MeshBuilderSlot::Invalid() const
{
	// WARNING: Always respect the order from RAS_DisplayArray::PrimitiveType.
	static const unsigned short itemsCount[] = {
		3, // TRIANGLES
		2 // LINES
	};

	const unsigned short count = itemsCount[m_array->GetPrimitiveType()];
	return ((m_array->GetPrimitiveIndexCount() % count) != 0 || (m_array->GetTriangleIndexCount() % count) != 0);
}

RAS_DisplayArray *KX_MeshBuilderSlot::GetDisplayArray() const
{
	return m_array;
}

PyTypeObject KX_MeshBuilderSlot::Type = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"KX_MeshBuilderSlot",
	sizeof(EXP_PyObjectPlus_Proxy),
	0,
	py_base_dealloc,
	0,
	0,
	0,
	0,
	py_base_repr,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	0, 0, 0, 0, 0, 0, 0,
	Methods,
	0,
	0,
	&EXP_Value::Type,
	0, 0, 0, 0, 0, 0,
	py_base_new
};

PyMethodDef KX_MeshBuilderSlot::Methods[] = {
	{"addVertex", (PyCFunction)KX_MeshBuilderSlot::sPyAddVertex, METH_VARARGS | METH_KEYWORDS},
	{"addIndex", (PyCFunction)KX_MeshBuilderSlot::sPyAddIndex, METH_O},
	{"removeVertex", (PyCFunction)KX_MeshBuilderSlot::sPyRemoveVertex, METH_VARARGS},
	{"addPrimitiveIndex", (PyCFunction)KX_MeshBuilderSlot::sPyAddPrimitiveIndex, METH_O},
	{"removePrimitiveIndex", (PyCFunction)KX_MeshBuilderSlot::sPyRemovePrimitiveIndex, METH_VARARGS},
	{"addTriangleIndex", (PyCFunction)KX_MeshBuilderSlot::sPyAddTriangleIndex, METH_O},
	{"removeTriangleIndex", (PyCFunction)KX_MeshBuilderSlot::sPyRemoveTriangleIndex, METH_VARARGS},
	{"recalculateNormals", (PyCFunction)KX_MeshBuilderSlot::sPyRecalculateNormals, METH_NOARGS},
	{nullptr, nullptr} // Sentinel
};

PyAttributeDef KX_MeshBuilderSlot::Attributes[] = {
	EXP_PYATTRIBUTE_RO_FUNCTION("vertices", KX_MeshBuilderSlot, pyattr_get_vertices),
	EXP_PYATTRIBUTE_RO_FUNCTION("indices", KX_MeshBuilderSlot, pyattr_get_indices),
	EXP_PYATTRIBUTE_RO_FUNCTION("triangleIndices", KX_MeshBuilderSlot, pyattr_get_triangleIndices),
	EXP_PYATTRIBUTE_RW_FUNCTION("material", KX_MeshBuilderSlot, pyattr_get_material, pyattr_set_material),
	EXP_PYATTRIBUTE_RO_FUNCTION("uvCount", KX_MeshBuilderSlot, pyattr_get_uvCount),
	EXP_PYATTRIBUTE_RO_FUNCTION("colorCount", KX_MeshBuilderSlot, pyattr_get_colorCount),
	EXP_PYATTRIBUTE_RO_FUNCTION("primitive", KX_MeshBuilderSlot, pyattr_get_primitive),
	EXP_PYATTRIBUTE_NULL // Sentinel
};

template <KX_MeshBuilderSlot::GetSizeFunc Func>
unsigned int KX_MeshBuilderSlot::get_size_cb()
{
	return (m_array->*Func)();
}

PyObject *KX_MeshBuilderSlot::get_item_vertices_cb(unsigned int index)
{
	KX_VertexProxy *vert = new KX_VertexProxy(m_array, index);

	return vert->NewProxy(true);
}

template <KX_MeshBuilderSlot::GetIndexFunc Func>
PyObject *KX_MeshBuilderSlot::get_item_indices_cb(unsigned int index)
{
	return PyLong_FromLong((m_array->*Func)(index));
}

PyObject *KX_MeshBuilderSlot::pyattr_get_vertices(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef)
{
	return (new EXP_ListWrapper<KX_MeshBuilderSlot,
			&KX_MeshBuilderSlot::get_size_cb<&RAS_DisplayArray::GetVertexCount>,
			&KX_MeshBuilderSlot::get_item_vertices_cb>
			(self_v))->NewProxy(true);
}

PyObject *KX_MeshBuilderSlot::pyattr_get_indices(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef)
{
	return (new EXP_ListWrapper<KX_MeshBuilderSlot,
			&KX_MeshBuilderSlot::get_size_cb<&RAS_DisplayArray::GetPrimitiveIndexCount>,
			&KX_MeshBuilderSlot::get_item_indices_cb<&RAS_DisplayArray::GetPrimitiveIndex>>
			(self_v))->NewProxy(true);
}

PyObject *KX_MeshBuilderSlot::pyattr_get_triangleIndices(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef)
{
	return (new EXP_ListWrapper<KX_MeshBuilderSlot,
			&KX_MeshBuilderSlot::get_size_cb<&RAS_DisplayArray::GetTriangleIndexCount>,
			&KX_MeshBuilderSlot::get_item_indices_cb<&RAS_DisplayArray::GetTriangleIndex>>
			(self_v))->NewProxy(true);
}

PyObject *KX_MeshBuilderSlot::pyattr_get_material(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef)
{
	KX_MeshBuilderSlot *self = static_cast<KX_MeshBuilderSlot *>(self_v);
	return self->GetMaterial()->GetProxy();
}

int KX_MeshBuilderSlot::pyattr_set_material(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value)
{
	KX_MeshBuilderSlot *self = static_cast<KX_MeshBuilderSlot *>(self_v);
	KX_BlenderMaterial *mat;
	if (ConvertPythonToMaterial(value, &mat, false, "slot.material = material; KX_MeshBuilderSlot excepted a KX_BlenderMaterial.")) {
		return PY_SET_ATTR_FAIL;
	}

	self->SetMaterial(mat);
	return PY_SET_ATTR_SUCCESS;
}

PyObject *KX_MeshBuilderSlot::pyattr_get_uvCount(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef)
{
	KX_MeshBuilderSlot *self = static_cast<KX_MeshBuilderSlot *>(self_v);
	return PyBool_FromLong(self->GetDisplayArray()->GetFormat().uvSize);
}

PyObject *KX_MeshBuilderSlot::pyattr_get_colorCount(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef)
{
	KX_MeshBuilderSlot *self = static_cast<KX_MeshBuilderSlot *>(self_v);
	return PyBool_FromLong(self->GetDisplayArray()->GetFormat().colorSize);
}

PyObject *KX_MeshBuilderSlot::pyattr_get_primitive(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef)
{
	KX_MeshBuilderSlot *self = static_cast<KX_MeshBuilderSlot *>(self_v);
	return PyBool_FromLong(self->GetDisplayArray()->GetPrimitiveType());
}

PyObject *KX_MeshBuilderSlot::PyAddVertex(PyObject *args, PyObject *kwds)
{
	PyObject *pypos;
	PyObject *pynormal = nullptr;
	PyObject *pytangent = nullptr;
	PyObject *pyuvs = nullptr;
	PyObject *pycolors = nullptr;

	if (!EXP_ParseTupleArgsAndKeywords(args, kwds, "O|OOOO:addVertex",
			{"position", "normal", "tangent", "uvs", "colors", 0},
			&pypos, &pynormal, &pytangent, &pyuvs, &pycolors))
	{
		return nullptr;
	}

	mt::vec3_packed pos;
	if (!PyVecTo(pypos, pos)) {
		return nullptr;
	}

	mt::vec3_packed normal = mt::vec3_packed(mt::axisZ3);
	if (pynormal && !PyVecTo(pynormal, normal)) {
		return nullptr;
	}

	mt::vec4_packed tangent = mt::vec4_packed(mt::one4);
	if (pytangent && !PyVecTo(pytangent, tangent)) {
		return nullptr;
	}

	const RAS_DisplayArray::Format& format = m_array->GetFormat();

	mt::vec2_packed uvs[RAS_Texture::MaxUnits] = {mt::vec2_packed(mt::zero2)};
	if (pyuvs) {
		if (!PySequence_Check(pyuvs)) {
			return nullptr;
		}

		const unsigned short size = max_ii(format.uvSize, PySequence_Size(pyuvs));
		for (unsigned short i = 0; i < size; ++i) {
			if (!PyVecTo(PySequence_GetItem(pyuvs, i), uvs[i])) {
				return nullptr;
			}
		}
	}

	unsigned int colors[RAS_Texture::MaxUnits] = {0xFFFFFFFF};
	if (pycolors) {
		if (!PySequence_Check(pycolors)) {
			return nullptr;
		}

		const unsigned short size = max_ii(format.colorSize, PySequence_Size(pycolors));
		for (unsigned short i = 0; i < size; ++i) {
			mt::vec4 color;
			if (!PyVecTo(PySequence_GetItem(pycolors, i), color)) {
				return nullptr;
			}
			rgba_float_to_uchar(reinterpret_cast<unsigned char (&)[4]>(colors[i]), color.Data());
		}
	}

	const unsigned index = m_array->AddVertex(pos, normal, tangent, uvs, colors, m_origIndexCounter++, 0);

	return PyLong_FromLong(index);
}

PyObject *KX_MeshBuilderSlot::PyAddIndex(PyObject *value)
{
	if (!PySequence_Check(value)) {
		PyErr_Format(PyExc_TypeError, "expected a list");
		return nullptr;
	}

	const bool isTriangle = (m_array->GetPrimitiveType() == RAS_DisplayArray::TRIANGLES);

	for (unsigned int i = 0, size = PySequence_Size(value); i < size; ++i) {
		const int val = PyLong_AsLong(PySequence_GetItem(value, i));

		if (val < 0 && PyErr_Occurred()) {
			PyErr_Format(PyExc_TypeError, "expected a list of positive integer");
			return nullptr;
		}

		m_array->AddPrimitiveIndex(val);
		if (isTriangle) {
			m_array->AddTriangleIndex(val);
		}
	}

	Py_RETURN_NONE;
}

PyObject *KX_MeshBuilderSlot::PyAddPrimitiveIndex(PyObject *value)
{
	if (!PySequence_Check(value)) {
		PyErr_Format(PyExc_TypeError, "expected a list");
		return nullptr;
	}

	for (unsigned int i = 0, size = PySequence_Size(value); i < size; ++i) {
		const int val = PyLong_AsLong(PySequence_GetItem(value, i));

		if (val < 0 && PyErr_Occurred()) {
			PyErr_Format(PyExc_TypeError, "expected a list of positive integer");
			return nullptr;
		}

		m_array->AddPrimitiveIndex(val);
	}

	Py_RETURN_NONE;
}

PyObject *KX_MeshBuilderSlot::PyAddTriangleIndex(PyObject *value)
{
	if (!PySequence_Check(value)) {
		PyErr_Format(PyExc_TypeError, "expected a list");
		return nullptr;
	}

	for (unsigned int i = 0, size = PySequence_Size(value); i < size; ++i) {
		const int val = PyLong_AsLong(PySequence_GetItem(value, i));

		if (val < 0 && PyErr_Occurred()) {
			PyErr_Format(PyExc_TypeError, "expected a list of positive integer");
			return nullptr;
		}

		m_array->AddTriangleIndex(val);
	}

	Py_RETURN_NONE;
}

static bool removeDataCheck(int start, int end, unsigned int size, const std::string& errmsg)
{
	if (start < 0 || start >= size || (end != -1 && (end > size || end <= start))) {
		PyErr_Format(PyExc_TypeError, "%s: range invalid or empty, must be included in [0, %i[", errmsg.c_str(), size);
		return false;
	}

	return true;
}

PyObject *KX_MeshBuilderSlot::PyRemoveVertex(PyObject *args)
{
	int start;
	int end = -1;

	if (!PyArg_ParseTuple(args, "i|i:removeVertex", &start, &end)) {
		return nullptr;
	}

	if (!removeDataCheck(start, end, m_array->GetVertexCount(), "slot.removeVertex(start, end)")) {
		return nullptr;
	}

	m_array->RemoveVertex(start, end);

	Py_RETURN_NONE;
}

PyObject *KX_MeshBuilderSlot::PyRemovePrimitiveIndex(PyObject *args)
{
	int start;
	int end = -1;

	if (!PyArg_ParseTuple(args, "i|i:removePrimitiveIndex", &start, &end)) {
		return nullptr;
	}

	if (!removeDataCheck(start, end, m_array->GetPrimitiveIndexCount(), "slot.removePrimitiveIndex(start, end)")) {
		return nullptr;
	}

	m_array->RemovePrimitiveIndex(start, end);

	Py_RETURN_NONE;
}

PyObject *KX_MeshBuilderSlot::PyRemoveTriangleIndex(PyObject *args)
{
	int start;
	int end = -1;

	if (!PyArg_ParseTuple(args, "i|i:removeTriangleIndex", &start, &end)) {
		return nullptr;
	}

	if (!removeDataCheck(start, end, m_array->GetTriangleIndexCount(), "slot.removeTriangleIndex(start, end)")) {
		return nullptr;
	}

	m_array->RemoveTriangleIndex(start, end);

	Py_RETURN_NONE;
}

PyObject *KX_MeshBuilderSlot::PyRecalculateNormals()
{
	if (Invalid()) {
		PyErr_SetString(PyExc_TypeError, "slot.recalculateNormals(): slot has an invalid number of indices");
		return nullptr;
	}

	std::vector<mt::vec3, mt::simd_allocator<mt::vec3> > normals(m_array->GetVertexCount(), mt::zero3);

	for (unsigned int i = 0, size = m_array->GetPrimitiveIndexCount(); i < size; i += 3) {
		float normal[3];
		normal_tri_v3(normal,
				m_array->GetPosition(m_array->GetPrimitiveIndex(i)).data,
				m_array->GetPosition(m_array->GetPrimitiveIndex(i + 1)).data,
				m_array->GetPosition(m_array->GetPrimitiveIndex(i + 2)).data);

		const mt::vec3 vnormal(normal);
		for (unsigned short j = 0; j < 3; ++j) {
			normals[m_array->GetPrimitiveIndex(i + j)] += vnormal;
		}
	}

	for (unsigned int i = 0, size = normals.size(); i < size; ++i) {
		m_array->SetNormal(i, normals[i].SafeNormalized(mt::zero3));
	}

	Py_RETURN_NONE;
}

KX_MeshBuilder::KX_MeshBuilder(const std::string& name, KX_Scene *scene, const RAS_Mesh::LayersInfo& layersInfo,
		const RAS_DisplayArray::Format& format)
	:m_name(name),
	m_layersInfo(layersInfo),
	m_format(format),
	m_scene(scene),
	m_origIndexCounter(0)
{
}

KX_MeshBuilder::KX_MeshBuilder(const std::string& name, KX_Mesh *mesh)
	:m_name(name),
	m_layersInfo(mesh->GetLayersInfo()),
	m_scene(mesh->GetScene())
{
	m_format = {(uint8_t)max_ii(m_layersInfo.uvLayers.size(), 1), (uint8_t)max_ii(m_layersInfo.colorLayers.size(), 1)};

	for (RAS_MeshMaterial *meshmat : mesh->GetMeshMaterialList()) {
		KX_MeshBuilderSlot *slot = new KX_MeshBuilderSlot(meshmat, m_format, m_origIndexCounter);
		m_slots.Add(slot);
	}
}

KX_MeshBuilder::~KX_MeshBuilder()
{
}

std::string KX_MeshBuilder::GetName()
{
	return m_name;
}

EXP_ListValue<KX_MeshBuilderSlot>& KX_MeshBuilder::GetSlots()
{
	return m_slots;
}

static bool convertPythonListToLayers(PyObject *list, KX_Mesh::LayerList& layers, const std::string& errmsg)
{
	if (list == Py_None) {
		return true;
	}

	if (!PySequence_Check(list)) {
		PyErr_Format(PyExc_TypeError, "%s expected a list", errmsg.c_str());
		return false;
	}

	const unsigned short size = PySequence_Size(list);
	if (size > RAS_Texture::MaxUnits) {
		PyErr_Format(PyExc_TypeError, "%s excepted a list of maximum %i items", errmsg.c_str(), RAS_Texture::MaxUnits);
		return false;
	}

	for (unsigned short i = 0; i < size; ++i) {
		PyObject *value = PySequence_GetItem(list, i);

		if (!PyUnicode_Check(value)) {
			PyErr_Format(PyExc_TypeError, "%s excepted a list of string", errmsg.c_str());
		}

		const std::string name = std::string(_PyUnicode_AsString(value));
		layers.push_back({i, name});
	}

	return true;
}

static PyObject *py_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	const char *name;
	PyObject *pyscene;
	PyObject *pyuvs = Py_None;
	PyObject *pycolors = Py_None;

	if (!EXP_ParseTupleArgsAndKeywords(args, kwds, "sO|OO:KX_MeshBuilder",
			{"name", "scene", "uvs", "colors", 0}, &name, &pyscene, &pyuvs, &pycolors))
	{
		return nullptr;
	}

	KX_Scene *scene;
	if (!ConvertPythonToScene(pyscene, &scene, false, "KX_MeshBuilder(name, scene, uvs, colors): scene must be KX_Scene")) {
		return nullptr;
	}

	KX_Mesh::LayersInfo layersInfo;
	if (!convertPythonListToLayers(pyuvs, layersInfo.uvLayers, "KX_MeshBuilder(name, scene, uvs, colors): uvs:") ||
		!convertPythonListToLayers(pycolors, layersInfo.colorLayers, "KX_MeshBuilder(name, scene, uvs, colors): colors:"))
	{
		return nullptr;
	}

	RAS_DisplayArray::Format format{(uint8_t)max_ii(layersInfo.uvLayers.size(), 1), (uint8_t)max_ii(layersInfo.colorLayers.size(), 1)};

	KX_MeshBuilder *builder = new KX_MeshBuilder(name, scene, layersInfo, format);

	return builder->NewProxy(true);
}

static PyObject *py_new_from_mesh(PyObject *UNUSED(self), PyObject *args)
{
	PyObject *pymesh;
	const char *name;

	if (!PyArg_ParseTuple(args, "sO:FromMesh", &name, &pymesh)) {
		return nullptr;
	}

	KX_Mesh *mesh;
	if (!ConvertPythonToMesh(KX_GetActiveScene()->GetLogicManager(), pymesh, &mesh, false, "KX_MeshBuilder.FromMesh")) {
		return nullptr;
	}

	KX_MeshBuilder *builder = new KX_MeshBuilder(name, mesh);

	return builder->NewProxy(true);
}

PyTypeObject KX_MeshBuilder::Type = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"KX_MeshBuilder",
	sizeof(EXP_PyObjectPlus_Proxy),
	0,
	py_base_dealloc,
	0,
	0,
	0,
	0,
	py_base_repr,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	0, 0, 0, 0, 0, 0, 0,
	Methods,
	0,
	0,
	&EXP_Value::Type,
	0, 0, 0, 0, 0, 0,
	py_new
};

PyMethodDef KX_MeshBuilder::Methods[] = {
	{"FromMesh", (PyCFunction)py_new_from_mesh, METH_STATIC | METH_VARARGS},
	{"addSlot", (PyCFunction)KX_MeshBuilder::sPyAddSlot, METH_VARARGS | METH_KEYWORDS},
	{"finish", (PyCFunction)KX_MeshBuilder::sPyFinish, METH_NOARGS},
	{nullptr, nullptr} // Sentinel
};

PyAttributeDef KX_MeshBuilder::Attributes[] = {
	EXP_PYATTRIBUTE_RO_FUNCTION("slots", KX_MeshBuilder, pyattr_get_slots),
	EXP_PYATTRIBUTE_NULL // Sentinel
};

PyObject *KX_MeshBuilder::pyattr_get_slots(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef)
{
	KX_MeshBuilder *self = static_cast<KX_MeshBuilder *>(self_v);
	return self->GetSlots().GetProxy();
}

PyObject *KX_MeshBuilder::PyAddSlot(PyObject *args, PyObject *kwds)
{
	PyObject *pymat;
	int primitive;

	if (!EXP_ParseTupleArgsAndKeywords(args, kwds, "O|i:addSlot", {"material", "primitive", 0}, &pymat, &primitive)) {
		return nullptr;
	}

	KX_BlenderMaterial *material;
	if (!ConvertPythonToMaterial(pymat, &material, false, "meshBuilder.addSlot(...): material must be a KX_BlenderMaterial")) {
		return nullptr;
	}

	if (!ELEM(primitive, RAS_DisplayArray::LINES, RAS_DisplayArray::TRIANGLES)) {
		PyErr_SetString(PyExc_TypeError, "meshBuilder.addSlot(...): primitive value invalid");
		return nullptr;
	}

	KX_MeshBuilderSlot *slot = new KX_MeshBuilderSlot(material, (RAS_DisplayArray::PrimitiveType)primitive, m_format, m_origIndexCounter);
	m_slots.Add(slot);

	return slot->GetProxy();
}

PyObject *KX_MeshBuilder::PyFinish()
{
	if (m_slots.GetCount() == 0) {
		PyErr_SetString(PyExc_TypeError, "meshBuilder.finish(): no mesh data found");
		return nullptr;
	}

	for (KX_MeshBuilderSlot *slot : m_slots) {
		if (slot->Invalid()) {
			PyErr_Format(PyExc_TypeError, "meshBuilder.finish(): slot (%s) has an invalid number of indices",
					slot->GetName().c_str());
			return nullptr;
		}
	}

	KX_Mesh *mesh = new KX_Mesh(m_scene, m_name, m_layersInfo);

	RAS_BucketManager *bucketManager = m_scene->GetBucketManager();
	for (unsigned short i = 0, size = m_slots.GetCount(); i < size; ++i) {
		KX_MeshBuilderSlot *slot = m_slots.GetValue(i);
		bool created;
		RAS_MaterialBucket *bucket = bucketManager->FindBucket(slot->GetMaterial(), created);
		mesh->AddMaterial(bucket, i, slot->GetDisplayArray());
	}

	mesh->EndConversion(m_scene->GetBoundingBoxManager());

	KX_GetActiveEngine()->GetConverter()->RegisterMesh(m_scene, mesh);

	return mesh->GetProxy();
}
