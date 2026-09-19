#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ACT 10001
#define MAX_ID 16
#define MAX_NOMBRE 64
#define MAX_DEPS 20

typedef struct {
    char id[MAX_ID];
    char nombre[MAX_NOMBRE];
    long tiempo_ms;
    char deps[MAX_DEPS][MAX_ID];
    int n_deps;
    int in_degree;
} Actividad;

Actividad actividades[MAX_ACT];
int n_actividades = 0;

// Estas tres nuevas nos ayudan a llevar la cuenta del scheduler
int lanzada[MAX_ACT];      // 1 si ya se hizo fork() para esta actividad
int terminada[MAX_ACT];    // 1 si su hijo ya termino
pid_t hijo_pid[MAX_ACT];   // el pid del hijo que ejecuta esta actividad

char *limpiar(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *fin = s + strlen(s) - 1;
    while (fin > s && (*fin == ' ' || *fin == '\t' || *fin == '\n' || *fin == '\r')) {
        *fin = '\0';
        fin--;
    }
    return s;
}

void leer_plan(const char *path) {
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror("No se pudo abrir el archivo");
        exit(1);
    }

    char linea[512];
    while (fgets(linea, sizeof(linea), f) != NULL) {
        char *l = limpiar(linea);
        if (l[0] == '\0') continue;

        Actividad *a = &actividades[n_actividades];
        memset(a, 0, sizeof(Actividad));

        char *campo_id     = strtok(l, ":");
        char *campo_nombre = strtok(NULL, ":");
        char *campo_tiempo = strtok(NULL, ":");
        char *campo_deps   = strtok(NULL, "");

        if (campo_id == NULL || campo_nombre == NULL) continue;

        strncpy(a->id, limpiar(campo_id), MAX_ID - 1);
        strncpy(a->nombre, limpiar(campo_nombre), MAX_NOMBRE - 1);

        if (campo_tiempo != NULL && strlen(limpiar(campo_tiempo)) > 0) {
            a->tiempo_ms = atol(limpiar(campo_tiempo));
        } else {
            a->tiempo_ms = 100 + rand() % (5000 - 100 + 1);
        }

        a->n_deps = 0;
        if (campo_deps != NULL) {
            char *d = limpiar(campo_deps);
            int len = strlen(d);
            if (len >= 2 && d[0] == '[' && d[len - 1] == ']') {
                d[len - 1] = '\0';
                d++;
            }
            char *tok = strtok(d, ",");
            while (tok != NULL) {
                char *dep = limpiar(tok);
                if (strlen(dep) > 0) {
                    strncpy(a->deps[a->n_deps], dep, MAX_ID - 1);
                    a->n_deps++;
                }
                tok = strtok(NULL, ",");
            }
        }

        a->in_degree = a->n_deps;
        n_actividades++;
    }

    fclose(f);
}

void ejecutar_actividad(Actividad *a) {
    printf("  [hijo pid=%d] empezando '%s' (%ld ms)\n", getpid(), a->nombre, a->tiempo_ms);
    usleep(a->tiempo_ms * 1000);
    printf("  [hijo pid=%d] termine '%s'\n", getpid(), a->nombre);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s plan.txt K\n", argv[0]);
        return 1;
    }

    leer_plan(argv[1]);
    int K = atoi(argv[2]);

    printf("Actividades cargadas: %d | K=%d\n\n", n_actividades, K);

    int activos = 0;
    int terminadas = 0;

    while (terminadas < n_actividades) {

        // 1) lanzar todas las que se pueda: listas (in_degree==0) y con cupo (activos<K)
        for (int i = 0; i < n_actividades; i++) {
            if (!lanzada[i] && actividades[i].in_degree == 0 && activos < K) {
                pid_t pid = fork();

                if (pid < 0) {
                    perror("fork");
                    exit(1);
                } else if (pid == 0) {
                    // HIJO
                    ejecutar_actividad(&actividades[i]);
                    exit(0);
                } else {
                    // PADRE
                    hijo_pid[i] = pid;
                    lanzada[i] = 1;
                    activos++;
                    printf("[padre] lance '%s' (pid=%d) — activos=%d/%d\n",
                           actividades[i].nombre, pid, activos, K);
                }
            }
        }

        // 2) si aun falta gente por terminar, esperar a que alguien termine
        if (terminadas < n_actividades) {
            int status;
            pid_t pid_term = waitpid(-1, &status, 0); // espera a CUALQUIER hijo

            // averiguar a que actividad pertenecia ese pid
            int idx = -1;
            for (int i = 0; i < n_actividades; i++) {
                if (lanzada[i] && !terminada[i] && hijo_pid[i] == pid_term) {
                    idx = i;
                    break;
                }
            }
            if (idx == -1) continue;

            terminada[idx] = 1;
            terminadas++;
            activos--;

            if (WIFEXITED(status)) {
                printf("[padre] '%s' termino con codigo %d\n",
                       actividades[idx].nombre, WEXITSTATUS(status));
            }

            // avisar a quienes dependian de esta actividad: bajarles el in_degree
            for (int j = 0; j < n_actividades; j++) {
                for (int d = 0; d < actividades[j].n_deps; d++) {
                    if (strcmp(actividades[j].deps[d], actividades[idx].id) == 0) {
                        actividades[j].in_degree--;
                    }
                }
            }
        }
    }

    printf("\n[padre] todas las actividades terminaron\n");
    return 0;
}
