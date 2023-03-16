#ifndef STANDARDPORT_H
#define STANDARDPORT_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#define SPPDATA(ADDRESS) ADDRESS
#define SPPSTATUS(ADDRESS) ADDRESS+1
#define SPPCONTROL(ADDRESS) ADDRESS+2

typedef struct {
    PyObject_HEAD
    uint16_t spp_address;
} StandardPortObject;

extern PyTypeObject StandardPortType;

#endif /* STANDARDPORT_H */
