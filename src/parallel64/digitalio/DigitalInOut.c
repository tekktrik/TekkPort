// SPDX-FileCopyrightText: 2023 Alec Delaney
//
// SPDX-License-Identifier: MIT

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "core/portio.h"
#include "helper/enumfactory.h"
#include "DigitalInOut.h"
#include "hardware/Pin.h"


static PyObject* DigitalInOut_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    return (PyObject *)PyObject_GC_NewVar(DigitalInOutObject, type, 0);
}


static int DigitalInOut_init(DigitalInOutObject *self, PyObject *args, PyObject *kwds) {

    // Parse arguments (unique)
    PyObject *pin;

    static char *keywords[] = {"pin", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", keywords, &pin)) {
        return -1;
    }

    PyObject *hw_mod = PyImport_ImportModule("parallel64.hardware");
    PyObject *pin_class = PyObject_GetAttrString(hw_mod, "Pin");
    if (!PyObject_IsInstance(pin, pin_class)) {
        PyErr_SetString(
            PyExc_TypeError,
            "`pin` must be an instance of parallel64.hardware.Pin"
        );
        return -1;
    };
    PinObject *temp = self->pin;
    Py_INCREF(pin);
    self->pin = (PinObject *)pin;
    Py_XDECREF(temp);

    self->pin->in_use = true;

    return 0;

}


static int DigitalInOut_traverse(DigitalInOutObject *self, visitproc visit, void *arg) {
    Py_VISIT(self->pin);
    return 0;
}

static int DigitalInOut_clear(DigitalInOutObject *self) {
    Py_CLEAR(self->pin);
    return 0;
}

static void DigitalInOut_dealloc(DigitalInOutObject *self) {
    PyObject_GC_UnTrack(self);
    DigitalInOut_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyObject* DigitalInOut_get_direction(PyObject *self, void *closure) {
    return generate_enum("parallel64.digitalio", "Direction", DIGINOUT_PIN(self)->direction == 0);
}

static PyObject* DigitalInOut_get_pull(PyObject *self, void *closure) {
    if (DIGINOUT_PIN(self)->direction == PORT_DIR_FORWARD) {
        PyErr_SetString(PyExc_AttributeError, "Not an input");
        return NULL;
    }
    if (DIGINOUT_PIN(self)->pull == PULL_NONE) {
        Py_RETURN_NONE;
    }
    return generate_enum("parallel64.digitalio", "Pull", !DIGINOUT_PIN(self)->pull);
}

static PyObject* DigitalInOut_get_drivemode(PyObject *self, void *closure) {
    if (DIGINOUT_PIN(self)->direction == PORT_DIR_REVERSE) {
        PyErr_SetString(PyExc_AttributeError, "Not an output");
        return NULL;
    }
    return generate_enum("parallel64.digitalio", "DriveMode", DIGINOUT_PIN(self)->drive_mode);
}

static PyObject* DigitalInOut_get_value(PyObject *self, void *closure) {
    uint8_t reg_value = readport(DIGINOUT_PIN(self)->reg_addr);
    bool bit_value = P64_CHECKBIT_UINT8(reg_value, DIGINOUT_PIN(self)->bit_index);
    return PyBool_FromLong(bit_value);
}


static int DigitalInOut_set_direction(PyObject *self, PyObject *value, void *closure) {
    const uint16_t reg_addr = DIGINOUT_PIN(self)->reg_addr;
    PyObject *dirobjvalue = PyObject_GetAttrString(value, "value");
    port_dir_t dirvalue = (port_dir_t)PyLong_AsLong(dirobjvalue);
    dirvalue = !dirvalue;
    if (dirvalue == 0 && !DIGINOUT_PIN(self)->output_allowed) {
        PyErr_SetString(PyExc_ValueError, "The pin cannot be use as an input");
        return -1;
    }
    else if (dirvalue == 1 && !DIGINOUT_PIN(self)->input_allowed) {
        PyErr_SetString(PyExc_ValueError, "The pin cannot be use as an output");
        return -1;
    }
    if (DIGINOUT_PIN(self)->propagate_dir) {
        portio_set_port_direction(reg_addr, dirvalue == 0);
    }
    DIGINOUT_PIN(self)->direction = dirvalue;

    return 0;
}

static int DigitalInOut_set_pull(PyObject *self, PyObject *value, void *closure) {
    port_dir_t current_dir = DIGINOUT_PIN(self)->direction;
    if (current_dir != PORT_DIR_REVERSE) {
        PyErr_SetString(PyExc_AttributeError, "Not an input");
        return -1;
    }
    pull_mode_t pull = DIGINOUT_PIN(self)->pull;
    pull_mode_t pullvalue;
    if (Py_IsNone(value)) {
        pullvalue = PULL_NONE;
    }
    else {
        PyObject *pullobjvalue = PyObject_GetAttrString(value, "value");
        pullvalue = (pull_mode_t)PyLong_AsLong(pullobjvalue);
    }
    if (pullvalue != pull) {
        PyErr_SetString(PyExc_ValueError, "Pin pull modes cannot be changed from their default");
        return -1;
    }

    return 0;
}

static int DigitalInOut_set_drivemode(PyObject *self, PyObject *value, void *closure) {
    drive_mode_t drive_mode = DIGINOUT_PIN(self)->drive_mode;
    PyObject *dmobjvalue = PyObject_GetAttrString(value, "value");
    drive_mode_t dmvalue = (drive_mode_t)PyLong_AsLong(dmobjvalue);
    if (dmvalue != drive_mode) {
        PyErr_SetString(PyExc_ValueError, "Pin drive modes cannot be changed from their default");
        return -1;
    }

    return 0;
}

static int DigitalInOut_set_value(PyObject *self, PyObject *value, void *closure) {
    port_dir_t current_dir = DIGINOUT_PIN(self)->direction;
    if (current_dir != PORT_DIR_FORWARD) {
        PyErr_SetString(PyExc_AttributeError, "Not an output");
        return -1;
    }
    bool pinvalue = PyObject_IsTrue(value);
    uint16_t reg_addr = DIGINOUT_PIN(self)->reg_addr;
    uint8_t bit_index = DIGINOUT_PIN(self)->bit_index;
    uint8_t current_value = readport(reg_addr);
    writeport(reg_addr, P64_SETBIT(current_value, bit_index, pinvalue));
    return 0;
}


static PyObject* GPIO_switch_to_output(PyObject *self, PyObject *args, PyObject *kwds) {

    PyObject *dio_mod = PyImport_AddModule("parallel64.digitalio");
    PyObject *direction_enum = PyObject_GetAttrString(dio_mod, "Direction");
    PyObject *drive_mode_enum = PyObject_GetAttrString(dio_mod, "DriveMode");

    PyObject *value = Py_False;
    PyObject *drive_mode = PyObject_GetAttrString(drive_mode_enum, "PUSH_PULL");

    static char *keywords[] = {"value", "drive_mode", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO", keywords, &value, &drive_mode)) {
        return NULL;
    }

    PyObject_SetAttrString(self, "direction", PyObject_GetAttrString(direction_enum, "OUTPUT"));
    PyObject_SetAttrString(self, "value", value);
    PyObject_SetAttrString(self, "drive_mode", drive_mode);

    Py_RETURN_NONE;

}

static PyObject* GPIO_switch_to_input(PyObject *self, PyObject *args, PyObject *kwds) {

    PyObject *dio_mod = PyImport_AddModule("parallel64.digitalio");
    PyObject *direction_enum = PyObject_GetAttrString(dio_mod, "Direction");

    PyObject *pull = Py_None;

    static char *keywords[] = {"pull", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", keywords, &pull)) {
        return NULL;
    }

    PyObject_SetAttrString(self, "direction", PyObject_GetAttrString(direction_enum, "INPUT"));
    PyObject_SetAttrString(self, "pull", pull);

    Py_RETURN_NONE;

}


static PyGetSetDef DigitalInOut_getsetters[] = {
    {"direction", (getter)DigitalInOut_get_direction, (setter)DigitalInOut_set_direction, "The digital pin direction", NULL},
    {"pull", (getter)DigitalInOut_get_pull, (setter)DigitalInOut_set_pull, "The pin pull direction", NULL},
    {"drive_mode", (getter)DigitalInOut_get_drivemode, (setter)DigitalInOut_set_drivemode, "The digital pin drive mode", NULL},
    {"value", (getter)DigitalInOut_get_value, (setter)DigitalInOut_set_value, "The digital pin value", NULL},
    {NULL}
};


static PyMethodDef GPIO_methods[] = {
    {"switch_to_output", (PyCFunctionWithKeywords)GPIO_switch_to_output, METH_VARARGS | METH_KEYWORDS, "Switch the digital pin mode to output"},
    {"switch_to_input", (PyCFunctionWithKeywords)GPIO_switch_to_input, METH_VARARGS | METH_KEYWORDS, "Switch the digital pin mode to input"},
    {NULL}
};


PyTypeObject DigitalInOutType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "parallel64.digitalio.DigitalInOut",
    .tp_basicsize = sizeof(DigitalInOutObject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .tp_doc = "Class for interacting with digital inputs and outputs",
    .tp_alloc = PyType_GenericAlloc,
    .tp_dealloc = (destructor)DigitalInOut_dealloc,
    .tp_traverse = (traverseproc)DigitalInOut_traverse,
    .tp_clear = (inquiry)DigitalInOut_clear,
    .tp_new = (newfunc)DigitalInOut_new,
    .tp_init = (initproc)DigitalInOut_init,
    .tp_free = PyObject_GC_Del,
    .tp_getset = DigitalInOut_getsetters,
    .tp_methods = GPIO_methods
};
