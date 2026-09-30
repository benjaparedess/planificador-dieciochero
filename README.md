# Planificador Dieciochero

Tarea 1 de Sistemas Operativos (UDP). Simula un día de Fiestas Patrias donde
las actividades (prender el carbón, comprar la carne, armar el choripán,
etc.) dependen unas de otras. Se modela como un grafo acíclico dirigido
(DAG) y cada actividad corre en su propio proceso.

## Estado del proyecto

- [x] Parseo de plan.txt
- [x] Modelado del DAG
- [x] Creación de procesos (fork + waitpid)
- [x] Control de concurrencia (K)
- [x] Paso de mensajes por pipes
- [x] Aislamiento de errores
- [x] Manejo de SIGINT
- [x] Prueba de carga de estrés (10.000 actividades)

## Requisitos

- g++ con soporte para C++17
- Sistema Unix (probado en Fedora y Ubuntu)

## Compilación

    make

## Uso

    ./planificador plan.txt K

`plan.txt` sigue el formato del enunciado (`ID : nombre : tiempo_ms : deps`)
y `K` define el máximo de procesos corriendo al mismo tiempo.

Ejemplos:

    ./planificador tests/plan_ejemplo.txt 2
    ./planificador tests/plan_estres_10000.txt 50

## Estructura del proyecto

    planificador-dieciochero/
    ├── src/
    │   ├── main.cpp        # arma el grafo y llama al planificador
    │   ├── grafo.hpp/.cpp  # parseo de plan.txt y modelado del DAG
    │   └── scheduler.hpp/.cpp  # fork, control de concurrencia, pipes y señales
    ├── tests/
    │   ├── plan_ejemplo.txt
    │   ├── plan_con_falla.txt
    │   ├── plan_dos_fallas.txt
    │   ├── plan_fuga.txt
    │   ├── plan_senal.txt
    │   ├── plan_estres_10000.txt
    │   └── pruebas_estres.py   # genera planes grandes para probar carga
    ├── Makefile
    └── README.md

## Decisiones de diseño

- No se usan threads ni mecanismos de sincronización de hilos (la tarea los
  prohíbe). El control de concurrencia lo hace el proceso padre esperando
  con `waitpid`, sin busy-waiting.
- El paso de mensajes entre actividades se hace con pipes, uno por cada
  dependencia. Como una actividad recién se lanza cuando todas sus
  dependencias ya terminaron, nunca se queda esperando un mensaje que no
  va a llegar.
- Con el plan de 10.000 actividades, recorrer todo el arreglo en cada vuelta
  buscando cuáles estaban listas se ponía lento, así que se usa una cola
  para eso en vez de un `for` completo.
- Al cargar el plan se valida que no haya ids repetidos, que las
  dependencias apunten a actividades que existen, que el tiempo sea válido
  y que el grafo no tenga ciclos (con el algoritmo de Kahn, el mismo que se
  usa para el orden de ejecución). Si algo de esto falla, el programa
  reporta el error y termina antes de simular nada.
- Con 10.000 actividades y un pipe por cada arista, el límite de file
  descriptors por defecto del sistema (típicamente 1024) se puede quedar
  corto. Al iniciar, el programa sube ese límite al máximo permitido con
  `setrlimit`.
- El Makefile compila con `-lpthread` aunque el programa no usa threads,
  porque la rúbrica lo pide textualmente en la línea de compilación para
  C++.
- En `main()`, antes de cualquier `fork()`, se fuerza `std::cout` a modo
  `unitbuf`. Sin esto nos encontramos con que la salida del padre quedaba
  duplicada en cada hijo al redirigir la salida a un archivo: el buffer de
  `cout` no se vaciaba antes del `fork()`, así que el hijo se llevaba una
  copia con líneas pendientes y las volvía a imprimir al salir. En la
  terminal no se notaba porque ahí `cout` es line-buffered por defecto; se
  detectó probando con la salida redirigida, que es justo cómo la va a
  capturar el corrector.

## Aislamiento de errores

Una actividad se considera fallida si su proceso termina con un código de
salida distinto de 0 o si muere por una señal (por ejemplo un `kill -9`).
El padre lo detecta con `WIFEXITED` / `WEXITSTATUS` y `WIFSIGNALED` después
del `waitpid`.

Cuando una actividad falla, las que dependen de ella (directa o
indirectamente) se marcan como `SALTADA` y nunca se lanzan. El resto del
plan sigue corriendo normal. Esto lo hace `saltarRama`, que recorre los
dependientes con una cola en vez de recursión, porque con 10.000
actividades una cadena larga podía llenar la pila.

Detalles que nos tocó arreglar en el camino:

- Las actividades saltadas hay que sumarlas al contador de terminadas. Si
  no, el ciclo principal se quedaba esperando algo que nunca iba a pasar.
- Los pipes que el padre tenía guardados para una actividad saltada se
  cierran, y ya no se crean pipes hacia actividades que no van a correr.
  Sin eso se filtraba un descriptor por cada una.

Para poder probar esto sin depender del azar, una actividad falla si su
nombre empieza con `falla` o si su tiempo es negativo.

## Manejo de SIGINT (Ctrl+C)

Un handler de `sigaction` marca una bandera (`volatile sig_atomic_t`) cuando
llega SIGINT, y no hace nada más: dentro de un manejador de señal no es
seguro llamar a funciones como `cout` o `kill`, así que esa parte se maneja
después, en el ciclo principal.

El `sigaction` se registra sin `SA_RESTART`, para que el `waitpid` que está
bloqueado salga con `EINTR` apenas llega la señal, en vez de seguir
esperando. El ciclo principal revisa la bandera, y si está activa, manda
`SIGKILL` a todos los procesos hijos que seguían corriendo y los espera con
`waitpid` hasta que no quede ninguno, para no dejar procesos zombies. El
programa termina con código 130 (128 + señal SIGINT), la misma convención
que usa bash.

## Pruebas realizadas

| Plan | K | Resultado esperado |
|---|---|---|
| `tests/plan_ejemplo.txt` | 2 | 6 hechas, 0 fallidas, 0 saltadas |
| `tests/plan_con_falla.txt` | 2 | 2 hechas, 1 fallida, 3 saltadas |
| `tests/plan_dos_fallas.txt` | 2 | 0 hechas, 2 fallidas, 2 saltadas |
| `tests/plan_fuga.txt` | 1 | 1 hecha, 1 fallida, 1 saltada |
| `tests/plan_senal.txt` (con `kill -9` a un hijo) | 2 | 2 hechas, 1 fallida, 1 saltada |
| plan de 10.000 con una actividad del medio fallando | 50 | 8206 hechas, 1 fallida, 1793 saltadas |
| `tests/plan_ejemplo.txt`, interrumpido con Ctrl+C a medio correr | 2 | termina con código 130, sin procesos zombies |

El plan de 10.000 con falla se genera así:

    python3 tests/pruebas_estres.py 10000 | sed 's/: act_5000 :/: falla_5000 :/' > /tmp/falla10000.txt
    ./planificador /tmp/falla10000.txt 50

## Autores

- Benjamín Paredes Rojas
- Christián Pereira Contreras
