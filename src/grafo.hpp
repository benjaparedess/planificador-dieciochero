#ifndef GRAFO_HPP
#define GRAFO_HPP
#include <string>
#include <vector>
#include <sys/types.h>

// estados posibles de una actividad durante la simulación
enum class Estado {
    PENDIENTE,
    LISTA,
    CORRIENDO,
    HECHA,
    FALLIDA,
    SALTADA // no continua porque posiblemente fallo alguna dependencia (ej.paso 4)
};

struct Actividad {
    std::string id;
    std::string nombre;
    long tiempo_ms = 0;

    std::vector<std::string> dep_ids_raw; // ids en texto, siguendo el estilo del plan_ejemplo.txt

    std::vector<int> deps;         // de quienes dependo
    std::vector<int> dependientes; // quienes dependen de mi

    int pending_deps = 0; // cuantas de mis deps todavia no terminan
    Estado estado = Estado::PENDIENTE;
    pid_t pid = -1;

    // fds para el paso de mensajes (pipes), uno por arista
    std::vector<int> fd_escritura_dependientes;
    std::vector<int> fd_lectura_deps;
};

class Grafo {
public:
    std::vector<Actividad> acts;

    // acá lee plan_ejemplo.txt y llena los campos crudos de cada actividad
    // no resolver todavía las dependencias (eso elo haremos en construirDAG)
    void parsearArchivo(const std::string &ruta);

    // resuelve los ids de texto a indices y arma deps/dependientes
    void construirDAG();

    void cargarDesdeArchivo(const std::string &ruta); // esto hace parsearArchivo + construirDAG

    int buscarIndice(const std::string &id) const;

    void imprimir() const;
};

#endif