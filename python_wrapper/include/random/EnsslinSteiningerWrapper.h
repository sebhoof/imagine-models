#include <pybind11/pybind11.h>

#include "../../../c_library/headers/EnsslinSteininger.h"

namespace py = pybind11;
using namespace pybind11::literals;

void EnsslinSteininger(py::module_ &m) {
    py::class_<ESRandomField, RandomVectorField>(m, "ESRandomField")
        .def(py::init<>())
        .def(py::init<std::array<int, 3> &, std::array<double, 3> &, std::array<double, 3> &>())

        .def_readwrite("spectral_amplitude", &ESRandomField::spectral_amplitude)
        .def_readwrite("spectral_offset", &ESRandomField::spectral_offset)
        .def_readwrite("spectral_slope", &ESRandomField::spectral_slope)

        .def_readwrite("r0", &ESRandomField::r0)
        .def_readwrite("z0", &ESRandomField::z0);

}