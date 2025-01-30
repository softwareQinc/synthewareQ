import pystaq

# Programs
circuit = pystaq.parse_file("./shor_n3.qasm")
print("STARTING CIRCUIT\n")
print(circuit)

# Optimization level(0-3)
optimization_level = 3

# Compile
pystaq.compile(circuit, inline_stdlib=True, optimization_level=optimization_level)

# Map
pystaq.map(
    circuit,
    layout="bestfit",
    mapper="steiner",
    evaluate_all=False,
    device_json_file="./device.json",
)

print(f"COMPILED CIRCUIT (OPT={optimization_level})\n")
print(circuit)
