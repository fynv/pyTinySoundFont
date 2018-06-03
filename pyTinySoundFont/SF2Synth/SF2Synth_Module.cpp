#include <Python.h>
#include "Synth.h"

static PyObject* S16ToF32(PyObject *self, PyObject *args)
{
	char* p;

	PyObject* o_s16bytes = PyTuple_GetItem(args, 0);
	ssize_t len;
	PyBytes_AsStringAndSize(o_s16bytes, &p, &len);
	len /= sizeof(short);
	short* s16bytes = (short*)p;

	PyObject* o_f32bytes = PyBytes_FromStringAndSize(nullptr, len*sizeof(float));
	PyBytes_AsStringAndSize(o_f32bytes, &p, &len);
	len /= sizeof(float);
	float* f32bytes = (float*)p;

	for (ssize_t i = 0; i < len; i++)
		f32bytes[i] = (float)s16bytes[i] / 32767.0f;

	return o_f32bytes;
}

static PyObject* F32ToS16(PyObject *self, PyObject *args)
{
	char* p;

	PyObject* o_f32bytes = PyTuple_GetItem(args, 0);
	ssize_t len;
	PyBytes_AsStringAndSize(o_f32bytes, &p, &len);
	len /= sizeof(float);
	float* f32bytes = (float*)p;

	float amplitude = (float)PyFloat_AsDouble(PyTuple_GetItem(args, 1));

	PyObject* o_s16bytes = PyBytes_FromStringAndSize(nullptr, len*sizeof(short));
	PyBytes_AsStringAndSize(o_s16bytes, &p, &len);
	len /= sizeof(short);
	short* s16bytes = (short*)p;

	for (ssize_t i = 0; i < len; i++)
		s16bytes[i] = (short)(f32bytes[i] * 32767.0f*amplitude + 0.5f);

	return o_s16bytes;
}

static PyObject* MaxValueF32(PyObject *self, PyObject *args)
{
	char* p;

	PyObject* o_f32bytes = PyTuple_GetItem(args, 0);
	ssize_t len;
	PyBytes_AsStringAndSize(o_f32bytes, &p, &len);
	len /= sizeof(float);
	float* f32bytes = (float*)p;

	float maxV = 0.0f;
	for (ssize_t i = 0; i < len; i++)
	{
		float v = fabsf(f32bytes[i]);
		if (v > maxV) maxV = v;
	}

	return PyFloat_FromDouble((double)maxV);
}

static PyObject* ZeroBuf(PyObject *self, PyObject *args)
{
	char* p;
	PyObject* bytes = PyTuple_GetItem(args, 0);
	ssize_t len;
	PyBytes_AsStringAndSize(bytes, &p, &len);
	memset(p, 0, len);

	return PyLong_FromLong(0);
}

static PyObject* MixF32(PyObject *self, PyObject *args)
{
	PyObject* list = PyTuple_GetItem(args, 0);
	unsigned numBufs = (unsigned)PyList_Size(list);
	unsigned maxLen = 0;
	for (unsigned i = 0; i < numBufs; i++)
	{
		PyObject* o_f32bytes = PyList_GetItem(list, i);
		ssize_t len = PyBytes_Size(o_f32bytes);
		len /= sizeof(float);

		if (maxLen < len)
			maxLen = (unsigned)len;
	}
	char* pOut;
	PyObject* outBuf = PyBytes_FromStringAndSize(nullptr, maxLen*sizeof(float));
	ssize_t outlen;
	PyBytes_AsStringAndSize(outBuf, &pOut, &outlen);
	outlen /= sizeof(float);
	float* f32Out = (float*)pOut;
	memset(f32Out, 0, maxLen*sizeof(float));

	for (unsigned i = 0; i < numBufs; i++)
	{
		char* p;
		PyObject* o_f32bytes = PyList_GetItem(list, i);
		ssize_t len;
		PyBytes_AsStringAndSize(o_f32bytes, &p, &len);
		len /= sizeof(float);
		float* f32bytes = (float*)p;

		for (unsigned j = 0; j < len; j++)
			f32Out[j] += f32bytes[j];
	}
	
	return outBuf;
}

static PyObject* Synth(PyObject *self, PyObject *args)
{
	PyObject* obj_input = PyTuple_GetItem(args, 0);

	ssize_t len_in;
	char* p_in;
	PyBytes_AsStringAndSize(obj_input, &p_in, &len_in);
	len_in /= sizeof(float);
	float* input = (float*)p_in;

	PyObject* obj_outbuf = PyTuple_GetItem(args, 1);

	ssize_t len_out;
	char* p_out;
	PyBytes_AsStringAndSize(obj_outbuf, &p_out, &len_out);
	len_out /= sizeof(float);
	float* outputBuffer = (float*)p_out;

	unsigned numSamples = (unsigned) PyLong_AsUnsignedLong(PyTuple_GetItem(args, 2));

	NoteState ns;
	PyObject* obj_ns = PyTuple_GetItem(args, 3);
	ns.sourceSamplePosition = PyFloat_AsDouble(PyDict_GetItemString(obj_ns, "sourceSamplePosition"));
	PyObject* obj_lowpass = PyDict_GetItemString(obj_ns, "lowPass");
	ns.lowPass.z1 = PyFloat_AsDouble(PyDict_GetItemString(obj_lowpass, "z1"));
	ns.lowPass.z2 = PyFloat_AsDouble(PyDict_GetItemString(obj_lowpass, "z2"));

	SynthCtrl control;
	PyObject* obj_ctrl = PyTuple_GetItem(args, 4);
	control.outputmode = (OutputMode)PyLong_AsUnsignedLong(PyDict_GetItemString(obj_ctrl, "outputmode"));
	control.loopStart = (unsigned)PyLong_AsUnsignedLong(PyDict_GetItemString(obj_ctrl, "loopStart"));
	control.loopEnd = (unsigned)PyLong_AsUnsignedLong(PyDict_GetItemString(obj_ctrl, "loopEnd"));
	control.end = (unsigned)PyLong_AsUnsignedLong(PyDict_GetItemString(obj_ctrl, "end"));
	control.panFactorLeft = (float)PyFloat_AsDouble(PyDict_GetItemString(obj_ctrl, "panFactorLeft"));
	control.panFactorRight = (float)PyFloat_AsDouble(PyDict_GetItemString(obj_ctrl, "panFactorRight"));
	control.effect_sample_block = (unsigned)PyLong_AsUnsignedLong(PyDict_GetItemString(obj_ctrl, "effect_sample_block"));
	PyObject* obj_controlPnts = PyDict_GetItemString(obj_ctrl, "controlPnts");

	unsigned numCtrlPns = (unsigned)PyList_Size(obj_controlPnts);
	for (unsigned i = 0; i < numCtrlPns; i++)
	{
		PyObject* obj_ctrlPnt = PyList_GetItem(obj_controlPnts, i);
		SynthCtrlPnt ctrlPnt;
		ctrlPnt.looping = PyObject_IsTrue(PyDict_GetItemString(obj_ctrlPnt, "looping"))?1:0;
		ctrlPnt.gainMono = (float)PyFloat_AsDouble(PyDict_GetItemString(obj_ctrlPnt, "gainMono"));
		ctrlPnt.pitchRatio = PyFloat_AsDouble(PyDict_GetItemString(obj_ctrlPnt, "pitchRatio"));

		PyObject* obj_lowPass = PyDict_GetItemString(obj_ctrlPnt, "lowPass");
		ctrlPnt.lowPass.active = PyObject_IsTrue(PyDict_GetItemString(obj_lowPass, "active")) ? 1 : 0;
		ctrlPnt.lowPass.a0 = PyFloat_AsDouble(PyDict_GetItemString(obj_lowPass, "a0"));
		ctrlPnt.lowPass.a1 = PyFloat_AsDouble(PyDict_GetItemString(obj_lowPass, "a1"));
		ctrlPnt.lowPass.b1 = PyFloat_AsDouble(PyDict_GetItemString(obj_lowPass, "b1"));
		ctrlPnt.lowPass.b2 = PyFloat_AsDouble(PyDict_GetItemString(obj_lowPass, "b2"));

		control.controlPnts.push_back(ctrlPnt);
	}

	Synth(input, outputBuffer, numSamples, ns, control);

	PyDict_SetItemString(obj_ns, "sourceSamplePosition", PyFloat_FromDouble(ns.sourceSamplePosition));
	PyDict_SetItemString(obj_lowpass, "z1", PyFloat_FromDouble(ns.lowPass.z1));
	PyDict_SetItemString(obj_lowpass, "z2", PyFloat_FromDouble(ns.lowPass.z2));

	return PyLong_FromLong(0);
}

static PyMethodDef s_Methods[] = {
	{
		"S16ToF32",
		S16ToF32,
		METH_VARARGS,
		""
	},
	{
		"F32ToS16",
		F32ToS16,
		METH_VARARGS,
		""
	},
	{
		"MaxValueF32",
		MaxValueF32,
		METH_VARARGS,
		""
	},
	{
		"ZeroBuf",
		ZeroBuf,
		METH_VARARGS,
		""
	},
	{
		"MixF32",
		MixF32,
		METH_VARARGS,
		""
	},
	{
		"Synth",
		Synth,
		METH_VARARGS,
		""
	},
	{ NULL, NULL, 0, NULL }
};


static struct PyModuleDef cModPyDem =
{
	PyModuleDef_HEAD_INIT,
	"SF2Synth_module", /* name of module */
	"",          /* module documentation, may be NULL */
	-1,          /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
	s_Methods
};

PyMODINIT_FUNC PyInit_PySF2Synth(void)
{
	return PyModule_Create(&cModPyDem);
}
