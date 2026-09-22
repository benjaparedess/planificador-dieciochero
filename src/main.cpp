#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>

struct Actividad {
    std::string id;
    std::string nombre;
    long tiempo_ms;

    std::vector<std::string> deps_ids; // los ids tal cual vienen del txt
    std::vector<int> deps_idx;         // ya resueltos a indice del vector
    std::vector<int> sucesores_idx;    // quienes dependen de mi

    int in_degree;
};

std::vector<Actividad> actividades;

std::string limpiar(const std::string &s) {
    size_t inicio = s.find_first_not_of(" \t\r\n");
    if (inicio == std::string::npos) return "";
    size_t fin = s.find_last_not_of(" \t\r\n");
    return s.substr(inicio, fin - inicio + 1);
}

// lee el plan.txt linea por linea y guarda los datos crudos de cada actividad
// ojo que aca todavia no validamos nada de las dependencias, eso lo hacemos
// despues en construir_dag()
void parsear_plan(const std::string &path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "No se pudo abrir " << path << std::endl;
        exit(1);
    }

    std::string linea;
    while (std::getline(f, linea)) {
        std::string l = limpiar(linea);
        if (l.empty()) continue;

        std::stringstream ss(l);
        std::string campo_id, campo_nombre, campo_tiempo, campo_deps;

        std::getline(ss, campo_id, ':');
        std::getline(ss, campo_nombre, ':');
        std::getline(ss, campo_tiempo, ':');
        std::getline(ss, campo_deps);

        Actividad a;
        a.id = limpiar(campo_id);
        a.nombre = limpiar(campo_nombre);

        std::string t = limpiar(campo_tiempo);
        if (t.empty()) {
            a.tiempo_ms = 100 + rand() % (5000 - 100 + 1);
        } else {
            a.tiempo_ms = std::stol(t);
        }

        std::string d = limpiar(campo_deps);
        if (!d.empty() && d.front() == '[' && d.back() == ']') {
            d = d.substr(1, d.size() - 2);
        }
        std::stringstream dss(d);
        std::string dep;
        while (std::getline(dss, dep, ',')) {
            dep = limpiar(dep);
            if (!dep.empty()) {
                a.deps_ids.push_back(dep);
            }
        }

        actividades.push_back(a);
    }
}

// aca recien armamos el grafo en si: convertimos los ids de texto en indices
// reales y calculamos los sucesores, que el archivo no nos da directo
void construir_dag() {
    std::map<std::string, int> id_a_indice;
    for (size_t i = 0; i < actividades.size(); i++) {
        id_a_indice[actividades[i].id] = static_cast<int>(i);
    }

    for (size_t i = 0; i < actividades.size(); i++) {
        Actividad &a = actividades[i];

        for (const std::string &dep_id : a.deps_ids) {
            auto it = id_a_indice.find(dep_id);
            if (it == id_a_indice.end()) {
                std::cerr << "la dependencia '" << dep_id << "' de '" << a.id
                          << "' no existe, revisa el plan.txt\n";
                exit(1);
            }

            int idx_dep = it->second;
            a.deps_idx.push_back(idx_dep);

            // le avisamos a la dependencia que nosotros somos su sucesor
            actividades[idx_dep].sucesores_idx.push_back(static_cast<int>(i));
        }

        a.in_degree = static_cast<int>(a.deps_idx.size());
    }
}

void imprimir_dag() {
    std::cout << "Actividades cargadas: " << actividades.size() << "\n\n";
    for (size_t i = 0; i < actividades.size(); i++) {
        const Actividad &a = actividades[i];
        std::cout << "[" << i << "] id=" << a.id << " nombre=" << a.nombre
                   << " tiempo=" << a.tiempo_ms << " ms in_degree=" << a.in_degree
                   << "\n      deps=(";
        for (size_t j = 0; j < a.deps_idx.size(); j++) {
            std::cout << actividades[a.deps_idx[j]].id;
            if (j + 1 < a.deps_idx.size()) std::cout << ",";
        }
        std::cout << ") sucesores=(";
        for (size_t j = 0; j < a.sucesores_idx.size(); j++) {
            std::cout << actividades[a.sucesores_idx[j]].id;
            if (j + 1 < a.sucesores_idx.size()) std::cout << ",";
        }
        std::cout << ")\n";
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " plan.txt K" << std::endl;
        return 1;
    }

    parsear_plan(argv[1]);
    construir_dag();
    imprimir_dag();

    return 0;
}
