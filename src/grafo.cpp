#include "grafo.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <iostream>
#include <cstdlib>
#include <ctime>

static std::string trim(const std::string &s) {
    size_t ini = s.find_first_not_of(" \t\r\n");
    if (ini == std::string::npos) return "";
    size_t fin = s.find_last_not_of(" \t\r\n");
    return s.substr(ini, fin - ini + 1);
}

// ID : Nombre : tiempo_ms : dep1, dep2, ...
// las deps pueden venir con corchetes ([1, 2]) o sin ellos (1, 2)
// en el enunciado aparece usando corchetes en la definicion del formato, pero no en el ejemplo, asi que soportamos las dos
static bool parsearLinea(const std::string &linea, Actividad &out) {
    std::vector<std::string> campos;
    size_t pos = 0, ant = 0;
    for (int i = 0; i < 3; i++) {
        pos = linea.find(':', ant);
        if (pos == std::string::npos) return false;
        campos.push_back(linea.substr(ant, pos - ant));
        ant = pos + 1;
    }
    campos.push_back(linea.substr(ant));

    std::string id     = trim(campos[0]);
    std::string nombre = trim(campos[1]);
    std::string tiempo = trim(campos[2]);
    std::string deps   = trim(campos[3]);

    if (id.empty()) return false;

    out.id = id;
    out.nombre = nombre;
    out.tiempo_ms = tiempo.empty()
        ? (100 + rand() % (5000 - 100 + 1))
        : std::stol(tiempo);

    if (!deps.empty() && deps.front() == '[' && deps.back() == ']') {
        deps = trim(deps.substr(1, deps.size() - 2));
    }

    out.dep_ids_raw.clear();
    if (!deps.empty()) {
        std::stringstream ss(deps);
        std::string tok;
        while (std::getline(ss, tok, ',')) {
            std::string d = trim(tok);
            if (!d.empty()) out.dep_ids_raw.push_back(d);
        }
    }

    out.estado = Estado::PENDIENTE;
    out.pid = -1;
    return true;
}

void Grafo::parsearArchivo(const std::string &ruta) {
    std::ifstream f(ruta);
    if (!f.is_open()) {
        throw std::runtime_error("No se pudo abrir " + ruta);
    }

    srand((unsigned)time(nullptr));
    acts.clear();

    std::string linea;
    int n_linea = 0;
    while (std::getline(f, linea)) {
        n_linea++;
        std::string l = trim(linea);
        if (l.empty() || l[0] == '#') continue;

        Actividad a;
        if (!parsearLinea(l, a)) {
            std::cerr << "linea " << n_linea << " mal formada, la ignoro: " << l << "\n";
            continue;
        }
        acts.push_back(std::move(a));
    }
}

void Grafo::construirDAG() {
    // map de id -> indice para no andar buscando con for anidados
    // (con 10000 actividades eso se pone lento altiro)
    std::unordered_map<std::string, int> tabla;
    tabla.reserve(acts.size() * 2);
    for (size_t i = 0; i < acts.size(); i++) tabla[acts[i].id] = (int)i;

    for (size_t i = 0; i < acts.size(); i++) {
        Actividad &a = acts[i];
        a.deps.clear();
        for (const auto &dep_id : a.dep_ids_raw) {
            auto it = tabla.find(dep_id);
            if (it == tabla.end()) {
                throw std::runtime_error(
                    "'" + a.id + "' depende de '" + dep_id + "', que no existe en el plan");
            }
            int idx_dep = it->second;
            a.deps.push_back(idx_dep);
            acts[idx_dep].dependientes.push_back((int)i);
        }
        a.pending_deps = (int)a.deps.size();
    }

    // como el enunciado dice, asumimos que el plan_ejemplo.txt siempre es un DAG valido
}

void Grafo::cargarDesdeArchivo(const std::string &ruta) {
    parsearArchivo(ruta);
    construirDAG();
}

int Grafo::buscarIndice(const std::string &id) const {
    for (size_t i = 0; i < acts.size(); i++) {
        if (acts[i].id == id) return (int)i;
    }
    return -1;
}

void Grafo::imprimir() const {
    for (size_t i = 0; i < acts.size(); i++) {
        const Actividad &a = acts[i];
        std::cout << "[" << a.id << "] " << a.nombre << " (" << a.tiempo_ms << " ms) - deps: ";
        if (a.deps.empty()) std::cout << "(ninguna)";
        for (size_t k = 0; k < a.deps.size(); k++) {
            std::cout << acts[a.deps[k]].id << (k + 1 < a.deps.size() ? ", " : "");
        }
        std::cout << " | dependientes: ";
        if (a.dependientes.empty()) std::cout << "(ninguno)";
        for (size_t k = 0; k < a.dependientes.size(); k++) {
            std::cout << acts[a.dependientes[k]].id << (k + 1 < a.dependientes.size() ? ", " : "");
        }
        std::cout << "\n";
    }
}
