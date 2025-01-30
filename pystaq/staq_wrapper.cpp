/*
 * This file is part of pystaq.
 *
 * Copyright (c) 2019 - 2024 softwareQ Inc. All rights reserved.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pystaq/pystaq_common.h"

class Program {
    qasmtools::ast::ptr<qasmtools::ast::Program> prog_;

  public:
    Program(qasmtools::ast::ptr<qasmtools::ast::Program> prog)
        : prog_(std::move(prog)) {}
    /**
     * \brief Print the formatted QASM source code
     */
    friend std::ostream& operator<<(std::ostream& os, const Program& p) {
        return os << *(p.prog_);
    }
    // transformations/optimizations/etc.
    void desugar() { staq::transformations::desugar(*prog_); }
    void inline_prog(bool clear_decls = false, bool inline_stdlib = false,
                     const std::string& ancilla_name = "anc") {
        using namespace staq;
        std::set<std::string_view> overrides =
            inline_stdlib ? std::set<std::string_view>()
                          : transformations::default_overrides;
        transformations::inline_ast(*prog_,
                                    {!clear_decls, overrides, ancilla_name});
    }
    void map(const std::string& layout = "linear",
             const std::string& mapper = "swap", bool evaluate_all = false,
             std::optional<std::string> device_json_file = std::nullopt) {
        using namespace staq;
        // Inline fully first
        transformations::inline_ast(*prog_, {false, {}, "anc"});

        // Physical device
        mapping::Device dev;
        if (device_json_file.has_value()) {
            dev = mapping::parse_json(device_json_file.value());
        } else {
            dev = mapping::fully_connected(tools::estimate_qubits(*prog_));
        }

        // Initial layout
        mapping::layout physical_layout;
        if (layout == "linear") {
            physical_layout = mapping::compute_basic_layout(dev, *prog_);
        } else if (layout == "eager") {
            physical_layout = mapping::compute_eager_layout(dev, *prog_);
        } else if (layout == "bestfit") {
            physical_layout = mapping::compute_bestfit_layout(dev, *prog_);
        } else {
            std::cerr << "Error: invalid layout algorithm\n";
            return;
        }
        mapping::apply_layout(physical_layout, dev, *prog_);

        // Mapping
        if (mapper == "swap") {
            mapping::map_onto_device(dev, *prog_);
        } else if (mapper == "steiner") {
            mapping::steiner_mapping(dev, *prog_);
        } else {
            std::cerr << "Error: invalid mapping algorithm\n";
            return;
        }

        /* Evaluating symbolic expressions */
        if (evaluate_all) {
            transformations::expr_simplify(*prog_, true);
        }
    }
    void rotation_fold(bool no_correction = false) {
        staq::optimization::fold_rotations(*prog_, {!no_correction});
    }
    void cnot_resynth() { staq::optimization::optimize_CNOT(*prog_); }
    void simplify(bool no_fixpoint = false) {
        staq::transformations::expr_simplify(*prog_);
        staq::optimization::simplify(*prog_, {!no_fixpoint});
    }
    void synthesize_oracles() {
        staq::transformations::synthesize_oracles(*prog_);
    }
#ifdef GRID_SYNTH
    void qasm_synth(long int prec, int factor_effort, bool check, bool details,
                    bool verbose) {
        staq::grid_synth::GridSynthOptions opt{prec, factor_effort, check,
                                               details, verbose};
        staq::transformations::qasm_synth(*prog_, opt);
    }
#endif /* GRID_SYNTH */
    void optimize_level_0() {
        desugar();
    }
    void optimize_level_1(bool no_correction = false,
                          bool no_fixpoint = false) {
        rotation_fold(no_correction);
        simplify(no_fixpoint);
    }
    void optimize_level_2(bool no_correction = false, bool no_fixpoint = false,
                          bool clear_decls = false, bool inline_stdlib = false,
                          const std::string& ancilla_name = "anc") {
        inline_prog(clear_decls, inline_stdlib, ancilla_name);
        simplify(no_fixpoint);
        rotation_fold(no_correction);
        simplify(no_fixpoint);
    }
    void optimize_level_3(bool no_correction = false, bool no_fixpoint = false,
                          bool clear_decls = false, bool inline_stdlib = false,
                          const std::string& ancilla_name = "anc") {
        inline_prog(clear_decls, inline_stdlib, ancilla_name);
        simplify(no_fixpoint);
        rotation_fold(no_correction);
        simplify(no_fixpoint);
        cnot_resynth();
        simplify(no_fixpoint);
    }
    // output (these methods return a string)
    std::string get_resources(bool box_gates = false, bool unbox_qelib = false,
                              bool no_merge_dagger = false) {
        std::set<std::string_view> overrides =
            unbox_qelib ? std::set<std::string_view>()
                        : qasmtools::ast::qelib_defs;
        auto count = staq::tools::estimate_resources(
            *prog_, {!box_gates, !no_merge_dagger, overrides});

        std::ostringstream oss;
        oss << "Resources used:\n";
        for (auto& [name, num] : count) {
            oss << "  " << name << ": " << num << "\n";
        }
        return oss.str();
    }
    std::string to_cirq() {
        std::ostringstream oss;
        staq::output::CirqOutputter outputter(oss);
        outputter.run(*prog_);
        return oss.str();
    }
    std::string to_projectq() {
        std::ostringstream oss;
        staq::output::ProjectQOutputter outputter(oss);
        outputter.run(*prog_);
        return oss.str();
    }
    std::string to_qsharp() {
        std::ostringstream oss;
        staq::output::QSharpOutputter outputter(oss);
        outputter.run(*prog_);
        return oss.str();
    }
    std::string to_quil() {
        std::ostringstream oss;
        staq::output::QuilOutputter outputter(oss);
        outputter.run(*prog_);
        return oss.str();
    }
    std::string to_ionq() {
        std::ostringstream oss;
        staq::output::IonQOutputter outputter(oss);
        outputter.run(*prog_);
        return oss.str();
    }
    std::string lattice_surgery() {
        return staq::output::lattice_surgery(*prog_);
    }
};

Program parse_str(const std::string& s) {
    return qasmtools::parser::parse_string(s);
}
Program parse_file(const std::string& fname) {
    return qasmtools::parser::parse_file(fname);
}

void desugar(Program& prog) { prog.desugar(); }
void inline_prog(Program& prog, bool clear_decls, bool inline_stdlib,
                 const std::string& ancilla_name) {
    prog.inline_prog(clear_decls, inline_stdlib, ancilla_name);
}
void map(Program& prog, const std::string& layout, const std::string& mapper,
         bool evaluate_all, std::optional<std::string> device_json_file) {
    prog.map(layout, mapper, evaluate_all, device_json_file);
}
void rotation_fold(Program& prog, bool no_correction) {
    prog.rotation_fold(no_correction);
}
void cnot_resynth(Program& prog) { prog.cnot_resynth(); }
void simplify(Program& prog, bool no_fixpoint) { prog.simplify(no_fixpoint); }
void synthesize_oracles(Program& prog) { prog.synthesize_oracles(); }
#ifdef GRID_SYNTH
void grid_synth(const std::vector<std::string>& thetas, long int prec,
                int factor_effort, bool check, bool details, bool verbose,
                bool timer) {
    using namespace staq::grid_synth;
    if (verbose) {
        std::cerr << thetas.size() << " angle(s) read." << '\n';
    }
    GridSynthOptions opt{prec, factor_effort, check, details, verbose, timer};
    GridSynthesizer synthesizer = make_synthesizer(opt);
    std::random_device rd;
    random_numbers.seed(rd());
    for (const auto& angle : thetas) {
        str_t op_str = synthesizer.get_op_str(real_t(angle));
        for (char c : op_str) {
            std::cout << c << ' ';
        }
        std::cout << '\n';
    }
}
void grid_synth(const std::string& theta, long int prec, int factor_effort,
                bool check, bool details, bool verbose, bool timer) {
    grid_synth(std::vector<std::string>{theta}, prec, factor_effort, check,
               details, verbose, timer);
}
void qasm_synth(Program& prog, long int prec, int factor_effort, bool check,
                bool details, bool verbose) {
    prog.qasm_synth(prec, factor_effort, check, details, verbose);
}
#endif /* GRID_SYNTH */
std::string lattice_surgery(Program& prog) { return prog.lattice_surgery(); }

void compile(Program& prog, int optimization_level,
             bool no_correction,
             bool no_fixpoint,
             bool clear_decls, bool inline_stdlib,
             const std::string& ancilla_name) {
        switch (optimization_level) {
        case 0:
                prog.optimize_level_0();
                break;
        case 1:
                prog.optimize_level_1(no_correction, no_fixpoint);
                break;
        case 2:
                prog.optimize_level_2(no_correction, no_fixpoint, clear_decls,
                                      inline_stdlib, ancilla_name);
                break;
        case 3:
                prog.optimize_level_3(no_correction, no_fixpoint, clear_decls,
                                      inline_stdlib, ancilla_name);
                break;
        default:
                throw std::invalid_argument("Invalid optimization level");
                break;
        }
}

static double FIDELITY_1 = staq::mapping::FIDELITY_1;

class Device {
    int n_;
    std::vector<double> sq_fi_;
    std::vector<std::vector<bool>> adj_;
    std::vector<std::vector<double>> tq_fi_;

  public:
    Device(int n)
        : n_(n), sq_fi_(n_, FIDELITY_1), adj_(n_, std::vector<bool>(n_)),
          tq_fi_(n_, std::vector<double>(n_, FIDELITY_1)) {
        if (n_ <= 0) {
            throw std::logic_error("Invalid device qubit count");
        }
    }
    void add_edge(int control, int target, bool directed = false,
                  double fidelity = FIDELITY_1) {
        if (control < 0 || control >= n_ || target < 0 || target >= n_) {
            std::cerr << "Qubit(s) out of range: " << control << "," << target
                      << "\n";
        } else {
            adj_[control][target] = true;
            if (!directed) {
                adj_[target][control] = true;
            }
            if (fidelity != FIDELITY_1) {
                if (fidelity < 0 || fidelity > 1) {
                    std::cerr << "Fidelity out of range: " << fidelity << "\n";
                } else {
                    tq_fi_[control][target] = fidelity;
                    if (!directed) {
                        tq_fi_[target][control] = fidelity;
                    }
                }
            }
        }
    }
    void set_fidelity(int qubit, double fidelity) {
        if (qubit < 0 || qubit >= n_) {
            std::cerr << "Qubit out of range: " << qubit;
        } else if (fidelity < 0 || fidelity > 1) {
            std::cerr << "Fidelity out of range: " << fidelity;
        } else {
            sq_fi_[qubit] = fidelity;
        }
    }
    std::string to_string() const {
        staq::mapping::Device dev("Custom Device", n_, adj_, sq_fi_, tq_fi_);
        return dev.to_json();
    }
};

PYBIND11_MODULE(pystaq, m) {
    m.doc() = "Python wrapper for staq (https://github.com/softwareQinc/staq)";

    py::class_<Program>(m, "Program")
        .def("get_resources", &Program::get_resources, "Get circuit statistics",
             py::arg("box_gates") = false, py::arg("unbox_qelib") = false,
             py::arg("no_merge_dagger") = false)
        .def("to_cirq", &Program::to_cirq, "Get the Cirq representation")
        .def("to_projectq", &Program::to_projectq,
             "Get the ProjectQ representation")
        .def("to_qsharp", &Program::to_qsharp, "Get the Q# representation")
        .def("to_quil", &Program::to_quil, "Get the Quil representation")
        .def("to_ionq", &Program::to_ionq, "Get the IonQ representation")
        .def("__repr__", [](const Program& p) {
            std::ostringstream oss;
            oss << p;
            return oss.str();
        });

    m.def("parse_str", &parse_str, "Parse OpenQASM program string");
    m.def("parse_file", &parse_file, "Parse OpenQASM program file");
    m.def("desugar", &desugar, "Expand out gates applied to registers");
    m.def("inline", &inline_prog, "Inline the OpenQASM source code",
          py::arg("prog"), py::arg("clear_decls") = false,
          py::arg("inline_stdlib") = false, py::arg("ancilla_name") = "anc");
    m.def("map", &map,
          "Map circuit to a physical device. The device connectivity "
          "specification must be loaded from the file 'device_json_file'; if "
          "absent/None (default), assumes full connectivity.",
          py::arg("prog"), py::arg("layout") = "linear",
          py::arg("mapper") = "swap", py::arg("evaluate_all") = false,
          py::arg("device_json_file") = std::nullopt);
    m.def("rotation_fold", &rotation_fold,
          "Reduce the number of small-angle rotation gates in all Pauli bases",
          py::arg("prog"), py::arg("no_correction") = false);
    m.def("cnot_resynth", &cnot_resynth,
          "Re-synthesize CNOT-dihedral subcircuits to reduce CNOT gates");
    m.def("simplify", &simplify, "Apply basic circuit simplifications",
          py::arg("prog"), py::arg("no_fixpoint") = false);
    m.def("synthesize_oracles", &synthesize_oracles,
          "Synthesizes oracles declared by verilog files");
#ifdef GRID_SYNTH
    m.def("grid_synth",
          py::overload_cast<const std::vector<std::string>&, long int, int,
                            bool, bool, bool, bool>(&grid_synth),
          "Approximate Z-rotation angle(s) (in units of PI)", py::arg("theta"),
          py::arg("prec"),
          py::arg("pollard-rho") = staq::grid_synth::MAX_ATTEMPTS_POLLARD_RHO,
          py::arg("check") = false, py::arg("details") = false,
          py::arg("verbose") = false, py::arg("timer") = false);
    m.def("grid_synth",
          py::overload_cast<const std::string&, long int, int, bool, bool, bool,
                            bool>(&grid_synth),
          "Approximate Z-rotation angle(s) (in units of PI)", py::arg("theta"),
          py::arg("prec"),
          py::arg("pollard-rho") = staq::grid_synth::MAX_ATTEMPTS_POLLARD_RHO,
          py::arg("check") = false, py::arg("details") = false,
          py::arg("verbose") = false, py::arg("timer") = false);
    m.def("qasm_synth", &qasm_synth,
          "Replaces rx/ry/rz gates with grid_synth approximations",
          py::arg("prog"), py::arg("prec"),
          py::arg("pollard-rho") = staq::grid_synth::MAX_ATTEMPTS_POLLARD_RHO,
          py::arg("check") = false, py::arg("details") = false,
          py::arg("verbose") = false);
#endif /* GRID_SYNTH */
    m.def("lattice_surgery", &lattice_surgery,
          "Compiles OpenQASM 2.0 to lattice surgery instruction set",
          py::arg("prog"));

    m.def("compile", &compile, "compile", py::arg("prog"),
          py::arg("optimization_level"), py::arg("no_correction") = false,
          py::arg("no_fixpoint") = false, py::arg("clear_decls") = false,
          py::arg("inline_stdlib") = false, py::arg("ancilla_name") = "anc");

    py::class_<Device>(m, "Device")
        .def(py::init<int>())
        .def("add_edge", &Device::add_edge, "Add edge with optional fidelity",
             py::arg("control"), py::arg("target"), py::arg("directed") = false,
             py::arg("fidelity") = FIDELITY_1)
        .def("set_fidelity", &Device::set_fidelity, "Set single qubit fidelity")
        .def("__repr__", [](const Device& d) { return d.to_string(); });
}
