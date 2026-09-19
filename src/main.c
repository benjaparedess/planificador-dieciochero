#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s plan.txt K\n", argv[0]);
        return 1;
    }

    leer_plan(argv[1]);

    printf("Actividades cargadas: %d\n\n", n_actividades);
    for (int i = 0; i < n_actividades; i++) {
        Actividad *a = &actividades[i];
        printf("[%d] id=%s nombre=%s tiempo=%ld ms in_degree=%d deps=(",
               i, a->id, a->nombre, a->tiempo_ms, a->in_degree);
        for (int j = 0; j < a->n_deps; j++) {
            printf("%s", a->deps[j]);
            if (j < a->n_deps - 1) printf(",");
        }
        printf(")\n");
    }

    return 0;
}
