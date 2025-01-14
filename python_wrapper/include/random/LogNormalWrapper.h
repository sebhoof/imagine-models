#include <pybind11/pybind11.h>

#include "../../../c_library/headers/LogNormal.h"

namespace py = pybind11;
using namespace pybind11::literals;

void LogNormal(py::module_ &m) {
    py::class_<LogNormalScalarField, RandomScalarField>(m, "LogNormalScalarField")
        .def(py::init<>())
        .def(py::init<std::array<int, 3> &, std::array<double, 3> &, std::array<double, 3> &>())

        .def_readwrite("log_mean", &LogNormalScalarField::log_mean)
        .def_readwrite("spectral_amplitude", &LogNormalScalarField::spectral_amplitude)
        .def_readwrite("spectral_offset", &LogNormalScalarField::spectral_offset)
        .def_readwrite("spectral_slope", &LogNormalScalarField::spectral_slope);

}