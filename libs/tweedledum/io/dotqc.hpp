/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "../gates/gate_base.hpp"
#include "../gates/gate_lib.hpp"
#include "../networks/io_id.hpp"

#include <cassert>
#include <tweedledee/dotqc/dotqc.hpp>

namespace tweedledum {

struct identify_gate {
    gate_base operator()(std::string_view gate_label) {
        switch (gate_label[0]) {
            case 'H':
                return gate::hadamard;

            case 'S':
            case 'P':
                if (gate_label.size() == 2 && gate_label[1] == '*') {
                    return gate::phase_dagger;
                }
                return gate::phase;

            case 'T':
                if (gate_label.size() == 2 && gate_label[1] == '*') {
                    return gate::t_dagger;
                }
                return gate::t;

            case 'X':
                return gate::pauli_x;

            case 'Y':
                return gate::pauli_y;

            case 'Z':
                // This will be a ugly hack (:
                if (gate_label.size() == 2 && gate_label[1] == 'd') {
                    return gate_base(gate_lib::undefined);
                }
                return gate::pauli_z;

            default:
                break;
        }
        if (gate_label == "tof") {
            return gate::cx;
        }
        return gate_base(gate_lib::unknown);
    }
};

template <typename Network>
class dotqc_reader : public tweedledee::dotqc_reader<gate_base> {
  public:
    explicit dotqc_reader(Network& network) : network_(network) {}

    void on_qubit(std::string qubit_label) { network_.add_qubit(qubit_label); }

    void on_input(std::string qubit_label) {
        (void) qubit_label;
        // network_.mark_as_input(qubit_label);
    }

    void on_output(std::string qubit_label) {
        (void) qubit_label;
        // network_.mark_as_output(qubit_label);
    }

    void on_gate(gate_base gate, std::string const& target)

    {
        network_.add_gate(gate, target);
    }

    void on_gate(gate_base gate, std::vector<std::string> const& controls,
                 std::vector<std::string> const& targets) {
        switch (gate.operation()) {
            case gate_lib::pauli_x:
                if (controls.size() == 1) {
                    gate = gate::cx;
                } else if (controls.size() >= 2) {
                    gate = gate::mcx;
                }
                break;

            case gate_lib::pauli_z:
                if (controls.size() == 1) {
                    gate = gate::cz;
                } else if (controls.size() >= 2) {
                    gate = gate::mcz;
                }
                break;

            // Hack to handle 'Zd' gate
            case gate_lib::undefined:
                network_.add_gate(gate::t_dagger, controls[0]);
                network_.add_gate(gate::t_dagger, controls[1]);
                network_.add_gate(gate::t_dagger, targets[0]);

                network_.add_gate(gate::cx, controls[0], controls[1]);
                network_.add_gate(gate::cx, controls[1], targets[0]);
                network_.add_gate(gate::cx, targets[0], controls[0]);

                network_.add_gate(gate::t, controls[0]);
                network_.add_gate(gate::t, controls[1]);
                network_.add_gate(gate::t_dagger, targets[0]);

                network_.add_gate(gate::cx, controls[1], controls[0]);
                network_.add_gate(gate::t, controls[0]);
                network_.add_gate(gate::cx, controls[1], targets[0]);
                network_.add_gate(gate::cx, targets[0], controls[0]);
                network_.add_gate(gate::cx, controls[0], controls[1]);
                return;

            default:
                break;
        }
        network_.add_gate(gate, controls, targets);
    }

  private:
    Network& network_;
};

/*! \brief Writes network in dotQC format into output stream
 *
 * An overloaded variant exists that writes the network into a file.
 *
 * **Required gate functions:**
 * - `foreach_control`
 * - `foreach_target`
 * - `op`
 *
 * **Required network functions:**
 * - `foreach_node`
 * - `foreach_qubit`
 * - `num_qubits`
 *
 * \param network A quantum network
 * \param os Output stream
 */
template <typename Network>
void write_dotqc(Network const& network, std::ostream& os) {
    os << fmt::format("# Generated by tweedledum\n");
    os << fmt::format(".v");
    network.foreach_io([&](auto id, auto const& name) {
        if (id.is_qubit()) {
            os << fmt::format(" {}", name);
        }
    });
    os << fmt::format("\nBEGIN\n\n");
    network.foreach_gate([&](auto& node) {
        switch (node.gate.operation()) {
            case gate_lib::pauli_x:
                os << 'X';
                break;

            case gate_lib::cx:
                os << "tof";
                break;

            case gate_lib::mcx:
                os << "tof";
                break;

            case gate_lib::pauli_z:
            case gate_lib::cz:
            case gate_lib::mcz:
                os << 'Z';
                break;

            case gate_lib::hadamard:
                os << 'H';
                break;

            case gate_lib::phase:
                os << "S";
                break;

            case gate_lib::phase_dagger:
                os << "S*";
                break;

            case gate_lib::t:
                os << "T";
                break;

            case gate_lib::t_dagger:
                os << "T*";
                break;

            default:
                break;
        }
        node.gate.foreach_control([&](auto qubit) {
            os << fmt::format(" {}", network.io_label(qubit));
        });
        os << fmt::format(" {}\n", network.io_label(node.gate.target()));
    });
    os << fmt::format("\nEND\n");
}

/*! \brief Writes network in dotQC format into a file
 *
 * **Required gate functions:**
 * - `foreach_control`
 * - `foreach_target`
 * - `op`
 *
 * **Required network functions:**
 * - `foreach_node`
 * - `foreach_qubit`
 * - `num_qubits`
 *
 * \param network A quantum network
 * \param filename Filename
 * \param color_marked_gates Flag to draw marked nodes in red
 */
template <typename Network>
void write_dotqc(Network const& network, std::string const& filename) {
    std::ofstream os(filename.c_str(), std::ofstream::out);
    write_dotqc(network, os);
}

} // namespace tweedledum
