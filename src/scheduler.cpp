#include "scheduler.hpp"
#include <queue>
#include <unordered_map>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>

#define MSG_MAX 128

// crea un pipe por cada arista que sale de "i", justo antes de su fork,
// para que el hijo se lleve los extremos de escritura cuando nazca
static void prepararPipesSalida(Grafo &g, int i) {
    Actividad &a = g.acts[i];
    for (int suc : a.dependientes) {
        if (g.acts[suc].estado == Estado::SALTADA) continue; // ya no va a nacer, no le hago pipe
        int fd[2];
        if (pipe(fd) != 0) {
            perror("pipe");
            exit(1);
        }
        a.fd_escritura_dependientes.push_back(fd[1]);
        g.acts[suc].fd_lectura_deps.push_back(fd[0]);
    }
}

// una actividad falla si su nombre empieza con "falla" (asi probamos con
// tests/plan_con_falla.txt) o si el tiempo es negativo, que no tiene sentido xd
static bool debeFallar(const Actividad &a) {
    return a.tiempo_ms < 0 || a.nombre.rfind("falla", 0) == 0;
}

// esto corre en el hijo
static void ejecutarActividad(const Actividad &a) {
    std::cout << "  [hijo pid=" << getpid() << "] empezando '" << a.nombre
               << "' (" << a.tiempo_ms << " ms)\n";

    for (int fd : a.fd_lectura_deps) {
        char buf[MSG_MAX] = {0};
        ssize_t leido = read(fd, buf, sizeof(buf) - 1);
        if (leido > 0) std::cout << "    [" << a.nombre << "] recibi: " << buf << "\n";
        close(fd);
    }

    // si falla salimos con codigo 1 sin escribir nada a los pipes de salida,
    // el padre se entera por el status del waitpid
    if (debeFallar(a)) {
        std::cerr << "  [hijo pid=" << getpid() << "] '" << a.nombre << "' FALLO\n";
        _exit(1);
    }

    usleep((useconds_t)a.tiempo_ms * 1000);

    char msg[MSG_MAX];
    snprintf(msg, sizeof(msg), "%s:OK", a.id.c_str());
    for (int fd : a.fd_escritura_dependientes) {
        write(fd, msg, strlen(msg) + 1);
        close(fd);
    }

    std::cout << "  [hijo pid=" << getpid() << "] termine '" << a.nombre << "'\n";
}

// cuando una actividad falla, sus dependientes (y los de ellos, y asi) nunca  van a poder correr, los marcamos SALTADA y los sumamos a terminadas para que
// el while principal no se quede esperandolos. es iterativo con una cola porque
// con una cadena de 10000 actividades la recursion se puede pasar de la raya
static void saltarRama(Grafo &g, int origen, int &terminadas) {
    std::queue<int> por_revisar;
    for (int suc : g.acts[origen].dependientes) por_revisar.push(suc);

    while (!por_revisar.empty()) {
        int i = por_revisar.front();
        por_revisar.pop();

        Actividad &a = g.acts[i];
        if (a.estado != Estado::PENDIENTE) continue; // ya estaba saltada

        a.estado = Estado::SALTADA;
        terminadas++;
        std::cout << "[padre] '" << a.nombre << "' SALTADA (una dependencia fallo)\n";

        // esta actividad nunca va a nacer, asi que cerramos los pipes de
        // lectura que el padre tenia guardados para ella
        for (int fd : a.fd_lectura_deps) close(fd);
        a.fd_lectura_deps.clear();

        for (int suc : a.dependientes) por_revisar.push(suc);
    }
}

void ejecutarPlan(Grafo &g, int K) {
    // con 10000 actividades el default de fds abiertos se puede quedar corto, asi que lo subimos al maximo
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
        rl.rlim_cur = rl.rlim_max;
        setrlimit(RLIMIT_NOFILE, &rl);
    }

    int n = (int)g.acts.size();
    int terminadas = 0;
    int activos = 0;

    std::queue<int> listas;
    for (int i = 0; i < n; i++) {
        if (g.acts[i].pending_deps == 0) {
            g.acts[i].estado = Estado::LISTA;
            listas.push(i);
        }
    }

    std::unordered_map<pid_t, int> pid_a_indice;
    pid_a_indice.reserve((size_t)n * 2);

    while (terminadas < n) {
        while (!listas.empty() && activos < K) {
            int i = listas.front();
            listas.pop();
            Actividad &a = g.acts[i];

            prepararPipesSalida(g, i);

            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                exit(1);
            } else if (pid == 0) {
                ejecutarActividad(a);
                _exit(0);
            } else {
                // el hijo ya tiene sus copias de estos fds, en el padre no
                // los necesitamos más
                for (int fd : a.fd_lectura_deps) close(fd);
                for (int fd : a.fd_escritura_dependientes) close(fd);

                a.pid = pid;
                a.estado = Estado::CORRIENDO;
                pid_a_indice[pid] = i;
                activos++;
                std::cout << "[padre] lance '" << a.nombre << "' (pid=" << pid
                           << ") - activos=" << activos << "/" << K << "\n";
            }
        }

        if (terminadas >= n) break;

        int status;
        pid_t pid_term = waitpid(-1, &status, 0); // esto bloquea, sin tener el busy-wait
        if (pid_term < 0) break;

        auto it = pid_a_indice.find(pid_term);
        if (it == pid_a_indice.end()) continue;
        int idx = it->second;
        pid_a_indice.erase(it);

        Actividad &term = g.acts[idx];
        activos--;
        terminadas++;

        bool ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
        term.estado = ok ? Estado::HECHA : Estado::FALLIDA;

        std::cout << "[padre] '" << term.nombre << "' termino ("
                   << (ok ? "OK" : "FALLO") << ")\n";
        if (WIFSIGNALED(status)) {
            std::cout << "[padre] '" << term.nombre << "' murio por la senal "
                       << WTERMSIG(status) << "\n";
        }

        if (ok) {
            for (int suc : term.dependientes) {
                Actividad &s = g.acts[suc];
                s.pending_deps--;
                if (s.pending_deps == 0 && s.estado == Estado::PENDIENTE) {
                    s.estado = Estado::LISTA;
                    listas.push(suc);
                }
            }
        } else {
            // si fallo no le bajamos el contador a nadie: cortamos la rama
            saltarRama(g, idx, terminadas);
        }
    }

    int hechas = 0, fallidas = 0, saltadas = 0;
    for (const Actividad &a : g.acts) {
        if (a.estado == Estado::HECHA) hechas++;
        else if (a.estado == Estado::FALLIDA) fallidas++;
        else if (a.estado == Estado::SALTADA) saltadas++;
    }
    std::cout << "\n[padre] fin de la simulacion: " << hechas << " hechas, "
               << fallidas << " fallidas, " << saltadas << " saltadas (de " << n << ")\n";
}
