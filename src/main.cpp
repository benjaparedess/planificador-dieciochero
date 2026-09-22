#include <iostream>
#include <cstdlib>
#include "grafo.hpp"
#include "scheduler.hpp"

int main(int argc, char *argv[]) {
    // ojo con esto, vimos que sin unitbuf el cout del padre, se duplicaba en cada hijo
    // (buffer sin vaciar antes del fork, el hijo se lo llevaba de copia)
    std::cout << std::unitbuf;

    if (argc < 3) {
        std::cerr << "Uso: " << argv[0] << " plan_ejemplo.txt K\n";
        return 1;
    }

    Grafo g;
    try {
        g.parsearArchivo(argv[1]);
        g.construirDAG();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    int K = std::atoi(argv[2]);
    if (K <= 0) {
        std::cerr << "K tiene que ser mayor a 0\n";
        return 1;
    }

    std::cout << "Plan cargado: " << g.acts.size() << " actividades. K=" << K << "\n\n";

    ejecutarPlan(g, K);

    return 0;
}
